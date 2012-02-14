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
 *		 - GNU General Public License Version 2.1
 *
 *  GNU General Public License Version 2.1
 *  =============================================
 *  Copyright 2012 by Planamesa Inc.
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
 *************************************************************************/

#ifndef _UPDATEI18N_COCOA_HXX
#define _UPDATEI18N_COCOA_HXX

#import <com/sun/star/lang/Locale.hpp>
#import <sal/types.h>
#import "../../../../vcl/inc/vcl/svids.hrc"

#include <premac.h>
#import <Cocoa/Cocoa.h>
#include "postmac.h"

#define UPDATEBACK "back"
#define UPDATEDOWNLOADCANCELED "download.canceled"
#define UPDATEDOWNLOADFAILED "download.failed"
#define UPDATEDOWNLOADINGFILE "downloading.file"
#define UPDATEERROR "error"
#define UPDATELOADING "loading"
#define UPDATEMEGABYTE "megabyte"
#define UPDATEOPENFILEFAILED "open.file.failed"
#define UPDATEOPENINGFILE "opening.file"
#define UPDATEREDOWNLOADFILE "redownload.file"

/**
 * Returns the application's locale.
 */
SAL_DLLPRIVATE ::com::sun::star::lang::Locale GetApplicationLocale();

/**
 * Lookup a string and retrieve a translated string.  If no translation
 * is available, default to english.
 */
SAL_DLLPRIVATE NSString *UpdateGetLocalizedString( const sal_Char *key );
SAL_DLLPRIVATE NSString *UpdateGetLocalizedDecimalSeparator();
SAL_DLLPRIVATE NSString *UpdateGetVCLResString( int nId );

#endif	// _UPDATEI18N_COCOA_HXX
