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
 *   Modified November 2016 by Patrick Luby. NeoOffice is only distributed
 *   under the GNU General Public License, Version 3 as allowed by Section 3.3
 *   of the Mozilla Public License, v. 2.0.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "pdfexport.hxx"
#include "impdialog.hxx"

#include "pdf.hrc"
#include "tools/urlobj.hxx"
#include "tools/fract.hxx"
#include "tools/poly.hxx"
#include "vcl/mapmod.hxx"
#include "vcl/virdev.hxx"
#include "vcl/metaact.hxx"
#include "vcl/gdimtf.hxx"
#include "vcl/jobset.hxx"
#include "vcl/bmpacc.hxx"
#include "vcl/svapp.hxx"
#include "toolkit/awt/vclxdevice.hxx"
#include "unotools/localfilehelper.hxx"
#include <vcl/FilterConfigItem.hxx>
#include <vcl/graphicfilter.hxx>
#include <vcl/settings.hxx>
#include "svl/solar.hrc"
#include "comphelper/string.hxx"
#include "comphelper/storagehelper.hxx"
#include "unotools/streamwrap.hxx"
#include "com/sun/star/io/XSeekable.hpp"

#include "basegfx/polygon/b2dpolygon.hxx"
#include "basegfx/polygon/b2dpolypolygon.hxx"
#include "basegfx/polygon/b2dpolygontools.hxx"

#include "unotools/saveopt.hxx"

#include "vcl/graphictools.hxx"
#include "com/sun/star/beans/XPropertySet.hpp"
#include "com/sun/star/configuration/theDefaultProvider.hpp"
#include "com/sun/star/awt/Rectangle.hpp"
#include "com/sun/star/awt/XDevice.hpp"
#include "com/sun/star/util/MeasureUnit.hpp"
#include "com/sun/star/frame/XModel.hpp"
#include "com/sun/star/frame/ModuleManager.hpp"
#include "com/sun/star/frame/XStorable.hpp"
#include "com/sun/star/frame/XController.hpp"
#include "com/sun/star/document/XDocumentProperties.hpp"
#include "com/sun/star/document/XDocumentPropertiesSupplier.hpp"
#include "com/sun/star/container/XNameAccess.hpp"
#include "com/sun/star/view/XViewSettingsSupplier.hpp"
#include "com/sun/star/task/XInteractionRequest.hpp"
#include "com/sun/star/task/PDFExportException.hpp"

#include "unotools/configmgr.hxx"
#include "cppuhelper/exc_hlp.hxx"
#include "cppuhelper/compbase1.hxx"
#include "cppuhelper/basemutex.hxx"

#include "com/sun/star/lang/XServiceInfo.hpp"
#include "com/sun/star/drawing/XShapes.hpp"
#include "com/sun/star/graphic/XGraphicProvider.hpp"
#include <boost/scoped_ptr.hpp>

using namespace ::vcl;
using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::view;
using namespace ::com::sun::star::graphic;


// - PDFExport -


PDFExport::PDFExport( const Reference< XComponent >& rxSrcDoc,
                      const Reference< task::XStatusIndicator >& rxStatusIndicator,
                      const Reference< task::XInteractionHandler >& rxIH,
                      const Reference< XComponentContext >& xContext ) :
    mxSrcDoc                    ( rxSrcDoc ),
    mxContext                   ( xContext ),
    mxStatusIndicator           ( rxStatusIndicator ),
    mxIH                        ( rxIH ),
    mbUseTaggedPDF              ( false ),
    mnPDFTypeSelection          ( 0 ),
    mbExportNotes               ( true ),
    mbViewPDF                   ( true ),
    mbExportNotesPages          ( false ),
    mbUseTransitionEffects      ( true ),
    mbExportBookmarks           ( true ),
    mbExportHiddenSlides        ( false ),
    mnOpenBookmarkLevels        ( -1 ),
#ifdef USE_JAVA
    mbThumbnail                 ( false ),
#endif	// USE_JAVA
    mbUseLosslessCompression    ( false ),
    mbReduceImageResolution     ( true ),
    mbSkipEmptyPages            ( true ),
    mbAddStream                 ( false ),
    mnMaxImageResolution        ( 300 ),
    mnQuality                   ( 90 ),
    mnFormsFormat               ( 0 ),
    mbExportFormFields          ( true ),
    mbAllowDuplicateFieldNames  ( false ),
    mnProgressValue             ( 0 ),
    mbRemoveTransparencies      ( false ),
    mbWatermark                 ( false ),

    mbHideViewerToolbar         ( false ),
    mbHideViewerMenubar         ( false ),
    mbHideViewerWindowControls  ( false ),
    mbFitWindow                 ( false ),
    mbCenterWindow              ( false ),
    mbOpenInFullScreenMode      ( false ),
    mbDisplayPDFDocumentTitle   ( true ),
    mnPDFDocumentMode           ( 0 ),
    mnPDFDocumentAction         ( 0 ),
    mnZoom                      ( 100 ),
    mnInitialPage               ( 1 ),
    mnPDFPageLayout             ( 0 ),
    mbFirstPageLeft             ( false ),

    mbEncrypt                   ( false ),
    mbRestrictPermissions       ( false ),
    mnPrintAllowed              ( 2 ),
    mnChangesAllowed            ( 4 ),
    mbCanCopyOrExtract          ( true ),
    mbCanExtractForAccessibility( true ),

//--->i56629
    mbExportRelativeFsysLinks       ( false ),
    mnDefaultLinkAction         ( 0 ),
    mbConvertOOoTargetToPDFTarget( false ),
    mbExportBmkToDest           ( false ),
    mbSignPDF                   ( false )
{
}



PDFExport::~PDFExport()
{
}



