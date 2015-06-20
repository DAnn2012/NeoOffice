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

#include <osl/diagnose.h>
#include <com/sun/star/uno/XComponentContext.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/drawing/XDrawPageSupplier.hpp>
#include <com/sun/star/io/XInputStream.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#if SUPD == 310
#include <comphelper/mediadescriptor.hxx>
#else	// SUPD == 310
#include <unotools/mediadescriptor.hxx>
#endif	// SUPD == 310
#include <cppuhelper/supportsservice.hxx>
#include <oox/core/filterdetect.hxx>
#include <dmapper/DomainMapper.hxx>
#include <WriterFilter.hxx>
#include <ooxml/OOXMLDocument.hxx>
#ifdef DEBUG_WRITERFILTER
#include <iostream>
#include <osl/process.h>
#endif

#include <resourcemodel/TagLogger.hxx>
#include <oox/ole/olestorage.hxx>
#include <oox/ole/vbaproject.hxx>
#include <oox/helper/graphichelper.hxx>

#if SUPD == 310
#include <sal/log.hxx>
#endif	// SUPD == 310

using namespace ::rtl;
using namespace ::com::sun::star;
#if SUPD == 310
using comphelper::MediaDescriptor;
#else	// SUPD == 310
using utl::MediaDescriptor;
#endif	// SUPD == 310



sal_Bool WriterFilter::filter( const uno::Sequence< beans::PropertyValue >& aDescriptor )
#if SUPD == 310
   throw (uno::RuntimeException)
#else	// SUPD == 310
   throw (uno::RuntimeException, std::exception)
