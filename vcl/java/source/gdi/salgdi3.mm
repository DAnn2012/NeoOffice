/*************************************************************************
 *
 *  $RCSfile$
 *
 *  $Revision$
 *
 *  last change: $Author$ $Date$
 *
 *  The Contents of this file are made available subject to the terms of
 *  either of the following licenses
 *
 *         - GNU General Public License Version 2.1
 *
 *  Patrick Luby, June 2003
 *
 *  GNU General Public License Version 2.1
 *  =============================================
 *  Copyright 2003 Planamesa Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License version 2.1, as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 *  MA  02111-1307  USA
 *
 ************************************************************************/

#include <salatslayout.hxx>
#include <saldata.hxx>
#include <salinst.h>
#include <vcl/sallayout.hxx>
#include <vcl/impfont.hxx>
#include <vcl/outdev.h>
#include <vcl/unohelp.hxx>
#if !defined USE_NATIVE_WINDOW || !defined USE_NATIVE_VIRTUAL_DEVICE || !defined USE_NATIVE_PRINTING
#include <com/sun/star/vcl/VCLFont.hxx>
#include <com/sun/star/vcl/VCLGraphics.hxx>
#endif	// !USE_NATIVE_WINDOW || !USE_NATIVE_VIRTUAL_DEVICE || !USE_NATIVE_PRINTING
#include <basegfx/polygon/b2dpolypolygon.hxx>
#include <rtl/process.h>

#include <premac.h>
#import <Cocoa/Cocoa.h>
#include <postmac.h>

#include "salgdi3_cocoa.h"

static void ImplFontListChangedCallback( ATSFontNotificationInfoRef aInfo, void *pData );

static ATSFontNotificationRef aFontNotification = NULL;
static bool bNativeFontsLoaded = false;

using namespace basegfx;
using namespace rtl;
#if !defined USE_NATIVE_WINDOW || !defined USE_NATIVE_VIRTUAL_DEVICE || !defined USE_NATIVE_PRINTING
using namespace vcl;
#endif	// !USE_NATIVE_WINDOW || !USE_NATIVE_VIRTUAL_DEVICE || !USE_NATIVE_PRINTING
using namespace vos;

@interface VCLLoadNativeFonts : NSObject
+ (id)create;
- (void)loadNativeFonts:(id)pObject;
@end

@implementation VCLLoadNativeFonts

+ (id)create
{
	VCLLoadNativeFonts *pRet = [[VCLLoadNativeFonts alloc] init];
	[pRet autorelease];
	return pRet;
}

- (void)loadNativeFonts:(id)pObject
{
	ImplFontListChangedCallback( NULL, NULL );
}

@end

// ============================================================================

