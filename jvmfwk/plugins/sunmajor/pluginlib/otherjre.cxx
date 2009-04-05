/*************************************************************************
 *
 *  $RCSfile$
 *
 *  $Revision$
 *
 *  last change: $Author$ $Date$
 *
 *  The Contents of this file are made available subject to
 *  the terms of GNU General Public License Version 2.1.
 *
 *
 *    GNU General Public License Version 2.1
 *    =============================================
 *    Copyright 2005 by Sun Microsystems, Inc.
 *    901 San Antonio Road, Palo Alto, CA 94303, USA
 *
 *    This library is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU General Public
 *    License version 2.1, as published by the Free Software Foundation.
 *
 *    This library is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    General Public License for more details.
 *
 *    You should have received a copy of the GNU General Public
 *    License along with this library; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 *    MA  02111-1307  USA
 *
 *    Modified January 2006 by Patrick Luby. NeoOffice is distributed under
 *    GPL only under modification term 3 of the LGPL.
 *
 ************************************************************************/

// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_jvmfwk.hxx"

#include "osl/thread.h"
#include "otherjre.hxx"

#ifdef USE_JAVA

#include "sunversion.hxx"
#include "diagnostics.h"

#include <premac.h>
#include <Carbon/Carbon.h>
#include <postmac.h>

#define OUSTR(x) ::rtl::OUString( RTL_CONSTASCII_USTRINGPARAM(x) )

#endif	// USE_JAVA

using namespace rtl;
using namespace std;


namespace jfw_plugin
{

Reference<VendorBase> OtherInfo::createInstance()
{
    return new OtherInfo;
}


char const* const* OtherInfo::getJavaExePaths(int * size)
{
    static char const * ar[] = {
#ifdef WNT
        "bin/java.exe",
        "jre/bin/java.exe"
#elif UNX
        "bin/java",
        "jre/bin/java"
#endif
    };
        *size = sizeof (ar) / sizeof (char*);
    return ar;
}

char const* const* OtherInfo::getRuntimePaths(int * size)
{
    static char const* ar[]= {
#ifdef WNT
        "/bin/client/jvm.dll",
        "/bin/hotspot/jvm.dll",
        "/bin/classic/jvm.dll",
	"/bin/jrockit/jvm.dll"
#elif UNX
#ifdef MACOSX
#ifdef USE_JAVA
        // Fix bug 1257 by explicitly loading the JVM instead of loading the
        // shared JavaVM library
        "/../Libraries/libjvm.dylib"
#else	// USE_JAVA
        "/../../../JavaVM"
#endif	// USE_JAVA
#else
	"/bin/classic/libjvm.so", // for IBM Java
        "/jre/bin/classic/libjvm.so", // for IBM Java
        "/lib/" JFW_PLUGIN_ARCH "/client/libjvm.so", // for Blackdown PPC
        "/lib/" JFW_PLUGIN_ARCH "/server/libjvm.so", // for Blackdown AMD64
        "/lib/" JFW_PLUGIN_ARCH "/classic/libjvm.so", // for Blackdown PPC
	"/lib/" JFW_PLUGIN_ARCH "/jrockit/libjvm.so" // for Java of BEA Systems
#endif
#endif

    };
    *size = sizeof(ar) / sizeof (char*);
    return ar;
}

char const* const* OtherInfo::getLibraryPaths(int* size)
{

#ifdef UNX        
    static char const * ar[] = {
#ifdef MACOSX
        "/../Libraries",
        "/lib"
#else
        "/bin",
        "/jre/bin",
        "/bin/classic",
        "/jre/bin/classic",
        "/lib/" JFW_PLUGIN_ARCH "/client",
        "/lib/" JFW_PLUGIN_ARCH "/server",
        "/lib/" JFW_PLUGIN_ARCH "/classic",
        "/lib/" JFW_PLUGIN_ARCH "/jrockit",
        "/lib/" JFW_PLUGIN_ARCH "/native_threads",
        "/lib/" JFW_PLUGIN_ARCH
#endif
    };

    *size = sizeof(ar) / sizeof (char*);
    return ar;
#else
    size = 0;
    return NULL;
#endif
}

#ifdef USE_JAVA
int OtherInfo::compareVersions(const rtl::OUString& sSecond) const
#else	// USE_JAVA
int OtherInfo::compareVersions(const rtl::OUString& /*sSecond*/) const
#endif	// USE_JAVA
{
#ifdef USE_JAVA
    OUString sFirst = getVersion();
      
    SunVersion version1(sFirst);
    JFW_ENSURE(version1, OUSTR("[Java framework] sunjavaplugin"SAL_DLLEXTENSION
                               " does not know the version: ")
               + sFirst + OUSTR(" as valid for a SUN JRE."));
    // If we are running Leopard, don't allow loading of any JVM earlier than
    // Java 1.5.0
    static bool initializedOnce = false;
    static bool isLaterThanLeopard = false;
    static bool isLeopard = false;
    if ( ! initializedOnce )
    {
        long res = 0;
        Gestalt( gestaltSystemVersion, &res );
        if ( ( ( res >> 8 ) & 0x00FF ) == 0x10 )
		{
        	if ( ( ( res >> 4 ) & 0x000F ) > 0x5 )
        		isLaterThanLeopard = true;
        	else if ( ( ( res >> 4 ) & 0x000F ) == 0x5 )
        		isLeopard = true;
		}
        initializedOnce = true;
    }

    // Only run Java 1.5.x on Leopard as Java 1.4.x is crashy and Java 1.6.x
	// will hang on anything before Snow Leopard
    if ( isLaterThanLeopard && version1 < SunVersion( ::rtl::OUString::createFromAscii( "1.6.0" ) ) )
        return -1;
    else if ( !isLaterThanLeopard && version1 > SunVersion( ::rtl::OUString::createFromAscii( "1.5.999" ) ) )
        return -1;
    else if ( isLeopard && version1 < SunVersion( ::rtl::OUString::createFromAscii( "1.5.0" ) ) )
        return -1;

    SunVersion version2(sSecond);
    if ( ! version2)
        throw MalformedVersionException(); 
 
    if(version1 == version2)
        return 0;
    if(version1 > version2)
        return 1;
    else
        return -1;
#else	// USE_JAVA
    //Need to provide an own algorithm for comparing version. 
    //Because this function returns always 0, which means the version of
    //this JRE and the provided version "sSecond" are equal, one cannot put
    //any excludeVersion entries in the javavendors.xml file.
    return 0;
#endif	// USE_JAVA
}

}
