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
 * Modified November 2008 by Patrick Luby. NeoOffice is distributed under
 * GPL only under modification term 2 of the LGPL.
 *
 ************************************************************************/

// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_shell.hxx"

//------------------------------------------------------------------------
// includes
//------------------------------------------------------------------------
#include <osl/diagnose.h>
#include <osl/thread.h>

#include <rtl/bootstrap.hxx>

#include <osl/file.hxx>
#include <rtl/strbuf.hxx>
#include "cmdmailsuppl.hxx"
#include "cmdmailmsg.hxx"
#include <com/sun/star/system/SimpleMailClientFlags.hpp>
#include <com/sun/star/container/XNameAccess.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>

#include <string.h>
#include <errno.h>
#include <unistd.h>

#ifdef USE_JAVA
#include "cmdmailsuppl_cocoa.h"
#endif  // USE_JAVA

//------------------------------------------------------------------------
// namespace directives
//------------------------------------------------------------------------

using com::sun::star::beans::PropertyValue;
using com::sun::star::system::XSimpleMailClientSupplier;
using com::sun::star::system::XSimpleMailClient;
using com::sun::star::system::XSimpleMailMessage;
using com::sun::star::container::XNameAccess;
using com::sun::star::container::NoSuchElementException;
using rtl::OUString;
using rtl::OUStringToOString;
using rtl::OString;
using rtl::OStringBuffer;
using osl::MutexGuard;
using osl::FileBase;

using namespace cppu;
using namespace com::sun::star::system::SimpleMailClientFlags;
using namespace com::sun::star::uno;
using namespace com::sun::star::lang;

//------------------------------------------------------------------------
// defines
//------------------------------------------------------------------------

#define COMP_IMPL_NAME  "com.sun.star.comp.system.SimpleCommandMail2"

//------------------------------------------------------------------------
// helper functions
//------------------------------------------------------------------------

namespace // private
{
    Sequence< OUString > SAL_CALL Component_getSupportedServiceNames()
    {
        Sequence< OUString > aRet(1);
        aRet[0] = OUString::createFromAscii("com.sun.star.system.SimpleCommandMail");
        return aRet;
    }

} // end private namespace

//-------------------------------------------------
//
//-------------------------------------------------

CmdMailSuppl::CmdMailSuppl( const Reference< XComponentContext >& xContext ) : 
	WeakImplHelper3< XSimpleMailClientSupplier, XSimpleMailClient, XServiceInfo >()
{
    Reference< XMultiComponentFactory > xServiceManager = xContext->getServiceManager();
            
    if ( xServiceManager.is() ) {
        m_xConfigurationProvider = Reference< XMultiServiceFactory > (
            xServiceManager->createInstanceWithContext( 
                OUString::createFromAscii( "com.sun.star.configuration.ConfigurationProvider" ), xContext ),
            UNO_QUERY );
    }
}

//-------------------------------------------------
// XSimpleMailClientSupplier
//-------------------------------------------------

Reference< XSimpleMailClient > SAL_CALL CmdMailSuppl::querySimpleMailClient(  ) 
    throw (RuntimeException)
{
    return static_cast < XSimpleMailClient * > (this);
}

//------------------------------------------------
// XSimpleMailClient
//------------------------------------------------

Reference< XSimpleMailMessage > SAL_CALL CmdMailSuppl::createSimpleMailMessage(  ) 
        throw (::com::sun::star::uno::RuntimeException)
{
    return Reference< XSimpleMailMessage >( new CmdMailMsg(  ) );
}

//------------------------------------------------
// XSimpleMailClient
//------------------------------------------------