static void ImplFontListChangedCallback( ATSFontNotificationInfoRef aInfo, void *pData )
{
	static bool bInLoad = false;

	if ( bInLoad )
		return;

	bInLoad = true;

	if ( !Application::IsShutDown() )
	{
		IMutex& rSolarMutex = Application::GetSolarMutex();
		rSolarMutex.acquire();
		if ( !Application::IsShutDown() )
		{
			SalData *pSalData = GetSalData();

			// Clean out caches
			SalATSLayout::ClearLayoutDataCache();
			JavaImplFont::clearNativeFonts();
			JavaImplFontData::ClearNativeFonts();
			for ( ::std::map< String, JavaImplFontData* >::const_iterator dfnit = pSalData->maFontNameMapping.begin(); dfnit != pSalData->maFontNameMapping.end(); ++dfnit )
				delete dfnit->second;
			pSalData->maFontNameMapping.clear();
			pSalData->maJavaFontNameMapping.clear();
			pSalData->maNativeFontMapping.clear();
			pSalData->maPlainNativeFontMapping.clear();
			pSalData->maBoldNativeFontMapping.clear();
			pSalData->maItalicNativeFontMapping.clear();
			pSalData->maBoldItalicNativeFontMapping.clear();

			if ( !Application::IsShutDown() )
			{
#if !defined USE_NATIVE_WINDOW || !defined USE_NATIVE_VIRTUAL_DEVICE || !defined USE_NATIVE_PRINTING
				VCLThreadAttach t;
				if ( t.pEnv )
				{
#endif	// !USE_NATIVE_WINDOW || !USE_NATIVE_VIRTUAL_DEVICE || !USE_NATIVE_PRINTING
					NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

					// Update cached fonts
					NSArray *pFonts = NSFontManager_getAllFonts();
					if ( pFonts )
					{
						const OUString aCourier( OUString::createFromAscii( "Courier" ) );
						const OUString aFontSeparator( OUString::createFromAscii( ";" ) );
						const OUString aLastResort( OUString::createFromAscii( "LastResort" ) );
						const OUString aMincho( OUString::createFromAscii( "Mincho" ) );
						const OUString aMing( OUString::createFromAscii( "Ming" ) );
						const OUString aMyungjo( OUString::createFromAscii( "Myungjo" ) );
						const OUString aRoman( OUString::createFromAscii( "Roman" ) );
						const OUString aSans( OUString::createFromAscii( "Sans" ) );
						const OUString aSerif( OUString::createFromAscii( "Serif" ) );
						const OUString aSong( OUString::createFromAscii( "Song" ) );
						const OUString aSung( OUString::createFromAscii( "Sung" ) );
						const OUString aSymbol( OUString::createFromAscii( "Symbol" ) );
						const OUString aNeoSymbol( OUString::createFromAscii( "Neo Symbol" ) );
						const OUString aNeo3Symbol( OUString::createFromAscii( "Neo3Symbol" ) );
						const OUString aOpenSymbol( OUString::createFromAscii( "OpenSymbol" ) );
						const OUString aRegular( OUString::createFromAscii( " Regular" ) );
						const OUString aStarSymbol( OUString::createFromAscii( "StarSymbol" ) );
						const OUString aTimes( OUString::createFromAscii( "Times" ) );
						const OUString aTimesRoman( OUString::createFromAscii( "Times Roman" ) );

						unsigned int i = 0;
						unsigned int nCount = [pFonts count];

						sal_uInt32 nActualCount = 0;
						for ( i = 0; i < nCount; i++ )
						{
							NSFont *pNSFont = [pFonts objectAtIndex:i];
							if ( !pNSFont )
								continue;
#ifdef USE_CORETEXT_TEXT_RENDERING
							CTFontRef aFont = (CTFontRef)pNSFont;
#else	// USE_CORETEXT_TEXT_RENDERING
							ATSFontRef aFont = NSFont_getATSFontRef( pNSFont );
							if ( !aFont )
								continue;
#endif	// USE_CORETEXT_TEXT_RENDERING

							// Get font attributes
							FontWeight nWeight = (FontWeight)NSFontManager_weightOfFont( pNSFont );
							FontItalic nItalic = ( NSFontManager_isItalic( pNSFont ) ? ITALIC_NORMAL : ITALIC_NONE );
							FontWidth nWidth = (FontWidth)NSFontManager_widthOfFont( pNSFont );
							FontPitch nPitch = ( NSFontManager_isFixedPitch( pNSFont ) ? PITCH_FIXED : PITCH_VARIABLE );

#ifdef USE_CORETEXT_TEXT_RENDERING
							CFStringRef aPSString = CTFontCopyPostScriptName( aFont );
#else	// USE_CORETEXT_TEXT_RENDERING
							CFStringRef aPSString;
							if ( ATSFontGetPostScriptName( aFont, kATSOptionFlagsDefault, &aPSString ) != noErr )
								continue;
#endif	// USE_CORETEXT_TEXT_RENDERING

							OUString aPSName;
							if ( aPSString )
							{
								CFIndex nPSLen = CFStringGetLength( aPSString );
								CFRange aPSRange = CFRangeMake( 0, nPSLen );
								sal_Unicode pPSBuffer[ nPSLen + 1 ];
								CFStringGetCharacters( aPSString, aPSRange, pPSBuffer );
								pPSBuffer[ nPSLen ] = 0;
								CFRelease( aPSString );
								aPSName = OUString( pPSBuffer );
							}

							if ( !aPSName.getLength() )
								continue;

							// Get the font family name
#ifdef USE_CORETEXT_TEXT_RENDERING
							CFStringRef aFamilyString = CTFontCopyFamilyName( aFont );
#else	// USE_CORETEXT_TEXT_RENDERING
							CFStringRef aFamilyString = NSFont_familyName( pNSFont );
#endif	// USE_CORETEXT_TEXT_RENDERING
							if ( !aFamilyString )
								continue;

							CFIndex nFamilyLen = CFStringGetLength( aFamilyString );
							CFRange aFamilyRange = CFRangeMake( 0, nFamilyLen );
							sal_Unicode pFamilyBuffer[ nFamilyLen + 1 ];
							CFStringGetCharacters( aFamilyString, aFamilyRange, pFamilyBuffer );
							pFamilyBuffer[ nFamilyLen ] = 0;
							CFRelease( aFamilyString );

							// Ignore empty family names or family names that
							// start with a "."
							OUString aFamilyName( pFamilyBuffer );
							if ( !aFamilyName.getLength() || aFamilyName.toChar() == (sal_Unicode)'.' )
								continue;

#ifdef USE_CORETEXT_TEXT_RENDERING
							sal_IntPtr nNativeFont = (sal_IntPtr)aFont;

							CFStringRef aDisplayString = CTFontCopyFullName( aFont );
							if ( !aDisplayString )
								continue;
#else	// USE_CORETEXT_TEXT_RENDERING
							sal_IntPtr nNativeFont = SalATSLayout::GetNativeFontFromATSFontRef( aFont );
							if ( (ATSUFontID)nNativeFont == kATSUInvalidFontID )
								continue;

							// Get the ATS font name as the Cocoa name on some
							// Mac OS X versions adds extraneous words
							CFStringRef aDisplayString;
							if ( ATSFontGetName( aFont, kATSOptionFlagsDefault, &aDisplayString ) != noErr )
								continue;
#endif	// USE_CORETEXT_TEXT_RENDERING

							CFIndex nDisplayLen = CFStringGetLength( aDisplayString );
							CFRange aDisplayRange = CFRangeMake( 0, nDisplayLen );
							sal_Unicode pDisplayBuffer[ nDisplayLen + 1 ];
							CFStringGetCharacters( aDisplayString, aDisplayRange, pDisplayBuffer );
							pDisplayBuffer[ nDisplayLen ] = 0;
							CFRelease( aDisplayString );

							OUString aMapName( aPSName );
							OUString aDisplayName( pDisplayBuffer );
							sal_Int32 nColon = aDisplayName.indexOf( (sal_Unicode)':' );
							if ( nColon >= 0 )
							{
								aDisplayName = OUString( aDisplayName.getStr(), nColon );
								aMapName += aFontSeparator + aDisplayName;
							}

							// Ignore empty font names or font names that start
							// with a "."
							if ( !aDisplayName.getLength() || aDisplayName.toChar() == (sal_Unicode)'.' )
								continue;

							if ( aDisplayName == aOpenSymbol || aDisplayName == aStarSymbol || aDisplayName == aNeoSymbol )
							{
								// Don't allow Sun's symbol fonts our older
								// NeoOffice fonts to override our symbol font
								continue;
							}
							else if ( aDisplayName == aNeo3Symbol )
							{
								aDisplayName = OUString( aOpenSymbol );
								aMapName += aFontSeparator + aSymbol + aFontSeparator + aNeo3Symbol;
							}
							else if ( aDisplayName == aLastResort )
							{
								// Ignore this Java font as it will mess up
								// our font fallback process
								continue;
							}
							else if ( aDisplayName == aTimesRoman )
							{
								aMapName += aFontSeparator + aTimes;
							}
							else if ( aDisplayName == aFamilyName + aRegular )
							{
								// Fix bug 3668 by adding family name to map
								// for "regular" fonts
								aMapName += aFontSeparator + aFamilyName;
							}

							String aXubMapName( aMapName );
							String aXubDisplayName( aDisplayName );

							// Skip the font if we already have it
							::std::map< String, JavaImplFontData* >::iterator it = pSalData->maFontNameMapping.find( aXubDisplayName );
							if ( it != pSalData->maFontNameMapping.end() )
								continue;

							ImplDevFontAttributes aAttributes;

							// Determine pitch and family type
							FontFamily nFamily;
							if ( nPitch == PITCH_FIXED )
								nFamily = FAMILY_MODERN;
							else if ( aPSName.indexOf( aSans ) >= 0 )
								nFamily = FAMILY_SWISS;
							else if ( aPSName.indexOf( aCourier ) >= 0 || aPSName.indexOf( aMincho ) >= 0 || aPSName.indexOf( aMing ) >= 0 || aPSName.indexOf( aMyungjo ) >= 0 || aPSName.indexOf( aRoman ) >= 0 || aPSName.indexOf( aSerif ) >= 0 || aPSName.indexOf( aTimes ) >= 0 || aPSName.indexOf( aSong ) >= 0 || aPSName.indexOf( aSung ) >= 0 )
								nFamily = FAMILY_ROMAN;
							else
								nFamily = FAMILY_SWISS;

							aAttributes.maName = aXubDisplayName;
							aAttributes.meWeight = nWeight;
							aAttributes.meItalic = nItalic;
							aAttributes.meFamily = nFamily;
							aAttributes.mePitch = nPitch;
							aAttributes.meWidthType = nWidth;
							aAttributes.mbSymbolFlag = false;
							aAttributes.maMapNames = aXubMapName;
							aAttributes.mnQuality = 0;
							aAttributes.mbOrientation = true;
							aAttributes.mbDevice = false;
							aAttributes.mbSubsettable = true;
							aAttributes.mbEmbeddable = false;

							JavaImplFontData *pFontData = new JavaImplFontData( aAttributes, aPSName, nNativeFont, aFamilyName );
							pSalData->maFontNameMapping[ aXubDisplayName ] = pFontData;

							// Multiple native fonts can map to the same font
							// due to disabling and reenabling of fonts with
							// the same name. Also, note that multiple font
							// names can map to a single native font so do not
							// rely on the native font to look up the font name.
							pSalData->maNativeFontMapping[ nNativeFont ] = pFontData;
							pSalData->maJavaFontNameMapping[ aPSName ] = pFontData;

							nActualCount++;
						}

						// Cache matching bold, italic, and bold italic fonts
						for ( i = 0; i < nCount; i++ )
						{
							NSFont *pNSFont = [pFonts objectAtIndex:i];
							if ( !pNSFont )
								continue;
#ifdef USE_CORETEXT_TEXT_RENDERING
							CTFontRef aFont = (CTFontRef)pNSFont;
							sal_IntPtr nNativeFont = (sal_IntPtr)aFont;
							if ( !nNativeFont )
								continue;
#else	// USE_CORETEXT_TEXT_RENDERING
							ATSFontRef aFont = NSFont_getATSFontRef( pNSFont );
							if ( !aFont )
								continue;

							sal_IntPtr nNativeFont = SalATSLayout::GetNativeFontFromATSFontRef( aFont );
							if ( (ATSUFontID)nNativeFont == kATSUInvalidFontID )
								continue;
#endif	// USE_CORETEXT_TEXT_RENDERING

							::std::hash_map< sal_IntPtr, JavaImplFontData* >::const_iterator nfit = pSalData->maNativeFontMapping.find( nNativeFont );
							if ( nfit == pSalData->maNativeFontMapping.end() )
								continue;

							JavaImplFontData *pFontData = nfit->second;
							bool bIsPlainFont = ( pFontData->meWeight <= WEIGHT_MEDIUM && pFontData->meItalic != ITALIC_OBLIQUE && pFontData->meItalic != ITALIC_NORMAL );

							// Try bold
							NSFont *pBoldFont = NSFont_findFontWithStyle( pNSFont, TRUE, FALSE );
							if ( pBoldFont )
							{
#ifdef USE_CORETEXT_TEXT_RENDERING
								CTFontRef aBoldFont = (CTFontRef)pBoldFont;
								sal_IntPtr nBoldNativeFont = (sal_IntPtr)aBoldFont;
#else	// USE_CORETEXT_TEXT_RENDERING
								ATSFontRef aBoldFont = NSFont_getATSFontRef( pBoldFont );
								sal_IntPtr nBoldNativeFont = SalATSLayout::GetNativeFontFromATSFontRef( aBoldFont );
#endif	// USE_CORETEXT_TEXT_RENDERING
								if ( nBoldNativeFont && nBoldNativeFont != nNativeFont )
								{
									nfit = pSalData->maNativeFontMapping.find( nBoldNativeFont );
									if ( nfit != pSalData->maNativeFontMapping.end() )
									{
										pSalData->maBoldNativeFontMapping[ nNativeFont ] = nfit->second;
										if ( bIsPlainFont )
											pSalData->maPlainNativeFontMapping[ nBoldNativeFont ] = pFontData;
									}
								}

								[pBoldFont release];
							}

							// Try italic
							NSFont *pItalicFont = NSFont_findFontWithStyle( pNSFont, FALSE, TRUE );
							if ( pItalicFont )
							{
#ifdef USE_CORETEXT_TEXT_RENDERING
								CTFontRef aItalicFont = (CTFontRef)pItalicFont;
								sal_IntPtr nItalicNativeFont = (sal_IntPtr)aItalicFont;
#else	// USE_CORETEXT_TEXT_RENDERING
								ATSFontRef aItalicFont = NSFont_getATSFontRef( pItalicFont );
								sal_IntPtr nItalicNativeFont = SalATSLayout::GetNativeFontFromATSFontRef( aItalicFont );
#endif	// USE_CORETEXT_TEXT_RENDERING
								if ( nItalicNativeFont && nItalicNativeFont != nNativeFont )
								{
									nfit = pSalData->maNativeFontMapping.find( nItalicNativeFont );
									if ( nfit != pSalData->maNativeFontMapping.end() )
									{
										pSalData->maItalicNativeFontMapping[ nNativeFont ] = nfit->second;
										if ( bIsPlainFont )
											pSalData->maPlainNativeFontMapping[ nItalicNativeFont ] = pFontData;
									}
								}

								[pItalicFont release];
							}

							// Try bold italic
							NSFont *pBoldItalicFont = NSFont_findFontWithStyle( pNSFont, TRUE, TRUE );
							if ( pBoldItalicFont )
							{
#ifdef USE_CORETEXT_TEXT_RENDERING
								CTFontRef aBoldItalicFont = (CTFontRef)pBoldItalicFont;
								sal_IntPtr nBoldItalicNativeFont = (sal_IntPtr)aBoldItalicFont;
#else	// USE_CORETEXT_TEXT_RENDERING
								ATSFontRef aBoldItalicFont = NSFont_getATSFontRef( pBoldItalicFont );
								sal_IntPtr nBoldItalicNativeFont = SalATSLayout::GetNativeFontFromATSFontRef( aBoldItalicFont );
#endif	// USE_CORETEXT_TEXT_RENDERING
								if ( nBoldItalicNativeFont && nBoldItalicNativeFont != nNativeFont )
								{
									nfit = pSalData->maNativeFontMapping.find( nBoldItalicNativeFont );
									if ( nfit != pSalData->maNativeFontMapping.end() )
									{
										pSalData->maBoldItalicNativeFontMapping[ nNativeFont ] = nfit->second;
										if ( bIsPlainFont )
											pSalData->maPlainNativeFontMapping[ nBoldItalicNativeFont ] = pFontData;
									}
								}

								[pBoldItalicFont release];
							}
						}

						[pFonts release];
					}

					[pPool release];

#if !defined USE_NATIVE_WINDOW || !defined USE_NATIVE_VIRTUAL_DEVICE || !defined USE_NATIVE_PRINTING
				}
#endif	// !USE_NATIVE_WINDOW || !USE_NATIVE_VIRTUAL_DEVICE || !USE_NATIVE_PRINTING
			}

			// Fix bug 3095 by handling font change notifications
			if ( !aFontNotification )
				ATSFontNotificationSubscribe( ImplFontListChangedCallback, kATSFontNotifyOptionDefault, NULL, &aFontNotification );

#ifndef USE_CORETEXT_TEXT_RENDERING
			SalATSLayout::SetFontFallbacks();
#endif	// !USE_CORETEXT_TEXT_RENDERING
			OutputDevice::ImplUpdateAllFontData( true );

			rSolarMutex.release();
		}
	}

	bNativeFontsLoaded = true;
	bInLoad = false;
}