bool PDFExport::ExportSelection( vcl::PDFWriter& rPDFWriter,
    Reference< com::sun::star::view::XRenderable >& rRenderable,
    const Any& rSelection,
    const StringRangeEnumerator& rRangeEnum,
    Sequence< PropertyValue >& rRenderOptions,
    sal_Int32 nPageCount )
{
    bool        bRet = false;
    try
    {
        Any* pFirstPage = NULL;
        Any* pLastPage = NULL;

        bool bExportNotesPages = false;

        for( sal_Int32 nData = 0, nDataCount = rRenderOptions.getLength(); nData < nDataCount; ++nData )
        {
            if ( rRenderOptions[ nData ].Name == "IsFirstPage" )
                pFirstPage = &rRenderOptions[ nData ].Value;
            else if ( rRenderOptions[ nData ].Name == "IsLastPage" )
                pLastPage = &rRenderOptions[ nData ].Value;
            else if ( rRenderOptions[ nData ].Name == "ExportNotesPages" )
                rRenderOptions[ nData ].Value >>= bExportNotesPages;
        }

        OutputDevice* pOut = rPDFWriter.GetReferenceDevice();

        if( pOut )
        {
            vcl::PDFExtOutDevData* pPDFExtOutDevData = PTR_CAST( vcl::PDFExtOutDevData, pOut->GetExtOutDevData() );
            if ( nPageCount )
            {
                pPDFExtOutDevData->SetIsExportNotesPages( bExportNotesPages );

                sal_Int32 nCurrentPage(0);
                StringRangeEnumerator::Iterator aIter = rRangeEnum.begin();
                StringRangeEnumerator::Iterator aEnd  = rRangeEnum.end();
                while ( aIter != aEnd )
                {
                    Sequence< PropertyValue >   aRenderer( rRenderable->getRenderer( *aIter, rSelection, rRenderOptions ) );
                    awt::Size                   aPageSize;

                    for( sal_Int32 nProperty = 0, nPropertyCount = aRenderer.getLength(); nProperty < nPropertyCount; ++nProperty )
                    {
                        if ( aRenderer[ nProperty ].Name == "PageSize" )
                            aRenderer[ nProperty].Value >>= aPageSize;
                    }

                    pPDFExtOutDevData->SetCurrentPageNumber( nCurrentPage );

                    GDIMetaFile                 aMtf;
                    const MapMode               aMapMode( MAP_100TH_MM );
                    const Size                  aMtfSize( aPageSize.Width, aPageSize.Height );

                    pOut->Push();
                    pOut->EnableOutput( false );
                    pOut->SetMapMode( aMapMode );

                    aMtf.SetPrefSize( aMtfSize );
                    aMtf.SetPrefMapMode( aMapMode );
                    aMtf.Record( pOut );

                    // #i35176#
                    // IsLastPage property.
                    const sal_Int32 nCurrentRenderer = *aIter;
                    ++aIter;
                    if ( pLastPage && aIter == aEnd )
                        *pLastPage <<= sal_True;

                    rRenderable->render( nCurrentRenderer, rSelection, rRenderOptions );

                    aMtf.Stop();
                    aMtf.WindStart();

                    if( aMtf.GetActionSize() &&
                             ( !mbSkipEmptyPages || aPageSize.Width || aPageSize.Height ) )
                        bRet = ImplExportPage( rPDFWriter, *pPDFExtOutDevData, aMtf ) || bRet;

                    pOut->Pop();

                    if ( mxStatusIndicator.is() )
                        mxStatusIndicator->setValue( mnProgressValue );
                    if ( pFirstPage )
                        *pFirstPage <<= sal_False;

                    ++mnProgressValue;
                    ++nCurrentPage;
                }
            }
            else
            {
                bRet = true;                        // #i18334# SJ: nPageCount == 0,
                rPDFWriter.NewPage( 10000, 10000 );     // creating dummy page
                rPDFWriter.SetMapMode( MAP_100TH_MM );
            }
        }
    }
    catch(const RuntimeException &)
    {
    }
    return bRet;
}

class PDFExportStreamDoc : public vcl::PDFOutputStream
{
    Reference< XComponent >             m_xSrcDoc;
    Sequence< beans::NamedValue >       m_aPreparedPassword;
    public:
        PDFExportStreamDoc( const Reference< XComponent >& xDoc, const Sequence<beans::NamedValue>& rPwd )
    : m_xSrcDoc( xDoc ),
      m_aPreparedPassword( rPwd )
    {}
    virtual ~PDFExportStreamDoc();

    virtual void write( const Reference< XOutputStream >& xStream ) SAL_OVERRIDE;
};

PDFExportStreamDoc::~PDFExportStreamDoc()
{
}

void PDFExportStreamDoc::write( const Reference< XOutputStream >& xStream )
{
    Reference< com::sun::star::frame::XStorable > xStore( m_xSrcDoc, UNO_QUERY );
    if( xStore.is() )
    {
        Sequence< beans::PropertyValue > aArgs( 2 + ((m_aPreparedPassword.getLength() > 0) ? 1 : 0) );
        aArgs.getArray()[0].Name = "FilterName";
        aArgs.getArray()[1].Name = "OutputStream";
        aArgs.getArray()[1].Value <<= xStream;
        if( m_aPreparedPassword.getLength() )
        {
            aArgs.getArray()[2].Name = "EncryptionData";
            aArgs.getArray()[2].Value <<= m_aPreparedPassword;
        }

        try
        {
            xStore->storeToURL( "private:stream", aArgs );
        }
        catch( const IOException& )
        {
        }
    }
}

