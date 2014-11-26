/*************************************************************************
 *
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 * 
 * Copyright 2008 by Sun Microsystems, Inc.
 *
 * OpenOffice.org - a multi-platform office productivity suite
 *
 * $RCSfile$
 * $Revision$
 *
 * This file is part of OpenOffice.org.
 *
 * OpenOffice.org is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License version 3
 * only, as published by the Free Software Foundation.
 *
 * OpenOffice.org is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License version 3 for more details
 * (a copy is included in the LICENSE file that accompanied this code).
 *
 * You should have received a copy of the GNU Lesser General Public License
 * version 3 along with OpenOffice.org.  If not, see
 * <http://www.openoffice.org/license.html>
 * for a copy of the LGPLv3 License.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Portions of this file are part of the LibreOffice project.
 *
 *   This Source Code Form is subject to the terms of the Mozilla Public
 *   License, v. 2.0. If a copy of the MPL was not distributed with this
 *   file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 ************************************************************************/

#include "docxexportfilter.hxx"
#include "docxexport.hxx"

#include <docsh.hxx>
#include <pam.hxx>
#include <unotxdoc.hxx>

#include <cppuhelper/factory.hxx>

#if SUPD == 310
#include <comphelper/processfactory.hxx>
#endif	// SUPD == 310

using namespace ::comphelper;
using namespace ::com::sun::star;
using ::rtl::OUString;

#define S( x ) OUString( RTL_CONSTASCII_USTRINGPARAM( x ) )

DocxExportFilter::DocxExportFilter( const uno::Reference< lang::XMultiServiceFactory >& rMSF )
#if SUPD == 310
    : oox::core::XmlFilterBase( ::getProcessComponentContext() )
#else	// SUPD == 310
    : oox::core::XmlFilterBase( rMSF )
#endif	// SUPD == 310
{
}

bool DocxExportFilter::exportDocument()
{
    fprintf( stderr, "DocxExportFilter::exportDocument()\n" ); // DEBUG remove me

    // get SwDoc*
    uno::Reference< uno::XInterface > xIfc( getModel(), uno::UNO_QUERY );
    SwXTextDocument *pTxtDoc = dynamic_cast< SwXTextDocument * >( xIfc.get() );
    if ( !pTxtDoc )
        return false;

    SwDoc *pDoc = pTxtDoc->GetDocShell()->GetDoc();
    if ( !pDoc )
        return false;

    // get SwPaM*
    // FIXME so far we get SwPaM for the entire document; probably we should
    // be able to output just the selection as well - though no idea how to
    // get the correct SwPaM* then...
    SwPaM aPam( pDoc->GetNodes().GetEndOfContent() );
    aPam.SetMark();
    aPam.Move( fnMoveBackward, fnGoDoc );

    SwPaM *pCurPam = new SwPaM( *aPam.End(), *aPam.Start() );

    // export the document
    // (in a separate block so that it's destructed before the commit)
    {
        DocxExport aExport( this, pDoc, pCurPam, &aPam );
        aExport.ExportDocument( true ); // FIXME support exporting selection only
    }

#if SUPD == 310
    commitStorage();
#else	// SUPD == 310
    commit();
#endif	// SUPD == 310

    // delete the pCurPam
    if ( pCurPam )
    {
        while ( pCurPam->GetNext() != pCurPam )
            delete pCurPam->GetNext();
        delete pCurPam;
    }

    return true;
}

//////////////////////////////////////////////////////////////////////////
// UNO stuff so that the filter is registered
//////////////////////////////////////////////////////////////////////////

#define IMPL_NAME "com.sun.star.comp.Writer.DocxExport"

OUString DocxExport_getImplementationName()
{
    return OUString( RTL_CONSTASCII_USTRINGPARAM( IMPL_NAME ) );
}

OUString DocxExportFilter::implGetImplementationName() const
{
    return DocxExport_getImplementationName();
}

uno::Sequence< OUString > SAL_CALL DocxExport_getSupportedServiceNames() throw()
{
    const OUString aServiceName( RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.document.ExportFilter" ) );
    const uno::Sequence< OUString > aSeq( &aServiceName, 1 );
    return aSeq;
}

uno::Reference< uno::XInterface > SAL_CALL DocxExport_createInstance(const uno::Reference< lang::XMultiServiceFactory > & rSMgr ) throw( uno::Exception )
{
    return (cppu::OWeakObject*) new DocxExportFilter( rSMgr );
}

#ifdef __cplusplus
extern "C"
{
#endif

SAL_DLLPUBLIC_EXPORT void SAL_CALL component_getImplementationEnvironment( const sal_Char ** ppEnvTypeName, uno_Environment ** /* ppEnv */ )
{
    *ppEnvTypeName = CPPU_CURRENT_LANGUAGE_BINDING_NAME;
}

SAL_DLLPUBLIC_EXPORT sal_Bool SAL_CALL component_writeInfo( void* /* pServiceManager */, void* pRegistryKey )
{
    sal_Bool bRet = sal_False;

    if( pRegistryKey )
    {
        try
        {
            uno::Reference< registry::XRegistryKey > xNewKey1(
                    static_cast< registry::XRegistryKey* >( pRegistryKey )->createKey(                                
                        OUString::createFromAscii( IMPL_NAME "/UNO/SERVICES/" ) ) );
            xNewKey1->createKey( DocxExport_getSupportedServiceNames().getConstArray()[0] );

            bRet = sal_True;
        }
        catch( registry::InvalidRegistryException& )
        {
            OSL_ENSURE( sal_False, "### InvalidRegistryException!" );
        }
    }

    return bRet;
}

// ------------------------
// - component_getFactory -
// ------------------------

SAL_DLLPUBLIC_EXPORT void* SAL_CALL component_getFactory( const sal_Char* pImplName, void* pServiceManager, void* /* pRegistryKey */ )
{
    uno::Reference< lang::XSingleServiceFactory > xFactory;
    void* pRet = 0;

    if ( rtl_str_compare( pImplName, IMPL_NAME ) == 0 )
    {
        const OUString aServiceName( OUString::createFromAscii( IMPL_NAME ) );

        xFactory = uno::Reference< lang::XSingleServiceFactory >( ::cppu::createSingleFactory(
                    reinterpret_cast< lang::XMultiServiceFactory* >( pServiceManager ),
                    DocxExport_getImplementationName(),
                    DocxExport_createInstance,
                    DocxExport_getSupportedServiceNames() ) );
    }

    if ( xFactory.is() )
    {
        xFactory->acquire();
        pRet = xFactory.get();
    }

    return pRet;
}

#ifdef __cplusplus
}
#endif

/* vi:set tabstop=4 shiftwidth=4 expandtab: */