// -----------------------------------------------------------------------

static const JavaImplFontData *ImplGetFontVariant( const JavaImplFontData *pFontData, BOOL bAddBold, BOOL bAddItalic )
{
	if ( pFontData )
	{
		SalData *pSalData = GetSalData();
		const JavaImplFontData *pPlainFontData = pFontData;
		sal_IntPtr nPlainNativeFont = pPlainFontData->GetFontId();

		// Fix bug 3031 by caching the bold, italic, and bold italic variants
		// of each font
		if ( bAddBold && bAddItalic && pFontData == pPlainFontData )
		{
			::std::hash_map< sal_IntPtr, JavaImplFontData* >::const_iterator bifit = pSalData->maBoldItalicNativeFontMapping.find( nPlainNativeFont );
			if ( bifit != pSalData->maBoldItalicNativeFontMapping.end() )
				pFontData = bifit->second;
		}

		if ( bAddBold && pFontData == pPlainFontData )
		{
			::std::hash_map< sal_IntPtr, JavaImplFontData* >::const_iterator bfit = pSalData->maBoldNativeFontMapping.find( nPlainNativeFont );
			if ( bfit != pSalData->maBoldNativeFontMapping.end() )
				pFontData = bfit->second;
		}

		if ( bAddItalic && pFontData == pPlainFontData )
		{
			::std::hash_map< sal_IntPtr, JavaImplFontData* >::const_iterator ifit = pSalData->maItalicNativeFontMapping.find( nPlainNativeFont );
			if ( ifit != pSalData->maItalicNativeFontMapping.end() )
				pFontData = ifit->second;
		}
	}

	return pFontData;
}