#endif	// SUPD == 310
{
    if( m_xSrcDoc.is() )
    {
        uno::Reference< lang::XMultiServiceFactory > xMSF(m_xContext->getServiceManager(), uno::UNO_QUERY_THROW);
        uno::Reference< uno::XInterface > xIfc( xMSF->createInstance("com.sun.star.comp.Writer.DocxExport"), uno::UNO_QUERY_THROW);
        if (!xIfc.is())
            return sal_False;
        uno::Reference< document::XExporter > xExprtr(xIfc, uno::UNO_QUERY_THROW);
        uno::Reference< document::XFilter > xFltr(xIfc, uno::UNO_QUERY_THROW);
        if (!xExprtr.is() || !xFltr.is())
            return sal_False;
        xExprtr->setSourceDocument(m_xSrcDoc);
        return xFltr->filter(aDescriptor);
    }
    else if (m_xDstDoc.is())
    {
#if SUPD == 310
        comphelper::MediaDescriptor aMediaDesc( aDescriptor );
#else	// SUPD == 310
        utl::MediaDescriptor aMediaDesc( aDescriptor );
#endif	// SUPD == 310
        bool bRepairStorage = aMediaDesc.getUnpackedValueOrDefault( "RepairPackage", false );

        uno::Reference< io::XInputStream > xInputStream;
        try
        {
            // use the oox.core.FilterDetect implementation to extract the decrypted ZIP package
            ::oox::core::FilterDetect aDetector( m_xContext );
            xInputStream = aDetector.extractUnencryptedPackage( aMediaDesc );
        }
        catch( uno::Exception& )
        {
        }

        if ( !xInputStream.is() )
            return sal_False;

#ifdef DEBUG_WRITERFILTER
        OUString sURL = aMediaDesc.getUnpackedValueOrDefault( utl::MediaDescriptor::PROP_URL(), OUString() );
        ::std::string sURLc = OUStringToOString(sURL, RTL_TEXTENCODING_ASCII_US).getStr();

        writerfilter::TagLogger::Pointer_t dmapper_logger
        (writerfilter::TagLogger::getInstance("DOMAINMAPPER"));
        if (getenv("SW_DEBUG_WRITERFILTER"))
            dmapper_logger->setFileName(sURLc);
        dmapper_logger->startDocument();
#endif

        writerfilter::dmapper::SourceDocumentType eType = writerfilter::dmapper::DOCUMENT_OOXML;
        writerfilter::dmapper::DomainMapper* aDomainMapper = new writerfilter::dmapper::DomainMapper(m_xContext, xInputStream, m_xDstDoc, bRepairStorage, eType, uno::Reference<text::XTextRange>());
        writerfilter::Stream::Pointer_t pStream(aDomainMapper);
        //create the tokenizer and domain mapper
        writerfilter::ooxml::OOXMLStream::Pointer_t pDocStream = writerfilter::ooxml::OOXMLDocumentFactory::createStream(m_xContext, xInputStream, bRepairStorage);
#if SUPD == 310
        uno::Reference<task::XStatusIndicator> xStatusIndicator = aMediaDesc.getUnpackedValueOrDefault(comphelper::MediaDescriptor::PROP_STATUSINDICATOR(), uno::Reference<task::XStatusIndicator>());
#else	// SUPD == 310
        uno::Reference<task::XStatusIndicator> xStatusIndicator = aMediaDesc.getUnpackedValueOrDefault(utl::MediaDescriptor::PROP_STATUSINDICATOR(), uno::Reference<task::XStatusIndicator>());
#endif	// SUPD == 310
        writerfilter::ooxml::OOXMLDocument::Pointer_t pDocument(writerfilter::ooxml::OOXMLDocumentFactory::createDocument(pDocStream, xStatusIndicator));

        uno::Reference<frame::XModel> xModel(m_xDstDoc, uno::UNO_QUERY_THROW);
        pDocument->setModel(xModel);

        uno::Reference<drawing::XDrawPageSupplier> xDrawings
            (m_xDstDoc, uno::UNO_QUERY_THROW);
        uno::Reference<drawing::XDrawPage> xDrawPage
            (xDrawings->getDrawPage(), uno::UNO_SET_THROW);
        pDocument->setDrawPage(xDrawPage);

        pDocument->resolve(*pStream);

        // Adding some properties to the document's grab bag for interoperability purposes:
        comphelper::SequenceAsHashMap aGrabBagProperties;

        // Adding the saved Theme DOM
        aGrabBagProperties["OOXTheme"] = uno::makeAny( pDocument->getThemeDom() );

        // Adding the saved custom xml DOM
        aGrabBagProperties["OOXCustomXml"] = uno::makeAny( pDocument->getCustomXmlDomList() );
        aGrabBagProperties["OOXCustomXmlProps"] = uno::makeAny( pDocument->getCustomXmlDomPropsList() );

        // Adding the saved ActiveX DOM
        aGrabBagProperties["OOXActiveX"] = uno::makeAny( pDocument->getActiveXDomList() );
        aGrabBagProperties["OOXActiveXBin"] = uno::makeAny( pDocument->getActiveXBinList() );

        // Adding the saved w:themeFontLang setting
        aGrabBagProperties["ThemeFontLangProps"] = uno::makeAny( aDomainMapper->GetThemeFontLangProperties() );

        // Adding the saved Glossary Documnet DOM to the document's grab bag
        aGrabBagProperties["OOXGlossary"] = uno::makeAny( pDocument->getGlossaryDocDom() );
        aGrabBagProperties["OOXGlossaryDom"] = uno::makeAny( pDocument->getGlossaryDomList() );

        // Adding the saved embedding document to document's grab bag
        aGrabBagProperties["OOXEmbeddings"] = uno::makeAny( pDocument->getEmbeddingsList() );

        // Adding the saved compat settings
        aGrabBagProperties["CompatSettings"] = uno::makeAny( aDomainMapper->GetCompatSettings() );

        putPropertiesToDocumentGrabBag( aGrabBagProperties );

        writerfilter::ooxml::OOXMLStream::Pointer_t  pVBAProjectStream(writerfilter::ooxml::OOXMLDocumentFactory::createStream( pDocStream, writerfilter::ooxml::OOXMLStream::VBAPROJECT ));
        oox::StorageRef xVbaPrjStrg( new ::oox::ole::OleStorage( m_xContext, pVBAProjectStream->getDocumentStream(), false ) );
        if( xVbaPrjStrg.get() && xVbaPrjStrg->isStorage() )
        {
            ::oox::ole::VbaProject aVbaProject( m_xContext, xModel, "Writer" );
#if SUPD == 310
            uno::Reference< frame::XFrame > xFrame = aMediaDesc.getUnpackedValueOrDefault(  comphelper::MediaDescriptor::PROP_FRAME(), uno::Reference< frame::XFrame > () );
#else	// SUPD == 310
            uno::Reference< frame::XFrame > xFrame = aMediaDesc.getUnpackedValueOrDefault(  utl::MediaDescriptor::PROP_FRAME(), uno::Reference< frame::XFrame > () );
#endif	// SUPD == 310

            // if no XFrame try fallback to what we can glean from the Model
            if ( !xFrame.is() )
            {
                uno::Reference< frame::XController > xController =  xModel->getCurrentController();
                xFrame =  xController.is() ? xController->getFrame() : nullptr;
            }

            oox::GraphicHelper gHelper( m_xContext, xFrame, xVbaPrjStrg );
            aVbaProject.importVbaProject( *xVbaPrjStrg, gHelper );
        }

        // Document signature.
        writerfilter::ooxml::OOXMLStream::Pointer_t pSignatureStream;
        pSignatureStream = writerfilter::ooxml::OOXMLDocumentFactory::createStream(m_xContext, xInputStream, bRepairStorage, writerfilter::ooxml::OOXMLStream::SIGNATURE);
        if (pSignatureStream->getDocumentStream().is())
        {
            // TODO found, handle it.
        }

        pStream.reset();

#ifdef DEBUG_WRITERFILTER
        dmapper_logger->endDocument();
#endif

        return sal_True;
    }
    return sal_False;
}


