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

#ifndef INCLUDED_FILTER_SOURCE_PDF_PDFEXPORT_HXX
#define INCLUDED_FILTER_SOURCE_PDF_PDFEXPORT_HXX

#include "pdffilter.hxx"
#include <tools/multisel.hxx>
#include <vcl/pdfwriter.hxx>
#include <vcl/pdfextoutdevdata.hxx>
#include <com/sun/star/view/XRenderable.hpp>

class GDIMetaFile;
class Size;

namespace vcl { class PDFWriter; }


// - PDFExport -


class PDFExport
{
private:

    Reference< XComponent > mxSrcDoc;
    Reference< uno::XComponentContext > mxContext;
    Reference< task::XStatusIndicator > mxStatusIndicator;
    Reference< task::XInteractionHandler > mxIH;

    bool                mbUseTaggedPDF;
    sal_Int32               mnPDFTypeSelection;
    bool                mbExportNotes;
    bool                mbViewPDF;
    bool                mbExportNotesPages;
    bool                mbUseTransitionEffects;
    bool                mbExportBookmarks;
    bool                mbExportHiddenSlides;
    sal_Int32               mnOpenBookmarkLevels;
#ifdef USE_JAVA
    bool                mbThumbnail;
#endif	// USE_JAVA

    bool                mbUseLosslessCompression;
    bool                mbReduceImageResolution;
    bool                mbSkipEmptyPages;
    bool                mbAddStream;
    sal_Int32               mnMaxImageResolution;
    sal_Int32               mnQuality;
    sal_Int32               mnFormsFormat;
    bool                mbExportFormFields;
    bool                mbAllowDuplicateFieldNames;
    sal_Int32               mnProgressValue;
    bool                mbRemoveTransparencies;

    bool                mbWatermark;
    uno::Any                maWatermark;

//these variable are here only to have a location in filter/pdf to set the default
//to be used by the macro (when the FilterData are set by the macro itself)
    bool                mbHideViewerToolbar;
    bool                mbHideViewerMenubar;
    bool                mbHideViewerWindowControls;
    bool                mbFitWindow;
    bool                mbCenterWindow;
    bool                mbOpenInFullScreenMode;
    bool                mbDisplayPDFDocumentTitle;
    sal_Int32               mnPDFDocumentMode;
    sal_Int32               mnPDFDocumentAction;
    sal_Int32               mnZoom;
    sal_Int32               mnInitialPage;
    sal_Int32               mnPDFPageLayout;
    bool                mbFirstPageLeft;

    bool                mbEncrypt;
    bool                mbRestrictPermissions;
    sal_Int32               mnPrintAllowed;
    sal_Int32               mnChangesAllowed;
    bool                mbCanCopyOrExtract;
    bool                mbCanExtractForAccessibility;

    SvtGraphicFill          maCacheFill;

//--->i56629
    bool                mbExportRelativeFsysLinks;
    sal_Int32               mnDefaultLinkAction;
    bool                mbConvertOOoTargetToPDFTarget;
    bool                mbExportBmkToDest;
    bool                ImplExportPage( ::vcl::PDFWriter& rWriter, ::vcl::PDFExtOutDevData& rPDFExtOutDevData,
                                                const GDIMetaFile& rMtf );

    bool                mbSignPDF;
    OUString                msSignLocation;
    OUString                msSignContact;
    OUString                msSignReason;
    OUString                msSignPassword;
    Reference< security::XCertificate > maSignCertificate;

    void                    ImplWriteWatermark( ::vcl::PDFWriter& rWriter, const Size& rPageSize );
public:

                            PDFExport( const Reference< XComponent >& rxSrcDoc,
                                       const Reference< task::XStatusIndicator >& xStatusIndicator,
                                      const Reference< task::XInteractionHandler >& xIH,
                                       const Reference< uno::XComponentContext >& xFact );
                            ~PDFExport();

    bool                ExportSelection( vcl::PDFWriter& rPDFWriter,
                                Reference< com::sun::star::view::XRenderable >& rRenderable,
                                const Any& rSelection,
                                const StringRangeEnumerator& rRangeEnum,
                                Sequence< PropertyValue >& rRenderOptions,
                                sal_Int32 nPageCount );

    bool                Export( const OUString& rFile, const Sequence< PropertyValue >& rFilterData );

    void                    showErrors( const std::set<vcl::PDFWriter::ErrorCode>& );
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
