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
 *	 - GNU General Public License Version 2.1
 *
 *  Edward Peterlin, September 2004
 *
 *  GNU General Public License Version 2.1
 *  =============================================
 *  Copyright 2004 Planamesa Inc.
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

#ifndef _SV_COM_SUN_STAR_VCL_VCLMENU_HXX
#define _SV_COM_SUN_STAR_VCL_VCLMENU_HXX

#include <salframe.h>
#include <java/lang/Object.hxx>
#include <sal/types.h>
#include <tools/gen.hxx>

#ifndef USE_NATIVE_WINDOW

namespace vcl {

class com_sun_star_vcl_VCLMenuItemData;

class SAL_DLLPRIVATE com_sun_star_vcl_VCLMenu : public java_lang_Object
{
protected:
	static jclass		theClass;

public:
	static jclass		getMyClass();
	
						com_sun_star_vcl_VCLMenu( jobject myObj ) : java_lang_Object( myObj ) {};
				
						com_sun_star_vcl_VCLMenu();
	virtual				~com_sun_star_vcl_VCLMenu() {};
	
	void				dispose( );
	
	com_sun_star_vcl_VCLMenuItemData *	getMenuItemDataObject();
	
	void				insertItem(com_sun_star_vcl_VCLMenuItemData *_par0, short _par1);
	void				removeItem(short _par0);
	void				checkItem(short _par0, bool _par1);
	void				enableItem(short _par0, bool _par1);
	void				attachSubmenu(com_sun_star_vcl_VCLMenuItemData *_par0, short par1);
};

} // namespace vcl

#endif	// !USE_NATIVE_WINDOW

#endif // _SV_COM_SUN_STAR_VCL_VCLMENU_HXX
