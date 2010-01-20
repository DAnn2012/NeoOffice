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
 *    Modified January 2008 by Patrick Luby. NeoOffice is distributed under
 *    GPL only under modification term 3 of the LGPL.
 *
 ************************************************************************/

// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_shell.hxx"

#ifndef _OSL_DIAGNOSE_H_
#include <osl/diagnose.h>
#endif

#ifndef _OSL_THREAD_H_
#include <osl/thread.h>
#endif

#ifndef _OSL_PROCESS_H_
#include <osl/process.h>
#endif

#ifndef _OSL_FILE_HXX_
#include <osl/file.hxx>
#endif

#ifndef _RTL_USTRBUF_HXX_
#include <rtl/ustrbuf.hxx>
#endif

#ifndef _RTL_URI_H_
#include <rtl/uri.hxx>
#endif

#ifndef _SHELLEXEC_HXX_
#include "shellexec.hxx"
#endif

#ifndef _COM_SUN_STAR_SYSTEM_SYSTEMSHELLEXECUTEFLAGS_HPP_
#include <com/sun/star/system/SystemShellExecuteFlags.hpp>
#endif

#ifndef _COM_SUN_STAR_URI_XEXTERNALURIREFERENCETRANSLATOR_HPP_
#include <com/sun/star/uri/XExternalUriReferenceTranslator.hpp>
#endif

#ifndef _COM_SUN_STAR_URI_EXTERNALURIREFERENCETRANSLATOR_HPP_
#include <com/sun/star/uri/ExternalUriReferenceTranslator.hpp>
#endif

#include "uno/current_context.hxx"

#include <string.h>
#include <errno.h>
#include <unistd.h>

//------------------------------------------------------------------------
// namespace directives
//------------------------------------------------------------------------

using com::sun::star::system::XSystemShellExecute;
using com::sun::star::system::SystemShellExecuteException;

using rtl::OString;
using rtl::OUString;
using rtl::OStringBuffer;
using rtl::OUStringBuffer;
using osl::FileBase;

using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::system::SystemShellExecuteFlags;
using namespace cppu;

//------------------------------------------------------------------------
// defines
//------------------------------------------------------------------------

#define SHELLEXEC_IMPL_NAME  "com.sun.star.comp.system.SystemShellExecute2"

//------------------------------------------------------------------------
// helper functions
//------------------------------------------------------------------------

namespace // private
{
    Sequence< OUString > SAL_CALL ShellExec_getSupportedServiceNames()
    {
        Sequence< OUString > aRet(1);
        aRet[0] = OUString::createFromAscii("com.sun.star.sys.shell.SystemShellExecute");
        return aRet;
    }
}

void escapeForShell( rtl::OStringBuffer & rBuffer, const rtl::OString & rURL)
{
    sal_Int32 nmax = rURL.getLength();
    for(sal_Int32 n=0; n < nmax; ++n)
    {
        // escape every non alpha numeric characters (excluding a few "known good") by prepending a '\'
        sal_Char c = rURL[n];
        if( ( c < 'A' || c > 'Z' ) && ( c < 'a' || c > 'z' ) && ( c < '0' || c > '9' )  && c != '/' && c != '.' )
            rBuffer.append( '\\' );
        
        rBuffer.append( c );
    }
}

//-----------------------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------------------

ShellExec::ShellExec( const Reference< XComponentContext >& xContext ) : 
    WeakImplHelper2< XSystemShellExecute, XServiceInfo >(),
    m_xContext(xContext)
{
    try {
        Reference< XCurrentContext > xCurrentContext(getCurrentContext());
        
        if (xCurrentContext.is())
        {
            Any aValue = xCurrentContext->getValueByName(
                OUString( RTL_CONSTASCII_USTRINGPARAM( "system.desktop-environment" ) ) );
        
            OUString aDesktopEnvironment;
            if (aValue >>= aDesktopEnvironment)
            {
                m_aDesktopEnvironment = OUStringToOString(aDesktopEnvironment, RTL_TEXTENCODING_ASCII_US);
            }
        }
    } catch (RuntimeException e) {
    }
}

//-------------------------------------------------
//
//-------------------------------------------------

