/*************************************************************************
 *
 * Copyright 2008 by Sun Microsystems, Inc.
 *
 * $RCSfile$
 * $Revision$
 *
 * This file is part of NeoOffice.
 *
 * NeoOffice is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3
 * only, as published by the Free Software Foundation.
 *
 * NeoOffice is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 3 for more details
 * (a copy is included in the LICENSE file that accompanied this code).
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with NeoOffice.  If not, see
 * <http://www.gnu.org/licenses/gpl-3.0.txt>
 * for a copy of the GPLv3 License.
 *
 * Modified September 2007 by Patrick Luby. NeoOffice is distributed under
 * GPL only under modification term 2 of the LGPL.
 *
 ************************************************************************/

// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_connectivity.hxx"
#include <MNSFolders.hxx>

#ifdef UNIX
#include <sys/types.h>
#include <strings.h>
#include <string.h>
#endif // End UNIX

#ifdef WNT
#include "pre_include_windows.h"
#include <windows.h>
#include <stdlib.h>
#include <shlobj.h>
#include <objidl.h>
#include "post_include_windows.h"
#endif // End WNT
#include <osl/security.hxx>
#include <osl/file.hxx>
#include <osl/thread.h>

using namespace ::com::sun::star::mozilla;

namespace
{
    #if defined(XP_MAC) || defined(XP_MACOSX) || defined(MACOSX) 
        #define APP_REGISTRY_NAME "Application Registry"
    #elif defined(XP_WIN) || defined(XP_OS2)
        #define APP_REGISTRY_NAME "registry.dat"
    #else
        #define APP_REGISTRY_NAME "appreg"
    #endif

    // -------------------------------------------------------------------
    static ::rtl::OUString lcl_getUserDataDirectory()
    {
        ::osl::Security   aSecurity;
        ::rtl::OUString   aConfigPath;

#ifdef USE_JAVA
        // Fix bug 3393 by using the HOME directory
        aSecurity.getHomeDir( aConfigPath );
#else	// USE_JAVA
        aSecurity.getConfigDir( aConfigPath );
#endif	// USE_JAVA
        return aConfigPath + ::rtl::OUString::createFromAscii( "/" );
    }

    // -------------------------------------------------------------------
    static const char* DefaultProductDir[3][3] =
    {
    #if defined(XP_WIN)
        { "Mozilla/", NULL, NULL },
        { "Mozilla/Firefox/", NULL, NULL },
        { "Thunderbird/", "Mozilla/Thunderbird/", NULL }
    #elif(MACOSX)
#ifdef USE_JAVA
        { "Library/Mozilla/", NULL, NULL },
        { "Library/Application Support/Firefox/", NULL, NULL },
        { "Library/Thunderbird/", NULL, NULL }
#else	// USE_JAVA
        { "../Mozilla/", NULL, NULL },
        { "Firefox/", NULL, NULL },
        { "../Thunderbird/", NULL, NULL }
#endif	// USE_JAVA
    #else
        { ".mozilla/", NULL, NULL },
        { ".mozilla/firefox/", NULL, NULL },
        { ".thunderbird/", ".mozilla-thunderbird/", ".mozilla/thunderbird/" }
    #endif
    };

    static const char* ProductRootEnvironmentVariable[3] =
    {
        "MOZILLA_PROFILE_ROOT",
        "MOZILLA_FIREFOX_PROFILE_ROOT",
        "MOZILLA_THUNDERBIRD_PROFILE_ROOT",
    };

    // -------------------------------------------------------------------
    static ::rtl::OUString lcl_guessProfileRoot( MozillaProductType _product )
    {
        size_t productIndex = _product - 1;

        static ::rtl::OUString s_productDirectories[3];

        if ( !s_productDirectories[ productIndex ].getLength() )
        {
            ::rtl::OUString sProductPath;

            // check whether we have an anevironment variable which helps us
            const char* pProfileByEnv = getenv( ProductRootEnvironmentVariable[ productIndex ] );
            if ( pProfileByEnv )
            {
                sProductPath = ::rtl::OUString( pProfileByEnv, rtl_str_getLength( pProfileByEnv ), osl_getThreadTextEncoding() );
                // asume that this is fine, no further checks
            }
            else
            {
                ::rtl::OUString sProductDirCandidate;
                const char* pProfileRegistry = ( _product == MozillaProductType_Mozilla ) ? APP_REGISTRY_NAME : "profiles.ini";

                // check all possible candidates
                for ( size_t i=0; i<3; ++i )
                {
                    if ( NULL == DefaultProductDir[ productIndex ][ i ] )
                        break;

                    sProductDirCandidate = lcl_getUserDataDirectory() +
                        ::rtl::OUString::createFromAscii( DefaultProductDir[ productIndex ][ i ] );

                    // check existence
                    ::osl::DirectoryItem aRegistryItem;
                    ::osl::FileBase::RC result = ::osl::DirectoryItem::get( sProductDirCandidate + ::rtl::OUString::createFromAscii( pProfileRegistry ), aRegistryItem );
                    if ( result == ::osl::FileBase::E_None  )
                    {
                        ::osl::FileStatus aStatus( FileStatusMask_Validate );
                        result = aRegistryItem.getFileStatus( aStatus );
                        if ( result == ::osl::FileBase::E_None  )
                        {
                            // the registry file exists
                            break;
                        }
                    }
                }

                ::osl::FileBase::getSystemPathFromFileURL( sProductDirCandidate, sProductPath );
            }

            s_productDirectories[ productIndex ] = sProductPath;
        }

        return s_productDirectories[ productIndex ];
    }
}

// -----------------------------------------------------------------------
::rtl::OUString getRegistryDir(MozillaProductType product)
{
	if (product == MozillaProductType_Default)
		return ::rtl::OUString();

    return lcl_guessProfileRoot( product );
}
#ifndef MINIMAL_PROFILEDISCOVER
// -----------------------------------------------------------------------
::rtl::OUString getRegistryFileName(MozillaProductType product)
{
	if (product == MozillaProductType_Default)
		return ::rtl::OUString();

	return getRegistryDir(product) + ::rtl::OUString::createFromAscii(APP_REGISTRY_NAME);
}
#endif