static OUString getMimetypeForDocument( const Reference< XComponentContext >& xContext,
                                        const Reference< XComponent >& xDoc ) throw()
{
    OUString aDocMimetype;
    try
    {
        // get document service name
        Reference< com::sun::star::frame::XStorable > xStore( xDoc, UNO_QUERY );
        Reference< frame::XModuleManager2 > xModuleManager = frame::ModuleManager::create(xContext);
        if( xStore.is() )
        {
            OUString aDocServiceName = xModuleManager->identify( Reference< XInterface >( xStore, uno::UNO_QUERY ) );
            if ( !aDocServiceName.isEmpty() )
            {
                // get the actual filter name
                OUString aFilterName;
                Reference< lang::XMultiServiceFactory > xConfigProvider =
                    configuration::theDefaultProvider::get( xContext );
                uno::Sequence< uno::Any > aArgs( 1 );
                beans::NamedValue aPathProp;
                aPathProp.Name = "nodepath";
                aPathProp.Value <<= OUString( "/org.openoffice.Setup/Office/Factories/" );
                aArgs[0] <<= aPathProp;

                Reference< container::XNameAccess > xSOFConfig(
                    xConfigProvider->createInstanceWithArguments(
                        "com.sun.star.configuration.ConfigurationAccess", aArgs ),
                    uno::UNO_QUERY );

                Reference< container::XNameAccess > xApplConfig;
                xSOFConfig->getByName( aDocServiceName ) >>= xApplConfig;
                if ( xApplConfig.is() )
                {
                    xApplConfig->getByName( "ooSetupFactoryActualFilter" ) >>= aFilterName;
                    if( !aFilterName.isEmpty() )
                    {
                        // find the related type name
                        OUString aTypeName;
                        Reference< container::XNameAccess > xFilterFactory(
                            xContext->getServiceManager()->createInstanceWithContext("com.sun.star.document.FilterFactory", xContext),
                            uno::UNO_QUERY );

                        Sequence< beans::PropertyValue > aFilterData;
                        xFilterFactory->getByName( aFilterName ) >>= aFilterData;
                        for ( sal_Int32 nInd = 0; nInd < aFilterData.getLength(); nInd++ )
                            if ( aFilterData[nInd].Name == "Type" )
                                aFilterData[nInd].Value >>= aTypeName;

                        if ( !aTypeName.isEmpty() )
                        {
                            // find the mediatype
                            Reference< container::XNameAccess > xTypeDetection(
                                xContext->getServiceManager()->createInstanceWithContext("com.sun.star.document.TypeDetection", xContext),
                                UNO_QUERY );

                            Sequence< beans::PropertyValue > aTypeData;
                            xTypeDetection->getByName( aTypeName ) >>= aTypeData;
                            for ( sal_Int32 nInd = 0; nInd < aTypeData.getLength(); nInd++ )
                                if ( aTypeData[nInd].Name == "MediaType" )
                                    aTypeData[nInd].Value >>= aDocMimetype;
                        }
                    }
                }
            }
        }
    }
    catch (...)
    {
    }
    return aDocMimetype;
}

