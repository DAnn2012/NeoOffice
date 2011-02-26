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

#define _SV_COM_SUN_STAR_VCL_VCLGRAPHICS_CXX

#include <list>
#include <map>

#ifndef _SV_SALBMP_H
#include <salbmp.h>
#endif
#ifndef _SV_SALDATA_HXX
#include <saldata.hxx>
#endif
#ifndef _SV_JAVA_LANG_CLASS_HXX
#include <java/lang/Class.hxx>
#endif
#ifndef _SV_COM_SUN_STAR_VCL_VCLBITMAP_HXX
#include <com/sun/star/vcl/VCLBitmap.hxx>
#endif
#ifndef _SV_COM_SUN_STAR_VCL_VCLGRAPHICS_HXX
#include <com/sun/star/vcl/VCLGraphics.hxx>
#endif
#ifndef _SV_COM_SUN_STAR_VCL_VCLFONT_HXX
#include <com/sun/star/vcl/VCLFont.hxx>
#endif
#ifndef _SV_COM_SUN_STAR_VCL_VCLIMAGE_HXX
#include <com/sun/star/vcl/VCLImage.hxx>
#endif
#ifndef _SV_COM_SUN_STAR_VCL_VCLPATH_HXX
#include <com/sun/star/vcl/VCLPath.hxx>
#endif
#include <basegfx/vector/b2enums.hxx>

#include "VCLGraphics_cocoa.h"

static ::std::list< CGImageRef > aCGImageList;
static ::std::list< CGPathRef > aCGPathList;
static ::osl::Mutex aBitmapBufferMutex;
static ::std::map< BitmapBuffer*, USHORT > aBitmapBufferMap;
static ::std::list< jlong > aEPSDataList;
static ::std::list< jlong > aGlyphDataList;
static ::std::map< sal_IntPtr, CGFontRef > aNativeFontMap;

using namespace osl;
using namespace rtl;
using namespace vcl;

// ============================================================================

static void CacheCGPath( CGPathRef aPath )
{
	if ( aPath )
	{
		bool bFound = false;
		for ( ::std::list< CGPathRef >::const_iterator it = aCGPathList.begin(); it != aCGPathList.end(); ++it )
		{
			if ( *it == aPath )
			{
				bFound = true;
				break;
			}
		}

		if ( !bFound )
			aCGPathList.push_back( aPath );
	}
}

// ----------------------------------------------------------------------------
 
static void CacheBitmapBuffer( BitmapBuffer *pBuffer )
{
	if ( pBuffer )
	{
		MutexGuard aGuard( aBitmapBufferMutex );
		::std::map< BitmapBuffer*, USHORT >::iterator it = aBitmapBufferMap.find( pBuffer );
		if ( it != aBitmapBufferMap.end() )
			it->second++;
		else
			aBitmapBufferMap[ pBuffer ] = 1;
	}
}

// ----------------------------------------------------------------------------
 
static void CacheEPSData( jlong nData )
{
	if ( nData )
	{
		bool bFound = false;
		for ( ::std::list< jlong >::const_iterator eit = aEPSDataList.begin(); eit != aEPSDataList.end(); ++eit )
		{
			if ( *eit == nData )
			{
				bFound = true;
				break;
			}
		}

		if ( !bFound )
			aEPSDataList.push_back( nData );

	}
}

// ----------------------------------------------------------------------------
 
static void CacheGlyphData( jlong nData )
{
	if ( nData )
	{
		bool bFound = false;
		for ( ::std::list< jlong >::const_iterator it = aGlyphDataList.begin(); it != aGlyphDataList.end(); ++it )
		{
			if ( *it == nData )
			{
				bFound = true;
				break;
			}
		}

		if ( !bFound )
			aGlyphDataList.push_back( nData );
	}
}

// ----------------------------------------------------------------------------

static CGFontRef CacheCGFont( sal_IntPtr nFont )
{
	CGFontRef aRet = NULL;
#ifdef USE_CORETEXT_TEXT_RENDERING
	if ( !nFont )
#else	// USE_CORETEXT_TEXT_RENDERING
	if ( nFont != kATSUInvalidFontID )
#endif	// USE_CORETEXT_TEXT_RENDERING
	{
		::std::map< sal_IntPtr, CGFontRef >::const_iterator it = aNativeFontMap.find( nFont );
		if ( it == aNativeFontMap.end() )
		{
#ifdef USE_CORETEXT_TEXT_RENDERING
			CTFont aCTFont = CTFontCreateWithPlatformFont( (ATSFontRef)nFont, 0, NULL, NULL );
			if ( aCTFont )
			{
				aRet = CTFontCopyGraphicsFont( aCTFont, NULL );
				[CFRelease aCTFont];
			}
#else	// USE_CORETEXT_TEXT_RENDERING
			aRet = CGFontCreateWithPlatformFont( (void *)&nFont );
#endif	// USE_CORETEXT_TEXT_RENDERING

			if ( aRet )
				aNativeFontMap[ nFont ] = aRet;
		}
		else
		{
			aRet = it->second;
		}
	}

	return aRet;
}

// ----------------------------------------------------------------------------
 
static void ReleaseBytePointerCallback( void *pInfo, const void *pPointer, size_t nSize )
{
	BYTE *pBits = (BYTE *)pPointer;
	if ( pBits )
		delete[] pBits;
}

// ----------------------------------------------------------------------------
 
static void ReleaseBitmapBufferCallback( void *pInfo, const void *pPointer, size_t nSize )
{
	BitmapBuffer *pBuffer = (BitmapBuffer *)pInfo;
	if ( pBuffer )
	{
		MutexGuard aGuard( aBitmapBufferMutex );
		::std::map< BitmapBuffer*, USHORT >::iterator it = aBitmapBufferMap.find( pBuffer );
		if ( it != aBitmapBufferMap.end() )
		{
			it->second--;
			if ( !it->second )
			{
				aBitmapBufferMap.erase( it );
				if ( pBuffer->mpBits )
					delete[] pBuffer->mpBits;
				delete pBuffer;
			}
		}
	}
}

// ----------------------------------------------------------------------------

JNIEXPORT void JNICALL Java_com_sun_star_vcl_VCLGraphics_drawBitmap0( JNIEnv *pEnv, jobject object, jintArray _par0, jint _par1, jint _par2, jint _par3, jint _par4, jint _par5, jint _par6, jfloat _par7, jfloat _par8, jfloat _par9, jfloat _par10, jfloatArray _par11, jboolean _par12, jfloat _par13, jfloat _par14, jfloat _par15, jfloat _par16, jfloat _par17 )
{
	CGPathRef aPath = (CGPathRef)_par11;
	CacheCGPath( aPath );

	if ( !_par0 )
		return;

	float fScaleX = _par9 / _par5;
	float fScaleY = _par10 / _par6;

	// Adjust for negative source origin
	if ( _par3 < 0 )
	{
		_par7 -= fScaleX * _par3;
		_par9 += fScaleX * _par3;
		_par5 += _par3;
		_par3 = 0;
	}
	if ( _par4 < 0 )
	{
		_par8 -= fScaleY * _par4;
		_par10 += fScaleY * _par4;
		_par6 += _par4;
		_par4 = 0;
	}
	
	// Adjust for size outside of the source image
	jint nExtraWidth = _par3 + _par5 - _par1;
	if ( nExtraWidth > 0 )
	{
		_par9 -= fScaleX * nExtraWidth;
		_par5 -= nExtraWidth;
	}
	jint nExtraHeight = _par4 + _par6 - _par2;
	if ( nExtraHeight > 0 )
	{
		_par10 -= fScaleY * nExtraHeight;
		_par6 -= nExtraHeight;
	}

	jboolean bCopy( sal_False );
	jint *pJavaBits = (jint *)pEnv->GetPrimitiveArrayCritical( _par0, &bCopy );
	if ( !pJavaBits )
		return;

	size_t nRowSize = _par5 * sizeof( jint );
	size_t nSize = nRowSize * _par6;
	jint *pCGBits = new jint[ _par5 * _par6 ];
	if ( pCGBits )
	{
		// Copy the subimage
		jint *pBitsIn = pJavaBits + ( _par4 * _par1 ) + _par3;
		jint *pBitsOut = pCGBits;
		for ( jint i = 0; i < _par6; i++ )
		{
			memcpy( pBitsOut, pBitsIn, nRowSize );
			pBitsIn += _par1;
			pBitsOut += _par5;
		}

		pEnv->ReleasePrimitiveArrayCritical( _par0, pJavaBits, JNI_ABORT );
	}
	else
	{
		pEnv->ReleasePrimitiveArrayCritical( _par0, pJavaBits, JNI_ABORT );
		return;
	}

	CGDataProviderRef aProvider = CGDataProviderCreateWithData( NULL, pCGBits, nSize, ReleaseBytePointerCallback );
	if ( !aProvider )
	{
		delete[] pCGBits;
		return;
	}

	CGColorSpaceRef aColorSpace = CGColorSpaceCreateDeviceRGB();
	if ( !aColorSpace )
	{
		CGDataProviderRelease( aProvider );
		return;
	}

#ifdef POWERPC
	CGImageRef aImage = CGImageCreate( _par5, _par6, 8, sizeof( jint ) * 8, nRowSize, aColorSpace, kCGImageAlphaPremultipliedFirst, aProvider, NULL, false, kCGRenderingIntentDefault );
#else	// POWERPC
	CGImageRef aImage = CGImageCreate( _par5, _par6, 8, sizeof( jint ) * 8, nRowSize, aColorSpace, kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little, aProvider, NULL, false, kCGRenderingIntentDefault );
#endif	// POWERPC
	CGColorSpaceRelease( aColorSpace );
	CGDataProviderRelease( aProvider );

	if ( aImage )
	{
		aCGImageList.push_back( aImage );
		CGImageRef_drawInRect( aImage, _par7, _par8, _par9, _par10, aPath, _par12, _par13, _par14, _par15, _par16, _par17 );
	}
}