// =======================================================================

::std::map< sal_IntPtr, sal_IntPtr > JavaImplFontData::maBadNativeFontIDMap;

// -----------------------------------------------------------------------

::std::map< JavaImplFontData*, JavaImplFontData* > JavaImplFontData::maInstancesMap;

// -----------------------------------------------------------------------

void JavaImplFontData::ClearNativeFonts()
{
	JavaImplFontData::maBadNativeFontIDMap.clear();

#ifdef USE_CORETEXT_TEXT_RENDERING
	for ( ::std::map< JavaImplFontData*, JavaImplFontData* >::const_iterator it = JavaImplFontData::maInstancesMap.begin(); it != JavaImplFontData::maInstancesMap.end(); ++it )
	{
		if ( it->second->mnNativeFontID )
		{
			CFRelease( (CTFontRef)it->second->mnNativeFontID );
			it->second->mnNativeFontID = 0;
		}
	}
#endif	// USE_CORETEXT_TEXT_RENDERING
}

// -----------------------------------------------------------------------

void JavaImplFontData::HandleBadFont( JavaImplFontData *pFontData )
{
	if ( !pFontData )
		return;

	// Fix bug 3446 by reloading native fonts without any known bad fonts
	bool bReloadFonts = false;
	::std::map< sal_IntPtr, sal_IntPtr >::const_iterator bit = maBadNativeFontIDMap.find( pFontData->mnNativeFontID );
	if ( bit == maBadNativeFontIDMap.end() )
	{
		bReloadFonts = true;
		maBadNativeFontIDMap[ pFontData->mnNativeFontID ] = pFontData->mnNativeFontID;
	}

	// Find any fonts that have the same family as the current font and mark
	// those as bad fonts
	SalData *pSalData = GetSalData();
	for ( ::std::map< String, JavaImplFontData* >::const_iterator it = pSalData->maFontNameMapping.begin(); it != pSalData->maFontNameMapping.end(); ++it )
	{
		if ( it->second->maFamilyName == pFontData->maFamilyName )
		{
			bit = maBadNativeFontIDMap.find( it->second->mnNativeFontID );
			if ( bit == maBadNativeFontIDMap.end() )
			{
				bReloadFonts = true;
				maBadNativeFontIDMap[ it->second->mnNativeFontID ] = it->second->mnNativeFontID;
			}
		}
	}

	// Fix bug 3576 by updating the fonts after all currently queued
	// event are dispatched
	if ( bReloadFonts )
		Application::PostUserEvent( STATIC_LINK( NULL, JavaImplFontData, RunNativeFontsTimer ) );
}