bool PDFExport::Export( const OUString& rFile, const Sequence< PropertyValue >& rFilterData )
{
    INetURLObject   aURL( rFile );
    bool        bRet = false;

    std::set< PDFWriter::ErrorCode > aErrors;

    if( aURL.GetProtocol() != INET_PROT_FILE )
    {
        OUString aTmp;

        if( ::utl::LocalFileHelper::ConvertPhysicalNameToURL( rFile, aTmp ) )
            aURL = INetURLObject(aTmp);
    }

    if( aURL.GetProtocol() == INET_PROT_FILE )
    {
        Reference< XRenderable > xRenderable( mxSrcDoc, UNO_QUERY );

        if( xRenderable.is() )
        {
            VCLXDevice*                 pXDevice = new VCLXDevice;
            OUString                    aPageRange;
            Any                         aSelection;
            PDFWriter::PDFWriterContext aContext;
            OUString aOpenPassword, aPermissionPassword;
            Reference< beans::XMaterialHolder > xEnc;
            Sequence< beans::NamedValue > aPreparedPermissionPassword;


            // getting the string for the creator
            OUString aCreator;
            Reference< XServiceInfo > xInfo( mxSrcDoc, UNO_QUERY );
            if ( xInfo.is() )
            {
                if ( xInfo->supportsService( "com.sun.star.presentation.PresentationDocument" ) )
                    aCreator += "Impress";
                else if ( xInfo->supportsService( "com.sun.star.drawing.DrawingDocument" ) )
                    aCreator += "Draw";
                else if ( xInfo->supportsService( "com.sun.star.text.TextDocument" ) )
                    aCreator += "Writer";
                else if ( xInfo->supportsService( "com.sun.star.sheet.SpreadsheetDocument" ) )
                    aCreator += "Calc";
                else if ( xInfo->supportsService( "com.sun.star.formula.FormulaProperties"  ) )
                    aCreator += "Math";
            }

            Reference< document::XDocumentPropertiesSupplier > xDocumentPropsSupplier( mxSrcDoc, UNO_QUERY );
            if ( xDocumentPropsSupplier.is() )
            {
                Reference< document::XDocumentProperties > xDocumentProps( xDocumentPropsSupplier->getDocumentProperties() );
                if ( xDocumentProps.is() )
                {
                    aContext.DocumentInfo.Title = xDocumentProps->getTitle();
                    aContext.DocumentInfo.Author = xDocumentProps->getAuthor();
                    aContext.DocumentInfo.Subject = xDocumentProps->getSubject();
                    aContext.DocumentInfo.Keywords = ::comphelper::string::convertCommaSeparated(xDocumentProps->getKeywords());
                }
            }
            // getting the string for the producer
            aContext.DocumentInfo.Producer =
                utl::ConfigManager::getProductName() +
                " " +
                utl::ConfigManager::getProductVersion();
            aContext.DocumentInfo.Creator = aCreator;

            for( sal_Int32 nData = 0, nDataCount = rFilterData.getLength(); nData < nDataCount; ++nData )
            {
                if ( rFilterData[ nData ].Name == "PageRange" )
                    rFilterData[ nData ].Value >>= aPageRange;
                else if ( rFilterData[ nData ].Name == "Selection" )
                    rFilterData[ nData ].Value >>= aSelection;
                else if ( rFilterData[ nData ].Name == "UseLosslessCompression" )
                    rFilterData[ nData ].Value >>= mbUseLosslessCompression;
                else if ( rFilterData[ nData ].Name == "Quality" )
                    rFilterData[ nData ].Value >>= mnQuality;
                else if ( rFilterData[ nData ].Name == "ReduceImageResolution" )
                    rFilterData[ nData ].Value >>= mbReduceImageResolution;
                else if ( rFilterData[ nData ].Name == "IsSkipEmptyPages" )
                    rFilterData[ nData ].Value >>= mbSkipEmptyPages;
                else if ( rFilterData[ nData ].Name == "MaxImageResolution" )
                    rFilterData[ nData ].Value >>= mnMaxImageResolution;
                else if ( rFilterData[ nData ].Name == "UseTaggedPDF" )
                    rFilterData[ nData ].Value >>= mbUseTaggedPDF;
                else if ( rFilterData[ nData ].Name == "SelectPdfVersion" )
                    rFilterData[ nData ].Value >>= mnPDFTypeSelection;
                else if ( rFilterData[ nData ].Name == "ExportNotes" )
                    rFilterData[ nData ].Value >>= mbExportNotes;
                else if ( rFilterData[ nData ].Name == "ViewPDFAfterExport" )
                    rFilterData[ nData ].Value >>= mbViewPDF;
                else if ( rFilterData[ nData ].Name == "ExportNotesPages" )
                    rFilterData[ nData ].Value >>= mbExportNotesPages;
                else if ( rFilterData[ nData ].Name == "UseTransitionEffects" )
                    rFilterData[ nData ].Value >>= mbUseTransitionEffects;
                else if ( rFilterData[ nData ].Name == "ExportFormFields" )
                    rFilterData[ nData ].Value >>= mbExportFormFields;
                else if ( rFilterData[ nData ].Name == "FormsType" )
                    rFilterData[ nData ].Value >>= mnFormsFormat;
                else if ( rFilterData[ nData ].Name == "AllowDuplicateFieldNames" )
                    rFilterData[ nData ].Value >>= mbAllowDuplicateFieldNames;
//viewer properties
                else if ( rFilterData[ nData ].Name == "HideViewerToolbar" )
                    rFilterData[ nData ].Value >>= mbHideViewerToolbar;
                else if ( rFilterData[ nData ].Name == "HideViewerMenubar" )
                    rFilterData[ nData ].Value >>= mbHideViewerMenubar;
                else if ( rFilterData[ nData ].Name == "HideViewerWindowControls" )
                    rFilterData[ nData ].Value >>= mbHideViewerWindowControls;
                else if ( rFilterData[ nData ].Name == "ResizeWindowToInitialPage" )
                    rFilterData[ nData ].Value >>= mbFitWindow;
                else if ( rFilterData[ nData ].Name == "CenterWindow" )
                    rFilterData[ nData ].Value >>= mbCenterWindow;
                else if ( rFilterData[ nData ].Name == "OpenInFullScreenMode" )
                    rFilterData[ nData ].Value >>= mbOpenInFullScreenMode;
                else if ( rFilterData[ nData ].Name == "DisplayPDFDocumentTitle" )
                    rFilterData[ nData ].Value >>= mbDisplayPDFDocumentTitle;
                else if ( rFilterData[ nData ].Name == "InitialView" )
                    rFilterData[ nData ].Value >>= mnPDFDocumentMode;
                else if ( rFilterData[ nData ].Name == "Magnification" )
                    rFilterData[ nData ].Value >>= mnPDFDocumentAction;
                else if ( rFilterData[ nData ].Name == "Zoom" )
                    rFilterData[ nData ].Value >>= mnZoom;
                else if ( rFilterData[ nData ].Name == "InitialPage" )
                    rFilterData[ nData ].Value >>= mnInitialPage;
                else if ( rFilterData[ nData ].Name == "PageLayout" )
                    rFilterData[ nData ].Value >>= mnPDFPageLayout;
                else if ( rFilterData[ nData ].Name == "FirstPageOnLeft" )
                    rFilterData[ nData ].Value >>= aContext.FirstPageLeft;
                else if ( rFilterData[ nData ].Name == "IsAddStream" )
                    rFilterData[ nData ].Value >>= mbAddStream;
                else if ( rFilterData[ nData ].Name == "Watermark" )
                {
                    maWatermark = rFilterData[ nData ].Value;
                    mbWatermark = true;
                }
//now all the security related properties...
                else if ( rFilterData[ nData ].Name == "EncryptFile" )
                    rFilterData[ nData ].Value >>= mbEncrypt;
                else if ( rFilterData[ nData ].Name == "DocumentOpenPassword" )
                    rFilterData[ nData ].Value >>= aOpenPassword;
                else if ( rFilterData[ nData ].Name == "RestrictPermissions" )
                    rFilterData[ nData ].Value >>= mbRestrictPermissions;
                else if ( rFilterData[ nData ].Name == "PermissionPassword" )
                    rFilterData[ nData ].Value >>= aPermissionPassword;
                else if ( rFilterData[ nData ].Name == "PreparedPasswords" )
                    rFilterData[ nData ].Value >>= xEnc;
                else if ( rFilterData[ nData ].Name == "PreparedPermissionPassword" )
                    rFilterData[ nData ].Value >>= aPreparedPermissionPassword;
                else if ( rFilterData[ nData ].Name == "Printing" )
                    rFilterData[ nData ].Value >>= mnPrintAllowed;
                else if ( rFilterData[ nData ].Name == "Changes" )
                    rFilterData[ nData ].Value >>= mnChangesAllowed;
                else if ( rFilterData[ nData ].Name == "EnableCopyingOfContent" )
                    rFilterData[ nData ].Value >>= mbCanCopyOrExtract;
                else if ( rFilterData[ nData ].Name == "EnableTextAccessForAccessibilityTools" )
                    rFilterData[ nData ].Value >>= mbCanExtractForAccessibility;
//--->i56629 links extra (relative links and other related stuff)
                 else if ( rFilterData[ nData ].Name == "ExportLinksRelativeFsys" )
                     rFilterData[ nData ].Value >>= mbExportRelativeFsysLinks;
                 else if ( rFilterData[ nData ].Name == "PDFViewSelection" )
                     rFilterData[ nData ].Value >>= mnDefaultLinkAction;
                 else if ( rFilterData[ nData ].Name == "ConvertOOoTargetToPDFTarget" )
                     rFilterData[ nData ].Value >>= mbConvertOOoTargetToPDFTarget;
                  else if ( rFilterData[ nData ].Name == "ExportBookmarksToPDFDestination" )
                      rFilterData[ nData ].Value >>= mbExportBmkToDest;
                else if ( rFilterData[ nData ].Name == "ExportBookmarks" )
                    rFilterData[ nData ].Value >>= mbExportBookmarks;
                else if ( rFilterData[ nData ].Name == "ExportHiddenSlides" )
                    rFilterData[ nData ].Value >>= mbExportHiddenSlides;
                else if ( rFilterData[ nData ].Name == "OpenBookmarkLevels" )
                    rFilterData[ nData ].Value >>= mnOpenBookmarkLevels;
#ifdef USE_JAVA
                else if ( rFilterData[ nData ].Name == "IsThumbnail" )
                    rFilterData[ nData ].Value >>= mbThumbnail;
#endif	// USE_JAVA
                else if ( rFilterData[ nData ].Name == "SignPDF" )
                    rFilterData[ nData ].Value >>= mbSignPDF;
                else if ( rFilterData[ nData ].Name == "SignatureLocation" )
                    rFilterData[ nData ].Value >>= msSignLocation;
                else if ( rFilterData[ nData ].Name == "SignatureReason" )
                    rFilterData[ nData ].Value >>= msSignReason;
                else if ( rFilterData[ nData ].Name == "SignatureContactInfo" )
                    rFilterData[ nData ].Value >>= msSignContact;
                else if ( rFilterData[ nData ].Name == "SignaturePassword" )
                    rFilterData[ nData ].Value >>= msSignPassword;
                else if ( rFilterData[ nData ].Name == "SignatureCertificate" )
                    rFilterData[ nData ].Value >>= maSignCertificate;
            }
            aContext.URL        = aURL.GetMainURL(INetURLObject::DECODE_TO_IURI);

//set the correct version, depending on user request
            switch( mnPDFTypeSelection )
            {
            default:
            case 0:
                aContext.Version    = PDFWriter::PDF_1_4;
                break;
            case 1:
                aContext.Version    = PDFWriter::PDF_A_1;
                //force the tagged PDF as well
                mbUseTaggedPDF = true;
                //force disabling of form conversion
                mbExportFormFields = false;
                // PDF/A does not allow transparencies
                mbRemoveTransparencies = true;
                // no encryption
                mbEncrypt = false;
                xEnc.clear();
                break;
            }

//copy in context the values default in the contructor or set by the FilterData sequence of properties
            aContext.Tagged     = mbUseTaggedPDF;

//values used in viewer
            aContext.HideViewerToolbar          = mbHideViewerToolbar;
            aContext.HideViewerMenubar          = mbHideViewerMenubar;
            aContext.HideViewerWindowControls   = mbHideViewerWindowControls;
            aContext.FitWindow                  = mbFitWindow;
            aContext.CenterWindow               = mbCenterWindow;
            aContext.OpenInFullScreenMode       = mbOpenInFullScreenMode;
            aContext.DisplayPDFDocumentTitle    = mbDisplayPDFDocumentTitle;
            aContext.InitialPage                = mnInitialPage-1;
            aContext.OpenBookmarkLevels         = mnOpenBookmarkLevels;

            switch( mnPDFDocumentMode )
            {
                default:
                case 0:
                    aContext.PDFDocumentMode = PDFWriter::ModeDefault;
                    break;
                case 1:
                    aContext.PDFDocumentMode = PDFWriter::UseOutlines;
                    break;
                case 2:
                    aContext.PDFDocumentMode = PDFWriter::UseThumbs;
                    break;
            }
            switch( mnPDFDocumentAction )
            {
                default:
                case 0:
                    aContext.PDFDocumentAction = PDFWriter::ActionDefault;
                    break;
                case 1:
                    aContext.PDFDocumentAction = PDFWriter::FitInWindow;
                    break;
                case 2:
                    aContext.PDFDocumentAction = PDFWriter::FitWidth;
                    break;
                case 3:
                    aContext.PDFDocumentAction = PDFWriter::FitVisible;
                    break;
                case 4:
                    aContext.PDFDocumentAction = PDFWriter::ActionZoom;
                    aContext.Zoom = mnZoom;
                    break;
            }

            switch( mnPDFPageLayout )
            {
                default:
                case 0:
                    aContext.PageLayout = PDFWriter::DefaultLayout;
                    break;
                case 1:
                    aContext.PageLayout = PDFWriter::SinglePage;
                    break;
                case 2:
                    aContext.PageLayout = PDFWriter::Continuous;
                    break;
                case 3:
                    aContext.PageLayout = PDFWriter::ContinuousFacing;
                    break;
            }

            aContext.FirstPageLeft = mbFirstPageLeft;

//check if PDF/A, which does not allow encryption
            if( aContext.Version != PDFWriter::PDF_A_1 )
            {
//set values needed in encryption
//set encryption level, fixed, but here it can set by the UI if needed.
// true is 128 bit, false 40
//note that in 40 bit mode the UI needs reworking, since the current UI is meaningfull only for
//128bit security mode
                aContext.Encryption.Security128bit = true;

//set check for permission change password
// if not enabled and no permission password, force permissions to default as if PDF where without encryption
                if( mbRestrictPermissions && (xEnc.is() || !aPermissionPassword.isEmpty()) )
                {
                    mbEncrypt = true;
//permission set as desired, done after
                }
                else
                {
//force permission to default
                    mnPrintAllowed                  = 2 ;
                    mnChangesAllowed                = 4 ;
                    mbCanCopyOrExtract              = true;
                    mbCanExtractForAccessibility    = true ;
                }

                switch( mnPrintAllowed )
                {
                case 0: //initialized when aContext is build, means no printing
                    break;
                default:
                case 2:
                    aContext.Encryption.CanPrintFull            = true;
                case 1:
                    aContext.Encryption.CanPrintTheDocument     = true;
                    break;
                }

                switch( mnChangesAllowed )
                {
                case 0: //already in struct PDFSecPermissions CTOR
                    break;
                case 1:
                    aContext.Encryption.CanAssemble             = true;
                    break;
                case 2:
                    aContext.Encryption.CanFillInteractive      = true;
                    break;
                case 3:
                    aContext.Encryption.CanAddOrModify          = true;
                    break;
                default:
                case 4:
                    aContext.Encryption.CanModifyTheContent     =
                        aContext.Encryption.CanCopyOrExtract    =
                        aContext.Encryption.CanAddOrModify      =
                        aContext.Encryption.CanFillInteractive  = true;
                    break;
                }

                aContext.Encryption.CanCopyOrExtract                = mbCanCopyOrExtract;
                aContext.Encryption.CanExtractForAccessibility  = mbCanExtractForAccessibility;
                if( mbEncrypt && ! xEnc.is() )
                    xEnc = PDFWriter::InitEncryption( aPermissionPassword, aOpenPassword, aContext.Encryption.Security128bit );
                if( mbEncrypt && !aPermissionPassword.isEmpty() && ! aPreparedPermissionPassword.getLength() )
                    aPreparedPermissionPassword = comphelper::OStorageHelper::CreatePackageEncryptionData( aPermissionPassword );
            }
            // after this point we don't need the legacy clear passwords anymore
            // however they are still inside the passed filter data sequence
            // which is sadly out out our control
            aPermissionPassword.clear();
            aOpenPassword.clear();

            /*
            * FIXME: the entries are only implicitly defined by the resource file. Should there
            * ever be an additional form submit format this could get invalid.
            */
            switch( mnFormsFormat )
            {
                case 1:
                    aContext.SubmitFormat = PDFWriter::PDF;
                    break;
                case 2:
                    aContext.SubmitFormat = PDFWriter::HTML;
                    break;
                case 3:
                    aContext.SubmitFormat = PDFWriter::XML;
                    break;
                default:
                case 0:
                    aContext.SubmitFormat = PDFWriter::FDF;
                    break;
            }
            aContext.AllowDuplicateFieldNames = mbAllowDuplicateFieldNames;

            //get model
            Reference< frame::XModel > xModel( mxSrcDoc, UNO_QUERY );
            {
//---> i56629 Relative link stuff
//set the base URL of the file:
//then base URL
                aContext.BaseURL = xModel->getURL();
//relative link option is private to PDF Export filter and limited to local filesystem only
                aContext.RelFsys = mbExportRelativeFsysLinks;
//determine the default acton for PDF links
                switch( mnDefaultLinkAction )
                {
                default:
//default: URI, without fragment conversion (the bookmark in PDF may not work)
                case 0:
                    aContext.DefaultLinkAction = PDFWriter::URIAction;
                    break;
//view PDF through the reader application
                case 1:
                    aContext.ForcePDFAction = true;
                    aContext.DefaultLinkAction = PDFWriter::LaunchAction;
                    break;
//view PDF through an Internet browser
                case 2:
                    aContext.DefaultLinkAction = PDFWriter::URIActionDestination;
                    break;
                }
                aContext.ConvertOOoTargetToPDFTarget = mbConvertOOoTargetToPDFTarget;
// check for Link Launch action, not allowed on PDF/A-1
// this code chunk checks when the filter is called from scripting
                if( aContext.Version == PDFWriter::PDF_A_1 &&
                    aContext.DefaultLinkAction == PDFWriter::LaunchAction )
                {   //force the similar allowed URI action
                    aContext.DefaultLinkAction = PDFWriter::URIActionDestination;
                    //and remove the remote goto action forced on PDF file
                    aContext.ForcePDFAction = false;
                }
            }

            aContext.SignPDF = mbSignPDF;
            aContext.SignLocation = msSignLocation;
            aContext.SignContact = msSignContact;
            aContext.SignReason = msSignReason;
            aContext.SignPassword = msSignPassword;
            aContext.SignCertificate = maSignCertificate;

// all context data set, time to create the printing device
            boost::scoped_ptr<PDFWriter> pPDFWriter(new PDFWriter( aContext, xEnc ));
            OutputDevice*       pOut = pPDFWriter->GetReferenceDevice();

            DBG_ASSERT( pOut, "PDFExport::Export: no reference device" );
            pXDevice->SetOutputDevice( pOut );

            if( mbAddStream )
            {
                // export stream
                // get mimetype
                OUString aSrcMimetype = getMimetypeForDocument( mxContext, mxSrcDoc );
                pPDFWriter->AddStream( aSrcMimetype,
                                       new PDFExportStreamDoc( mxSrcDoc, aPreparedPermissionPassword ),
                                       false
                                       );
            }

            if ( pOut )
            {
                DBG_ASSERT( pOut->GetExtOutDevData() == NULL, "PDFExport: ExtOutDevData already set!!!" );
                boost::scoped_ptr<vcl::PDFExtOutDevData> pPDFExtOutDevData(new vcl::PDFExtOutDevData( *pOut ));
                pOut->SetExtOutDevData( pPDFExtOutDevData.get() );
                pPDFExtOutDevData->SetIsExportNotes( mbExportNotes );
                pPDFExtOutDevData->SetIsExportTaggedPDF( mbUseTaggedPDF );
                pPDFExtOutDevData->SetIsExportTransitionEffects( mbUseTransitionEffects );
                pPDFExtOutDevData->SetFormsFormat( mnFormsFormat );
                pPDFExtOutDevData->SetIsExportFormFields( mbExportFormFields );
                pPDFExtOutDevData->SetIsExportBookmarks( mbExportBookmarks );
                pPDFExtOutDevData->SetIsExportHiddenSlides( mbExportHiddenSlides );
                pPDFExtOutDevData->SetIsLosslessCompression( mbUseLosslessCompression );
                pPDFExtOutDevData->SetIsReduceImageResolution( mbReduceImageResolution );
                pPDFExtOutDevData->SetIsExportNamedDestinations( mbExportBmkToDest );

#ifdef USE_JAVA
                Sequence< PropertyValue > aRenderOptions( 7 );
#else	// USE_JAVA
                Sequence< PropertyValue > aRenderOptions( 6 );
#endif	// USE_JAVA
                aRenderOptions[ 0 ].Name = "RenderDevice";
                aRenderOptions[ 0 ].Value <<= Reference< awt::XDevice >( pXDevice );
                aRenderOptions[ 1 ].Name = "ExportNotesPages";
                aRenderOptions[ 1 ].Value <<= sal_False;
                Any& rExportNotesValue = aRenderOptions[ 1 ].Value;
                aRenderOptions[ 2 ].Name = "IsFirstPage";
                aRenderOptions[ 2 ].Value <<= sal_True;
                aRenderOptions[ 3 ].Name = "IsLastPage";
                aRenderOptions[ 3 ].Value <<= sal_False;
                aRenderOptions[ 4 ].Name = "IsSkipEmptyPages";
                aRenderOptions[ 4 ].Value <<= mbSkipEmptyPages;
                aRenderOptions[ 5 ].Name = "PageRange";
                aRenderOptions[ 5 ].Value <<= aPageRange;
#ifdef USE_JAVA
                aRenderOptions[ 6 ].Name = "IsThumbnail";
                aRenderOptions[ 6 ].Value <<= mbThumbnail;
#endif	// USE_JAVA

                if( !aPageRange.isEmpty() || !aSelection.hasValue() )
                {
                    aSelection = Any();
                    aSelection <<= mxSrcDoc;
                }
                bool        bSecondPassForImpressNotes = false;
                bool bReChangeToNormalView = false;
                  OUString sShowOnlineLayout( "ShowOnlineLayout" );
                  uno::Reference< beans::XPropertySet > xViewProperties;

                if ( aCreator.equalsAscii( "Writer" ) )
                {
                    //i92835 if Writer is in web layout mode this has to be switched to normal view and back to web view in the end
                    try
                    {
                        Reference< view::XViewSettingsSupplier > xVSettingsSupplier( xModel->getCurrentController(), uno::UNO_QUERY_THROW );
                        xViewProperties =  xVSettingsSupplier->getViewSettings();
                        xViewProperties->getPropertyValue( sShowOnlineLayout ) >>= bReChangeToNormalView;
                        if( bReChangeToNormalView )
                        {
                            xViewProperties->setPropertyValue( sShowOnlineLayout, uno::makeAny( false ) );
                        }
                    }
                    catch( const uno::Exception& )
                    {
                    }

                }

                const sal_Int32 nPageCount = xRenderable->getRendererCount( aSelection, aRenderOptions );

                if ( mbExportNotesPages && aCreator.equalsAscii( "Impress" ) )
                {
                    uno::Reference< drawing::XShapes > xShapes;     // sj: do not allow to export notes when
                    if ( ! ( aSelection >>= xShapes ) )             // exporting a selection -> todo: in the dialog
                        bSecondPassForImpressNotes = true;      // the export notes checkbox needs to be disabled
                }

                if( aPageRange.isEmpty() )
                {
                    aPageRange = OUString::number( 1 ) + "-" + OUString::number(nPageCount );
                }
                StringRangeEnumerator aRangeEnum( aPageRange, 0, nPageCount-1 );

                if ( mxStatusIndicator.is() )
                {
                    boost::scoped_ptr<ResMgr> pResMgr(ResMgr::CreateResMgr( "pdffilter", Application::GetSettings().GetUILanguageTag() ));
                    if ( pResMgr )
                    {
                        sal_Int32 nTotalPageCount = aRangeEnum.size();
                        if ( bSecondPassForImpressNotes )
                            nTotalPageCount *= 2;
                        mxStatusIndicator->start( OUString( ResId( PDF_PROGRESS_BAR, *pResMgr ) ), nTotalPageCount );
                    }
                }

                if( nPageCount > 0 )
                    bRet = ExportSelection( *pPDFWriter, xRenderable, aSelection, aRangeEnum, aRenderOptions, nPageCount );
                else
                    bRet = false;

                if ( bRet && bSecondPassForImpressNotes )
                {
                    rExportNotesValue <<= sal_True;
                    bRet = ExportSelection( *pPDFWriter, xRenderable, aSelection, aRangeEnum, aRenderOptions, nPageCount );
                }
                if ( mxStatusIndicator.is() )
                    mxStatusIndicator->end();

                // if during the export the doc locale was set copy it to PDF writer
                const com::sun::star::lang::Locale& rLoc( pPDFExtOutDevData->GetDocumentLocale() );
                if( !rLoc.Language.isEmpty() )
                    pPDFWriter->SetDocumentLocale( rLoc );

                if( bRet )
                {
                    pPDFExtOutDevData->PlayGlobalActions( *pPDFWriter );
                    bRet = pPDFWriter->Emit();
                    aErrors = pPDFWriter->GetErrors();
                }
                pOut->SetExtOutDevData( NULL );
                if( bReChangeToNormalView )
                {
                    try
                    {
                        xViewProperties->setPropertyValue( sShowOnlineLayout, uno::makeAny( true ) );
                    }
                    catch( const uno::Exception& )
                    {
                    }
                }
            }
        }
    }

    // show eventual errors during export
    showErrors( aErrors );

    return bRet;
}