// ----------------------------------------------------------------------------

JNIEXPORT void JNICALL Java_com_sun_star_vcl_VCLGraphics_drawBitmapBuffer0( JNIEnv *pEnv, jobject object, jlong _par0, jint _par1, jint _par2, jint _par3, jint _par4, jfloat _par5, jfloat _par6, jfloat _par7, jfloat _par8, jlong _par9, jboolean _par10, jfloat _par11, jfloat _par12, jfloat _par13, jfloat _par14, jfloat _par15 )
{
	CGPathRef aPath = (CGPathRef)_par9;
	CacheCGPath( aPath );

	BitmapBuffer *pBuffer = (BitmapBuffer *)_par0;
	if ( !pBuffer )
		return;

	if ( !pBuffer->mpBits )
	{
		delete pBuffer;
		return;
	}

	float fScaleX = _par7 / _par3;
	float fScaleY = _par8 / _par4;

	// Adjust for negative source origin
	if ( _par1 < 0 )
	{
		_par5 -= fScaleX * _par1;
		_par7 += fScaleX * _par1;
		_par3 += _par1;
		_par1 = 0;
	}
	if ( _par2 < 0 )
	{
		_par6 -= fScaleY * _par2;
		_par8 += fScaleY * _par2;
		_par4 += _par2;
		_par2 = 0;
	}
	
	// Adjust for size outside of the source image
	long nWidth = pBuffer->mnWidth;
	long nHeight = pBuffer->mnHeight;
	jint nExtraWidth = _par1 + _par3 - nWidth;
	if ( nExtraWidth > 0 )
	{
		_par7 -= fScaleX * nExtraWidth;
		_par3 -= nExtraWidth;
	}
	jint nExtraHeight = _par2 + _par4 - nHeight;
	if ( nExtraHeight > 0 )
	{
		_par8 -= fScaleY * nExtraHeight;
		_par4 -= nExtraHeight;
	}

	BYTE *pCGBits = pBuffer->mpBits + ( _par2 * pBuffer->mnScanlineSize ) + ( _par1 * sizeof( jint ) );
	CGDataProviderRef aProvider = CGDataProviderCreateWithData( pBuffer, pCGBits, pBuffer->mnScanlineSize * ( _par4 - _par2 ), ReleaseBitmapBufferCallback );
	if ( !aProvider )
	{
		delete[] pBuffer->mpBits;
		delete pBuffer;
		return;
	}

	CacheBitmapBuffer( pBuffer );

	CGColorSpaceRef aColorSpace = CGColorSpaceCreateDeviceRGB();
	if ( !aColorSpace )
	{
		CGDataProviderRelease( aProvider );
		return;
	}

#ifdef POWERPC
	CGImageRef aImage = CGImageCreate( _par3, _par4, 8, sizeof( jint ) * 8, pBuffer->mnScanlineSize, aColorSpace, kCGImageAlphaPremultipliedFirst, aProvider, NULL, false, kCGRenderingIntentDefault );
#else	// POWERPC
	CGImageRef aImage = CGImageCreate( _par3, _par4, 8, sizeof( jint ) * 8, pBuffer->mnScanlineSize, aColorSpace, kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little, aProvider, NULL, false, kCGRenderingIntentDefault );
#endif	// POWERPC
	CGColorSpaceRelease( aColorSpace );
	CGDataProviderRelease( aProvider );

	if ( aImage )
	{
		aCGImageList.push_back( aImage );
		CGImageRef_drawInRect( aImage, _par5, _par6, _par7, _par8, aPath, _par10, _par11, _par12, _par13, _par14, _par15 );
	}
}

// ----------------------------------------------------------------------------

JNIEXPORT void JNICALL Java_com_sun_star_vcl_VCLGraphics_notifyGraphicsChanged( JNIEnv *pEnv, jobject object, jlongArray _par0, jboolean _par1 )
{
	if ( _par0 )
	{
		jboolean bCopy = sal_False;
		jsize elements = pEnv->GetArrayLength( _par0 );
		if ( elements )
		{
			jlong *pBitmaps = (jlong *)pEnv->GetPrimitiveArrayCritical( _par0, &bCopy );
			if ( pBitmaps )
			{
				SalData *pSalData = GetSalData();
				for ( jsize i = 0; i < elements; i++ )
				{
					JavaSalBitmap *pBitmap = (JavaSalBitmap *)pBitmaps[ i ];
					if ( !pBitmap )
						continue;

					for ( ::std::list< JavaSalBitmap* >::const_iterator it = pSalData->maBitmapList.begin(); it != pSalData->maBitmapList.end(); ++it )
					{
						if ( *it == pBitmap )
						{
							pBitmap->NotifyGraphicsChanged( _par1 );
							break;
						}
					}
				}

				pEnv->ReleasePrimitiveArrayCritical( _par0, pBitmaps, JNI_ABORT );
			}
		}
	}
}

// ----------------------------------------------------------------------------

JNIEXPORT void JNICALL Java_com_sun_star_vcl_VCLGraphics_releaseNativeBitmaps( JNIEnv *pEnv, jobject object, jboolean _par0 )
{
	// Release the initial retain but use a copy of the map to ensure that the
	// map does not change while iterating through the items
	ClearableMutexGuard aBitmapBufferGuard( aBitmapBufferMutex );
	::std::map< BitmapBuffer*, USHORT > aBitmapBufferMapCopy( aBitmapBufferMap );
	for ( ::std::map< BitmapBuffer*, USHORT >::const_iterator bbit = aBitmapBufferMapCopy.begin(); bbit != aBitmapBufferMapCopy.end(); ++bbit )
		ReleaseBitmapBufferCallback( bbit->first, NULL, 0 );
	aBitmapBufferGuard.clear();

	while ( aCGImageList.size() )
	{
		CGImageRelease( aCGImageList.front() );
		aCGImageList.pop_front();
	}

	while ( aCGPathList.size() )
	{
		CGPathRelease( aCGPathList.front() );
		aCGPathList.pop_front();
	}

	while ( aGlyphDataList.size() )
	{
		rtl_freeMemory( (void *)aGlyphDataList.front() );
		aGlyphDataList.pop_front();
	}

	if ( _par0 )
	{
		// If any of the EPS images are really PDF, we need to keep it around
		// until the end of the print job
		while ( aEPSDataList.size() )
		{
			rtl_freeMemory( (void *)aEPSDataList.front() );
			aEPSDataList.pop_front();
		}

		// Reuse CGFonts throughout the entire document
		for ( ::std::map< sal_IntPtr, CGFontRef >::const_iterator it = aNativeFontMap.begin(); it != aNativeFontMap.end(); ++it )
			CGFontRelease( it->second );
		aNativeFontMap.clear();
	}
}

// ----------------------------------------------------------------------------

JNIEXPORT void JNICALL Java_com_sun_star_vcl_VCLGraphics_drawEPS0( JNIEnv *pEnv, jobject object, jlong _par0, jlong _par1, jfloat _par2, jfloat _par3, jfloat _par4, jfloat _par5, jlong _par6, jboolean _par7, jfloat _par8, jfloat _par9, jfloat _par10, jfloat _par11, jfloat _par12 )
{
	CGPathRef aPath = (CGPathRef)_par6;
	CacheCGPath( aPath );
	CacheEPSData( _par0 );

	if ( _par0 )
		NSEPSImageRep_drawInRect( (void *)_par0, _par1, _par2, _par3, _par4, _par5, aPath, _par7, _par8, _par9, _par10, _par11, _par12 );
}

// ----------------------------------------------------------------------------

JNIEXPORT void JNICALL Java_com_sun_star_vcl_VCLGraphics_drawGlyphBuffer0( JNIEnv *pEnv, jobject object, jint _par0, jint _par1, jint _par2, jlong _par3, jlong _par4, jint _par5, jfloat _par6, jint _par7, jfloat _par8, jfloat _par9, jfloat _par10, jfloat _par11, jfloat _par12, jlong _par13, jboolean _par14, jfloat _par15, jfloat _par16, jfloat _par17, jfloat _par18, jfloat _par19 )
{
	// Mark the glyph data for deletion in case the Java drawing method
	// never calls any of the native methods
	CacheGlyphData( _par3 );

	// Mark the advance data for deletion in case the Java drawing method
	// never calls any of the native methods
	CacheGlyphData( _par4 );

	CGPathRef aPath = (CGPathRef)_par13;
	CacheCGPath( aPath );

	// Convert and cache font as a CGFontRef to reduce the number of font
	// subsets in the PDF
	sal_IntPtr nFont = (sal_IntPtr)_par5;
	CGFontRef aFont = CacheCGFont( nFont );
	if ( aFont && _par3 && _par4 )
		CGContext_drawGlyphs( _par0, _par1, _par2, (CGGlyph *)_par3, (CGSize*)_par4, aFont, _par6, _par7, _par8, _par9, _par10, _par11, _par12, aPath, _par14, _par15, _par16, _par17, _par18, _par19 );
}