// -----------------------------------------------------------------------

IMPL_STATIC_LINK_NOINSTANCE( JavaImplFontData, RunNativeFontsTimer, void*, pCallData )
{
	NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

	ULONG nCount = Application::ReleaseSolarMutex();
	VCLLoadNativeFonts *pVCLLoadNativeFonts = [VCLLoadNativeFonts create];
	NSArray *pModes = [NSArray arrayWithObjects:NSDefaultRunLoopMode, NSEventTrackingRunLoopMode, NSModalPanelRunLoopMode, @"AWTRunLoopMode", nil];
	[pVCLLoadNativeFonts performSelectorOnMainThread:@selector(loadNativeFonts:) withObject:pVCLLoadNativeFonts waitUntilDone:YES modes:pModes];
	Application::AcquireSolarMutex( nCount );

	[pPool release];

	return 0;
}

// -----------------------------------------------------------------------

JavaImplFontData::JavaImplFontData( const ImplDevFontAttributes& rAttributes, const OUString& rFontName, sal_IntPtr nNativeFontID, const OUString& rFamilyName ) : ImplFontData( rAttributes, 0 ), maFontName( rFontName ), mnNativeFontID( nNativeFontID ), maFamilyName( rFamilyName )
{
#ifdef USE_CORETEXT_TEXT_RENDERING
	if ( mnNativeFontID )
		CFRetain( (CTFontRef)mnNativeFontID );

	JavaImplFontData::maInstancesMap[ this ] = this;
#endif	// USE_CORETEXT_TEXT_RENDERING

	// [ed] 11/1/04 Scalable fonts should always report their width and height
	// as zero. The single size zero causes higher-level font elements to treat
	// fonts as infinitely scalable and provide lists of default font sizes.
	// The size of zero matches the unx implementation. Bug 196.
	SetBitmapSize( 0, 0 );
}

// -----------------------------------------------------------------------

JavaImplFontData::~JavaImplFontData()
{
#ifdef USE_CORETEXT_TEXT_RENDERING
	::std::map< JavaImplFontData*, JavaImplFontData* >::iterator it = JavaImplFontData::maInstancesMap.find( this );
	if ( it != JavaImplFontData::maInstancesMap.end() )
		JavaImplFontData::maInstancesMap.erase( it );

	if ( mnNativeFontID )
		CFRelease( (CTFontRef)mnNativeFontID );
#endif	// USE_CORETEXT_TEXT_RENDERING

	while ( maChildren.size() )
	{
		delete maChildren.front();
		maChildren.pop_front();
	}
}

// -----------------------------------------------------------------------

ImplFontEntry* JavaImplFontData::CreateFontInstance( ImplFontSelectData& rData ) const
{
    return new ImplFontEntry( rData );
}

// -----------------------------------------------------------------------

ImplFontData* JavaImplFontData::Clone() const
{
	return new JavaImplFontData( *this, maFontName, mnNativeFontID, maFamilyName );
}

// -----------------------------------------------------------------------

sal_IntPtr JavaImplFontData::GetFontId() const
{
	return mnNativeFontID;
}

// =======================================================================

void JavaSalGraphics::SetTextColor( SalColor nSalColor )
{
	mnTextColor = nSalColor | 0xff000000;
}

// -----------------------------------------------------------------------

