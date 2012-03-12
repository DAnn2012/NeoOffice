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
 *  Patrick Luby, February 2006
 *
 *  GNU General Public License Version 2.1
 *  =============================================
 *  Copyright 2006 Planamesa Inc.
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

#ifndef _SV_SALBMP_H
#define _SV_SALBMP_H

#include <salgdi.h>
#include <vcl/salbmp.hxx>
#include <vcl/sv.h>
#include <tools/gen.hxx>
#include <vcl/salbtype.hxx>
#include <com/sun/star/vcl/VCLBitmap.hxx>

struct BitmapBuffer;
class BitmapPalette;

namespace vcl
{
class com_sun_star_vcl_VCLGraphics;
}

// -----------------
// - JavaSalBitmap -
// -----------------

class JavaSalBitmap : public SalBitmap
{
	Point					maPoint;
	Size					maSize;
	USHORT					mnBitCount;
	BYTE*					mpBits;
	BitmapPalette			maPalette;
	BitmapBuffer*			mpBuffer;
	::vcl::com_sun_star_vcl_VCLGraphics*	mpVCLGraphics;
	JavaSalGraphics*		mpGraphics;

public:
	static ULONG			Get32BitNativeFormat();

							JavaSalBitmap();
	virtual					~JavaSalBitmap();

	bool					Create( BitmapBuffer *pBuffer );
	bool					Create( const Point& rPoint, const Size& rSize, JavaSalGraphics *pGraphics, const BitmapPalette& rPal );
	::vcl::com_sun_star_vcl_VCLBitmap*	CreateVCLBitmap( long nX, long nY, long nWidth, long nHeight );
	Point					GetPoint() const { return maPoint; }
	::vcl::com_sun_star_vcl_VCLGraphics*	GetVCLGraphics() { return mpVCLGraphics; }
	JavaSalGraphics*		GetGraphics() { return mpGraphics; }
	void					NotifyGraphicsChanged( bool bDisposed );
	void					ReleaseVCLBitmap( ::vcl::com_sun_star_vcl_VCLBitmap *pVCLBitmap );

	virtual bool			Create( const Size& rSize, USHORT nBitCount, const BitmapPalette& rPal );
	virtual bool			Create( const SalBitmap& rSalBmp );
	virtual bool			Create( const SalBitmap& rSalBmp, SalGraphics* pGraphics );
	virtual bool			Create( const SalBitmap& rSalBmp, USHORT nNewBitCount );
	virtual void			Destroy();
	virtual Size			GetSize() const { return maSize; }
	virtual USHORT			GetBitCount() const;
	virtual BitmapBuffer*	AcquireBuffer( bool bReadOnly );
	virtual void			ReleaseBuffer( BitmapBuffer* pBuffer, bool bReadOnly );
	virtual bool			GetSystemData( BitmapSystemData& rData );
};

extern "C" SAL_DLLPRIVATE void ReleaseBitmapBufferBytePointerCallback( void *pInfo, const void *pPointer, size_t nSize );

#endif // _SV_SALBMP_H
