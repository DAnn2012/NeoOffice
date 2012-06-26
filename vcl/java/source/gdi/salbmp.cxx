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

#include <salbmp.h>
#include <saldata.hxx>
#include <salgdi.h>
#include <salinst.h>
#include <vcl/bmpacc.hxx>

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

ULONG JavaSalBitmap::GetNativeDirectionFormat()
{
	return BMP_FORMAT_TOP_DOWN;
}

// ------------------------------------------------------------------

JavaSalBitmap::JavaSalBitmap() :
	maPoint( 0, 0 ),
	maSize( 0, 0 ),
	mnBitCount( 0 ),
	mpBits( NULL ),
	mpBuffer( NULL ),
	mpGraphics( NULL ),
	mpVirDev( NULL )
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

bool JavaSalBitmap::Create( const Point& rPoint, const Size& rSize, JavaSalGraphics *pSrcGraphics, const BitmapPalette& rPal )
{
	Destroy();

	if ( !pSrcGraphics )
		return false;

	maSize = Size( rSize.Width(), rSize.Height() );

	if ( maSize.Width() <= 0 || maSize.Height() <= 0 )
		return false;

	mnBitCount = pSrcGraphics->GetBitCount();

	// Save the palette
	USHORT nColors = ( ( mnBitCount <= 8 ) ? ( 1 << mnBitCount ) : 0 );
	if ( nColors )
	{
		maPalette = rPal;
		maPalette.SetEntryCount( nColors );
	}

	float fLineWidth = pSrcGraphics->getNativeLineWidth();
	if ( fLineWidth > 0 )
	{
		long nPadding = (long)( fLineWidth + 0.5 );
		maPoint = Point( nPadding, nPadding );
	}

	JavaSalInstance *pInst = GetSalData()->mpFirstInstance;
	if ( pInst )
	{
		mpVirDev = (JavaSalVirtualDevice *)pInst->CreateVirtualDevice( pSrcGraphics, maSize.Width() + ( maPoint.X() * 2 ), maSize.Height() + ( maPoint.Y() * 2 ), mnBitCount, NULL );
		if ( mpVirDev )
		{
			mpGraphics = (JavaSalGraphics *)mpVirDev->GetGraphics();
			if ( mpGraphics )
			{
				CGRect aUnflippedSrcRect = UnflipFlippedRect( CGRectMake( rPoint.X(), rPoint.Y(), maSize.Width(), maSize.Height() ), pSrcGraphics->maNativeBounds );
				mpGraphics->copyFromGraphics( pSrcGraphics, aUnflippedSrcRect, CGRectMake( maPoint.X(), maPoint.Y(), maSize.Width(), maSize.Height() ), false );
			}
		}
	}

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

	if ( mpVirDev )
	{
		if ( mpGraphics )
			mpVirDev->ReleaseGraphics( mpGraphics );

		JavaSalInstance *pInst = GetSalData()->mpFirstInstance;
		if ( pInst )
			pInst->DestroyVirtualDevice( mpVirDev );
	}

	mpGraphics = NULL;
	mpVirDev = NULL;
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
	pBuffer->mnFormat = JavaSalBitmap::GetNativeDirectionFormat();
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

			if ( mpGraphics )
			{
				CGColorSpaceRef aColorSpace = CGColorSpaceCreateDeviceRGB();
				if ( aColorSpace )
				{
					CGContextRef aContext = CGBitmapContextCreate( mpBits, pBuffer->mnWidth, pBuffer->mnHeight, 8, pBuffer->mnScanlineSize, aColorSpace, kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little );
					if ( aContext )
					{
						CGRect aSrcRect = CGRectMake( maPoint.X(), maPoint.Y(), pBuffer->mnWidth, pBuffer->mnHeight );
						CGRect aDestRect = CGRectMake( 0, 0, pBuffer->mnWidth, pBuffer->mnHeight );
						mpGraphics->copyToContext( NULL, NULL, false, false, aContext, aDestRect, aSrcRect, aDestRect );

						CGContextRelease( aContext );
					}

					CGColorSpaceRelease( aColorSpace );
				}
			}
		}
		else
		{
			delete pBuffer;
			return NULL;
		}
	}

	if ( !bReadOnly )
	{
		// Release the virtual device if the bits will change
		if ( mpVirDev )
		{
			if ( mpGraphics )
				mpVirDev->ReleaseGraphics( mpGraphics );

			JavaSalInstance *pInst = GetSalData()->mpFirstInstance;
			if ( pInst )
				pInst->DestroyVirtualDevice( mpVirDev );
		}

		maPoint = Point( 0, 0 );
		mpGraphics = NULL;
		mpVirDev = NULL;
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
