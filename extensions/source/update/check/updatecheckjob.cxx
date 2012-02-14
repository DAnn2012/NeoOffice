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
 * Modified February 2012 by Patrick Luby. NeoOffice is distributed under
 * GPL only under modification term 2 of the LGPL.
 *
 ************************************************************************/

// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_extensions.hxx"

#include "updatecheck.hxx"
#include "updatecheckconfig.hxx"
#include "updatehdl.hxx"
#include "updateprotocol.hxx"

#include <cppuhelper/implbase2.hxx>
#include <cppuhelper/implementationentry.hxx>

#include <com/sun/star/task/XJob.hpp>

namespace beans = com::sun::star::beans ;
namespace lang = com::sun::star::lang ;
namespace task = com::sun::star::task ;
namespace uno = com::sun::star::uno ;

#define UNISTRING(s) rtl::OUString(RTL_CONSTASCII_USTRINGPARAM(s))

namespace 
{ 

class UpdateCheckJob : 
    public ::cppu::WeakImplHelper2< task::XJob, lang::XServiceInfo >
{    
    virtual ~UpdateCheckJob();
             
public:
    
    UpdateCheckJob(const uno::Reference<uno::XComponentContext>& xContext);

    static uno::Sequence< rtl::OUString > getServiceNames();
    static rtl::OUString getImplName();

    // Allows runtime exceptions to be thrown by const methods
    inline SAL_CALL operator uno::Reference< uno::XInterface > () const
        { return const_cast< cppu::OWeakObject * > (static_cast< cppu::OWeakObject const * > (this)); };
    
    // XJob
    virtual uno::Any SAL_CALL execute(const uno::Sequence<beans::NamedValue>&) 
        throw (lang::IllegalArgumentException, uno::Exception);
        
    // XServiceInfo
    virtual rtl::OUString SAL_CALL getImplementationName() 
        throw (uno::RuntimeException);
    virtual sal_Bool SAL_CALL supportsService(rtl::OUString const & serviceName) 
        throw (uno::RuntimeException);
    virtual uno::Sequence< rtl::OUString > SAL_CALL getSupportedServiceNames() 
        throw (uno::RuntimeException);

private:
    uno::Reference<uno::XComponentContext> m_xContext;

    void handleExtensionUpdates( const uno::Sequence< beans::NamedValue > &rListProp );
};

//------------------------------------------------------------------------------

UpdateCheckJob::UpdateCheckJob(const uno::Reference<uno::XComponentContext>& xContext) : 
    m_xContext(xContext)
{
}

//------------------------------------------------------------------------------

UpdateCheckJob::~UpdateCheckJob()
{    
}

//------------------------------------------------------------------------------

uno::Sequence< rtl::OUString > 
UpdateCheckJob::getServiceNames()
{
    uno::Sequence< rtl::OUString > aServiceList(1);
    aServiceList[0] = UNISTRING( "com.sun.star.setup.UpdateCheck");
    return aServiceList;
};

//------------------------------------------------------------------------------

rtl::OUString 
UpdateCheckJob::getImplName()
{ 
    return UNISTRING( "vnd.sun.UpdateCheck");
}


//------------------------------------------------------------------------------

uno::Any 
UpdateCheckJob::execute(const uno::Sequence<beans::NamedValue>& namedValues) 
    throw (lang::IllegalArgumentException, uno::Exception)
{
    for ( sal_Int32 n=namedValues.getLength(); n-- > 0; )
    {
        if ( namedValues[ n ].Name.equalsAscii( "DynamicData" ) )
        {
            uno::Sequence<beans::NamedValue> aListProp;
            if ( namedValues[n].Value >>= aListProp )
            {
                for ( sal_Int32 i=aListProp.getLength(); i-- > 0; )
                {
                    if ( aListProp[ i ].Name.equalsAscii( "updateList" ) )
                    {
                        handleExtensionUpdates( aListProp );
                        return uno::Any();
                    }
                }
            }
        }
    }
    uno::Sequence<beans::NamedValue> aConfig = 
        getValue< uno::Sequence<beans::NamedValue> > (namedValues, "JobConfig");
    
    rtl::Reference<UpdateCheck> aController(UpdateCheck::get());
    aController->initialize(aConfig, m_xContext);
    
    /* Determine the way we got invoked here - 
     * see Developers Guide Chapter "4.7.2 Jobs" to understand the magic
     */
    
    uno::Sequence<beans::NamedValue> aEnvironment = 
        getValue< uno::Sequence<beans::NamedValue> > (namedValues, "Environment");
    
    rtl::OUString aEventName = getValue< rtl::OUString > (aEnvironment, "EventName");
    
    if( ! aEventName.equalsAscii("onFirstVisibleTask") )
    {
#ifdef USE_JAVA
        if( aEventName.equalsAscii("OnCloseApp") )
            aController->onCloseApp();
        else
#endif	// USE_JAVA
        aController->showDialog(true);
    }
    
    return uno::Any();
}

//------------------------------------------------------------------------------
void UpdateCheckJob::handleExtensionUpdates( const uno::Sequence< beans::NamedValue > &rListProp ) 
{
    try {
        uno::Sequence< uno::Sequence< rtl::OUString > > aList =
            getValue< uno::Sequence< uno::Sequence< rtl::OUString > > > ( rListProp, "updateList" );
        bool bPrepareOnly = getValue< bool > ( rListProp, "prepareOnly" );

        // we will first store any new found updates and then check, if there are any
        // pending updates. 
        storeExtensionUpdateInfos( m_xContext, aList );

        if ( bPrepareOnly )
            return;

        bool bHasUpdates = checkForPendingUpdates( m_xContext );

        rtl::Reference<UpdateCheck> aController( UpdateCheck::get() );
        if ( ! aController.is() )
            return;

        aController->setHasExtensionUpdates( bHasUpdates );

        if ( ! aController->hasOfficeUpdate() )
        {
            if ( bHasUpdates )
                aController->setUIState( UPDATESTATE_EXT_UPD_AVAIL, true );
            else
                aController->setUIState( UPDATESTATE_NO_UPDATE_AVAIL, true );
        }
    }
    catch( const uno::Exception& e )
    {
         OSL_TRACE( "Caught exception: %s\n thread terminated.\n",
            rtl::OUStringToOString(e.Message, RTL_TEXTENCODING_UTF8).getStr());
    }
}

//------------------------------------------------------------------------------

rtl::OUString SAL_CALL 
UpdateCheckJob::getImplementationName() throw (uno::RuntimeException)
{
    return getImplName();
}

//------------------------------------------------------------------------------

uno::Sequence< rtl::OUString > SAL_CALL 
UpdateCheckJob::getSupportedServiceNames() throw (uno::RuntimeException)
{
    return getServiceNames();
}

//------------------------------------------------------------------------------

sal_Bool SAL_CALL 
UpdateCheckJob::supportsService( rtl::OUString const & serviceName ) throw (uno::RuntimeException)
{
    uno::Sequence< rtl::OUString > aServiceNameList = getServiceNames();
    
    for( sal_Int32 n=0; n < aServiceNameList.getLength(); n++ )
        if( aServiceNameList[n].equals(serviceName) )
            return sal_True;
    
    return sal_False;
}

} // anonymous namespace

