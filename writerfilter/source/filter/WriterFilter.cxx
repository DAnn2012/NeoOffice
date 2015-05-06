/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 */

#include <cppuhelper/implementationentry.hxx>
#include <WriterFilter.hxx>
#include <WriterFilterDetection.hxx>
#include <RtfFilter.hxx>

using namespace ::com::sun::star;

WriterFilter::WriterFilter( const uno::Reference< uno::XComponentContext >& rxContext)  :
    m_xContext( rxContext )
{
}


WriterFilter::~WriterFilter()
{
}

extern "C"
{
/* shared lib exports implemented with helpers */
static const struct ::cppu::ImplementationEntry s_component_entries [] =
{
    { WriterFilter_createInstance, WriterFilter_getImplementationName, WriterFilter_getSupportedServiceNames, ::cppu::createSingleComponentFactory, nullptr, 0 },
    { WriterFilterDetection_createInstance, WriterFilterDetection_getImplementationName, WriterFilterDetection_getSupportedServiceNames, ::cppu::createSingleComponentFactory, nullptr, 0} ,
    { RtfFilter_createInstance, RtfFilter_getImplementationName, RtfFilter_getSupportedServiceNames, ::cppu::createSingleComponentFactory, nullptr, 0 },
    { nullptr, nullptr, nullptr, nullptr, nullptr, 0 } // terminate with NULL
};

#if SUPD == 310

void SAL_CALL component_getImplementationEnvironment(const sal_Char ** ppEnvTypeName, uno_Environment ** /*ppEnv*/ )
{
    *ppEnvTypeName = CPPU_CURRENT_LANGUAGE_BINDING_NAME;
}

sal_Bool SAL_CALL component_writeInfo( ::com::sun::star::lang::XMultiServiceFactory * xMgr, ::com::sun::star::registry::XRegistryKey * xRegistry )
{
    return ::cppu::component_writeInfoHelper( xMgr, xRegistry, s_component_entries );
}

void * SAL_CALL component_getFactory(sal_Char const * implName, ::com::sun::star::lang::XMultiServiceFactory * xMgr, ::com::sun::star::registry::XRegistryKey * xRegistry )
#else	// SUPD == 310
SAL_DLLPUBLIC_EXPORT void * SAL_CALL writerfilter_component_getFactory(sal_Char const * implName, void * xMgr, void * xRegistry )
#endif	// SUPD == 310
{
    return ::cppu::component_getFactoryHelper(implName, xMgr, xRegistry, s_component_entries );
}

} //extern "C"


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
