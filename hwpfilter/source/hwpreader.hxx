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
 *   Modified March 2018 by Patrick Luby. NeoOffice is only distributed
 *   under the GNU General Public License, Version 3 as allowed by Section 3.3
 *   of the Mozilla Public License, v. 2.0.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDED_HWPFILTER_SOURCE_HWPREADER_HXX
#define INCLUDED_HWPFILTER_SOURCE_HWPREADER_HXX
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sal/alloca.h>

#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/lang/XComponent.hpp>
#include <com/sun/star/io/XInputStream.hpp>
#include <com/sun/star/document/XFilter.hpp>
#include <com/sun/star/document/XImporter.hpp>
#include <com/sun/star/xml/sax/XDocumentHandler.hpp>

#include <com/sun/star/io/XActiveDataSink.hpp>
#include <com/sun/star/io/XActiveDataControl.hpp>
#include <com/sun/star/io/XStreamListener.hpp>
#include <com/sun/star/document/XExtendedFilterDetection.hpp>

#include <cppuhelper/factory.hxx>
#include <cppuhelper/implbase1.hxx>
#include <cppuhelper/implbase2.hxx>
#include <cppuhelper/implbase4.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <cppuhelper/weak.hxx>

using namespace ::cppu;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::io;
using namespace ::com::sun::star::registry;
using namespace ::com::sun::star::document;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::xml::sax;

#include <assert.h>

#include <unotools/mediadescriptor.hxx>

#include "hwpfile.h"
#include "hcode.h"
#include "hbox.h"
#include "htags.h"
#include "hstream.h"
#include "drawdef.h"
#include "attributes.hxx"

#define IMPLEMENTATION_NAME "com.sun.comp.hwpimport.HwpImportFilter"
#define SERVICE_NAME1 "com.sun.star.document.ImportFilter"
#define SERVICE_NAME2 "com.sun.star.document.ExtendedTypeDetection"
#define WRITER_IMPORTER_NAME "com.sun.star.comp.Writer.XMLImporter"

struct HwpReaderPrivate;
/**
 * This class implements the external Parser interface
 */
class HwpReader : public WeakImplHelper1<XFilter>
{

public:
    HwpReader();
    virtual ~HwpReader();

public:
    /**
     * parseStream does Parser-startup initializations
     */
    virtual sal_Bool SAL_CALL filter(const Sequence< PropertyValue >& aDescriptor) throw (RuntimeException, std::exception) SAL_OVERRIDE;
    virtual void SAL_CALL cancel() throw(RuntimeException, std::exception) SAL_OVERRIDE {}
    virtual void SAL_CALL setDocumentHandler(Reference< XDocumentHandler > xHandler)
    {
        m_rxDocumentHandler = xHandler;
    }
private:
    Reference< XDocumentHandler > m_rxDocumentHandler;
    Reference< XAttributeList > rList;
    AttributeListImpl *pList;
    HWPFile hwpfile;
    HwpReaderPrivate *d;
private:
    /* -------- Document Parsing --------- */
    void makeMeta();
    void makeStyles();
    void makeDrawMiscStyle(HWPDrawingObject *);
    void makeAutoStyles();
    void makeMasterStyles();
    void makeBody();

    void makeTextDecls();

    /* -------- Paragraph Parsing --------- */
    void parsePara(HWPPara *para, bool bParaStart = false);
    void make_text_p0(HWPPara *para, bool bParaStart = false);
    void make_text_p1(HWPPara *para, bool bParaStart = false);
    void make_text_p3(HWPPara *para, bool bParaStart = false);

    /* -------- rDocument->characters(x) --------- */
    void makeChars(hchar_string & rStr);