// ----------------------------------------------------------------------------

JNIEXPORT void JNICALL Java_com_sun_star_vcl_VCLGraphics_drawLine0( JNIEnv *pEnv, jobject object, jfloat _par0, jfloat _par1, jfloat _par2, jfloat _par3, jint _par4, jlong _par5, jboolean _par6, jfloat _par7, jfloat _par8, jfloat _par9, jfloat _par10, jfloat _par11 )
{
	CGPathRef aPath = (CGPathRef)_par5;
	CacheCGPath( aPath );

	CGContext_drawLine( _par0, _par1, _par2, _par3, _par4, aPath, _par6, _par7, _par8, _par9, _par10, _par11 );
}

// ----------------------------------------------------------------------------

JNIEXPORT void JNICALL Java_com_sun_star_vcl_VCLGraphics_drawPath0( JNIEnv *pEnv, jobject object, jint _par0, jboolean _par1, jlong _par2, jlong _par3, jboolean _par4, jfloat _par5, jfloat _par6, jfloat _par7, jfloat _par8, jfloat _par9 )
{
	CGPathRef aDrawPath = (CGPathRef)_par2;
	CacheCGPath( aDrawPath );

	CGPathRef aClipPath = (CGPathRef)_par3;
	CacheCGPath( aClipPath );

	CGContext_drawPath( _par0, _par1, aDrawPath, aClipPath, _par4, _par5, _par6, _par7, _par8, _par9 );
}

// ----------------------------------------------------------------------------

JNIEXPORT void JNICALL Java_com_sun_star_vcl_VCLGraphics_drawPathline0( JNIEnv *pEnv, jobject object, jint _par0, jfloat _par1, jint _par2, jlong _par3, jlong _par4, jboolean _par5, jfloat _par6, jfloat _par7, jfloat _par8, jfloat _par9, jfloat _par10 )
{
	CGPathRef aDrawPath = (CGPathRef)_par3;
	CacheCGPath( aDrawPath );

	CGPathRef aClipPath = (CGPathRef)_par4;
	CacheCGPath( aClipPath );

	CGLineJoin nJoin;
	switch ( _par2 )
	{
		case ::basegfx::B2DLINEJOIN_BEVEL:
			nJoin = kCGLineJoinBevel;
			break;
		case ::basegfx::B2DLINEJOIN_ROUND:
			nJoin = kCGLineJoinRound;
			break;
		default:
			nJoin = kCGLineJoinMiter;
			break;
	}

	CGContext_drawPathline( _par0, _par1, nJoin, aDrawPath, aClipPath, _par5, _par6, _par7, _par8, _par9, _par10 );
}

// ----------------------------------------------------------------------------

JNIEXPORT void JNICALL Java_com_sun_star_vcl_VCLGraphics_drawPolygon0( JNIEnv *pEnv, jobject object, jint _par0, jintArray _par1, jintArray _par2, jint _par3, jboolean _par4, jlong _par5, jboolean _par6, jfloat _par7, jfloat _par8, jfloat _par9, jfloat _par10, jfloat _par11 )
{
	CGPathRef aPath = (CGPathRef)_par5;
	CacheCGPath( aPath );

	if ( _par0 > 0 && _par1 && pEnv->GetArrayLength( _par1 ) >= _par0 && _par2 && pEnv->GetArrayLength( _par2 ) >= _par0 )
	{
		float aXPoints[ _par0 ];
		float aYPoints[ _par0 ];

		jboolean bCopy;
		bCopy = sal_False;
		jint *pXBits = (jint *)pEnv->GetPrimitiveArrayCritical( _par1, &bCopy );
		bCopy = sal_False;
		jint *pYBits = (jint *)pEnv->GetPrimitiveArrayCritical( _par2, &bCopy );
		for ( jint i = 0; i < _par0; i++ )
		{
			aXPoints[ i ] = (float)pXBits[ i ];
			aYPoints[ i ] = (float)pYBits[ i ];
		}
		pEnv->ReleasePrimitiveArrayCritical( _par2, pYBits, 0 );
		pEnv->ReleasePrimitiveArrayCritical( _par1, pXBits, 0 );
		
		CGContext_drawPolygon( _par0, aXPoints, aYPoints, _par3, _par4, aPath, _par6, _par7, _par8, _par9, _par10, _par11 );
	}
}

// ----------------------------------------------------------------------------

JNIEXPORT void JNICALL Java_com_sun_star_vcl_VCLGraphics_drawPolyline0( JNIEnv *pEnv, jobject object, jint _par0, jintArray _par1, jintArray _par2, jint _par3, jlong _par4, jboolean _par5, jfloat _par6, jfloat _par7, jfloat _par8, jfloat _par9, jfloat _par10 )
{
	CGPathRef aPath = (CGPathRef)_par4;
	CacheCGPath( aPath );

	if ( _par0 > 0 && _par1 && pEnv->GetArrayLength( _par1 ) >= _par0 && _par2 && pEnv->GetArrayLength( _par2 ) >= _par0 )
	{
		float aXPoints[ _par0 ];
		float aYPoints[ _par0 ];

		jboolean bCopy;
		bCopy = sal_False;
		jint *pXBits = (jint *)pEnv->GetPrimitiveArrayCritical( _par1, &bCopy );
		bCopy = sal_False;
		jint *pYBits = (jint *)pEnv->GetPrimitiveArrayCritical( _par2, &bCopy );
		for ( jint i = 0; i < _par0; i++ )
		{
			aXPoints[ i ] = (float)pXBits[ i ];
			aYPoints[ i ] = (float)pYBits[ i ];
		}
		pEnv->ReleasePrimitiveArrayCritical( _par2, pYBits, 0 );
		pEnv->ReleasePrimitiveArrayCritical( _par1, pXBits, 0 );
		
		CGContext_drawPolyline( _par0, aXPoints, aYPoints, _par3, aPath, _par5, _par6, _par7, _par8, _par9, _par10 );
	}
}

// ----------------------------------------------------------------------------

JNIEXPORT void JNICALL Java_com_sun_star_vcl_VCLGraphics_drawPolyPolygon0( JNIEnv *pEnv, jobject object, jint _par0, jintArray _par1, jobjectArray _par2, jobjectArray _par3, jint _par4, jboolean _par5, jlong _par6, jboolean _par7, jfloat _par8, jfloat _par9, jfloat _par10, jfloat _par11, jfloat _par12 )
{
	CGPathRef aPath = (CGPathRef)_par6;
	CacheCGPath( aPath );

	if ( _par0 > 0 && _par1 && pEnv->GetArrayLength( _par1 ) >= _par0 && _par2 && pEnv->GetArrayLength( _par2 ) >= _par0 && _par3 && pEnv->GetArrayLength( _par3 ) >= _par0 )
	{
		int aNPoints[ _par0 ];
		float *ppXPoints[ _par0 ];
		float *ppYPoints[ _par0 ];
		jboolean bCopy;
		bCopy = sal_False;
		jint *pCount = (jint *)pEnv->GetPrimitiveArrayCritical( _par1, &bCopy );
		if ( pCount )
		{
			jint i;
			for ( i = 0; i < _par0; i++ )
			{
				int nCount = (int)pCount[ i ];
				aNPoints[ i ] = nCount;
				ppXPoints[ i ] = new float[ nCount ];
				ppYPoints[ i ] = new float[ nCount ];

				jintArray aXArray = (jintArray)pEnv->GetObjectArrayElement( _par2, i );
				jintArray aYArray = (jintArray)pEnv->GetObjectArrayElement( _par3, i );
				if ( aXArray && aYArray )
				{
					bCopy = sal_False;
					jint *pXBits = (jint *)pEnv->GetPrimitiveArrayCritical( aXArray, &bCopy );
					bCopy = sal_False;
					jint *pYBits = (jint *)pEnv->GetPrimitiveArrayCritical( aYArray, &bCopy );
					for ( jint j = 0; j < nCount; j++ )
					{
						ppXPoints[ i ][ j ] = (float)pXBits[ j ];
						ppYPoints[ i ][ j ] = (float)pYBits[ j ];
					}
					pEnv->ReleasePrimitiveArrayCritical( aYArray, pYBits, 0 );
					pEnv->ReleasePrimitiveArrayCritical( aXArray, pXBits, 0 );
				}
			}

			pEnv->ReleasePrimitiveArrayCritical( _par1, pCount, 0 );

			CGContext_drawPolyPolygon( _par0, aNPoints, ppXPoints, ppYPoints, _par4, _par5, aPath, _par7, _par8, _par9, _par10, _par11, _par12 );

			for ( i = 0; i < _par0; i++ )
			{
				delete ppXPoints[ i ];
				delete ppYPoints[ i ];
			}
		}
	}
}

