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

#define _SV_SALBMP_CXX

#ifndef _SV_SALBMP_H
#include <salbmp.h>
#endif
#ifndef _SV_SALDATA_HXX
#include <saldata.hxx>
#endif
#ifndef _SV_SALGDI_HXX
#include <vcl/salgdi.hxx>
#endif
#ifndef _SV_COM_SUN_STAR_VCL_VCLGRAPHICS_HXX
#include <com/sun/star/vcl/VCLGraphics.hxx>
#endif
#ifndef _SV_BMPACC_HXX
#include <vcl/bmpacc.hxx>
#endif

using namespace vcl;

// ==================================================================
 
void ReleaseBitmapBufferBytePointerCallback( void *pInfo, const void *pPointer, size_t nSize )
{
	BYTE *pBits = (BYTE *)pPointer;
	if ( pBits )
		delete[] pBits;

}

// ==================================================================

ULONG JavaSalBitmap::Get32BitNativeFormat()
{
	return BMP_FORMAT_32BIT_TC_BGRA;
}

// ------------------------------------------------------------------

JavaSalBitmap::JavaSalBitmap() :
	maSize( 0, 0 ),
	mnBitCount( 0 ),
	mpBits( NULL ),
	mpBuffer( NULL ),
	mpVCLGraphics( NULL ),
	mpGraphics( NULL )
{
	GetSalData()->maBitmapList.push_back( this );
}

// ------------------------------------------------------------------

JavaSalBitmap::~JavaSalBitmap()
{
	GetSalData()->maBitmapList.remove( this );
	Destroy();
}

// ------------------------------------------------------------------