void SAL_CALL CmdMailSuppl::sendSimpleMailMessage( const Reference< XSimpleMailMessage >& xSimpleMailMessage, sal_Int32 /*aFlag*/ ) 
    throw (IllegalArgumentException, Exception, RuntimeException)
{
    if ( ! xSimpleMailMessage.is() )
    {
        throw ::com::sun::star::lang::IllegalArgumentException( 
            OUString(RTL_CONSTASCII_USTRINGPARAM( "No message specified" )), 
            static_cast < XSimpleMailClient * > (this), 1 );
    }

    if( ! m_xConfigurationProvider.is() )
    {
        throw ::com::sun::star::uno::Exception( 
            OUString(RTL_CONSTASCII_USTRINGPARAM( "Can not access configuration" )), 
            static_cast < XSimpleMailClient * > (this) );
    }

#ifdef USE_JAVA
    OUString aMailerPath;
#else	// USE_JAVA
    OStringBuffer aBuffer;
    aBuffer.append("\"");

    OUString aProgramURL(RTL_CONSTASCII_USTRINGPARAM("$OOO_BASE_DIR/program/senddoc"));
    rtl::Bootstrap::expandMacros(aProgramURL);
    
    OUString aProgram;
    if ( FileBase::E_None != FileBase::getSystemPathFromFileURL(aProgramURL, aProgram))
    {
        throw ::com::sun::star::uno::Exception(
            OUString(RTL_CONSTASCII_USTRINGPARAM("Cound not convert executable path")), 
            static_cast < XSimpleMailClient * > (this));
    }

    aBuffer.append(OUStringToOString(aProgram, osl_getThreadTextEncoding()));
    aBuffer.append("\" ");
#endif	// USE_JAVA

    try
    {
        // Query XNameAccess interface of the org.openoffice.Office.Common/ExternalMailer
        // configuration node to retriece the users preferred email application. This may
        // transparently by redirected to e.g. the corresponding GConf setting in GNOME.
        OUString aConfigRoot = OUString( 
            RTL_CONSTASCII_USTRINGPARAM( "org.openoffice.Office.Common/ExternalMailer" ) );

        PropertyValue aProperty;
        aProperty.Name = OUString::createFromAscii( "nodepath" );
        aProperty.Value = makeAny( aConfigRoot );

        Sequence< Any > aArgumentList( 1 );
        aArgumentList[0] = makeAny( aProperty );

        Reference< XNameAccess > xNameAccess = 
            Reference< XNameAccess > (
                m_xConfigurationProvider->createInstanceWithArguments(
                    OUString::createFromAscii( "com.sun.star.configuration.ConfigurationAccess" ),
                    aArgumentList ),
                UNO_QUERY );

        if( xNameAccess.is() )
        {
            OUString aMailer;

            // Retrieve the value for "Program" node and append it feed senddoc with it
            // using the (undocumented) --mailclient switch
            xNameAccess->getByName( OUString::createFromAscii( "Program" ) ) >>= aMailer;

            if( aMailer.getLength() )
            {
                // make sure we have a system path
                FileBase::getSystemPathFromFileURL( aMailer, aMailer );

#ifdef USE_JAVA
                aMailerPath = aMailer;
#else	// USE_JAVA
                aBuffer.append("--mailclient ");
                aBuffer.append(OUStringToOString( aMailer, osl_getThreadTextEncoding() ));
                aBuffer.append(" ");
#endif	// USE_JAVA
            }
#if defined MACOSX && !defined USE_JAVA
            else
                aBuffer.append("--mailclient Mail ");
#endif
        }

    }

    catch( RuntimeException e )
    {
        m_xConfigurationProvider.clear();
        OSL_TRACE( "RuntimeException caught accessing configuration provider." );
        OSL_TRACE( OUStringToOString( e.Message, RTL_TEXTENCODING_ASCII_US ).getStr() );
        throw e;                
    }
        
#ifndef USE_JAVA
    // Append originator if set in the message
    if ( xSimpleMailMessage->getOriginator().getLength() > 0 )
    {
        aBuffer.append("--from \"");
        aBuffer.append(OUStringToOString(xSimpleMailMessage->getOriginator(), osl_getThreadTextEncoding()));
        aBuffer.append("\" ");
    }

    // Append receipient if set in the message
    if ( xSimpleMailMessage->getRecipient().getLength() > 0 )
    {
        aBuffer.append("--to \"");
        aBuffer.append(OUStringToOString(xSimpleMailMessage->getRecipient(), osl_getThreadTextEncoding()));
        aBuffer.append("\" ");
    }
    
    // Append carbon copy receipients set in the message
    Sequence< OUString > aStringList = xSimpleMailMessage->getCcRecipient();
    sal_Int32 n, nmax = aStringList.getLength();
    for ( n = 0; n < nmax; n++ )
    {
        aBuffer.append("--cc \"");
        aBuffer.append(OUStringToOString(aStringList[n], osl_getThreadTextEncoding()));
        aBuffer.append("\" ");
    }

    // Append blind carbon copy receipients set in the message
    aStringList = xSimpleMailMessage->getBccRecipient();
    nmax = aStringList.getLength();
    for ( n = 0; n < nmax; n++ )
    {
        aBuffer.append("--bcc \"");
        aBuffer.append(OUStringToOString(aStringList[n], osl_getThreadTextEncoding()));
        aBuffer.append("\" ");
    }

    // Append subject if set in the message
    if ( xSimpleMailMessage->getSubject().getLength() > 0 )
    {
        aBuffer.append("--subject \"");
        aBuffer.append(OUStringToOString(xSimpleMailMessage->getSubject(), osl_getThreadTextEncoding()));
        aBuffer.append("\" ");
    }

    // Append attachments set in the message
    aStringList = xSimpleMailMessage->getAttachement();
    nmax = aStringList.getLength();
    for ( n = 0; n < nmax; n++ )
    {
        OUString aSystemPath;
        if ( FileBase::E_None == FileBase::getSystemPathFromFileURL(aStringList[n], aSystemPath) )
        {
            aBuffer.append("--attach \"");
            aBuffer.append(OUStringToOString(aSystemPath, osl_getThreadTextEncoding()));
            aBuffer.append("\" ");
        }
    }
#endif	// !USE_JAVA

#ifdef USE_JAVA
    Sequence< OUString > aStringList = xSimpleMailMessage->getAttachement();
    if ( !CmdMailSuppl_sendSimpleMailMessage( aStringList, aMailerPath ) )
#else	// USE_JAVA
    OString cmd = aBuffer.makeStringAndClear();
    if ( 0 != pclose(popen(cmd.getStr(), "w")) )
#endif	// USE_JAVA
    {
        throw ::com::sun::star::uno::Exception(
            OUString(RTL_CONSTASCII_USTRINGPARAM( "No mail client configured" )), 
            static_cast < XSimpleMailClient * > (this) );
    }   
}

// -------------------------------------------------
// XServiceInfo
// -------------------------------------------------

OUString SAL_CALL CmdMailSuppl::getImplementationName(  ) 
    throw( RuntimeException )
{
    return OUString::createFromAscii( COMP_IMPL_NAME );
}

// -------------------------------------------------
//	XServiceInfo
// -------------------------------------------------

sal_Bool SAL_CALL CmdMailSuppl::supportsService( const OUString& ServiceName ) 
    throw( RuntimeException )
{
    Sequence < OUString > SupportedServicesNames = Component_getSupportedServiceNames();

    for ( sal_Int32 n = SupportedServicesNames.getLength(); n--; )
        if (SupportedServicesNames[n].compareTo(ServiceName) == 0)
            return sal_True;

    return sal_False;
}

// -------------------------------------------------
//	XServiceInfo
// -------------------------------------------------

Sequence< OUString > SAL_CALL CmdMailSuppl::getSupportedServiceNames(	 ) 
    throw( RuntimeException )
{
    return Component_getSupportedServiceNames();
}