#if SUPD == 310
void WriterFilter::cancel(  ) throw (uno::RuntimeException)
#else	// SUPD == 310
void WriterFilter::cancel(  ) throw (uno::RuntimeException, std::exception)
#endif	// SUPD == 310
{
}



void WriterFilter::setTargetDocument( const uno::Reference< lang::XComponent >& xDoc )
#if SUPD == 310
   throw (lang::IllegalArgumentException, uno::RuntimeException)
#else	// SUPD == 310
   throw (lang::IllegalArgumentException, uno::RuntimeException, std::exception)
#endif	// SUPD == 310
{
   m_xDstDoc = xDoc;

   // Set some compatibility options that are valid for all the formats
   uno::Reference< lang::XMultiServiceFactory > xFactory( xDoc, uno::UNO_QUERY );
   uno::Reference< beans::XPropertySet > xSettings( xFactory->createInstance("com.sun.star.document.Settings"), uno::UNO_QUERY );

   xSettings->setPropertyValue( "AddFrameOffsets", uno::makeAny( sal_True ) );
   xSettings->setPropertyValue( "UseOldNumbering", uno::makeAny( sal_False ) );
   xSettings->setPropertyValue( "IgnoreFirstLineIndentInNumbering", uno::makeAny( sal_False ) );
   xSettings->setPropertyValue( "DoNotResetParaAttrsForNumFont", uno::makeAny( sal_False ) );
   xSettings->setPropertyValue( "UseFormerLineSpacing", uno::makeAny( sal_False ) );
   xSettings->setPropertyValue( "AddParaSpacingToTableCells", uno::makeAny( sal_True ) );
   xSettings->setPropertyValue( "UseFormerObjectPositioning", uno::makeAny( sal_False ) );
   xSettings->setPropertyValue( "ConsiderTextWrapOnObjPos", uno::makeAny( sal_True ) );
   xSettings->setPropertyValue( "UseFormerTextWrapping", uno::makeAny( sal_False ) );
   xSettings->setPropertyValue( "TableRowKeep", uno::makeAny( sal_True ) );
   xSettings->setPropertyValue( "IgnoreTabsAndBlanksForLineCalculation", uno::makeAny( sal_True ) );
   xSettings->setPropertyValue( "InvertBorderSpacing", uno::makeAny( sal_True ) );
   xSettings->setPropertyValue( "CollapseEmptyCellPara", uno::makeAny( sal_True ) );
   xSettings->setPropertyValue( "TabOverflow", uno::makeAny( sal_True ) );
   xSettings->setPropertyValue( "UnbreakableNumberings", uno::makeAny( sal_True ) );

   xSettings->setPropertyValue("FloattableNomargins", uno::makeAny( sal_True ));
   xSettings->setPropertyValue( "ClippedPictures", uno::makeAny( sal_True ) );
   xSettings->setPropertyValue( "BackgroundParaOverDrawings", uno::makeAny( sal_True ) );
   xSettings->setPropertyValue( "TabOverMargin", uno::makeAny( sal_True ) );
   xSettings->setPropertyValue("PropLineSpacingShrinksFirstLine", uno::makeAny(sal_True));
}