com_sun_star_vcl_VCLBitmap *JavaSalBitmap::CreateVCLBitmap( long nX, long nY, long nWidth, long nHeight )
{
	if ( nWidth < 1 || nHeight < 1 )
		return NULL;

	com_sun_star_vcl_VCLBitmap *pVCLBitmap = new com_sun_star_vcl_VCLBitmap( nWidth, nHeight, mnBitCount );
	if ( pVCLBitmap && pVCLBitmap->getJavaObject() )
	{
		// Fill the buffer with pointers to the Java buffer
		java_lang_Object *pData = pVCLBitmap->getData();
		if ( pData )
		{
			// Force copying of the buffer if it has not already been done
			BitmapBuffer *pBuffer = AcquireBuffer( FALSE );
			if ( pBuffer )
			{
				VCLThreadAttach t;
				if ( t.pEnv )
				{
					jboolean bCopy( sal_False );
					jint *pBits = (jint *)t.pEnv->GetPrimitiveArrayCritical( (jintArray)pData->getJavaObject(), &bCopy );
					if ( pBits )
					{
						Scanline pBitsIn = (Scanline)( pBuffer->mpBits + ( nY * pBuffer->mnScanlineSize ) + ( nX * pBuffer->mnBitCount / 8 ) );
						jint *pBitsOut = pBits;

						if ( pBuffer->mnFormat & BMP_FORMAT_1BIT_MSB_PAL )
						{
							FncGetPixel pFncGetPixel = BitmapReadAccess::GetPixelFor_1BIT_MSB_PAL;
							for ( long i = 0; i < nHeight; i++ )
							{
								for ( long j = 0; j < nWidth; j++ )
								{
									BitmapColor aColor( pBuffer->maPalette[ pFncGetPixel( pBitsIn, j, pBuffer->maColorMask ) ] );
									pBitsOut[ j ] = MAKE_SALCOLOR( aColor.GetRed(), aColor.GetGreen(), aColor.GetBlue() ) | 0xff000000;
								}
		
								pBitsIn += pBuffer->mnScanlineSize;
								pBitsOut += nWidth;
							}
						}
						else if ( pBuffer->mnFormat & BMP_FORMAT_4BIT_MSN_PAL )
						{
							FncGetPixel pFncGetPixel = BitmapReadAccess::GetPixelFor_4BIT_MSN_PAL;
							for ( long i = 0; i < nHeight; i++ )
							{
								for ( long j = 0; j < nWidth; j++ )
								{
									BitmapColor aColor( pBuffer->maPalette[ pFncGetPixel( pBitsIn, j, pBuffer->maColorMask ) ] );
									pBitsOut[ j ] = MAKE_SALCOLOR( aColor.GetRed(), aColor.GetGreen(), aColor.GetBlue() ) | 0xff000000;
								}
		
								pBitsIn += pBuffer->mnScanlineSize;
								pBitsOut += nWidth;
							}
						}
						else if ( pBuffer->mnFormat & BMP_FORMAT_8BIT_PAL )
						{
							FncGetPixel pFncGetPixel = BitmapReadAccess::GetPixelFor_8BIT_PAL;
							for ( long i = 0; i < nHeight; i++ )
							{
								for ( long j = 0; j < nWidth; j++ )
								{
									BitmapColor aColor( pBuffer->maPalette[ pFncGetPixel( pBitsIn, j, pBuffer->maColorMask ) ] );
									pBitsOut[ j ] = MAKE_SALCOLOR( aColor.GetRed(), aColor.GetGreen(), aColor.GetBlue() ) | 0xff000000;
								}
		
								pBitsIn += pBuffer->mnScanlineSize;
								pBitsOut += nWidth;
							}
						}
						else if ( pBuffer->mnFormat & BMP_FORMAT_16BIT_TC_MSB_MASK )
						{
							FncGetPixel pFncGetPixel = BitmapReadAccess::GetPixelFor_16BIT_TC_MSB_MASK;
							for ( long i = 0; i < nHeight; i++ )
							{
								for ( long j = 0; j < nWidth; j++ )
								{
									BitmapColor aColor( pFncGetPixel( pBitsIn, j, pBuffer->maColorMask ) );
									pBitsOut[ j ] = MAKE_SALCOLOR( aColor.GetRed(), aColor.GetGreen(), aColor.GetBlue() ) | 0xff000000;
								}
			
								pBitsIn += pBuffer->mnScanlineSize;
								pBitsOut += nWidth;
							}
						}
						else if ( pBuffer->mnFormat & BMP_FORMAT_24BIT_TC_RGB )
						{
							FncGetPixel pFncGetPixel = BitmapReadAccess::GetPixelFor_24BIT_TC_RGB;
							for ( long i = 0; i < nHeight; i++ )
							{
								for ( long j = 0; j < nWidth; j++ )
								{
									BitmapColor aColor( pFncGetPixel( pBitsIn, j, pBuffer->maColorMask ) );
									pBitsOut[ j ] = MAKE_SALCOLOR( aColor.GetRed(), aColor.GetGreen(), aColor.GetBlue() ) | 0xff000000;
								}
			
								pBitsIn += pBuffer->mnScanlineSize;
								pBitsOut += nWidth;
							}
						}
						else if ( pBuffer->mnFormat & BMP_FORMAT_32BIT_TC_BGRA )
						{
							long nByteCount = nWidth * sizeof( jint );
							for ( long i = 0; i < nHeight; i++ )
							{
								memcpy( pBitsOut, pBitsIn, nByteCount );

								pBitsIn += pBuffer->mnScanlineSize;
								pBitsOut += nWidth;
							}
						}

						t.pEnv->ReleasePrimitiveArrayCritical( (jintArray)pData->getJavaObject(), pBits, 0 );
					}
				}

				ReleaseBuffer( pBuffer, FALSE );
			}

			delete pData;
		}
		else
		{
			pVCLBitmap->dispose();
			delete pVCLBitmap;
			pVCLBitmap = NULL;
		}
	}
	else if ( pVCLBitmap )
	{
		delete pVCLBitmap;
		pVCLBitmap = NULL;
	}

	return pVCLBitmap;
}

// ------------------------------------------------------------------

