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
 * 
 *   Modified December 2016 by Patrick Luby. NeoOffice is only distributed
 *   under the GNU General Public License, Version 3 as allowed by Section 3.3
 *   of the Mozilla Public License, v. 2.0.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <iostream>
#include <boost/shared_ptr.hpp>
#include "OOXMLFastDocumentHandler.hxx"
#include "OOXMLFastContextHandler.hxx"
#include "oox/token/tokens.hxx"
#include "OOXMLFactory.hxx"

namespace writerfilter {
namespace ooxml
{
using namespace ::com::sun::star;
using namespace ::std;


OOXMLFastDocumentHandler::OOXMLFastDocumentHandler(
    uno::Reference< uno::XComponentContext > const & context,
    Stream* pStream,
    OOXMLDocumentImpl* pDocument,
    sal_Int32 nXNoteId )
    : m_xContext(context)
    , mpStream( pStream )
#ifdef DEBUG_WRITERFILTER
    , mpTmpStream()
#endif
    , mpDocument( pDocument )
    , mnXNoteId( nXNoteId )
    , mpContextHandler()
{
}

// ::com::sun::star::xml::sax::XFastContextHandler:
void SAL_CALL OOXMLFastDocumentHandler::startFastElement
(::sal_Int32
#ifdef DEBUG_WRITERFILTER
Element
#endif
, const uno::Reference< xml::sax::XFastAttributeList > & /*Attribs*/)
    throw (uno::RuntimeException, xml::sax::SAXException, std::exception)
{
#ifdef DEBUG_WRITERFILTER
    clog << this << ":start element:"
         << fastTokenToId(Element)
         << endl;
#endif
}

void SAL_CALL OOXMLFastDocumentHandler::startUnknownElement
(const OUString &
#ifdef DEBUG_WRITERFILTER
Namespace
#endif
, const OUString &
#ifdef DEBUG_WRITERFILTER
Name
#endif
,
 const uno::Reference< xml::sax::XFastAttributeList > & /*Attribs*/)
throw (uno::RuntimeException, xml::sax::SAXException, std::exception)
{
#ifdef DEBUG_WRITERFILTER
    clog << this << ":start unknown element:"
         << OUStringToOString(Namespace, RTL_TEXTENCODING_ASCII_US).getStr()
         << ":"
         << OUStringToOString(Name, RTL_TEXTENCODING_ASCII_US).getStr()
         << endl;
#endif
}

void SAL_CALL OOXMLFastDocumentHandler::endFastElement(::sal_Int32
#ifdef DEBUG_WRITERFILTER
Element
#endif
)
throw (uno::RuntimeException, xml::sax::SAXException, std::exception)
{
#ifdef DEBUG_WRITERFILTER
    clog << this << ":end element:"
         << fastTokenToId(Element)
         << endl;
#endif
}

void SAL_CALL OOXMLFastDocumentHandler::endUnknownElement
(const OUString &
#ifdef DEBUG_WRITERFILTER
Namespace
#endif
, const OUString &
#ifdef DEBUG_WRITERFILTER
Name
#endif
)
throw (uno::RuntimeException, xml::sax::SAXException, std::exception)
{
#ifdef DEBUG_WRITERFILTER
    clog << this << ":end unknown element:"
         << OUStringToOString(Namespace, RTL_TEXTENCODING_ASCII_US).getStr()
         << ":"
         << OUStringToOString(Name, RTL_TEXTENCODING_ASCII_US).getStr()
         << endl;
#endif
}

OOXMLFastContextHandler::Pointer_t
OOXMLFastDocumentHandler::getContextHandler() const
{
    if (mpContextHandler == OOXMLFastContextHandler::Pointer_t())
    {
#ifdef USE_JAVA
        // Fix crashing bug reported in the following Debian bug when opening
        // a .docx that has no <w:document> tag:
        // http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=550359
        if (!mpDocument)
            throw uno::RuntimeException("OOXMLFastDocumentHandler::mpDocument is NULL", uno::Reference< XInterface >());
#endif	// USE_JAVA
        mpContextHandler.reset
        (new OOXMLFastContextHandler(m_xContext));
        mpContextHandler->setStream(mpStream);
        mpContextHandler->setDocument(mpDocument);
        mpContextHandler->setXNoteId(mnXNoteId);
        mpContextHandler->setForwardEvents(true);
    }

    return mpContextHandler;
}

uno::Reference< xml::sax::XFastContextHandler > SAL_CALL
 OOXMLFastDocumentHandler::createFastChildContext
(::sal_Int32 Element,
 const uno::Reference< xml::sax::XFastAttributeList > & /*Attribs*/)
    throw (uno::RuntimeException, xml::sax::SAXException, std::exception)
{
#ifdef DEBUG_WRITERFILTER
    clog << this << ":createFastChildContext:"
         << fastTokenToId(Element)
         << endl;
#endif

    if ( mpStream == nullptr && mpDocument == nullptr )
    {
        // document handler has been created as unknown child - see <OOXMLFastDocumentHandler::createUnknownChildContext(..)>
        // --> do not provide a child context
        return nullptr;
    }

    return OOXMLFactory::getInstance()->createFastChildContextFromStart(getContextHandler().get(), Element);
}

uno::Reference< xml::sax::XFastContextHandler > SAL_CALL
OOXMLFastDocumentHandler::createUnknownChildContext
(const OUString &
#ifdef DEBUG_WRITERFILTER
Namespace
#endif
,
 const OUString &
#ifdef DEBUG_WRITERFILTER
Name
#endif
, const uno::Reference< xml::sax::XFastAttributeList > & /*Attribs*/)
    throw (uno::RuntimeException, xml::sax::SAXException, std::exception)
{
#ifdef DEBUG_WRITERFILTER
    clog << this << ":createUnknownChildContext:"
         << OUStringToOString(Namespace, RTL_TEXTENCODING_ASCII_US).getStr()
         << ":"
         << OUStringToOString(Name, RTL_TEXTENCODING_ASCII_US).getStr()
         << endl;
#endif

    return uno::Reference< xml::sax::XFastContextHandler >
        ( new OOXMLFastDocumentHandler( m_xContext, nullptr, nullptr, 0 ) );
}

void SAL_CALL OOXMLFastDocumentHandler::characters(const OUString & /*aChars*/)
    throw (uno::RuntimeException, xml::sax::SAXException, std::exception)
{
}

// ::com::sun::star::xml::sax::XFastDocumentHandler:
void SAL_CALL OOXMLFastDocumentHandler::startDocument()
    throw (uno::RuntimeException, xml::sax::SAXException, std::exception)
{
}

void SAL_CALL OOXMLFastDocumentHandler::endDocument()
    throw (uno::RuntimeException, xml::sax::SAXException, std::exception)
{
}

void SAL_CALL OOXMLFastDocumentHandler::setDocumentLocator
(const uno::Reference< xml::sax::XLocator > & /*xLocator*/)
    throw (uno::RuntimeException, xml::sax::SAXException, std::exception)
{
}

void OOXMLFastDocumentHandler::setIsSubstream( bool bSubstream )
{
    if ( mpStream != nullptr && mpDocument != nullptr )
    {
        getContextHandler( )->getParserState( )->setInSectionGroup( bSubstream );
    }
}

}}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