namespace
{

typedef cppu::WeakComponentImplHelper1< task::XInteractionRequest > PDFErrorRequestBase;

class PDFErrorRequest : private cppu::BaseMutex,
                        public PDFErrorRequestBase
{
    task::PDFExportException maExc;
public:
    PDFErrorRequest( const task::PDFExportException& i_rExc );

    // XInteractionRequest
    virtual uno::Any SAL_CALL getRequest() throw (uno::RuntimeException, std::exception) SAL_OVERRIDE;
    virtual uno::Sequence< uno::Reference< task::XInteractionContinuation > > SAL_CALL getContinuations() throw (uno::RuntimeException, std::exception) SAL_OVERRIDE;
};

PDFErrorRequest::PDFErrorRequest( const task::PDFExportException& i_rExc ) :
    PDFErrorRequestBase( m_aMutex ),
    maExc( i_rExc )
{
}

uno::Any SAL_CALL PDFErrorRequest::getRequest() throw (uno::RuntimeException, std::exception)
{
    osl::MutexGuard const guard( m_aMutex );

    uno::Any aRet;
    aRet <<= maExc;
    return aRet;
}

uno::Sequence< uno::Reference< task::XInteractionContinuation > > SAL_CALL PDFErrorRequest::getContinuations() throw (uno::RuntimeException, std::exception)
{
    return uno::Sequence< uno::Reference< task::XInteractionContinuation > >();
}

} // namespace

