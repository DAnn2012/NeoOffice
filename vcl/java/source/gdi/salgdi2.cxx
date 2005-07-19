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
 *  Copyright 2003 by Patrick Luby (patrick.luby@planamesa.com)
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

#define _SV_SALGDI2_CXX

#ifndef _SV_SALGDI_HXX
#include <salgdi.hxx>
#endif
#ifndef _SV_SALFRAME_HXX
#include <salframe.hxx>
#endif
#ifndef _SV_SALBMP_HXX
#include <salbmp.hxx>
#endif
#ifndef _SV_COM_SUN_STAR_VCL_VCLGRAPHICS_HXX
#include <com/sun/star/vcl/VCLGraphics.hxx>
#endif
#ifndef _SV_COM_SUN_STAR_VCL_VCLIMAGE_HXX
#include <com/sun/star/vcl/VCLImage.hxx>
#endif

using namespace vcl;

// =======================================================================

void SalGraphics::CopyBits( const SalTwoRect* pPosAry,
                            SalGraphics* pSrcGraphics,
                            const OutputDevice *pOutDev,
                            const OutputDevice *pSrcOutDev )
{
	if ( !pSrcGraphics )
		pSrcGraphics = this;

	maGraphicsData.mpVCLGraphics->copyBits( pSrcGraphics->maGraphicsData.mpVCLGraphics, pPosAry->mnSrcX, pPosAry->mnSrcY, pPosAry->mnSrcWidth, pPosAry->mnSrcHeight, pPosAry->mnDestX, pPosAry->mnDestY, pPosAry->mnDestWidth, pPosAry->mnDestHeight );
}

// -----------------------------------------------------------------------

void SalGraphics::CopyArea( long nDestX, long nDestY,
							long nSrcX, long nSrcY,
							long nSrcWidth, long nSrcHeight,
							USHORT nFlags, const OutputDevice *pOutDev )
{
	maGraphicsData.mpVCLGraphics->copyBits( maGraphicsData.mpVCLGraphics, nSrcX, nSrcY, nSrcWidth, nSrcHeight, nDestX, nDestY, nSrcWidth, nSrcHeight );
}

// -----------------------------------------------------------------------

void SalGraphics::DrawBitmap( const SalTwoRect* pPosAry,
							  const SalBitmap& rSalBitmap,
                              const OutputDevice *pOutDev )
{
	maGraphicsData.mpVCLGraphics->drawBitmap( rSalBitmap.mpVCLBitmap, pPosAry->mnSrcX, pPosAry->mnSrcY, pPosAry->mnSrcWidth, pPosAry->mnSrcHeight, pPosAry->mnDestX, pPosAry->mnDestY, pPosAry->mnDestWidth, pPosAry->mnDestHeight );
}

// -----------------------------------------------------------------------

void SalGraphics::DrawBitmap( const SalTwoRect* pPosAry,
							  const SalBitmap& rSalBitmap,
							  SalColor nTransparentColor,
                              const OutputDevice *pOutDev )
{
#ifdef DEBUG
	fprintf( stderr, "SalGraphics::DrawBitmap #2 not implemented\n" );
#endif
}

// -----------------------------------------------------------------------

void SalGraphics::DrawBitmap( const SalTwoRect* pPosAry,
							  const SalBitmap& rSalBitmap,
							  const SalBitmap& rTransparentBitmap,
                              const OutputDevice *pOutDev )
{
	maGraphicsData.mpVCLGraphics->drawBitmap( rSalBitmap.mpVCLBitmap, rTransparentBitmap.mpVCLBitmap, pPosAry->mnSrcX, pPosAry->mnSrcY, pPosAry->mnSrcWidth, pPosAry->mnSrcHeight, pPosAry->mnDestX, pPosAry->mnDestY, pPosAry->mnDestWidth, pPosAry->mnDestHeight );
}

// -----------------------------------------------------------------------

void SalGraphics::DrawMask( const SalTwoRect* pPosAry,
							const SalBitmap& rSalBitmap,
							SalColor nMaskColor,
                            const OutputDevice *pOutDev )
{
	maGraphicsData.mpVCLGraphics->drawMask( rSalBitmap.mpVCLBitmap, nMaskColor, pPosAry->mnSrcX, pPosAry->mnSrcY, pPosAry->mnSrcWidth, pPosAry->mnSrcHeight, pPosAry->mnDestX, pPosAry->mnDestY, pPosAry->mnDestWidth, pPosAry->mnDestHeight );
}

// -----------------------------------------------------------------------

SalBitmap* SalGraphics::GetBitmap( long nX, long nY, long nDX, long nDY,
                                   const OutputDevice *pOutDev )
{
	// Don't do anything if this is a printer
	if ( maGraphicsData.mpPrinter )
		return NULL;

	// Normalize rectangle
	nDX = abs( nDX );
	nDY = abs( nDY );

	SalBitmap *pBitmap = new SalBitmap();

	if ( !pBitmap->Create( Size( nDX, nDY ), GetBitCount(), BitmapPalette() ) )
	{
		delete pBitmap;
		pBitmap = NULL;
	}

	if ( pBitmap )
		pBitmap->mpVCLBitmap->copyBits( maGraphicsData.mpVCLGraphics, nX, nY, nDX, nDY, 0, 0 );

	return pBitmap;
}

// -----------------------------------------------------------------------

SalColor SalGraphics::GetPixel( long nX, long nY, const OutputDevice *pOutDev )
{
	return maGraphicsData.mpVCLGraphics->getPixel( nX, nY );
}

// -----------------------------------------------------------------------

void SalGraphics::Invert( long nX, long nY, long nWidth, long nHeight,
                          SalInvert nFlags, const OutputDevice *pOutDev )
{
	maGraphicsData.mpVCLGraphics->invert( nX, nY, nWidth, nHeight, nFlags );
}

// -----------------------------------------------------------------------

void SalGraphics::Invert( ULONG nPoints, const SalPoint* pPtAry,
                          SalInvert nFlags, const OutputDevice *pOutDev )
{
	long pXPoints[ nPoints + 1 ];
	long pYPoints[ nPoints + 1 ];
	for ( ULONG i = 0; i < nPoints; i++ )
	{
		pXPoints[ i ] = pPtAry->mnX;
		pYPoints[ i ] = pPtAry->mnY;
		pPtAry++;
	}

	maGraphicsData.mpVCLGraphics->invert( nPoints, pXPoints, pYPoints, nFlags );
}
