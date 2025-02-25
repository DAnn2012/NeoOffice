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
 *  Copyright 2008 by Planamesa Inc.
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

#ifndef _NEOMOBILEI18N_HXX
#define _NEOMOBILEI18N_HXX

#import <com/sun/star/lang/Locale.hpp>
#import <sal/types.h>

#include <premac.h>
#import <Cocoa/Cocoa.h>
#include "postmac.h"

#define NEOMOBILEABOUT "about"
#define NEOMOBILEBACK "back"
#define NEOMOBILECANCEL "cancel"
#define NEOMOBILECREATEACCOUNT "create.new.account"
#define NEOMOBILEDOWNLOADCANCELED "download.canceled"
#define NEOMOBILEDOWNLOADFAILED "download.failed"
#define NEOMOBILEDOWNLOADINGFILE "downloading.file"
#define NEOMOBILEERROR "error"
#define NEOMOBILEEXPORTINGFILE "exporting.file"
#define NEOMOBILEFORGOTPASSWORD "forgot.password"
#define NEOMOBILELOADING "loading"
#define NEOMOBILELOGIN "login"
#define NEOMOBILELOGINTITLE "login.title"
#define NEOMOBILEMEGABYTE "megabyte"
#define NEOMOBILEPASSWORD "password"
#define NEOMOBILEPRODUCTNAME "product.name"
#define NEOMOBILESAVEPASSWORD "save.password"
#define NEOMOBILEUPLOAD "upload"
#define NEOMOBILEUPLOADCONTINUE "upload.continue"
#define NEOMOBILEUPLOADINGFILE "uploading.file"
#define NEOMOBILEUPLOADPASSWORDPROTECTED "upload.password.protected"
#define NEOMOBILEUSERNAME "username"

/**
 * Returns the application's locale.
 */
SAL_DLLPRIVATE ::com::sun::star::lang::Locale NeoMobileGetApplicationLocale();

/**
 * Lookup a string and retrieve a translated string.  If no translation
 * is available, default to english.
 */
SAL_DLLPRIVATE NSString *NeoMobileGetLocalizedString( const sal_Char *key );
SAL_DLLPRIVATE NSString *NeoMobileGetLocalizedDecimalSeparator();

#endif	// _NEOMOBILEI18N_HXX