    /* -------- Special Char Parsing --------- */
    void makeFieldCode(hchar_string & rStr, FieldCode *hbox); //6
    void makeBookmark(Bookmark *hbox);      //6
    void makeDateFormat(DateCode *hbox);    //7
    void makeDateCode(DateCode *hbox);      //8
    void makeTab(Tab *hbox);            //9
    void makeTable(TxtBox *hbox);
    void makeTextBox(TxtBox *hbox);
    void makeFormula(TxtBox *hbox);
    void makeHyperText(TxtBox *hbox);
    void makePicture(Picture *hbox);
    void makePictureDRAW(HWPDrawingObject *drawobj, Picture *hbox);
    void makeLine(Line *hbox);
    void makeHidden(Hidden *hbox);
    void makeFootnote(Footnote *hbox);
    void makeAutoNum(AutoNum *hbox);
    void makeShowPageNum();
    void makeMailMerge(MailMerge *hbox);
    void makeTocMark(TocMark *hbox);
    void makeIndexMark(IndexMark *hbox);
    void makeOutline(Outline *hbox);

    /* --------- Styles Parsing ------------ */
    void makePageStyle();
    void makeColumns(ColumnDef *);
    void makeTStyle(CharShape *);
    void makePStyle(ParaShape *);
    void makeFStyle(FBoxStyle *);
    void makeCaptionStyle(FBoxStyle *);
    void makeDrawStyle(HWPDrawingObject *,FBoxStyle *);
    void makeTableStyle(Table *);
    void parseCharShape(CharShape *);
    void parseParaShape(ParaShape *);
    char* getTStyleName(int, char *);
    char* getPStyleName(int, char *);
};

class HwpImportFilter : public WeakImplHelper4< XFilter, XImporter, XServiceInfo, XExtendedFilterDetection >
{
public:
    HwpImportFilter( const Reference< XMultiServiceFactory > xFact );
    virtual ~HwpImportFilter();

public:
    static Sequence< OUString > getSupportedServiceNames_Static() throw();
    static OUString getImplementationName_Static() throw();

public:
    // XFilter
    virtual sal_Bool SAL_CALL filter( const Sequence< PropertyValue >& aDescriptor )
        throw( RuntimeException, std::exception ) SAL_OVERRIDE;
    virtual void SAL_CALL cancel() throw(RuntimeException, std::exception) SAL_OVERRIDE;

    // XImporter
    virtual void SAL_CALL setTargetDocument( const Reference< XComponent >& xDoc)
        throw( IllegalArgumentException, RuntimeException, std::exception ) SAL_OVERRIDE;

    // XServiceInfo
    OUString SAL_CALL getImplementationName() throw (RuntimeException, std::exception) SAL_OVERRIDE;
    Sequence< OUString > SAL_CALL getSupportedServiceNames(void) throw (::com::sun::star::uno::RuntimeException, std::exception) SAL_OVERRIDE;
    sal_Bool SAL_CALL supportsService(const OUString& ServiceName) throw (::com::sun::star::uno::RuntimeException, std::exception) SAL_OVERRIDE;

    //XExtendedFilterDetection
    virtual OUString SAL_CALL detect( ::com::sun::star::uno::Sequence< ::com::sun::star::beans::PropertyValue >& rDescriptor ) throw (::com::sun::star::uno::RuntimeException, std::exception) SAL_OVERRIDE;

public:
    Reference< XFilter > rFilter;
    Reference< XImporter > rImporter;
};

Reference< XInterface > HwpImportFilter_CreateInstance(
    const Reference< XMultiServiceFactory >& rSMgr ) throw( Exception )
{
    HwpImportFilter *p = new HwpImportFilter( rSMgr );

    return Reference< XInterface > ( (OWeakObject* )p );
}

Sequence< OUString > HwpImportFilter::getSupportedServiceNames_Static() throw ()
{
    Sequence< OUString > aRet(1);
    aRet.getArray()[0] = HwpImportFilter::getImplementationName_Static();
    return aRet;
}

HwpImportFilter::HwpImportFilter( const Reference< XMultiServiceFactory > xFact )
{
    OUString sService( WRITER_IMPORTER_NAME );
    try {
        Reference< XDocumentHandler >
            xHandler( xFact->createInstance( sService ), UNO_QUERY );

        HwpReader *p = new HwpReader;
        p->setDocumentHandler( xHandler );

        Reference< XImporter > xImporter = Reference< XImporter >( xHandler, UNO_QUERY );
        rImporter = xImporter;
        Reference< XFilter > xFilter = Reference< XFilter >( p );
        rFilter = xFilter;
    }
    catch( Exception & )
    {
        printf(" fail to instanciate %s\n", WRITER_IMPORTER_NAME );
        exit( 1 );
    }
}

