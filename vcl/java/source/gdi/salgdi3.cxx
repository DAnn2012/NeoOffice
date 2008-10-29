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

#define _SV_SALGDI3_CXX

#ifndef _SV_SALGDI_H
#include <salgdi.h>
#endif
#ifndef _SV_SALATSLAYOUT_HXX
#include <salatslayout.hxx>
#endif
#ifndef _SV_SALDATA_HXX
#include <saldata.hxx>
#endif
#ifndef _SV_SALINST_H
#include <salinst.h>
#endif
#ifndef _SV_SALLAYOUT_HXX
#include <vcl/sallayout.hxx>
#endif
#ifndef _SV_IMPFONT_HXX
#include <vcl/impfont.hxx>
#endif
#ifndef _SV_OUTDEV_H
#include <vcl/outdev.h>
#endif
#ifndef _VCL_UNOHELP_HXX
#include <vcl/unohelp.hxx>
#endif
#ifndef _SV_COM_SUN_STAR_VCL_VCLGRAPHICS_HXX
#include <com/sun/star/vcl/VCLGraphics.hxx>
#endif
#ifndef _BGFX_POLYGON_B2DPOLYPOLYGON_HXX
#include <basegfx/polygon/b2dpolypolygon.hxx>
#endif
#ifndef _OSL_CONDITN_HXX_
#include <osl/conditn.hxx>
#endif
#ifndef _OSL_PROCESS_H_
#include <rtl/process.h>
#endif
#ifndef _VOS_MODULE_HXX_
#include <vos/module.hxx>
#endif

#include <premac.h>
#include <Carbon/Carbon.h>
#include <postmac.h>

#include "salgdi3_cocoa.h"

typedef void NativeShutdownCancelledHandler_Type();

static ATSFontNotificationRef aFontNotification = NULL;
static EventLoopTimerUPP pLoadNativeFontsTimerUPP = NULL;
static ::osl::Condition aLoadNativeFontsCondition;
static ::vos::OModule aShutdownCancelledHandlerModule;
static NativeShutdownCancelledHandler_Type *pShutdownCancelledHandler = NULL;