// ----------------------------------------------------------------------------

JNIEXPORT void JNICALL Java_com_sun_star_vcl_VCLGraphics_drawRect0( JNIEnv *pEnv, jobject object, jfloat _par0, jfloat _par1, jfloat _par2, jfloat _par3, jint _par4, jboolean _par5, jlong _par6, jboolean _par7, jfloat _par8, jfloat _par9, jfloat _par10, jfloat _par11, jfloat _par12 )
{
	CGPathRef aPath = (CGPathRef)_par6;
	CacheCGPath( aPath );

	CGContext_drawRect( _par0, _par1, _par2, _par3, _par4, _par5, aPath, _par7, _par8, _par9, _par10, _par11, _par12 );
}

// ============================================================================

jclass com_sun_star_vcl_VCLGraphics::theClass = NULL;

// ----------------------------------------------------------------------------

jclass com_sun_star_vcl_VCLGraphics::getMyClass()
{
	if ( !theClass )
	{
		VCLThreadAttach t;
		if ( !t.pEnv ) return (jclass)NULL;

		jclass tempClass = t.pEnv->FindClass( "com/sun/star/vcl/VCLGraphics" );
		OSL_ENSURE( tempClass, "Java : FindClass not found!" );

		if ( tempClass )
		{
			// Register the native methods for our class
			JNINativeMethod pMethods[13]; 
			pMethods[0].name = "drawBitmap0";
			pMethods[0].signature = "([IIIIIIIFFFFJZFFFFF)V";
			pMethods[0].fnPtr = (void *)Java_com_sun_star_vcl_VCLGraphics_drawBitmap0;
			pMethods[1].name = "drawBitmapBuffer0";
			pMethods[1].signature = "(JIIIIFFFFJZFFFFF)V";
			pMethods[1].fnPtr = (void *)Java_com_sun_star_vcl_VCLGraphics_drawBitmapBuffer0;
			pMethods[2].name = "drawEPS0";
			pMethods[2].signature = "(JJFFFFJZFFFFF)V";
			pMethods[2].fnPtr = (void *)Java_com_sun_star_vcl_VCLGraphics_drawEPS0;
			pMethods[3].name = "drawGlyphBuffer0";
			pMethods[3].signature = "(IIIJJIFIFFFFFJZFFFFF)V";
			pMethods[3].fnPtr = (void *)Java_com_sun_star_vcl_VCLGraphics_drawGlyphBuffer0;
			pMethods[4].name = "drawLine0";
			pMethods[4].signature = "(FFFFIJZFFFFF)V";
			pMethods[4].fnPtr = (void *)Java_com_sun_star_vcl_VCLGraphics_drawLine0;
			pMethods[5].name = "drawPath0";
			pMethods[5].signature = "(IZJJZFFFFF)V";
			pMethods[5].fnPtr = (void *)Java_com_sun_star_vcl_VCLGraphics_drawPath0;
			pMethods[6].name = "drawPathline0";
			pMethods[6].signature = "(IFIJJZFFFFF)V";
			pMethods[6].fnPtr = (void *)Java_com_sun_star_vcl_VCLGraphics_drawPathline0;
			pMethods[7].name = "drawPolygon0";
			pMethods[7].signature = "(I[I[IIZJZFFFFF)V";
			pMethods[7].fnPtr = (void *)Java_com_sun_star_vcl_VCLGraphics_drawPolygon0;
			pMethods[8].name = "drawPolyline0";
			pMethods[8].signature = "(I[I[IIJZFFFFF)V";
			pMethods[8].fnPtr = (void *)Java_com_sun_star_vcl_VCLGraphics_drawPolyline0;
			pMethods[9].name = "drawPolyPolygon0";
			pMethods[9].signature = "(I[I[[I[[IIZJZFFFFF)V";
			pMethods[9].fnPtr = (void *)Java_com_sun_star_vcl_VCLGraphics_drawPolyPolygon0;
			pMethods[10].name = "drawRect0";
			pMethods[10].signature = "(FFFFIZJZFFFFF)V";
			pMethods[10].fnPtr = (void *)Java_com_sun_star_vcl_VCLGraphics_drawRect0;
			pMethods[11].name = "notifyGraphicsChanged";
			pMethods[11].signature = "([JZ)V";
			pMethods[11].fnPtr = (void *)Java_com_sun_star_vcl_VCLGraphics_notifyGraphicsChanged;
			pMethods[12].name = "releaseNativeBitmaps";
			pMethods[12].signature = "(Z)V";
			pMethods[12].fnPtr = (void *)Java_com_sun_star_vcl_VCLGraphics_releaseNativeBitmaps;
			t.pEnv->RegisterNatives( tempClass, pMethods, 13 );
		}

		theClass = (jclass)t.pEnv->NewGlobalRef( tempClass );
	}
	return theClass;
}

// ----------------------------------------------------------------------------

void com_sun_star_vcl_VCLGraphics::beep()
{
	static jmethodID mID = NULL;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "()V";
			mID = t.pEnv->GetStaticMethodID( getMyClass(), "beep", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
			t.pEnv->CallStaticVoidMethod( getMyClass(), mID );
	}
}

// ----------------------------------------------------------------------------

void com_sun_star_vcl_VCLGraphics::addGraphicsChangeListener( JavaSalBitmap *_par0 )
{
	static jmethodID mID = NULL;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "(J)V";
			mID = t.pEnv->GetMethodID( getMyClass(), "addGraphicsChangeListener", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
		{
			jvalue args[1];
			args[0].j = jlong( _par0 );
			t.pEnv->CallNonvirtualVoidMethodA( object, getMyClass(), mID, args );
		}
	}
}

// ----------------------------------------------------------------------------

void com_sun_star_vcl_VCLGraphics::beginSetClipRegion( sal_Bool _par0 )
{
	static jmethodID mID = NULL;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "(Z)V";
			mID = t.pEnv->GetMethodID( getMyClass(), "beginSetClipRegion", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
		{
			jvalue args[1];
			args[0].z = jboolean( _par0 );
			t.pEnv->CallNonvirtualVoidMethodA( object, getMyClass(), mID, args );
		}
	}
}

// ----------------------------------------------------------------------------

void com_sun_star_vcl_VCLGraphics::copyBits( const com_sun_star_vcl_VCLGraphics *_par0, long _par1, long _par2, long _par3, long _par4, long _par5, long _par6, long _par7, long _par8, sal_Bool _par9 )
{
	static jmethodID mID = NULL;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "(Lcom/sun/star/vcl/VCLGraphics;IIIIIIIIZ)V";
			mID = t.pEnv->GetMethodID( getMyClass(), "copyBits", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
		{
			jvalue args[10];
			args[0].l = _par0->getJavaObject();
			args[1].i = jint( _par1 );
			args[2].i = jint( _par2 );
			args[3].i = jint( _par3 );
			args[4].i = jint( _par4 );
			args[5].i = jint( _par5 );
			args[6].i = jint( _par6 );
			args[7].i = jint( _par7 );
			args[8].i = jint( _par8 );
			args[9].z = jboolean( _par9 );
			t.pEnv->CallNonvirtualVoidMethodA( object, getMyClass(), mID, args );
		}
	}
}

// ----------------------------------------------------------------------------

void com_sun_star_vcl_VCLGraphics::copyBits( BYTE *_par0, long _par1, long _par2, long _par3, long _par4, long _par5, long _par6, long _par7, long _par8, long _par9 )
{
	static jmethodID mID = NULL;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "(Ljava/nio/ByteBuffer;IIIIIIIII)V";
			mID = t.pEnv->GetMethodID( getMyClass(), "copyBits", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
		{
			jobject byteBuffer = t.pEnv->NewDirectByteBuffer( _par0, _par1 );
			if ( byteBuffer )
			{
				jvalue args[10];
				args[0].l = byteBuffer;
				args[1].i = jint( _par1 );
				args[2].i = jint( _par2 );
				args[3].i = jint( _par3 );
				args[4].i = jint( _par4 );
				args[5].i = jint( _par5 );
				args[6].i = jint( _par6 );
				args[7].i = jint( _par7 );
				args[8].i = jint( _par8 );
				args[9].i = jint( _par9 );
				t.pEnv->CallNonvirtualVoidMethodA( object, getMyClass(), mID, args );
			}
		}
	}
}

// ----------------------------------------------------------------------------