void JavaSalBitmap::NotifyGraphicsChanged( bool bDisposed )
{
	// Force copying of the buffer if it has not already been done
	if ( mpVCLGraphics )
	{
		mpVCLGraphics->removeGraphicsChangeListener( this );

		if ( !bDisposed && !mpBits )
		{
			// Force copying of the buffer
			long nCapacity = AlignedWidth4Bytes( mnBitCount * maSize.Width() ) * maSize.Height();
			try
			{
				mpBits = new BYTE[ nCapacity ];
			}
			catch( const std::bad_alloc& ) {}
			if ( mpBits )
			{
				memset( mpBits, 0, nCapacity );
				mpVCLGraphics->copyBits( mpBits, nCapacity, maPoint.X(), maPoint.Y(), maSize.Width(), maSize.Height(), 0, 0, maSize.Width(), maSize.Height() );
			}
		}

		delete mpVCLGraphics;
		mpVCLGraphics = NULL;
	}
	else if ( mpGraphics )
	{
		mpGraphics->removeGraphicsChangeListener( this );

		if ( !bDisposed && !mpBits && mpGraphics->useNativeDrawing() )
		{
			CGColorSpaceRef aColorSpace = CGColorSpaceCreateDeviceRGB();
			if ( aColorSpace )
			{
				long nScanlineSize = AlignedWidth4Bytes( mnBitCount * maSize.Width() );
				long nCapacity = nScanlineSize * maSize.Height();
				// Force copying of the buffer
				try
				{
					mpBits = new BYTE[ nCapacity ];
				}
				catch( const std::bad_alloc& ) {}
				if ( mpBits )
				{
					memset( mpBits, 0, nCapacity );

					CGContextRef aContext = CGBitmapContextCreate( mpBits, maSize.Width(), maSize.Height(), 8, nScanlineSize, aColorSpace, kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little );
					if ( aContext )
					{
						// Flip coordinates to VCL drawing coordinates
						CGContextTranslateCTM( aContext, 0, maSize.Height() );
						CGContextScaleCTM( aContext, 1.0f, -1.0f );

						mpGraphics->copyToContext( NULL, false, NULL, aContext, CGRectMake( 0, 0, maSize.Width(), maSize.Height() ), CGPointMake( maPoint.X(), maPoint.Y() ), CGRectMake( 0, 0, maSize.Width(), maSize.Height() ) );

						CGContextRelease( aContext );
					}
				}

				CGColorSpaceRelease( aColorSpace );
			}
		}
	}

	maPoint = Point( 0, 0 );
	mpGraphics = NULL;
}

// ------------------------------------------------------------------

void JavaSalBitmap::ReleaseVCLBitmap( com_sun_star_vcl_VCLBitmap *pVCLBitmap )
{
	if ( pVCLBitmap )
	{
		pVCLBitmap->dispose();
		delete pVCLBitmap;
	}
}

// ------------------------------------------------------------------

/**
 * Warning: this method takes ownership of the BitmapBuffer's mpBits member
 * so after calling this method, the mpBits member will be set to NULL.
 */
bool JavaSalBitmap::Create( BitmapBuffer *pBuffer )
{
	Destroy();

	bool bRet = false;

	if ( !pBuffer || !pBuffer->mpBits )
		return bRet;

	Size aSize( pBuffer->mnWidth, pBuffer->mnHeight );
	bRet = Create( aSize, pBuffer->mnBitCount, pBuffer->maPalette );
	if ( bRet )
	{
		mpBits = pBuffer->mpBits;
		pBuffer->mpBits = NULL;
	}

	return bRet;
}

// ------------------------------------------------------------------