USHORT JavaSalGraphics::SetFont( ImplFontSelectData* pFont, int nFallbackLevel )
{
	if ( !pFont || !pFont->mpFontData )
		return SAL_SETFONT_BADFONT;

	SalData *pSalData = GetSalData();

	const JavaImplFontData *pFontData = dynamic_cast<const JavaImplFontData *>( pFont->mpFontData );
	if ( !pFontData )
	{
		if ( pSalData->maJavaFontNameMapping.size() )
		{
			pFontData = pSalData->maJavaFontNameMapping.begin()->second;
		}
		else
		{
			// We should never get here as there should always be at least one
			// font
			return SAL_SETFONT_BADFONT;
		}
	}

	if ( nFallbackLevel )
	{
		// Retrieve the fallback font if one has been set by a text layout
		::std::hash_map< int, JavaImplFont* >::const_iterator ffit = maFallbackFonts.find( nFallbackLevel );
		if ( ffit != maFallbackFonts.end() )
		{
			sal_IntPtr nNativeFont = ffit->second->getNativeFont();
			::std::hash_map< sal_IntPtr, JavaImplFontData* >::const_iterator it = pSalData->maNativeFontMapping.find( nNativeFont );
			if ( it != pSalData->maNativeFontMapping.end() )
				pFontData = it->second;
		}
	}

	// Fix bugs 1813, 2964, 2968, 2971, and 2972 by tryng to find a matching
	// bold and/or italic font even if we are in a fallback level
	BOOL bAddBold;
	BOOL bAddItalic;
	if ( !nFallbackLevel )
	{
		bAddBold = ( pFont->GetWeight() > WEIGHT_MEDIUM && pFontData->GetWeight() <= WEIGHT_MEDIUM );
		bAddItalic = ( ( pFont->GetSlant() == ITALIC_OBLIQUE || pFont->GetSlant() == ITALIC_NORMAL ) && pFontData->GetSlant() != ITALIC_OBLIQUE && pFontData->GetSlant() != ITALIC_NORMAL );
	}
	else
	{
		bAddBold = ( mnFontWeight > WEIGHT_MEDIUM );
		bAddItalic = mbFontItalic;
	}

	if ( nFallbackLevel || bAddBold || bAddItalic )
	{
		const JavaImplFontData *pOldFontData = pFontData;
		sal_IntPtr nOldNativeFont = pOldFontData->GetFontId();

		// Remove any bold or italic variants so that we don't get drifting to
		// bold or italic in fallback levels where none was requested
		if ( nFallbackLevel )
		{
			::std::hash_map< sal_IntPtr, JavaImplFontData* >::const_iterator pfit = pSalData->maPlainNativeFontMapping.find( pFontData->GetFontId() );
			if ( pfit != pSalData->maPlainNativeFontMapping.end() )
				pFontData = pfit->second;
		}

		pFontData = ImplGetFontVariant( pFontData, bAddBold, bAddItalic );

		int nNativeFont = pFontData->GetFontId();
		if ( nNativeFont != nOldNativeFont )
		{
			if ( nFallbackLevel )
			{
				// Avoid selecting a font that has already been used
				for ( ::std::hash_map< int, JavaImplFont* >::const_iterator ffit = maFallbackFonts.begin(); ffit != maFallbackFonts.end(); ++ffit )
				{
					if ( ffit->first < nFallbackLevel && ffit->second->getNativeFont() == nNativeFont )
					{
						pFontData = pOldFontData;
						break;
					}
				}
			}
		}
		else
		{
			pFontData = pOldFontData;
		}
	}

	// Check that the font still exists as it might have been disabled or
	// removed by the ATS server
	::std::hash_map< sal_IntPtr, JavaImplFontData* >::const_iterator nfit = pSalData->maNativeFontMapping.find( pFont->mpFontData->GetFontId() );
	if ( nfit == pSalData->maNativeFontMapping.end() )
	{
		::std::hash_map< OUString, JavaImplFontData*, OUStringHash >::const_iterator jfnit = pSalData->maJavaFontNameMapping.find( pFontData->maFontName );
		if ( jfnit != pSalData->maJavaFontNameMapping.end() )
		{
			pFontData = jfnit->second;
		}
		else if ( pSalData->maJavaFontNameMapping.size() )
		{
			pFontData = pSalData->maJavaFontNameMapping.begin()->second;
		}
		else
		{
			// We should never get here as there should always be at least one
			// font
			return SAL_SETFONT_BADFONT;
		}
	}

	::std::hash_map< int, JavaImplFont* >::iterator ffit = maFallbackFonts.find( nFallbackLevel );
	if ( ffit != maFallbackFonts.end() )
	{
		delete ffit->second;
		maFallbackFonts.erase( ffit );
	}

	maFallbackFonts[ nFallbackLevel ] = new JavaImplFont( pFontData->maFontName, pFont->mfExactHeight, pFont->mnOrientation, !pFont->mbNonAntialiased, pFont->mbVertical, pFont->mnWidth ? (double)pFont->mnWidth / (double)pFont->mfExactHeight : 1.0 );

	// Update the native font as Java may be using a different font
	pFontData->mnNativeFontID = maFallbackFonts[ nFallbackLevel ]->getNativeFont();

	if ( !nFallbackLevel )
	{
		// Set font data for graphics device
		if ( mpFontData )
			delete mpFontData;
		mpFontData = (JavaImplFontData *)pFontData->Clone();

		// Set font for graphics device
		sal_IntPtr nOldNativeFont = 0;
		if ( mpFont )
		{
			nOldNativeFont = mpFont->getNativeFont();
			delete mpFont;
		}
		mpFont = new JavaImplFont( maFallbackFonts[ nFallbackLevel ] );

		mnFontFamily = pFont->GetFamilyType();
		mnFontWeight = pFont->GetWeight();
		mbFontItalic = ( pFont->GetSlant() == ITALIC_OBLIQUE || pFont->GetSlant() == ITALIC_NORMAL );
		mnFontPitch = pFont->GetPitch();

		// Fix bug 3446 by checking if the new font is a bad font
		if ( mpFont->getNativeFont() != nOldNativeFont )
		{
			// If the font is a bad font, select a different font
			ImplFontMetricData aMetricData( *pFont );
			GetFontMetric( &aMetricData );
			::std::map< sal_IntPtr, sal_IntPtr >::const_iterator bit = JavaImplFontData::maBadNativeFontIDMap.find( mpFont->getNativeFont() );
			if ( bit != JavaImplFontData::maBadNativeFontIDMap.end() )
			{
				for ( ::std::hash_map< OUString, JavaImplFontData*, OUStringHash >::const_iterator jfnit = pSalData->maJavaFontNameMapping.begin(); jfnit != pSalData->maJavaFontNameMapping.end(); ++jfnit )
				{
					pFontData = ImplGetFontVariant( jfnit->second, bAddBold, bAddItalic );

					// Reset font
					delete maFallbackFonts[ nFallbackLevel ];
					delete mpFont;
					delete mpFontData;
					maFallbackFonts[ nFallbackLevel ] = new JavaImplFont( pFontData->maFontName, pFont->mfExactHeight, pFont->mnOrientation, !pFont->mbNonAntialiased, pFont->mbVertical, pFont->mnWidth ? (double)pFont->mnWidth / (double)pFont->mfExactHeight : 1.0 );
					mpFont = new JavaImplFont( maFallbackFonts[ nFallbackLevel ] );
					mpFontData = (JavaImplFontData *)pFontData->Clone();

					GetFontMetric( &aMetricData );
					bit = JavaImplFontData::maBadNativeFontIDMap.find( mpFont->getNativeFont() );
					if ( bit == JavaImplFontData::maBadNativeFontIDMap.end() )
						break;
				}
			}
		}

		// Clone the new font data and make it a child of the requested font
		// data so that it will eventually get deleted
		if ( pFont->mpFontData != pFontData )
		{
			JavaImplFontData *pChildFontData = (JavaImplFontData *)pFontData->Clone();
			if ( pChildFontData )
			{
				((JavaImplFontData *)pFont->mpFontData)->maChildren.push_back( pChildFontData );
				pFont->mpFontData = pChildFontData;
			}
		}
	}
	else
	{
		// No need to clone as the select data is merely temporary data in
		// fallback levels
		pFont->mpFontData = pFontData;
	}

	return 0;
}