using namespace basegfx;
using namespace rtl;
using namespace vcl;
using namespace vos;

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
			for ( ::std::map< String, JavaImplFontData* >::const_iterator fnit = pSalData->maFontNameMapping.begin(); fnit != pSalData->maFontNameMapping.end(); ++fnit )
				delete fnit->second;
			pSalData->maFontNameMapping.clear();
			pSalData->maNativeFontMapping.clear();
			pSalData->maJavaFontNameMapping.clear();
			pSalData->maBoldNativeFontMapping.clear();
			pSalData->maItalicNativeFontMapping.clear();
			pSalData->maBoldItalicNativeFontMapping.clear();
			SalATSLayout::ClearLayoutDataCache();

			if ( !Application::IsShutDown() )
			{
				VCLThreadAttach t;
				if ( t.pEnv )
				{
					// Update cached fonts
					long *pFonts = NSFontManager_getAllFonts();
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
						const OUString aStarSymbol( OUString::createFromAscii( "StarSymbol" ) );
						const OUString aTimes( OUString::createFromAscii( "Times" ) );
						const OUString aTimesRoman( OUString::createFromAscii( "Times Roman" ) );

						int i;
						int nCount = 0;
						for ( i = 0; pFonts[ i ]; i++ )
							nCount++;

						sal_uInt32 nActualCount = 0;
						ATSUFontID aATSUFonts[ nCount ];
						for ( i = 0; i < nCount; i++ )
						{
							void *pNSFont = (void *)pFonts[ i ];

							ATSFontRef aFont = NSFont_getATSFontRef( pNSFont );
							if ( !aFont )
								continue;

							// Get font attributes
							FontWeight nWeight = (FontWeight)NSFontManager_weightOfFont( pNSFont );
							FontItalic nItalic = ( NSFontManager_isItalic( pNSFont ) ? ITALIC_NORMAL : ITALIC_NONE );
							FontWidth nWidth = (FontWidth)NSFontManager_widthOfFont( pNSFont );
							FontPitch nPitch = ( NSFontManager_isFixedPitch( pNSFont ) ? PITCH_FIXED : PITCH_VARIABLE );

							CFStringRef aPSString;
							if ( ATSFontGetPostScriptName( aFont, kATSOptionFlagsDefault, &aPSString ) != noErr )
								continue;

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

							sal_IntPtr nNativeFont = (sal_IntPtr)FMGetFontFromATSFontRef( aFont );
							if ( (ATSUFontID)nNativeFont == kATSUInvalidFontID )
								continue;

							// Get the ATS font name as the Cocoa name on some
							// Mac OS X versions adds extraneous words
							CFStringRef aDisplayString;
							if ( ATSFontGetName( aFont, kATSOptionFlagsDefault, &aDisplayString ) != noErr )
								continue;

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

							JavaImplFontData *pFontData = new JavaImplFontData( aAttributes, aPSName, nNativeFont );
							pSalData->maFontNameMapping[ aXubDisplayName ] = pFontData;

							// Multiple native fonts can map to the same font
							// due to disabling and reenabling of fonts with
							// the same name. Also, note that multiple font
							// names can map to a single native font so do not
							// rely on the native font to look up the font name.
							pSalData->maNativeFontMapping[ nNativeFont ] = pFontData;
							pSalData->maJavaFontNameMapping[ aPSName ] = pFontData;

							aATSUFonts[ nActualCount++ ] = nNativeFont;
						}

						// Cache matching bold, italic, and bold italic fonts
						for ( i = 0; i < nCount; i++ )
						{
							void *pNSFont = (void *)pFonts[ i ];

							ATSFontRef aFont = NSFont_getATSFontRef( pNSFont );
							if ( !aFont )
								continue;

							sal_IntPtr nNativeFont = (sal_IntPtr)FMGetFontFromATSFontRef( aFont );
							if ( (ATSUFontID)nNativeFont == kATSUInvalidFontID )
								continue;

							::std::hash_map< sal_IntPtr, JavaImplFontData* >::const_iterator nfit = pSalData->maNativeFontMapping.find( nNativeFont );
							if ( nfit == pSalData->maNativeFontMapping.end() )
								continue;

							JavaImplFontData *pFontData = nfit->second;
							bool bIsPlainFont = ( pFontData->meWeight <= WEIGHT_MEDIUM && pFontData->meItalic != ITALIC_OBLIQUE && pFontData->meItalic != ITALIC_NORMAL );

							// Try bold
							ATSFontRef aBoldFont = NSFont_findATSFontWithStyle( pNSFont, TRUE, FALSE );
							if ( aBoldFont )
							{
								sal_IntPtr nBoldNativeFont = (sal_IntPtr)FMGetFontFromATSFontRef( aBoldFont );
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
							}

							// Try italic
							ATSFontRef aItalicFont = NSFont_findATSFontWithStyle( pNSFont, FALSE, TRUE );
							if ( aItalicFont )
							{
								sal_IntPtr nItalicNativeFont = (sal_IntPtr)FMGetFontFromATSFontRef( aItalicFont );
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
							}

							// Try bold italic
							ATSFontRef aBoldItalicFont = NSFont_findATSFontWithStyle( pNSFont, TRUE, TRUE );
							if ( aBoldItalicFont )
							{
								sal_IntPtr nBoldItalicNativeFont = (sal_IntPtr)FMGetFontFromATSFontRef( aBoldItalicFont );
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
							}
						}

						NSFontManager_releaseAllFonts( pFonts );
					}
				}
			}

			// Fix bug 3095 by handling font change notifications
			if ( !aFontNotification )
				ATSFontNotificationSubscribe( ImplFontListChangedCallback, kATSFontNotifyOptionDefault, NULL, &aFontNotification );

			SalATSLayout::SetFontFallbacks();
			OutputDevice::ImplUpdateAllFontData( true );

			rSolarMutex.release();
		}
	}

	bInLoad = false;
}

// -----------------------------------------------------------------------

static void LoadNativeFontsTimerCallback( EventLoopTimerRef aTimer, void *pData )
{
	ImplFontListChangedCallback( NULL, NULL );

	// Release any waiting thread
	aLoadNativeFontsCondition.set();
}

// =======================================================================