void WriterFilter::setSourceDocument( const uno::Reference< lang::XComponent >& xDoc )
#if SUPD == 310
   throw (lang::IllegalArgumentException, uno::RuntimeException)
#else	// SUPD == 310
   throw (lang::IllegalArgumentException, uno::RuntimeException, std::exception)
#endif	// SUPD == 310
{
   m_xSrcDoc = xDoc;
}



#if SUPD == 310
void WriterFilter::initialize( const uno::Sequence< uno::Any >& /*aArguments*/ ) throw (uno::Exception, uno::RuntimeException)
#else	// SUPD == 310
void WriterFilter::initialize( const uno::Sequence< uno::Any >& /*aArguments*/ ) throw (uno::Exception, uno::RuntimeException, std::exception)
#endif	// SUPD == 310
{
}


OUString WriterFilter_getImplementationName () throw (uno::RuntimeException)
{
   return OUString ( "com.sun.star.comp.Writer.WriterFilter" );
}

uno::Sequence< OUString > WriterFilter_getSupportedServiceNames(  ) throw (uno::RuntimeException)
{
   uno::Sequence < OUString > aRet(2);
   OUString* pArray = aRet.getArray();
   pArray[0] = "com.sun.star.document.ImportFilter";
   pArray[1] = "com.sun.star.document.ExportFilter";
   return aRet;
}

uno::Reference< uno::XInterface > WriterFilter_createInstance( const uno::Reference< uno::XComponentContext >& xContext)
                throw( uno::Exception )
{
   return (cppu::OWeakObject*) new WriterFilter( xContext );
}



#if SUPD == 310
OUString WriterFilter::getImplementationName(  ) throw (uno::RuntimeException)
#else	// SUPD == 310
OUString WriterFilter::getImplementationName(  ) throw (uno::RuntimeException, std::exception)
#endif	// SUPD == 310
{
   return WriterFilter_getImplementationName();
}


#if SUPD == 310
sal_Bool WriterFilter::supportsService( const OUString& rServiceName ) throw (uno::RuntimeException)
#else	// SUPD == 310
sal_Bool WriterFilter::supportsService( const OUString& rServiceName ) throw (uno::RuntimeException, std::exception)
#endif	// SUPD == 310
{
    return cppu::supportsService( this, rServiceName );
}


#if SUPD == 310
uno::Sequence< OUString > WriterFilter::getSupportedServiceNames(  ) throw (uno::RuntimeException)
#else	// SUPD == 310
uno::Sequence< OUString > WriterFilter::getSupportedServiceNames(  ) throw (uno::RuntimeException, std::exception)
#endif	// SUPD == 310
{
    return WriterFilter_getSupportedServiceNames();
}

void WriterFilter::putPropertiesToDocumentGrabBag( const comphelper::SequenceAsHashMap& rProperties )
{
    try
    {
        uno::Reference<beans::XPropertySet> xDocProps(m_xDstDoc, uno::UNO_QUERY);
        if( xDocProps.is() )
        {
            uno::Reference<beans::XPropertySetInfo> xPropsInfo = xDocProps->getPropertySetInfo();

            const OUString aGrabBagPropName = "InteropGrabBag";
            if( xPropsInfo.is() && xPropsInfo->hasPropertyByName( aGrabBagPropName ) )
            {
                // get existing grab bag
                comphelper::SequenceAsHashMap aGrabBag(xDocProps->getPropertyValue(aGrabBagPropName));

                // put the new items
                aGrabBag.update(rProperties);

                // put it back to the document
                xDocProps->setPropertyValue(aGrabBagPropName, uno::Any(aGrabBag.getAsConstPropertyValueList()));
            }
        }
    }
    catch(const uno::Exception&)
    {
        SAL_WARN("writerfilter","Failed to save documents grab bag");
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