// -----------------------------------------------------------------------

void JavaSalGraphics::GetFontMetric( ImplFontMetricData* pMetric )
{
	if ( mpFont )
	{
		// Fix bug 3446 by only overriding the width if it is positive
		long nWidth = (long)( mpFont->getSize() + 0.5 );
		if ( nWidth >= 0 )
			pMetric->mnWidth = nWidth;
		pMetric->mnOrientation = mpFont->getOrientation();
	}

	if ( mpFontData )
	{
		if ( pMetric->mnWidth )
		{
#ifdef USE_CORETEXT_TEXT_RENDERING
			CTFontRef aFont = CTFontCreateCopyWithAttributes( (CTFontRef)mpFontData->mnNativeFontID, pMetric->mnWidth, NULL, NULL );
			if ( aFont )
			{
				// Mac OS X seems to overstate the leading for some fonts
				// (usually CJK fonts like Hiragino) so fix fix bugs 2827 and
				// 2847 by adding combining the leading with descent
				pMetric->mnAscent = (long)( CTFontGetAscent( aFont ) + 0.5 );
				// Fix bug 2881 by handling cases where font does not have
				// negative descent
				pMetric->mnDescent = (long)( CTFontGetLeading( aFont ) + fabs( CTFontGetDescent( aFont ) ) + 0.5 );
				if ( pMetric->mnDescent < 0 )
					pMetric->mnDescent = 0;

				CFRelease( aFont );
			}
#else	// USE_CORETEXT_TEXT_RENDERING
			ATSFontMetrics aFontMetrics;
			ATSFontRef aFont = SalATSLayout::GetATSFontRefFromNativeFont( mpFontData->mnNativeFontID );
			if ( ATSFontGetHorizontalMetrics( aFont, kATSOptionFlagsDefault, &aFontMetrics ) == noErr )
			{
				// Mac OS X seems to overstate the leading for some fonts
				// (usually CJK fonts like Hiragino) so fix fix bugs 2827 and
				// 2847 by adding combining the leading with descent
				pMetric->mnAscent = (long)( ( aFontMetrics.ascent * pMetric->mnWidth ) + 0.5 );
				// Fix bug 2881 by handling cases where font does not have
				// negative descent
				pMetric->mnDescent = (long)( ( ( aFontMetrics.leading + fabs( aFontMetrics.descent ) ) * pMetric->mnWidth ) + 0.5 );
				if ( pMetric->mnDescent < 0 )
					pMetric->mnDescent = 0;
			}
#endif	// USE_CORETEXT_TEXT_RENDERING
			else
			{
				// Fix bug 3446 by treating a font that don't have horizontal
				// metrics as a bad font
				JavaImplFontData::HandleBadFont( mpFontData );
			}

			if ( pMetric->mnAscent < 1 )
			{
				pMetric->mnAscent = pMetric->mnWidth - pMetric->mnDescent;
				if ( pMetric->mnAscent < 1 )
				{
					pMetric->mnAscent = pMetric->mnDescent;
					pMetric->mnDescent = 0;
				}
			}
		}

		pMetric->mbDevice = mpFontData->mbDevice;
		pMetric->mbScalableFont = true;
		pMetric->maName = mpFontData->GetFamilyName();
		pMetric->maStyleName = mpFontData->GetStyleName();
		pMetric->meWeight = mpFontData->GetWeight();
		pMetric->meFamily = mpFontData->GetFamilyType();
		pMetric->meItalic = mpFontData->GetSlant();
		pMetric->mePitch = mpFontData->GetPitch();
		pMetric->mbSymbolFlag = mpFontData->IsSymbolFont();
	}
	else
	{
		pMetric->mbDevice = false;
		pMetric->mbScalableFont = false;
		pMetric->maName = String();
		pMetric->maStyleName = String();
		pMetric->meWeight = WEIGHT_NORMAL;
		pMetric->meFamily = FAMILY_DONTKNOW;
		pMetric->meItalic = ITALIC_NONE;
		pMetric->mePitch = PITCH_VARIABLE;
		pMetric->mbSymbolFlag = false;
	}

	pMetric->mnIntLeading = 0;
	pMetric->mnExtLeading = 0;
	pMetric->mbKernableFont = false;
	pMetric->mnSlant = 0;
	pMetric->mnMinKashida = 0;
}

// -----------------------------------------------------------------------

ULONG JavaSalGraphics::GetKernPairs( ULONG nPairs, ImplKernPairData* pKernPairs )
{
	return 0;
}

// -----------------------------------------------------------------------

void JavaSalGraphics::GetDevFontList( ImplDevFontList* pList )
{
	// Only run the timer once since loading fonts is extremely expensive
	if ( !bNativeFontsLoaded )
	{
		// Invoke the native shutdown cancelled handler
		JavaSalEventQueue::setShutdownDisabled( sal_True );
		STATIC_LINK( NULL, JavaImplFontData, RunNativeFontsTimer ).Call( NULL );
		JavaSalEventQueue::setShutdownDisabled( sal_False );
	}

	SalData *pSalData = GetSalData();

	// Iterate through fonts and add each to the font list
	for ( ::std::map< String, JavaImplFontData* >::const_iterator it = pSalData->maFontNameMapping.begin(); it != pSalData->maFontNameMapping.end(); ++it )
		pList->Add( it->second->Clone() );
}