void com_sun_star_vcl_VCLGraphics::drawBitmap( const com_sun_star_vcl_VCLBitmap *_par0, long _par1, long _par2, long _par3, long _par4, long _par5, long _par6, long _par7, long _par8, CGPathRef _par9 )
{
	// Mark the clip path for deletion in case the Java drawing method
	// never calls any of the native methods
	CacheCGPath( _par9 );

	static jmethodID mID = NULL;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "(Lcom/sun/star/vcl/VCLBitmap;IIIIIIIIJ)V";
			mID = t.pEnv->GetMethodID( getMyClass(), "drawBitmap", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
		{
			jvalue args[10];
			args[0].l = _par0->getJavaObject();
			args[1].i = jint( _par1 );
			args[2].i = jint( _par2 );
			args[3].i = jint( _par3 );
			args[4].i = jint( _par4 );
			args[5].i = jint( _par5 );
			args[6].i = jint( _par6 );
			args[7].i = jint( _par7 );
			args[8].i = jint( _par8 );
			args[9].j = jlong( _par9 );
			t.pEnv->CallNonvirtualVoidMethodA( object, getMyClass(), mID, args );
		}
	}
}

// ----------------------------------------------------------------------------

void com_sun_star_vcl_VCLGraphics::drawBitmapBuffer( BitmapBuffer *_par0, long _par1, long _par2, long _par3, long _par4, long _par5, long _par6, long _par7, long _par8, CGPathRef _par9 )
{
	// Mark the bitmap buffer for deletion in case the Java drawing method
	// never calls any of the native methods
	CacheBitmapBuffer( _par0 );

	// Mark the clip path for deletion in case the Java drawing method
	// never calls any of the native methods
	CacheCGPath( _par9 );

	static jmethodID mID = NULL;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "(JIIIIIIIIJ)V";
			mID = t.pEnv->GetMethodID( getMyClass(), "drawBitmapBuffer", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
		{
			jvalue args[10];
			args[0].j = jlong( _par0 );
			args[1].i = jint( _par1 );
			args[2].i = jint( _par2 );
			args[3].i = jint( _par3 );
			args[4].i = jint( _par4 );
			args[5].i = jint( _par5 );
			args[6].i = jint( _par6 );
			args[7].i = jint( _par7 );
			args[8].i = jint( _par8 );
			args[9].j = jlong( _par9 );
			t.pEnv->CallNonvirtualVoidMethodA( object, getMyClass(), mID, args );
		}
	}
}

// ----------------------------------------------------------------------------

void com_sun_star_vcl_VCLGraphics::drawGlyphBuffer( int _par0, int _par1, int _par2, CGGlyph *_par3, CGSize *_par4, com_sun_star_vcl_VCLFont *_par5, SalColor _par6, int _par7, int _par8, float _par9, float _par10, float _par11, CGPathRef _par12 )
{
	// Mark the glyph array for deletion in case the Java drawing method
	// never calls any of the native methods
	CacheGlyphData( (jlong)_par3 );

	// Mark the advances array for deletion in case the Java drawing method
	// never calls any of the native methods
	CacheGlyphData( (jlong)_par4 );

	// Mark the clip path for deletion in case the Java drawing method
	// never calls any of the native methods
	CacheCGPath( _par12 );

	static jmethodID mID = NULL;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "(IIIJJIFDIIIFFFJ)V";
			mID = t.pEnv->GetMethodID( getMyClass(), "drawGlyphBuffer", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
		{
			jvalue args[15];
			args[0].i = jint( _par0 );
			args[1].i = jint( _par1 );
			args[2].i = jint( _par2 );
			args[3].j = jlong( _par3 );
			args[4].j = jlong( _par4 );
			args[5].i = jint( _par5->getNativeFont() );
			args[6].f = jfloat( _par5->getSize() );
			args[7].d = jdouble( _par5->getScaleX() );
			args[8].i = jint( _par6 );
			args[9].i = jint( _par7 );
			args[10].i = jint( _par8 );
			args[11].f = jfloat( _par9 );
			args[12].f = jfloat( _par10 );
			args[13].f = jfloat( _par11 );
			args[14].j = jlong( _par12 );
			t.pEnv->CallNonvirtualVoidMethodA( object, getMyClass(), mID, args );
		}
	}
}

// ----------------------------------------------------------------------------

void com_sun_star_vcl_VCLGraphics::drawGlyphs( long _par0, long _par1, int _par2, sal_GlyphId *_par3, float *_par4, com_sun_star_vcl_VCLFont *_par5, SalColor _par6, int _par7, int _par8, float _par9, float _par10, float _par11 )
{
	static jmethodID mID = NULL;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "(II[I[FLcom/sun/star/vcl/VCLFont;IIIFFF)V";
			mID = t.pEnv->GetMethodID( getMyClass(), "drawGlyphs", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
		{
			jboolean bCopy;
			jsize elements( _par2 );
			jintArray glypharray = t.pEnv->NewIntArray( elements );
			bCopy = sal_False;
			jint *pGlyphBits = (jint *)t.pEnv->GetPrimitiveArrayCritical( glypharray, &bCopy );
			memcpy( pGlyphBits, (jint *)_par3, elements * sizeof( jint ) );
			t.pEnv->ReleasePrimitiveArrayCritical( glypharray, pGlyphBits, 0 );
			jfloatArray advancearray = t.pEnv->NewFloatArray( elements );
			bCopy = sal_False;
			jfloat *pAdvanceBits = (jfloat *)t.pEnv->GetPrimitiveArrayCritical( advancearray, &bCopy );
			memcpy( pAdvanceBits, (jfloat *)_par4, elements * sizeof( jfloat ) );
			t.pEnv->ReleasePrimitiveArrayCritical( advancearray, pAdvanceBits, 0 );

			jvalue args[11];
			args[0].i = jint( _par0 );
			args[1].i = jint( _par1 );
			args[2].l = glypharray;
			args[3].l = advancearray;
			args[4].l = _par5->getJavaObject();
			args[5].i = jint( _par6 );
			args[6].i = jint( _par7 );
			args[7].i = jint( _par8 );
			args[8].f = jfloat( _par9 );
			args[9].f = jfloat( _par10 );
			args[10].f = jfloat( _par11 );
			t.pEnv->CallNonvirtualVoidMethodA( object, getMyClass(), mID, args );
		}
	}
}

// ----------------------------------------------------------------------------

void com_sun_star_vcl_VCLGraphics::drawEPS( void *_par0, long _par1, long _par2, long _par3, long _par4, long _par5, CGPathRef _par6 )
{
	// Mark the EPS data for deletion in case the Java drawing method
	// never calls any of the native methods
	CacheEPSData( (jlong)_par0 );

	// Mark the clip path for deletion in case the Java drawing method
	// never calls any of the native methods
	CacheCGPath( _par6 );

	static jmethodID mID = NULL;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "(JJIIIIJ)V";
			mID = t.pEnv->GetMethodID( getMyClass(), "drawEPS", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
		{
			jvalue args[7];
			args[0].j = jlong( _par0 );
			args[1].j = jlong( _par1 );
			args[2].i = jint( _par2 );
			args[3].i = jint( _par3 );
			args[4].i = jint( _par4 );
			args[5].i = jint( _par5 );
			args[6].j = jlong( _par6 );
			t.pEnv->CallNonvirtualVoidMethodA( object, getMyClass(), mID, args );
		}
	}
}

// ----------------------------------------------------------------------------

void com_sun_star_vcl_VCLGraphics::drawLine( long _par0, long _par1, long _par2, long _par3, SalColor _par4, CGPathRef _par5 )
{
	// Mark the clip path for deletion in case the Java drawing method
	// never calls any of the native methods
	CacheCGPath( _par5 );

	static jmethodID mID = NULL;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "(IIIIIJ)V";
			mID = t.pEnv->GetMethodID( getMyClass(), "drawLine", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
		{
			jvalue args[6];
			args[0].i = jint( _par0 );
			args[1].i = jint( _par1 );
			args[2].i = jint( _par2 );
			args[3].i = jint( _par3 );
			args[4].i = jint( _par4 );
			args[5].j = jlong( _par5 );
			t.pEnv->CallNonvirtualVoidMethodA( object, getMyClass(), mID, args );
		}
	}
}

// ----------------------------------------------------------------------------

void com_sun_star_vcl_VCLGraphics::drawPath( com_sun_star_vcl_VCLPath *_par0, SalColor _par1, sal_Bool _par2, sal_Bool _par3, CGPathRef _par4, CGPathRef _par5 )
{
	// Mark the clip path for deletion in case the Java drawing method
	// never calls any of the native methods
	CacheCGPath( _par4 );
	CacheCGPath( _par5 );

	static jmethodID mID = NULL;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "(Lcom/sun/star/vcl/VCLPath;IZZJJ)V";
			mID = t.pEnv->GetMethodID( getMyClass(), "drawPath", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
		{
			jvalue args[6];
			args[0].l = _par0->getJavaObject();
			args[1].i = jint( _par1 );
			args[2].z = jboolean( _par2 );
			args[3].z = jboolean( _par3 );
			args[4].j = jlong( _par4 );
			args[5].j = jlong( _par5 );
			t.pEnv->CallNonvirtualVoidMethodA( object, getMyClass(), mID, args );
		}
	}
}

// ----------------------------------------------------------------------------