//------------------------------------------------------------------------------

static uno::Reference<uno::XInterface> SAL_CALL 
createJobInstance(const uno::Reference<uno::XComponentContext>& xContext)
{
    return *new UpdateCheckJob(xContext);
}

//------------------------------------------------------------------------------

static uno::Reference<uno::XInterface> SAL_CALL 
createConfigInstance(const uno::Reference<uno::XComponentContext>& xContext)
{
    return *UpdateCheckConfig::get(xContext, *UpdateCheck::get());
}

//------------------------------------------------------------------------------

static const cppu::ImplementationEntry kImplementations_entries[] = 
{
    {
        createJobInstance,
        UpdateCheckJob::getImplName,
        UpdateCheckJob::getServiceNames,
        cppu::createSingleComponentFactory,
        NULL,
        0
    },
    {
        createConfigInstance,
        UpdateCheckConfig::getImplName,
        UpdateCheckConfig::getServiceNames,
        cppu::createSingleComponentFactory,
        NULL,
        0
    },
	{ NULL, NULL, NULL, NULL, NULL, 0 }
} ;

//------------------------------------------------------------------------------

extern "C" void SAL_CALL 
component_getImplementationEnvironment( const sal_Char **aEnvTypeName, uno_Environment **) 
{
    *aEnvTypeName = CPPU_CURRENT_LANGUAGE_BINDING_NAME ;
}

//------------------------------------------------------------------------------

extern "C" sal_Bool SAL_CALL 
component_writeInfo(void *pServiceManager, void *pRegistryKey) 
{
    return cppu::component_writeInfoHelper(
        pServiceManager,
        pRegistryKey,
        kImplementations_entries
    );
}

//------------------------------------------------------------------------------

extern "C" void *
component_getFactory(const sal_Char *pszImplementationName, void *pServiceManager, void *pRegistryKey) 
{
    return cppu::component_getFactoryHelper(
        pszImplementationName,
        pServiceManager,
        pRegistryKey,
        kImplementations_entries) ;
}
