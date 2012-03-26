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

#ifndef _SV_COM_SUN_STAR_VCL_VCLBITMAP_HXX
#define	_SV_COM_SUN_STAR_VCL_VCLBITMAP_HXX

#include <salframe.h>
#include <salprn.h>
#include <salvd.h>
#include <java/lang/Object.hxx>
#include <sal/types.h>

#if !defined USE_NATIVE_WINDOW || !defined USE_NATIVE_VIRTUAL_DEVICE || !defined USE_NATIVE_PRINTING
class BitmapPalette;

namespace vcl {

class com_sun_star_vcl_VCLBitmap;
class com_sun_star_vcl_VCLGraphics;

class SAL_DLLPRIVATE com_sun_star_vcl_VCLBitmap : public java_lang_Object
{
protected:
	static jclass		theClass;

public:
	static jclass		getMyClass();

						com_sun_star_vcl_VCLBitmap( jobject myObj ) : java_lang_Object( myObj ) {}
						com_sun_star_vcl_VCLBitmap( long nDX, long nDY, USHORT nBitCount );
	virtual				~com_sun_star_vcl_VCLBitmap() {};

	void				dispose();
	java_lang_Object*	getData();
};

} // namespace vcl

#endif	// !USE_NATIVE_WINDOW || !USE_NATIVE_VIRTUAL_DEVICE || !USE_NATIVE_PRINTING

#endif // _SV_COM_SUN_STAR_VCL_VCLBITMAP_HXX