HwpImportFilter::~HwpImportFilter()
{
}

sal_Bool HwpImportFilter::filter( const Sequence< PropertyValue >& aDescriptor )
    throw( RuntimeException, std::exception )
{
    // delegate to IchitaroImpoter
    return rFilter->filter( aDescriptor );
}

void HwpImportFilter::cancel() throw(::com::sun::star::uno::RuntimeException, std::exception)
{
    rFilter->cancel();
}

void HwpImportFilter::setTargetDocument( const Reference< XComponent >& xDoc )
    throw( IllegalArgumentException, RuntimeException, std::exception )
{
        // delegate
    rImporter->setTargetDocument( xDoc );
}

OUString HwpImportFilter::getImplementationName_Static() throw()
{
    return OUString( IMPLEMENTATION_NAME );
}

OUString HwpImportFilter::getImplementationName() throw(::com::sun::star::uno::RuntimeException, std::exception)
{
    return OUString( IMPLEMENTATION_NAME );
}

sal_Bool HwpImportFilter::supportsService( const OUString& ServiceName ) throw(::com::sun::star::uno::RuntimeException, std::exception)
{
    return cppu::supportsService(this, ServiceName);
}

//XExtendedFilterDetection
OUString HwpImportFilter::detect( ::com::sun::star::uno::Sequence< ::com::sun::star::beans::PropertyValue >& rDescriptor ) throw (::com::sun::star::uno::RuntimeException, std::exception)
{
#ifdef USE_JAVA
    // Fix crash on some machines when handling stream by catching any
    // unexpected exception types
    try
    {
#endif	// USE_JAVA
    OUString sTypeName;

    utl::MediaDescriptor aDescriptor(rDescriptor);
    aDescriptor.addInputStream();

    Reference< XInputStream > xInputStream(
        aDescriptor[utl::MediaDescriptor::PROP_INPUTSTREAM()], UNO_QUERY);

    if (xInputStream.is())
    {
        Sequence< sal_Int8 > aData;
        sal_Int32 nLen = HWPIDLen;
        if (
             nLen == xInputStream->readBytes(aData, nLen) &&
             detect_hwp_version(reinterpret_cast<const char*>(aData.getConstArray()))
           )
        {
            sTypeName = OUString("writer_MIZI_Hwp_97");
        }
    }

    return sTypeName;
#ifdef USE_JAVA
    }
    catch( const com::sun::star::uno::RuntimeException &e )
    {
        throw;
    }
    catch( const std::exception &e )
    {
        throw;
    }
    catch ( ... )
    {
        throw RuntimeException( "HwpImportFilter::detect method raised an unknown exception" );
    }
#endif	// USE_JAVA
}

Sequence< OUString> HwpImportFilter::getSupportedServiceNames() throw(::com::sun::star::uno::RuntimeException, std::exception)
{
    Sequence < OUString > aRet(2);
    OUString* pArray = aRet.getArray();
    pArray[0] = OUString(SERVICE_NAME1);
    pArray[1] = OUString(SERVICE_NAME2);
    return aRet;
}

extern "C"
{
    SAL_DLLPUBLIC_EXPORT void * SAL_CALL hwp_component_getFactory( const sal_Char * pImplName, void * pServiceManager, void *  )
    {
        void * pRet = 0;

        if (pServiceManager )
        {
            Reference< XSingleServiceFactory > xRet;
            Reference< XMultiServiceFactory > xSMgr = reinterpret_cast< XMultiServiceFactory * > ( pServiceManager );

            OUString aImplementationName = OUString::createFromAscii( pImplName );

            if (aImplementationName == IMPLEMENTATION_NAME )
            {
                xRet = createSingleFactory( xSMgr, aImplementationName,
                                            HwpImportFilter_CreateInstance,
                                            HwpImportFilter::getSupportedServiceNames_Static() );
            }
            if (xRet.is())
            {
                xRet->acquire();
                pRet = xRet.get();
            }
        }

        return pRet;
    }
}

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