void com_sun_star_vcl_VCLGraphics::drawPathline( com_sun_star_vcl_VCLPath *_par0, SalColor _par1, sal_Bool _par2, double _par3, ::basegfx::B2DLineJoin _par4, CGPathRef _par5, CGPathRef _par6 )
{
	// Mark the clip path for deletion in case the Java drawing method
	// never calls any of the native methods
	CacheCGPath( _par5 );
	CacheCGPath( _par6 );

	static jmethodID mID = NULL;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "(Lcom/sun/star/vcl/VCLPath;IZFIJJ)V";
			mID = t.pEnv->GetMethodID( getMyClass(), "drawPathline", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
		{
			jvalue args[7];
			args[0].l = _par0->getJavaObject();
			args[1].i = jint( _par1 );
			args[2].z = jboolean( _par2 );
			args[3].f = jfloat( _par3 );
			args[4].i = jint( _par4 );
			args[5].j = jlong( _par5 );
			args[6].j = jlong( _par6 );
			t.pEnv->CallNonvirtualVoidMethodA( object, getMyClass(), mID, args );
		}
	}
}

// ----------------------------------------------------------------------------

void com_sun_star_vcl_VCLGraphics::drawPolygon( ULONG _par0, const SalPoint *_par1, SalColor _par2, sal_Bool _par3, CGPathRef _par4 )
{
	// Mark the clip path for deletion in case the Java drawing method
	// never calls any of the native methods
	CacheCGPath( _par4 );

	static jmethodID mID = NULL;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "(I[I[IIZJ)V";
			mID = t.pEnv->GetMethodID( getMyClass(), "drawPolygon", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
		{
			jint nPoints = 0;
			jboolean bCopy;
			jsize elements( _par0 );
			jintArray xarray = t.pEnv->NewIntArray( elements );
			jintArray yarray = t.pEnv->NewIntArray( elements );
			bCopy = sal_False;
			jint *pXBits = (jint *)t.pEnv->GetPrimitiveArrayCritical( xarray, &bCopy );
			bCopy = sal_False;
			jint *pYBits = (jint *)t.pEnv->GetPrimitiveArrayCritical( yarray, &bCopy );
			for ( jsize i = 0; i < elements; i++ )
			{
				if ( i && _par1[ i ].mnX == _par1[ i - 1 ].mnX && _par1[ i ].mnY == _par1[ i - 1 ].mnY )
					continue;

				pXBits[ nPoints ] = _par1[ i ].mnX;
				pYBits[ nPoints ] = _par1[ i ].mnY;
				nPoints++;
			}
			t.pEnv->ReleasePrimitiveArrayCritical( yarray, pYBits, 0 );
			t.pEnv->ReleasePrimitiveArrayCritical( xarray, pXBits, 0 );

			jvalue args[6];
			args[0].i = nPoints;
			args[1].l = xarray;
			args[2].l = yarray;
			args[3].i = jint( _par2 );
			args[4].z = jboolean( _par3 );
			args[5].j = jlong( _par4 );
			t.pEnv->CallNonvirtualVoidMethodA( object, getMyClass(), mID, args );
		}
	}
}

// ----------------------------------------------------------------------------

void com_sun_star_vcl_VCLGraphics::drawPolyline( ULONG _par0, const SalPoint *_par1, SalColor _par2, CGPathRef _par3 )
{
	// Mark the clip path for deletion in case the Java drawing method
	// never calls any of the native methods
	CacheCGPath( _par3 );

	static jmethodID mID = NULL;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "(I[I[IIJ)V";
			mID = t.pEnv->GetMethodID( getMyClass(), "drawPolyline", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
		{
			jint nPoints = 0;
			jboolean bCopy;
			jsize elements( _par0 );
			jintArray xarray = t.pEnv->NewIntArray( elements );
			jintArray yarray = t.pEnv->NewIntArray( elements );
			bCopy = sal_False;
			jint *pXBits = (jint *)t.pEnv->GetPrimitiveArrayCritical( xarray, &bCopy );
			bCopy = sal_False;
			jint *pYBits = (jint *)t.pEnv->GetPrimitiveArrayCritical( yarray, &bCopy );
			for ( jsize i = 0; i < elements; i++ )
			{
				if ( i && _par1[ i ].mnX == _par1[ i - 1 ].mnX && _par1[ i ].mnY == _par1[ i - 1 ].mnY )
					continue;

				pXBits[ nPoints ] = _par1[ i ].mnX;
				pYBits[ nPoints ] = _par1[ i ].mnY;
				nPoints++;
			}
			t.pEnv->ReleasePrimitiveArrayCritical( yarray, pYBits, 0 );
			t.pEnv->ReleasePrimitiveArrayCritical( xarray, pXBits, 0 );

			jvalue args[5];
			args[0].i = nPoints;
			args[1].l = xarray;
			args[2].l = yarray;
			args[3].i = jint( _par2 );
			args[4].j = jlong( _par3 );
			t.pEnv->CallNonvirtualVoidMethodA( object, getMyClass(), mID, args );
		}
	}
}

// ----------------------------------------------------------------------------

void com_sun_star_vcl_VCLGraphics::drawPolyPolygon( ULONG _par0, const ULONG *_par1, PCONSTSALPOINT *_par2, SalColor _par3, sal_Bool _par4, CGPathRef _par5 )
{
	// Mark the clip path for deletion in case the Java drawing method
	// never calls any of the native methods
	CacheCGPath( _par5 );

	static jmethodID mID = NULL;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "(I[I[[I[[IIZJ)V";
			mID = t.pEnv->GetMethodID( getMyClass(), "drawPolyPolygon", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		jclass tempClass = t.pEnv->FindClass( "[I" );
		if ( mID && tempClass )
		{
			jboolean bCopy;
			jsize elements( _par0 );
			jintArray ptsarray = t.pEnv->NewIntArray( elements );
			jintArray tempArray = t.pEnv->NewIntArray( 0 );
			jobjectArray xptsarray = t.pEnv->NewObjectArray( elements, tempClass, tempArray );
			jobjectArray yptsarray = t.pEnv->NewObjectArray( elements, tempClass, tempArray );
			for ( jsize i = 0; i < elements; i++ )
			{
				jint nPoints = 0;
				jsize points( _par1[ i ] );
				const SalPoint *pPts = _par2[ i ];
				jintArray xarray = t.pEnv->NewIntArray( points );
				jintArray yarray = t.pEnv->NewIntArray( points );
				bCopy = sal_False;
				jint *pXBits = (jint *)t.pEnv->GetPrimitiveArrayCritical( xarray, &bCopy );
				bCopy = sal_False;
				jint *pYBits = (jint *)t.pEnv->GetPrimitiveArrayCritical( yarray, &bCopy );
				for ( jsize j = 0; j < points; j++ )
				{
					if ( j && pPts[ j ].mnX == pPts[ j - 1 ].mnX && pPts[ j ].mnY == pPts[ j - 1 ].mnY )
						continue;

					pXBits[ nPoints ] = pPts[ j ].mnX;
					pYBits[ nPoints ] = pPts[ j ].mnY;
					nPoints++;
				}
				t.pEnv->ReleasePrimitiveArrayCritical( yarray, pYBits, 0 );
				t.pEnv->ReleasePrimitiveArrayCritical( xarray, pXBits, 0 );
				t.pEnv->SetIntArrayRegion( ptsarray, i, 1, &nPoints );
				t.pEnv->SetObjectArrayElement( yptsarray, i, yarray );
				t.pEnv->SetObjectArrayElement( xptsarray, i, xarray );
			}

			jvalue args[7];
			args[0].i = jint( _par0 );
			args[1].l = ptsarray;
			args[2].l = xptsarray;
			args[3].l = yptsarray;
			args[4].i = jint( _par3 );
			args[5].z = jboolean( _par4 );
			args[6].j = jlong( _par5 );
			t.pEnv->CallNonvirtualVoidMethodA( object, getMyClass(), mID, args );
		}
	}
}

// ----------------------------------------------------------------------------

void com_sun_star_vcl_VCLGraphics::drawRect( long _par0, long _par1, long _par2, long _par3, SalColor _par4, sal_Bool _par5, CGPathRef _par6 )
{
	// Mark the clip path for deletion in case the Java drawing method
	// never calls any of the native methods
	CacheCGPath( _par6 );

	static jmethodID mID = NULL;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "(IIIIIZJ)V";
			mID = t.pEnv->GetMethodID( getMyClass(), "drawRect", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
		{
			jvalue args[7];
			args[0].i = jint( _par0 );
			args[1].i = jint( _par1 );
			args[2].i = jint( _par2 );
			args[3].i = jint( _par3 );
			args[4].i = jint( _par4 );
			args[5].z = jboolean( _par5 );
			args[6].j = jlong( _par6 );
			t.pEnv->CallNonvirtualVoidMethodA( object, getMyClass(), mID, args );
		}
	}
}

// ----------------------------------------------------------------------------

void com_sun_star_vcl_VCLGraphics::drawPushButton( long _par0, long _par1, long _par2, long _par3, ::rtl::OUString _par4, sal_Bool _par5, sal_Bool _par6, sal_Bool _par7, sal_Bool _par8 )
{
	static jmethodID mID = NULL;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "(IIIILjava/lang/String;ZZZZ)V";
			mID = t.pEnv->GetMethodID( getMyClass(), "drawPushButton", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
		{
			jvalue args[9];
			args[0].i = jint( _par0 );
			args[1].i = jint( _par1 );
			args[2].i = jint( _par2 );
			args[3].i = jint( _par3 );
			args[4].l = StringToJavaString( t.pEnv, _par4 );
			args[5].z = jboolean( _par5 );
			args[6].z = jboolean( _par6 );
			args[7].z = jboolean( _par7 );
			args[8].z = jboolean( _par8 );
			t.pEnv->CallNonvirtualVoidMethodA( object, getMyClass(), mID, args );
		}
	}
}

