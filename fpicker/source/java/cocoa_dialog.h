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
 *  Patrick Luby, July 2006
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

#ifndef _COCOA_FILEDIALOG_H_
#define _COCOA_FILEDIALOG_H_

#include <tools/solar.h>

#ifdef __cplusplus
#include <premac.h>
#include <CoreFoundation/CoreFoundation.h>
#include <postmac.h>

#ifndef __OBJC__
typedef void* id;
#endif
#endif

enum CocoaControlType {
	COCOA_CONTROL_TYPE_BUTTON,
	COCOA_CONTROL_TYPE_CHECKBOX,
	COCOA_CONTROL_TYPE_POPUP,
	MAX_COCOA_CONTROL_TYPE
};

// These are defined in reverse order of their appearance
enum CocoaControlID {
	COCOA_CONTROL_ID_AUTOEXTENSION,
	COCOA_CONTROL_ID_FILTEROPTIONS,
	COCOA_CONTROL_ID_IMAGE_TEMPLATE,
	COCOA_CONTROL_ID_LINK,
	COCOA_CONTROL_ID_PASSWORD,
	COCOA_CONTROL_ID_PLAY,
	COCOA_CONTROL_ID_PREVIEW,
	COCOA_CONTROL_ID_READONLY,
	COCOA_CONTROL_ID_SELECTION,
	COCOA_CONTROL_ID_TEMPLATE,
	COCOA_CONTROL_ID_VERSION,
	COCOA_CONTROL_ID_FILETYPE,
	MAX_COCOA_CONTROL_ID
};

#ifdef __cplusplus
BEGIN_C
#endif
void JavaFilePicker_controlStateChanged( int nID, void *pPicker );
void NSFileDialog_addFilter( id pDialog, CFStringRef aItem, CFStringRef aFilter );
void NSFileDialog_addItem( id pDialog, int nID, CFStringRef aItem );
void NSFileDialog_cancel( id pDialog );
int NSFileDialog_controlType( int nID );
id NSFileDialog_create( void *pPicker, sal_Bool bUseFileOpenDialog, sal_Bool bChooseFiles, sal_Bool bShowAutoExtension, sal_Bool bShowFilterOptions, sal_Bool bShowImageTemplate, sal_Bool bShowLink, sal_Bool bShowPassword, sal_Bool bShowReadOnly, sal_Bool bShowSelection, sal_Bool bShowTemplate, sal_Bool bShowVersion );
void NSFileDialog_deleteItem( id pDialog, int nID, CFStringRef aItem );
CFStringRef NSFileDialog_directory( id pDialog );
CFStringRef *NSFileDialog_URLs( id pDialog );
sal_Bool NSFileDialog_isChecked( id pDialog, int nID );
CFStringRef *NSFileDialog_items( id pDialog, int nID );
CFStringRef NSFileDialog_label( id pDialog, int nID );
void NSFileDialog_release( id pDialog );
void NSFileManager_releaseURLs( CFStringRef *pURLs );
void NSFileManager_releaseItems( CFStringRef *pItems );
CFStringRef NSFileDialog_selectedFilter( id pDialog );
CFStringRef NSFileDialog_selectedItem( id pDialog, int nID );
int NSFileDialog_selectedItemIndex( id pDialog, int nID );
void NSFileDialog_setChecked( id pDialog, int nID, sal_Bool bChecked );
void NSFileDialog_setDefaultName( id pDialog, CFStringRef aName );
void NSFileDialog_setDirectory( id pDialog, CFStringRef aDirectory );
void NSFileDialog_setEnabled( id pDialog, int nID, sal_Bool bEnabled );
void NSFileDialog_setLabel( id pDialog, int nID, CFStringRef aLabel );
void NSFileDialog_setMultiSelectionMode( id pDialog, sal_Bool bMultiSelectionMode );
void NSFileDialog_setSelectedFilter( id pDialog, CFStringRef aItem );
void NSFileDialog_setSelectedItem( id pDialog, int nID, int nItem );
void NSFileDialog_setTitle( id pDialog, CFStringRef aTitle );
short NSFileDialog_showFileDialog( id pDialog );
#ifdef __cplusplus
END_C
#endif

#endif	// _COCOA_FILEDIALOG_H_