bool JavaSalBitmap::Create( const Point& rPoint, const Size& rSize, JavaSalGraphics *pGraphics, const BitmapPalette& rPal )
{
	Destroy();

	// Fix bug 3642 by ensuring that the origin is not negative
	long nX = rPoint.X();
	long nY = rPoint.Y();
	long nWidth = rSize.Width();
	long nHeight = rSize.Height();
	if ( nX < 0 )
	{
		nWidth += nX;
		nX = 0;
	}
	if ( nY < 0 )
	{
		nHeight += nY;
		nY = 0;
	}

	maPoint = Point( nX, nY );
	maSize = Size( nWidth, nHeight );

	if ( !pGraphics || maSize.Width() <= 0 || maSize.Height() <= 0 )
		return false;

	if ( pGraphics->useNativeDrawing() )
	{
		mpGraphics = pGraphics;
		mnBitCount = mpGraphics->GetBitCount();
	}
	else if ( pGraphics->mpVCLGraphics )
	{
		mpVCLGraphics = new com_sun_star_vcl_VCLGraphics( pGraphics->mpVCLGraphics->getJavaObject() );
		if ( !mpVCLGraphics )
			return false;
		mnBitCount = mpVCLGraphics->getBitCount();
	}
	else
	{
		return false;
	}

	// Save the palette
	USHORT nColors = ( ( mnBitCount <= 8 ) ? ( 1 << mnBitCount ) : 0 );
	if ( nColors )
	{
		maPalette = rPal;
		maPalette.SetEntryCount( nColors );
	}

	if ( mpVCLGraphics )
		mpVCLGraphics->addGraphicsChangeListener( this );
	else if ( mpGraphics )
		NotifyGraphicsChanged( false );

	return true;
}

// ------------------------------------------------------------------

bool JavaSalBitmap::Create( const Size& rSize, USHORT nBitCount, const BitmapPalette& rPal )
{
	Destroy();

	maSize = Size( rSize );
	if ( maSize.Width() <= 0 || maSize.Height() <= 0 )
		return false;

	if ( nBitCount <= 1 )
		mnBitCount = 1;
	else if ( nBitCount <= 4 )
		mnBitCount = 4;
	else if ( nBitCount <= 8 )
		mnBitCount = 8;
	else if ( nBitCount <= 16 )
		mnBitCount = 16;
	else if ( nBitCount <= 24 )
		mnBitCount = 24;
	else
		mnBitCount = 32;

	// Save the palette
	USHORT nColors = ( ( mnBitCount <= 8 ) ? ( 1 << mnBitCount ) : 0 );
	if ( nColors )
	{
		maPalette = rPal;
		maPalette.SetEntryCount( nColors );
	}

	return true;
}

// ------------------------------------------------------------------

bool JavaSalBitmap::Create( const SalBitmap& rSalBmp )
{
	Destroy();

	JavaSalBitmap& rJavaSalBmp = (JavaSalBitmap&)rSalBmp;
	bool bRet = Create( rJavaSalBmp.GetSize(), rJavaSalBmp.GetBitCount(), rJavaSalBmp.maPalette );

	if ( bRet )
	{
		BitmapBuffer *pSrcBuffer = rJavaSalBmp.AcquireBuffer( TRUE );
		if ( pSrcBuffer )
		{
			BitmapBuffer *pDestBuffer = AcquireBuffer( FALSE );
			if ( pDestBuffer && pDestBuffer->mpBits && pDestBuffer->mnScanlineSize == pSrcBuffer->mnScanlineSize && pDestBuffer->mnHeight == pSrcBuffer->mnHeight )
			{
				memcpy( pDestBuffer->mpBits, pSrcBuffer->mpBits, pDestBuffer->mnScanlineSize * pDestBuffer->mnHeight );
				pDestBuffer->maColorMask = pSrcBuffer->maColorMask;
				pDestBuffer->maPalette = pSrcBuffer->maPalette;
				ReleaseBuffer( pDestBuffer, FALSE );
			}
			else
			{
				Destroy();
				bRet = false;
			}
			rJavaSalBmp.ReleaseBuffer( pSrcBuffer, TRUE );
		}
		else
		{
			Destroy();
			bRet = false;
		}
	}

	return bRet;
}

// ------------------------------------------------------------------

bool JavaSalBitmap::Create( const SalBitmap& rSalBmp, SalGraphics* pGraphics )
{
	return false;
}

// ------------------------------------------------------------------