// ----------------------------------------------------------------------------

const Rectangle com_sun_star_vcl_VCLGraphics::getPreferredPushButtonBounds( long _par0, long _par1, long _par2, long _par3, ::rtl::OUString _par4 )
{
	static jmethodID mID = NULL;
	static jfieldID fIDX = NULL;
	static jfieldID fIDY = NULL;
	static jfieldID fIDWidth = NULL;
	static jfieldID fIDHeight = NULL;
	Rectangle out( Point( 0, 0 ), Size( 0, 0 ) );
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "(IIIILjava/lang/String;)Ljava/awt/Rectangle;";
			mID = t.pEnv->GetMethodID( getMyClass(), "getPreferredPushButtonBounds", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
		{
			jvalue args[5];
			args[0].i = jint( _par0 );
			args[1].i = jint( _par1 );
			args[2].i = jint( _par2 );
			args[3].i = jint( _par3 );
			args[4].l = StringToJavaString( t.pEnv, _par4 );
			jobject tempObj = t.pEnv->CallNonvirtualObjectMethodA( object, getMyClass(), mID, args );
			if ( tempObj )
			{
				jclass tempObjClass = t.pEnv->GetObjectClass( tempObj );
				if ( !fIDX )
				{
					char *cSignature = "I";
					fIDX = t.pEnv->GetFieldID( tempObjClass, "x", cSignature );
				}
				OSL_ENSURE( fIDX, "Unknown field id!" );
				if ( !fIDY )
				{
					char *cSignature = "I";
					fIDY = t.pEnv->GetFieldID( tempObjClass, "y", cSignature );
				}
				OSL_ENSURE( fIDY, "Unknown field id!" );
				if ( !fIDWidth )
				{
					char *cSignature = "I";
					fIDWidth = t.pEnv->GetFieldID( tempObjClass, "width", cSignature );
				}
				OSL_ENSURE( fIDWidth, "Unknown field id!" );
				if ( !fIDHeight )
				{
					char *cSignature = "I";
					fIDHeight = t.pEnv->GetFieldID( tempObjClass, "height", cSignature );
				}
				OSL_ENSURE( fIDHeight, "Unknown field id!" );
				if ( fIDX && fIDY && fIDWidth && fIDHeight )
				{
					Point aPoint( (long)t.pEnv->GetIntField( tempObj, fIDX ), (long)t.pEnv->GetIntField( tempObj, fIDY ) );
					Size aSize( (long)t.pEnv->GetIntField( tempObj, fIDWidth ), (long)t.pEnv->GetIntField( tempObj, fIDHeight ) );
					out = Rectangle( aPoint, aSize );
				}
			}
		}
	}
	return out;
}

// ----------------------------------------------------------------------------

void com_sun_star_vcl_VCLGraphics::drawRadioButton( long _par0, long _par1, long _par2, long _par3, ::rtl::OUString _par4, sal_Bool _par5, sal_Bool _par6, sal_Bool _par7, long _par8 )
{
	static jmethodID mID = NULL;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "(IIIILjava/lang/String;ZZZI)V";
			mID = t.pEnv->GetMethodID( getMyClass(), "drawRadioButton", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
		{
			jvalue args[9];
			args[0].i = jint( _par0 );
			args[1].i = jint( _par1 );
			args[2].i = jint( _par2 );
			args[3].i = jint( _par3 );
			args[4].l = StringToJavaString( t.pEnv, _par4 );
			args[5].z = jboolean( _par5 );
			args[6].z = jboolean( _par6 );
			args[7].z = jboolean( _par7 );
			args[8].i = jint( _par8 );
			t.pEnv->CallNonvirtualVoidMethodA( object, getMyClass(), mID, args );
		}
	}
}

// ----------------------------------------------------------------------------

const Rectangle com_sun_star_vcl_VCLGraphics::getPreferredRadioButtonBounds( long _par0, long _par1, long _par2, long _par3, ::rtl::OUString _par4 )
{
	static jmethodID mID = NULL;
	static jfieldID fIDX = NULL;
	static jfieldID fIDY = NULL;
	static jfieldID fIDWidth = NULL;
	static jfieldID fIDHeight = NULL;
	Rectangle out( Point( 0, 0 ), Size( 0, 0 ) );
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "(IIIILjava/lang/String;)Ljava/awt/Rectangle;";
			mID = t.pEnv->GetMethodID( getMyClass(), "getPreferredRadioButtonBounds", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
		{
			jvalue args[5];
			args[0].i = jint( _par0 );
			args[1].i = jint( _par1 );
			args[2].i = jint( _par2 );
			args[3].i = jint( _par3 );
			args[4].l = StringToJavaString( t.pEnv, _par4 );
			jobject tempObj = t.pEnv->CallNonvirtualObjectMethodA( object, getMyClass(), mID, args );
			if ( tempObj )
			{
				jclass tempObjClass = t.pEnv->GetObjectClass( tempObj );
				if ( !fIDX )
				{
					char *cSignature = "I";
					fIDX = t.pEnv->GetFieldID( tempObjClass, "x", cSignature );
				}
				OSL_ENSURE( fIDX, "Unknown field id!" );
				if ( !fIDY )
				{
					char *cSignature = "I";
					fIDY = t.pEnv->GetFieldID( tempObjClass, "y", cSignature );
				}
				OSL_ENSURE( fIDY, "Unknown field id!" );
				if ( !fIDWidth )
				{
					char *cSignature = "I";
					fIDWidth = t.pEnv->GetFieldID( tempObjClass, "width", cSignature );
				}
				OSL_ENSURE( fIDWidth, "Unknown field id!" );
				if ( !fIDHeight )
				{
					char *cSignature = "I";
					fIDHeight = t.pEnv->GetFieldID( tempObjClass, "height", cSignature );
				}
				OSL_ENSURE( fIDHeight, "Unknown field id!" );
				if ( fIDX && fIDY && fIDWidth && fIDHeight )
				{
					Point aPoint( (long)t.pEnv->GetIntField( tempObj, fIDX ), (long)t.pEnv->GetIntField( tempObj, fIDY ) );
					Size aSize( (long)t.pEnv->GetIntField( tempObj, fIDWidth ), (long)t.pEnv->GetIntField( tempObj, fIDHeight ) );
					out = Rectangle( aPoint, aSize );
				}
			}
		}
	}
	return out;
}

// ----------------------------------------------------------------------------

void com_sun_star_vcl_VCLGraphics::endSetClipRegion( sal_Bool _par0 )
{
	static jmethodID mID = NULL;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "(Z)V";
			mID = t.pEnv->GetMethodID( getMyClass(), "endSetClipRegion", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
		{
			jvalue args[1];
			args[0].z = jboolean( _par0 );
			t.pEnv->CallNonvirtualVoidMethodA( object, getMyClass(), mID, args );
		}
	}
}

// ----------------------------------------------------------------------------

USHORT com_sun_star_vcl_VCLGraphics::getBitCount()
{
	static jmethodID mID = NULL;
	USHORT out = 0;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "()I";
			mID = t.pEnv->GetMethodID( getMyClass(), "getBitCount", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
			out = (USHORT)t.pEnv->CallNonvirtualIntMethod( object, getMyClass(), mID );
	}
	return out;
}

// ----------------------------------------------------------------------------

const Rectangle com_sun_star_vcl_VCLGraphics::getGlyphBounds( int _par0, com_sun_star_vcl_VCLFont *_par1, int _par2 )
{
	static jmethodID mID = NULL;
	static jfieldID fIDX = NULL;
	static jfieldID fIDY = NULL;
	static jfieldID fIDWidth = NULL;
	static jfieldID fIDHeight = NULL;
	Rectangle out( Point( 0, 0 ), Size( 0, 0 ) );
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "(ILcom/sun/star/vcl/VCLFont;I)Ljava/awt/Rectangle;";
			mID = t.pEnv->GetMethodID( getMyClass(), "getGlyphBounds", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
		{
			jvalue args[3];
			args[0].i = jint( _par0 );
			args[1].l = _par1->getJavaObject();
			args[2].i = jint( _par2 );
			jobject tempObj = t.pEnv->CallNonvirtualObjectMethodA( object, getMyClass(), mID, args );
			if ( tempObj )
			{
				jclass tempObjClass = t.pEnv->GetObjectClass( tempObj );
				if ( !fIDX )
				{
					char *cSignature = "I";
					fIDX = t.pEnv->GetFieldID( tempObjClass, "x", cSignature );
				}
				OSL_ENSURE( fIDX, "Unknown field id!" );
				if ( !fIDY )
				{
					char *cSignature = "I";
					fIDY = t.pEnv->GetFieldID( tempObjClass, "y", cSignature );
				}
				OSL_ENSURE( fIDY, "Unknown field id!" );
				if ( !fIDWidth )
				{
					char *cSignature = "I";
					fIDWidth = t.pEnv->GetFieldID( tempObjClass, "width", cSignature );
				}
				OSL_ENSURE( fIDWidth, "Unknown field id!" );
				if ( !fIDHeight )
				{
					char *cSignature = "I";
					fIDHeight = t.pEnv->GetFieldID( tempObjClass, "height", cSignature );
				}
				OSL_ENSURE( fIDHeight, "Unknown field id!" );
				if ( fIDX && fIDY && fIDWidth && fIDHeight )
				{
					Point aPoint( (long)t.pEnv->GetIntField( tempObj, fIDX ), (long)t.pEnv->GetIntField( tempObj, fIDY ) );
					Size aSize( (long)t.pEnv->GetIntField( tempObj, fIDWidth ), (long)t.pEnv->GetIntField( tempObj, fIDHeight ) );
					out = Rectangle( aPoint, aSize );
				}
			}
		}
	}
	return out;
}