JavaImplFontData::JavaImplFontData( const ImplDevFontAttributes& rAttributes, OUString aVCLFontName, sal_IntPtr nATSUFontID ) : ImplFontData( rAttributes, 0 ), maVCLFontName( aVCLFontName ), mnATSUFontID( nATSUFontID )
{

	// [ed] 11/1/04 Scalable fonts should always report their width and height
	// as zero. The single size zero causes higher-level font elements to treat
	// fonts as infinitely scalable and provide lists of default font sizes.
	// The size of zero matches the unx implementation. Bug 196.
	SetBitmapSize( 0, 0 );
}

// -----------------------------------------------------------------------

JavaImplFontData::~JavaImplFontData()
{
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
	return new JavaImplFontData( *this, maVCLFontName, mnATSUFontID );
}

// -----------------------------------------------------------------------

sal_IntPtr JavaImplFontData::GetFontId() const
{
	return mnATSUFontID;
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

	const JavaImplFontData *pFontData = dynamic_cast<const JavaImplFontData *>( pFont->mpFontData );
	if ( !pFontData )
		return SAL_SETFONT_BADFONT;

	SalData *pSalData = GetSalData();

	if ( nFallbackLevel )
	{
		// Retrieve the fallback font if one has been set by a text layout
		::std::hash_map< int, com_sun_star_vcl_VCLFont* >::const_iterator ffit = maFallbackFonts.find( nFallbackLevel );
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

		int nNativeFont = pFontData->GetFontId();
		if ( nNativeFont != nOldNativeFont )
		{
			if ( nFallbackLevel )
			{
				// Avoid selecting a font that has already been used
				for ( ::std::hash_map< int, com_sun_star_vcl_VCLFont* >::const_iterator ffit = maFallbackFonts.begin(); ffit != maFallbackFonts.end(); ++ffit )
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
		::std::hash_map< OUString, JavaImplFontData*, OUStringHash >::const_iterator jfnit = pSalData->maJavaFontNameMapping.find( pFontData->maVCLFontName );
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

	::std::hash_map< int, com_sun_star_vcl_VCLFont* >::iterator ffit = maFallbackFonts.find( nFallbackLevel );
	if ( ffit != maFallbackFonts.end() )
	{
		delete ffit->second;
		maFallbackFonts.erase( ffit );
	}

	maFallbackFonts[ nFallbackLevel ] = new com_sun_star_vcl_VCLFont( pFontData->maVCLFontName, pFont->mnHeight, pFont->mnOrientation, !pFont->mbNonAntialiased, pFont->mbVertical, pFont->mnWidth ? (double)pFont->mnWidth / (double)pFont->mnHeight : 1.0 );

	// Update the native font as Java may be using a different font
	pFontData->mnATSUFontID = maFallbackFonts[ nFallbackLevel ]->getNativeFont();

	if ( !nFallbackLevel )
	{
		// Set font data for graphics device
		if ( mpFontData )
			delete mpFontData;
		mpFontData = (JavaImplFontData *)pFontData->Clone();

		// Set font for graphics device
		if ( mpVCLFont )
			delete mpVCLFont;
		mpVCLFont = new com_sun_star_vcl_VCLFont( maFallbackFonts[ nFallbackLevel ] );

		mnFontFamily = pFont->GetFamilyType();
		mnFontWeight = pFont->GetWeight();
		mbFontItalic = ( pFont->GetSlant() == ITALIC_OBLIQUE || pFont->GetSlant() == ITALIC_NORMAL );
		mnFontPitch = pFont->GetPitch();

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
	if ( mpVCLFont )
	{
		pMetric->mnWidth = mpVCLFont->getSize();
		pMetric->mnOrientation = mpVCLFont->getOrientation();
	}
	else
	{
		pMetric->mnWidth = 0;
		pMetric->mnOrientation = 0;
	}

	if ( mpFontData )
	{
		if ( pMetric->mnWidth )
		{
			ATSFontMetrics aFontMetrics;
			ATSFontRef aFont = FMGetATSFontRefFromFont( mpFontData->mnATSUFontID );
			if ( ATSFontGetHorizontalMetrics( aFont, kATSOptionFlagsDefault, &aFontMetrics ) == noErr )
			{
				// Mac OS X seems to overstate the leading for some fonts
				// (usually CJK fonts like Hiragino) so fix fix bugs 2827 and
				// 2847 by adding combining the leading with descent
				pMetric->mnAscent = (long)( ( aFontMetrics.ascent * pMetric->mnWidth ) + 0.5 );
				if ( pMetric->mnAscent < 1 )
					pMetric->mnAscent = 1;
				// Fix bug 2881 by handling cases where font does not have
				// negative descent
				pMetric->mnDescent = (long)( ( ( aFontMetrics.leading + fabs( aFontMetrics.descent ) ) * pMetric->mnWidth ) + 0.5 );
				if ( pMetric->mnDescent < 0 )
					pMetric->mnDescent = 0;
			}
			else
			{
				pMetric->mnAscent = 0;
				pMetric->mnDescent = 0;
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
		pMetric->mnAscent = 0;
		pMetric->mnDescent = 0;
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
}

// -----------------------------------------------------------------------

ULONG JavaSalGraphics::GetKernPairs( ULONG nPairs, ImplKernPairData* pKernPairs )
{
	return 0;
}

// -----------------------------------------------------------------------

void JavaSalGraphics::GetDevFontList( ImplDevFontList* pList )
{
	SalData *pSalData = GetSalData();

	// Only run the timer once since loading fonts is extremely expensive
	if ( !pLoadNativeFontsTimerUPP )
	{
		pSalData->mpEventQueue->setShutdownDisabled( sal_True );

		// Load libsfx and invoke the native shutdown cancelled handler
		if ( !pShutdownCancelledHandler )
		{
			OUString aLibName = ::vcl::unohelper::CreateLibraryName( "sfx", TRUE );
			if ( aShutdownCancelledHandlerModule.load( aLibName ) )
				pShutdownCancelledHandler = (NativeShutdownCancelledHandler_Type *)aShutdownCancelledHandlerModule.getSymbol( OUString::createFromAscii( "NativeShutdownCancelledHandler" ) );
		}

		if ( pShutdownCancelledHandler )
			pShutdownCancelledHandler();

		pLoadNativeFontsTimerUPP = NewEventLoopTimerUPP( LoadNativeFontsTimerCallback );
		if ( pLoadNativeFontsTimerUPP )
		{
			if ( GetCurrentEventLoop() != GetMainEventLoop() )
			{
				aLoadNativeFontsCondition.reset();
				InstallEventLoopTimer( GetMainEventLoop(), 0.001, kEventDurationForever, pLoadNativeFontsTimerUPP, NULL, NULL );
				ULONG nCount = Application::ReleaseSolarMutex();
				aLoadNativeFontsCondition.wait();
				Application::AcquireSolarMutex( nCount );
			}
			else
			{
				LoadNativeFontsTimerCallback( NULL, NULL );
			}
		}

		pSalData->mpEventQueue->setShutdownDisabled( sal_False );
	}

	// Iterate through fonts and add each to the font list
	for ( ::std::map< String, JavaImplFontData* >::const_iterator it = pSalData->maFontNameMapping.begin(); it != pSalData->maFontNameMapping.end(); ++it )
		pList->Add( it->second->Clone() );
}

// -----------------------------------------------------------------------

BOOL JavaSalGraphics::GetGlyphBoundRect( long nIndex, Rectangle& rRect )
{
	rRect = Rectangle( Point( 0, 0 ), Size( 0, 0 ) );

	com_sun_star_vcl_VCLFont *pVCLFont = NULL;

	int nFallbackLevel = nIndex >> GF_FONTSHIFT;
	if ( !nFallbackLevel )
	{
		pVCLFont = mpVCLFont;
	}
	else
	{
		// Retrieve the fallback font if one has been set by a text layout
		::std::hash_map< int, com_sun_star_vcl_VCLFont* >::const_iterator ffit = maFallbackFonts.find( nFallbackLevel );
		if ( ffit != maFallbackFonts.end() )
			pVCLFont = ffit->second;
	}

	if ( pVCLFont )
	{
		rRect = mpVCLGraphics->getGlyphBounds( nIndex & GF_IDXMASK, pVCLFont, nIndex & GF_ROTMASK );
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