bool JavaSalBitmap::Create( const SalBitmap& rSalBmp, USHORT nNewBitCount )
{
	return false;
}

// ------------------------------------------------------------------

void JavaSalBitmap::Destroy()
{
	maPoint = Point( 0, 0 );
	maSize = Size( 0, 0 );
	mnBitCount = 0;

	if ( mpBits )
	{
		delete[] mpBits;
		mpBits = NULL;
	}

	if ( mpBuffer )
	{
		delete mpBuffer;
		mpBuffer = NULL;
	}

	maPalette.SetEntryCount( 0 );

	if ( mpVCLGraphics )
	{
		mpVCLGraphics->removeGraphicsChangeListener( this );
		delete mpVCLGraphics;
		mpVCLGraphics = NULL;
	}

	if ( mpGraphics )
	{
		mpGraphics->removeGraphicsChangeListener( this );
		mpGraphics = NULL;
	}
}

// ------------------------------------------------------------------

USHORT JavaSalBitmap::GetBitCount() const
{
	return mnBitCount;
}

// ------------------------------------------------------------------

BitmapBuffer* JavaSalBitmap::AcquireBuffer( bool bReadOnly )
{
	BitmapBuffer *pBuffer = new BitmapBuffer();

	pBuffer->mnBitCount = mnBitCount;
	pBuffer->mnFormat = BMP_FORMAT_TOP_DOWN;
	if ( mnBitCount <= 1 )
	{
		pBuffer->mnFormat |= BMP_FORMAT_1BIT_MSB_PAL;
	}
	else if ( mnBitCount <= 4 )
	{
		pBuffer->mnFormat |= BMP_FORMAT_4BIT_MSN_PAL;
	}
	else if ( mnBitCount <= 8 )
	{
		pBuffer->mnFormat |= BMP_FORMAT_8BIT_PAL;
	}
	else if ( mnBitCount <= 16 )
	{
		pBuffer->mnFormat |= BMP_FORMAT_16BIT_TC_MSB_MASK;
		pBuffer->maColorMask = ColorMask( 0x7c00, 0x03e0, 0x001f );
	}
	else if ( mnBitCount <= 24 )
	{
		pBuffer->mnFormat |= BMP_FORMAT_24BIT_TC_RGB;
	}
	else
	{
		pBuffer->mnFormat |= JavaSalBitmap::Get32BitNativeFormat();
	}

	pBuffer->mnWidth = maSize.Width();
	pBuffer->mnHeight = maSize.Height();
	pBuffer->mnScanlineSize = AlignedWidth4Bytes( mnBitCount * maSize.Width() );
	pBuffer->maPalette = maPalette;

	NotifyGraphicsChanged( false );

	if ( !mpBits )
	{
		try
		{
			mpBits = new BYTE[ pBuffer->mnScanlineSize * pBuffer->mnHeight ];
		}
		catch( const std::bad_alloc& ) {}
		if ( mpBits )
		{
			memset( mpBits, 0, pBuffer->mnScanlineSize * pBuffer->mnHeight );
		}
		else
		{
			delete pBuffer;
			return NULL;
		}
	}

	pBuffer->mpBits = mpBits;

	return pBuffer;
}

// ------------------------------------------------------------------

void JavaSalBitmap::ReleaseBuffer( BitmapBuffer* pBuffer, bool bReadOnly )
{
	if ( pBuffer )
	{
		if ( !bReadOnly )
		{
			// Save the palette
			USHORT nColors = ( ( mnBitCount <= 8 ) ? ( 1 << mnBitCount ) : 0 );
			if ( nColors )
			{
				maPalette = pBuffer->maPalette;
				maPalette.SetEntryCount( nColors );
			}
		}
		delete pBuffer;
	}
}

// ------------------------------------------------------------------

bool JavaSalBitmap::GetSystemData( BitmapSystemData& rData )
{
#ifdef DEBUG
	fprintf( stderr, "JavaSalBitmap::GetSystemData not implemented\n" );
#endif
	return false;
}