void PDFExport::showErrors( const std::set< PDFWriter::ErrorCode >& rErrors )
{
    if( ! rErrors.empty() && mxIH.is() )
    {
        task::PDFExportException aExc;
        aExc.ErrorCodes.realloc( sal_Int32(rErrors.size()) );
        sal_Int32 i = 0;
        for( std::set< PDFWriter::ErrorCode >::const_iterator it = rErrors.begin();
             it != rErrors.end(); ++it, i++ )
        {
            aExc.ErrorCodes.getArray()[i] = (sal_Int32)*it;
        }
        Reference< task::XInteractionRequest > xReq( new PDFErrorRequest( aExc ) );
        mxIH->handle( xReq );
    }
}



bool PDFExport::ImplExportPage( PDFWriter& rWriter, PDFExtOutDevData& rPDFExtOutDevData, const GDIMetaFile& rMtf )
{
    const Size      aSizePDF( OutputDevice::LogicToLogic( rMtf.GetPrefSize(), rMtf.GetPrefMapMode(), MAP_POINT ) );
    Point           aOrigin;
    Rectangle       aPageRect( aOrigin, rMtf.GetPrefSize() );
    bool        bRet = true;

    rWriter.NewPage( aSizePDF.Width(), aSizePDF.Height() );
    rWriter.SetMapMode( rMtf.GetPrefMapMode() );

    vcl::PDFWriter::PlayMetafileContext aCtx;
    GDIMetaFile aMtf;
    if( mbRemoveTransparencies )
    {
        aCtx.m_bTransparenciesWereRemoved = rWriter.GetReferenceDevice()->
            RemoveTransparenciesFromMetaFile( rMtf, aMtf, mnMaxImageResolution, mnMaxImageResolution,
                                              false, true, mbReduceImageResolution );
    }
    else
    {
        aMtf = rMtf;
    }
    aCtx.m_nMaxImageResolution      = mbReduceImageResolution ? mnMaxImageResolution : 0;
    aCtx.m_bOnlyLosslessCompression = mbUseLosslessCompression;
    aCtx.m_nJPEGQuality             = mnQuality;


    basegfx::B2DRectangle aB2DRect( aPageRect.Left(), aPageRect.Top(), aPageRect.Right(), aPageRect.Bottom() );
    rWriter.SetClipRegion( basegfx::B2DPolyPolygon( basegfx::tools::createPolygonFromRect( aB2DRect ) ) );

    rWriter.PlayMetafile( aMtf, aCtx, &rPDFExtOutDevData );

    rPDFExtOutDevData.ResetSyncData();

    if( mbWatermark )
        ImplWriteWatermark( rWriter, aSizePDF );

    return bRet;
}