// ----------------------------------------------------------------------------

SalColor com_sun_star_vcl_VCLGraphics::getPixel( long _par0, long _par1 )
{
	static jmethodID mID = NULL;
	SalColor out = 0;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "(II)I";
			mID = t.pEnv->GetMethodID( getMyClass(), "getPixel", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
		{
			jvalue args[2];
			args[0].i = jint( _par0 );
			args[1].i = jint( _par1 );
			out = (SalColor)t.pEnv->CallNonvirtualIntMethodA( object, getMyClass(), mID, args );
		}
	}
	return out;
}

// ----------------------------------------------------------------------------

const Size com_sun_star_vcl_VCLGraphics::getResolution()
{
	static jmethodID mID = NULL;
	static jfieldID fIDWidth = NULL;
	static jfieldID fIDHeight = NULL;	 
	Size out;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "()Ljava/awt/Dimension;";
			mID = t.pEnv->GetMethodID( getMyClass(), "getResolution", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
		{
			jobject tempObj = t.pEnv->CallNonvirtualObjectMethod( object, getMyClass(), mID );
			if ( tempObj )
			{
				jclass tempObjClass = t.pEnv->GetObjectClass( tempObj );
				OSL_ENSURE( tempObjClass, "Java : FindClass not found!" );
				if ( !fIDWidth )
				{
					char *cSignature = "I";
					fIDWidth = t.pEnv->GetFieldID( tempObjClass, "width", cSignature );
				}
				out.setWidth( (long)t.pEnv->GetIntField( tempObj, fIDWidth ) );
				if ( !fIDHeight )
				{
					char *cSignature = "I";
					fIDHeight = t.pEnv->GetFieldID( tempObjClass, "height", cSignature );
				}
				out.setHeight( (long)t.pEnv->GetIntField( tempObj, fIDHeight ) );
			}
		}
	}
    return out;
}

// ----------------------------------------------------------------------------

void com_sun_star_vcl_VCLGraphics::invert( long _par0, long _par1, long _par2, long _par3, SalInvert _par4 )
{
	static jmethodID mID = NULL;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "(IIIII)V";
			mID = t.pEnv->GetMethodID( getMyClass(), "invert", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
		{
			jvalue args[5];
			args[0].i = jint( _par0 );
			args[1].i = jint( _par1 );
			args[2].i = jint( _par2 );
			args[3].i = jint( _par3 );
			args[4].i = jint( _par4 );
			t.pEnv->CallNonvirtualVoidMethodA( object, getMyClass(), mID, args );
		}
	}
}

// ----------------------------------------------------------------------------

void com_sun_star_vcl_VCLGraphics::invert( ULONG _par0, const SalPoint *_par1, SalInvert _par2 )
{
	static jmethodID mID = NULL;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "(I[I[II)V";
			mID = t.pEnv->GetMethodID( getMyClass(), "invert", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
		{
			jint nPoints = 0;
			jboolean bCopy;
			jsize elements( _par0 );
			jintArray xarray = t.pEnv->NewIntArray( elements );
			jintArray yarray = t.pEnv->NewIntArray( elements );
			bCopy = sal_False;
			jint *pXBits = (jint *)t.pEnv->GetPrimitiveArrayCritical( xarray, &bCopy );
			bCopy = sal_False;
			jint *pYBits = (jint *)t.pEnv->GetPrimitiveArrayCritical( yarray, &bCopy );
			for ( jsize i = 0; i < elements; i++ )
			{
				if ( i && _par1[ i ].mnX == _par1[ i - 1 ].mnX && _par1[ i ].mnY == _par1[ i - 1 ].mnY )
					continue;

				pXBits[ nPoints ] = _par1[ i ].mnX;
				pYBits[ nPoints ] = _par1[ i ].mnY;
				nPoints++;
			}
			t.pEnv->ReleasePrimitiveArrayCritical( yarray, pYBits, 0 );
			t.pEnv->ReleasePrimitiveArrayCritical( xarray, pXBits, 0 );

			jvalue args[4];
			args[0].i = nPoints;
			args[1].l = xarray;
			args[2].l = yarray;
			args[3].i = jint( _par2 );
			t.pEnv->CallNonvirtualVoidMethodA( object, getMyClass(), mID, args );
		}
	}
}

// ----------------------------------------------------------------------------

void com_sun_star_vcl_VCLGraphics::removeGraphicsChangeListener( JavaSalBitmap *_par0 )
{
	static jmethodID mID = NULL;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "(J)V";
			mID = t.pEnv->GetMethodID( getMyClass(), "removeGraphicsChangeListener", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
		{
			jvalue args[1];
			args[0].j = jlong( _par0 );
			t.pEnv->CallNonvirtualVoidMethodA( object, getMyClass(), mID, args );
		}
	}
}

// ----------------------------------------------------------------------------

void com_sun_star_vcl_VCLGraphics::resetClipRegion( sal_Bool _par0 )
{
	static jmethodID mID = NULL;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "(Z)V";
			mID = t.pEnv->GetMethodID( getMyClass(), "resetClipRegion", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
		{
			jvalue args[1];
			args[0].z = jboolean( _par0 );
			t.pEnv->CallNonvirtualVoidMethodA( object, getMyClass(), mID, args );
		}
	}
}

// ----------------------------------------------------------------------------

void com_sun_star_vcl_VCLGraphics::resetGraphics()
{
	static jmethodID mID = NULL;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "()V";
			mID = t.pEnv->GetMethodID( getMyClass(), "resetGraphics", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
			t.pEnv->CallNonvirtualVoidMethod( object, getMyClass(), mID );
	}
}

// ----------------------------------------------------------------------------

void com_sun_star_vcl_VCLGraphics::setPixel( long _par0, long _par1, SalColor _par2, CGPathRef _par3 )
{
	// Mark the clip path for deletion in case the Java drawing method
	// never calls any of the native methods
	CacheCGPath( _par3 );

	static jmethodID mID = NULL;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "(IIIJ)V";
			mID = t.pEnv->GetMethodID( getMyClass(), "setPixel", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
		{
			jvalue args[4];
			args[0].i = jint( _par0 );
			args[1].i = jint( _par1 );
			args[2].i = jint( _par2 );
			args[3].j = jlong( _par3 );
			t.pEnv->CallNonvirtualVoidMethodA( object, getMyClass(), mID, args );
		}
	}
}

// ----------------------------------------------------------------------------

void com_sun_star_vcl_VCLGraphics::setXORMode( sal_Bool _par0 )
{
	static jmethodID mID = NULL;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "(Z)V";
			mID = t.pEnv->GetMethodID( getMyClass(), "setXORMode", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
		{
			jvalue args[1];
			args[0].z = jboolean( _par0 );
			t.pEnv->CallNonvirtualVoidMethodA( object, getMyClass(), mID, args );
		}
	}
}

// ----------------------------------------------------------------------------

void com_sun_star_vcl_VCLGraphics::unionClipPath( com_sun_star_vcl_VCLPath *_par0, sal_Bool _par1 )
{
	static jmethodID mID = NULL;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "(Lcom/sun/star/vcl/VCLPath;Z)V";
			mID = t.pEnv->GetMethodID( getMyClass(), "unionClipPath", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
		{
			jvalue args[2];
			args[0].l = _par0->getJavaObject();
			args[1].z = jboolean( _par1 );
			t.pEnv->CallNonvirtualVoidMethodA( object, getMyClass(), mID, args );
		}
	}
}

// ----------------------------------------------------------------------------

void com_sun_star_vcl_VCLGraphics::unionClipRegion( long _par0, long _par1, long _par2, long _par3, sal_Bool _par4 )
{
	static jmethodID mID = NULL;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "(IIIIZ)V";
			mID = t.pEnv->GetMethodID( getMyClass(), "unionClipRegion", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
		{
			jvalue args[5];
			args[0].i = jint( _par0 );
			args[1].i = jint( _par1 );
			args[2].i = jint( _par2 );
			args[3].i = jint( _par3 );
			args[4].z = jboolean( _par4 );
			t.pEnv->CallNonvirtualVoidMethodA( object, getMyClass(), mID, args );
		}
	}
}