void SAL_CALL ShellExec::execute( const OUString& aCommand, const OUString& aParameter, sal_Int32 /*nFlags*/ ) 
    throw (IllegalArgumentException, SystemShellExecuteException, RuntimeException)
{
    OStringBuffer aBuffer, aLaunchBuffer;

    // DESKTOP_LAUNCH, see http://freedesktop.org/pipermail/xdg/2004-August/004489.html
    static const char *pDesktopLaunch = getenv( "DESKTOP_LAUNCH" );
    
    // Check wether aCommand contains a document url or not
    sal_Int32 nIndex = aCommand.indexOf( OUString( RTL_CONSTASCII_USTRINGPARAM(":/") ) );
    
    if( nIndex > 0 || 0 == aCommand.compareToAscii("mailto:", 7) )
    {
        // It seems to be a url ..
        // We need to re-encode file urls because osl_getFileURLFromSystemPath converts
        // to UTF-8 before encoding non ascii characters, which is not what other apps
        // expect.
        OUString aURL(
            com::sun::star::uri::ExternalUriReferenceTranslator::create(
                m_xContext)->translateToExternal(aCommand));
        if ( aURL.getLength() == 0 && aCommand.getLength() != 0 )
        {
            throw RuntimeException(
                (OUString(
                    RTL_CONSTASCII_USTRINGPARAM(
                        "Cannot translate URI reference to external format: "))
                 + aCommand),
                static_cast< cppu::OWeakObject * >(this));
        }
        
#ifdef MACOSX
        aBuffer.append("open");
#else
        OUString aProgramURL;
        if ( osl_Process_E_None != osl_getExecutableFile(&aProgramURL.pData) )
        {
            throw SystemShellExecuteException(
                OUString(RTL_CONSTASCII_USTRINGPARAM("Cound not determine executable path")), 
                static_cast < XSystemShellExecute * > (this), ENOENT );
        }
        
        OUString aProgram;
        if ( FileBase::E_None != FileBase::getSystemPathFromFileURL(aProgramURL, aProgram))
        {
            throw SystemShellExecuteException(
                OUString(RTL_CONSTASCII_USTRINGPARAM("Cound not convert executable path")), 
                static_cast < XSystemShellExecute * > (this), ENOENT );
        }
        
        // The url launchers are expected to be in the same directory as the main executable,
        // so prefixing the launchers with the path of the executable including the last slash 
        OString aTmp = OUStringToOString(aProgram, osl_getThreadTextEncoding());
        nIndex = aTmp.lastIndexOf('/');
        if (nIndex > 0)
            escapeForShell(aBuffer, aTmp.copy(0, nIndex+1));
            
        // Respect the desktop environment - if there is an executable named 
        // <desktop-environement-is>-open-url, pass the url to this one instead
        // of the default "open-url" script.
        if ( m_aDesktopEnvironment.getLength() > 0 )
        {
            OString aDesktopEnvironment(m_aDesktopEnvironment.toAsciiLowerCase());
            OStringBuffer aCopy(aBuffer);
            
            aCopy.append(aDesktopEnvironment);
            aCopy.append("-open-url");
            
            if ( 0 == access( aCopy.getStr(), X_OK) )
            {
                aBuffer.append(aDesktopEnvironment);
                aBuffer.append("-");

                /* CDE requires file urls to be decoded */                
                if ( m_aDesktopEnvironment.equals("CDE") && 0 == aURL.compareToAscii("file://", 7) )
                {
                    aURL = rtl::Uri::decode(aURL, rtl_UriDecodeWithCharset, osl_getThreadTextEncoding());
                }
            }
        }
             
        aBuffer.append("open-url");
#endif
        aBuffer.append(" ");
        escapeForShell(aBuffer, OUStringToOString(aURL, osl_getThreadTextEncoding()));
        
        if ( pDesktopLaunch && *pDesktopLaunch )
        {
            aLaunchBuffer.append( pDesktopLaunch );
            aLaunchBuffer.append(" ");
            escapeForShell(aLaunchBuffer, OUStringToOString(aURL, osl_getThreadTextEncoding()));
        }
    } else {
        escapeForShell(aBuffer, OUStringToOString(aCommand, osl_getThreadTextEncoding()));
        aBuffer.append(" ");
        escapeForShell(aBuffer, OUStringToOString(aParameter, osl_getThreadTextEncoding()));
    }
    
#ifndef USE_JAVA
    // Prefer DESKTOP_LAUNCH when available
    if ( aLaunchBuffer.getLength() > 0 )
    {
        FILE *pLaunch = popen( aLaunchBuffer.makeStringAndClear().getStr(), "w" );
        if ( pLaunch != NULL )
        {
            if ( 0 == pclose( pLaunch ) )
                return;
        }
        // Failed, do not try DESKTOP_LAUNCH any more
        pDesktopLaunch = NULL;
    }
#endif	// !USE_JAVA

    OString cmd = aBuffer.makeStringAndClear();
#ifdef USE_JAVA
    // Fix bug 3584 by only throwing an exception if we can execute the command
    // and ignoring the return value of the command
    FILE *pLaunch = popen(cmd.getStr(), "w");
    if ( pLaunch )
    {
        pclose(pLaunch);
    }
    else
#else	// USE_JAVA
    if ( 0 != pclose(popen(cmd.getStr(), "w")) )
#endif	// USE_JAVA
    {
        int nerr = errno;
        throw SystemShellExecuteException(OUString::createFromAscii( strerror( nerr ) ), 
            static_cast < XSystemShellExecute * > (this), nerr );
    }
}


// -------------------------------------------------
// XServiceInfo
// -------------------------------------------------

OUString SAL_CALL ShellExec::getImplementationName(  ) 
    throw( RuntimeException )
{
	return OUString::createFromAscii( SHELLEXEC_IMPL_NAME );
}

// -------------------------------------------------
//	XServiceInfo
// -------------------------------------------------

sal_Bool SAL_CALL ShellExec::supportsService( const OUString& ServiceName ) 
    throw( RuntimeException )
{
    Sequence < OUString > SupportedServicesNames = ShellExec_getSupportedServiceNames();

    for ( sal_Int32 n = SupportedServicesNames.getLength(); n--; )
        if (SupportedServicesNames[n].compareTo(ServiceName) == 0)
            return sal_True;

    return sal_False;
}

// -------------------------------------------------
//	XServiceInfo
// -------------------------------------------------

Sequence< OUString > SAL_CALL ShellExec::getSupportedServiceNames(	 ) 
    throw( RuntimeException )
{
    return ShellExec_getSupportedServiceNames();
}