void PDFExport::ImplWriteWatermark( PDFWriter& rWriter, const Size& rPageSize )
{
    OUString aText( "Watermark" );
    Font aFont( OUString( "Helvetica" ), Size( 0, 3*rPageSize.Height()/4 ) );
    aFont.SetItalic( ITALIC_NONE );
    aFont.SetWidthType( WIDTH_NORMAL );
    aFont.SetWeight( WEIGHT_NORMAL );
    aFont.SetAlign( ALIGN_BOTTOM );
    long nTextWidth = rPageSize.Width();
    if( rPageSize.Width() < rPageSize.Height() )
    {
        nTextWidth = rPageSize.Height();
        aFont.SetOrientation( 2700 );
    }

    if( ! ( maWatermark >>= aText ) )
    {
        // more complicated watermark ?
    }

    // adjust font height for text to fit
    OutputDevice* pDev = rWriter.GetReferenceDevice();
    pDev->Push( PushFlags::ALL );
    pDev->SetFont( aFont );
    pDev->SetMapMode( MapMode( MAP_POINT ) );
    int w = 0;
    while( ( w = pDev->GetTextWidth( aText ) ) > nTextWidth )
    {
        if (w == 0)
            break;
        long nNewHeight = aFont.GetHeight() * nTextWidth / w;
        if( nNewHeight == aFont.GetHeight() )
        {
            nNewHeight--;
            if( nNewHeight <= 0 )
                break;
        }
        aFont.SetHeight( nNewHeight );
        pDev->SetFont( aFont );
    }
    long nTextHeight = pDev->GetTextHeight();
    // leave some maneuvering room for rounding issues, also
    // some fonts go a little outside ascent/descent
    nTextHeight += nTextHeight/20;
    pDev->Pop();

    rWriter.Push( PushFlags::ALL );
    rWriter.SetMapMode( MapMode( MAP_POINT ) );
    rWriter.SetFont( aFont );
    rWriter.SetTextColor( COL_LIGHTGREEN );
    Point aTextPoint;
    Rectangle aTextRect;
    if( rPageSize.Width() > rPageSize.Height() )
    {
        aTextPoint = Point( (rPageSize.Width()-w)/2,
                            rPageSize.Height()-(rPageSize.Height()-nTextHeight)/2 );
        aTextRect = Rectangle( Point( (rPageSize.Width()-w)/2,
                                      (rPageSize.Height()-nTextHeight)/2 ),
                               Size( w, nTextHeight ) );
    }
    else
    {
        aTextPoint = Point( (rPageSize.Width()-nTextHeight)/2,
                            (rPageSize.Height()-w)/2 );
        aTextRect = Rectangle( aTextPoint, Size( nTextHeight, w ) );
    }
    rWriter.SetClipRegion();
    rWriter.BeginTransparencyGroup();
    rWriter.DrawText( aTextPoint, aText );
    rWriter.EndTransparencyGroup( aTextRect, 50 );
    rWriter.Pop();
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
