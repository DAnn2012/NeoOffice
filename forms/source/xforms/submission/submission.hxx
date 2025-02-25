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

#ifndef INCLUDED_FORMS_SOURCE_XFORMS_SUBMISSION_SUBMISSION_HXX
#define INCLUDED_FORMS_SOURCE_XFORMS_SUBMISSION_SUBMISSION_HXX

#include <tools/urlobj.hxx>
#include <rtl/ustring.h>
#include <osl/conditn.hxx>
#include <osl/mutex.hxx>
#include <comphelper/processfactory.hxx>
#include <com/sun/star/uno/Reference.hxx>
#include <com/sun/star/uno/Any.hxx>
#include <com/sun/star/uno/Exception.hpp>
#include <com/sun/star/uno/RuntimeException.hpp>
#include <com/sun/star/xml/xpath/XXPathObject.hpp>
#include <com/sun/star/xml/dom/XDocumentFragment.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>

#include <com/sun/star/ucb/XCommandEnvironment.hpp>
#include <com/sun/star/ucb/XProgressHandler.hpp>

#include <com/sun/star/task/XInteractionHandler.hpp>

#include <com/sun/star/frame/XFrame.hpp>

#include <cppuhelper/implbase1.hxx>
#include <cppuhelper/implbase2.hxx>
#include <cppuhelper/implbase3.hxx>

#include "serialization.hxx"

#include <memory>

class CSubmissionPut;
class CSubmissionPost;
class CSubmissionGet;

class CCommandEnvironmentHelper : public cppu::WeakImplHelper1< css::ucb::XCommandEnvironment >
{
    friend class CSubmissionPut;
    friend class CSubmissionPost;
    friend class CSubmissionGet;
    friend class CSubmission;

protected:
    css::uno::Reference< css::task::XInteractionHandler >   m_aInteractionHandler;
    css::uno::Reference< css::ucb::XProgressHandler >       m_aProgressHandler;

public:
    virtual css::uno::Reference< css::task::XInteractionHandler > SAL_CALL getInteractionHandler() throw (css::uno::RuntimeException, std::exception) SAL_OVERRIDE
    {
        return m_aInteractionHandler;
    }
    virtual css::uno::Reference< css::ucb::XProgressHandler > SAL_CALL getProgressHandler() throw (css::uno::RuntimeException, std::exception) SAL_OVERRIDE
    {
        return m_aProgressHandler;
    }
};

class CProgressHandlerHelper : public cppu::WeakImplHelper1< css::ucb::XProgressHandler >
{
    friend class CSubmissionPut;
    friend class CSubmissionPost;
    friend class CSubmissionGet;
protected:
    osl::Condition m_cFinished;
    osl::Mutex m_mLock;
    sal_Int32 m_count;
public:
    CProgressHandlerHelper()
        : m_count(0)
    {}
    virtual void SAL_CALL push( const com::sun::star::uno::Any& /*aStatus*/) throw(com::sun::star::uno::RuntimeException, std::exception) SAL_OVERRIDE
    {
        m_mLock.acquire();
        m_count++;
        m_mLock.release();
    }
    virtual void SAL_CALL update(const com::sun::star::uno::Any& /*aStatus*/) throw(com::sun::star::uno::RuntimeException, std::exception) SAL_OVERRIDE
    {
    }
    virtual void SAL_CALL pop() throw(com::sun::star::uno::RuntimeException, std::exception) SAL_OVERRIDE
    {
        m_mLock.acquire();
        m_count--;
        if (m_count == 0)
            m_cFinished.set();
        m_mLock.release();
    }
};

class CSubmission
{

protected:
    INetURLObject m_aURLObj;
    css::uno::Reference< css::xml::xpath::XXPathObject >    m_aXPathObject;
    css::uno::Reference< css::xml::dom::XDocumentFragment > m_aFragment;
    css::uno::Reference< css::io::XInputStream >            m_aResultStream;
    css::uno::Reference< css::uno::XComponentContext >      m_xContext;
    OUString m_aEncoding;

    ::std::unique_ptr< CSerialization > createSerialization(const ::com::sun::star::uno::Reference< ::com::sun::star::task::XInteractionHandler >& aHandler
                                                  ,com::sun::star::uno::Reference<com::sun::star::ucb::XCommandEnvironment>& _rOutEnv);

public:
    enum SubmissionResult {
        SUCCESS,
        INVALID_METHOD,
        INVALID_URL,
        INVALID_ENCODING,
        E_TRANSMISSION,
        UNKNOWN_ERROR
    };

    CSubmission(const OUString& aURL, const css::uno::Reference< css::xml::dom::XDocumentFragment >& aFragment)
        : m_aURLObj(aURL)
        , m_aFragment(aFragment)
        , m_xContext(::comphelper::getProcessComponentContext())
    {}

#ifndef NO_LIBO_WEB_PROTOCOL_FIX
    bool IsWebProtocol() const
    {
        INetProtocol eProtocol = m_aURLObj.GetProtocol();
        return eProtocol == INET_PROT_HTTP || eProtocol == INET_PROT_HTTPS;
    }
#endif	// !NO_LIBO_WEB_PROTOCOL_FIX

    virtual ~CSubmission() {}

    virtual void setEncoding(const OUString& aEncoding)
    {
        m_aEncoding = aEncoding;
    }
    virtual SubmissionResult submit(const css::uno::Reference< css::task::XInteractionHandler >& ) = 0;

    virtual SubmissionResult replace(const OUString&, const css::uno::Reference< css::xml::dom::XDocument >&, const css::uno::Reference< css::frame::XFrame>&);

};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