// -----------------------------------------------------------------------

BOOL JavaSalGraphics::GetGlyphBoundRect( long nIndex, Rectangle& rRect )
{
	rRect = Rectangle( Point( 0, 0 ), Size( 0, 0 ) );

	JavaImplFont *pFont = NULL;

	int nFallbackLevel = nIndex >> GF_FONTSHIFT;
	if ( !nFallbackLevel )
	{
		pFont = mpFont;
	}
	else
	{
		// Retrieve the fallback font if one has been set by a text layout
		::std::hash_map< int, JavaImplFont* >::const_iterator ffit = maFallbackFonts.find( nFallbackLevel );
		if ( ffit != maFallbackFonts.end() )
			pFont = ffit->second;
	}

	if ( pFont )
	{
#ifdef USE_CORETEXT_TEXT_RENDERING
		SalATSLayout::GetGlyphBounds( nIndex, pFont, rRect );
#else	// USE_CORETEXT_TEXT_RENDERING
#ifdef DEBUG
#if !defined USE_NATIVE_WINDOW || !defined USE_NATIVE_VIRTUAL_DEVICE || !defined USE_NATIVE_PRINTING
		if ( useNativeDrawing() )
#endif	// !USE_NATIVE_WINDOW || !USE_NATIVE_VIRTUAL_DEVICE || !USE_NATIVE_PRINTING
			fprintf( stderr, "JavaSalGraphics::GetGlyphBoundRect not implemented\n" );
#endif
#if !defined USE_NATIVE_WINDOW || !defined USE_NATIVE_VIRTUAL_DEVICE || !defined USE_NATIVE_PRINTING
		if ( mpVCLGraphics )
			rRect = mpVCLGraphics->getGlyphBounds( nIndex & GF_IDXMASK, pFont, nIndex & GF_ROTMASK );
#endif	// !USE_NATIVE_WINDOW || !USE_NATIVE_VIRTUAL_DEVICE || !USE_NATIVE_PRINTING
#endif	// USE_CORETEXT_TEXT_RENDERING
		rRect.Justify();
	}

	// Fix bug 2191 by always returning true so that the OOo code doesn't
	// exeecute its "draw the glyph and see which pixels are black" code
	return true;
}

// -----------------------------------------------------------------------

BOOL JavaSalGraphics::GetGlyphOutline( long nIndex, B2DPolyPolygon& rPolyPoly )
{
#ifdef DEBUG
	fprintf( stderr, "JavaSalGraphics::GetGlyphOutline not implemented\n" );
#endif
	rPolyPoly.clear();
	return FALSE;
}

// -----------------------------------------------------------------------

void JavaSalGraphics::GetDevFontSubstList( OutputDevice* pOutDev )
{
#ifdef DEBUG
	fprintf( stderr, "JavaSalGraphics::GetDevFontSubstList not implemented\n" );
#endif
}

// -----------------------------------------------------------------------

bool JavaSalGraphics::AddTempDevFont( ImplDevFontList* pList, const String& rFileURL, const String& rFontName )
{
#ifdef DEBUG
	fprintf( stderr, "JavaSalGraphics::AddTempDevFont not implemented\n" );
#endif
	return false;
}

// -----------------------------------------------------------------------

BOOL JavaSalGraphics::CreateFontSubset( const rtl::OUString& rToFile,
                                    const ImplFontData* pFont, long* pGlyphIDs,
                                    sal_uInt8* pEncoding, sal_Int32* pWidths,
                                    int nGlyphs, FontSubsetInfo& rInfo )
{
#ifdef DEBUG
	fprintf( stderr, "JavaSalGraphics::CreateFontSubset not implemented\n" );
#endif
	return FALSE;
}

// -----------------------------------------------------------------------

const void* JavaSalGraphics::GetEmbedFontData( const ImplFontData* pFont,
                                           const sal_Ucs* pUnicodes,
                                           sal_Int32* pWidths,
                                           FontSubsetInfo& rInfo,
                                           long* pDataLen )
{
#ifdef DEBUG
	fprintf( stderr, "JavaSalGraphics::GetEmbedFontData not implemented\n" );
#endif
	return NULL;
}

// -----------------------------------------------------------------------

void JavaSalGraphics::FreeEmbedFontData( const void* pData, long nLen )
{
#ifdef DEBUG
	fprintf( stderr, "JavaSalGraphics::FreeEmbedFontData not implemented\n" );
#endif
}

// -----------------------------------------------------------------------

void JavaSalGraphics::GetGlyphWidths( const ImplFontData* pFont, bool bVertical, Int32Vector& rWidths, Ucs2UIntMap& rUnicodeEnc )
{
#ifdef DEBUG
	fprintf( stderr, "JavaSalGraphics::GetGlyphWidths not implemented\n" );
#endif
	rWidths.clear();
	rUnicodeEnc.clear();
}

// -----------------------------------------------------------------------

const Ucs2SIntMap* JavaSalGraphics::GetFontEncodingVector( const ImplFontData*, const Ucs2OStrMap** ppNonEncoded )
{
#ifdef DEBUG
	fprintf( stderr, "JavaSalGraphics::GetFontEncodingVector not implemented\n" );
#endif
	if ( ppNonEncoded )
		*ppNonEncoded = NULL;
	return NULL;
}

// -----------------------------------------------------------------------

void JavaSalGraphics::DrawServerFontLayout( const ServerFontLayout& )
{
}

// -----------------------------------------------------------------------

ImplFontCharMap* JavaSalGraphics::GetImplFontCharMap() const
{
	return ImplFontCharMap::GetDefaultMap();
}
