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
 *
 *  Sun Microsystems Inc., October, 2000
 *
 *  GNU General Public License Version 2.1
 *  =============================================
 *  Copyright 2000 by Sun Microsystems, Inc.
 *  901 San Antonio Road, Palo Alto, CA 94303, USA
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
 *
 *  
 *  =================================================
 *  Modified September 2002 by Edward Peterlin. SISSL Removed. NeoOffice is distributed under GPL only under modification term 3 of the LGPL.
 *
 *  Contributor(s): _______________________________________
 *
 *
 ************************************************************************/

#ifndef _SV_SALMENU_H
#define _SV_SALMENU_H

#ifndef _SV_SV_H
#include <sv.h>
#endif
#ifndef _SV_IMAGE_H
#include <image.h>
#endif

class SalMenu;
class SalMenuItem;
class SalFrame;
class Menu;

namespace vcl
{
class com_sun_star_vcl_VCLMenuBar;
class com_sun_star_vcl_VCLMenu;
class com_sun_star_vcl_VCLMenuItemData;
}

// =======================================================================

class SalMenuData
{
public:
        // Java VCL specific data
        
        // used for menubars only
        
        ::vcl::com_sun_star_vcl_VCLMenuBar *mpVCLMenuBar;
        
        // used for menus
        
        ::vcl::com_sun_star_vcl_VCLMenu *mpVCLMenu;
        
        // +++ ADD IN POINTER TO NATIVE MENU DATA
        
        // Generic data
        
	SalFrame *	mpParentFrame;		// pointer to the parent frame
	BOOL			mbIsMenuBarMenu;	// true for menu bars
	SalMenu *		mpParentMenu;		// Parent menu if this is a submenu
};

class SalMenuItemData
{
public:
	XubString			mText;			// the item text
	XubString			mAccelText;		// the accelerator string
	Bitmap			maBitmap;			// item image
	int				mnId;			// item id

        ::vcl::com_sun_star_vcl_VCLMenuItemData *mpVCLMenuItemData;
        
	SalMenu *			mpSalMenu;		// SalMenu into which this item is inserted
	SalMenu *			mpSalSubmenu;		// Submenu SalMenu if this item has a submenu
	Menu *			mpVCLMenu;		// VCL menu into which this item is inserted
};

#endif // _SV_SALMENU_H

