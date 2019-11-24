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


#include "impviewframe.hxx"
#include "statcach.hxx"
#include <sfx2/viewfac.hxx>
#include "workwin.hxx"

#include <sfx2/app.hxx>
#include <sfx2/bindings.hxx>
#include <sfx2/ctrlitem.hxx>
#include <sfx2/dispatch.hxx>
#include <sfx2/docfac.hxx>
#include <sfx2/docfile.hxx>
#include <sfx2/objitem.hxx>
#include <sfx2/objsh.hxx>
#include <sfx2/request.hxx>
#include <sfx2/viewfrm.hxx>
#include <sfx2/viewsh.hxx>

#include <com/sun/star/util/XCloseable.hpp>

#include <svtools/asynclink.hxx>
#include <svl/eitem.hxx>
#include <svl/intitem.hxx>
#include <svl/rectitem.hxx>
#include <svl/stritem.hxx>
#include <tools/diagnose_ex.h>
#include <tools/urlobj.hxx>
#include <vcl/window.hxx>

#if defined USE_JAVA && defined MACOSX

#include <osl/file.hxx>
#include <vcl/sysdata.hxx>
#include <vcl/syswin.hxx>

#include "topfrm_cocoa.hxx"

#endif	// USE_JAVA && MACOSX

using namespace ::com::sun::star;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::frame;
using namespace ::com::sun::star::util;
using namespace ::com::sun::star::container;
using namespace ::com::sun::star::beans;
using ::com::sun::star::lang::XMultiServiceFactory;
using ::com::sun::star::lang::XComponent;


void SfxFrameViewWindow_Impl::StateChanged( StateChangedType nStateChange )
{
    if ( nStateChange == StateChangedType::INITSHOW )
    {
        SfxObjectShell* pDoc = pFrame->GetObjectShell();
        if ( pDoc && !pFrame->IsVisible() )
            pFrame->Show();

        pFrame->Resize();
    }
    else
        Window::StateChanged( nStateChange );
}

void SfxFrameViewWindow_Impl::Resize()
{
    if ( IsReallyVisible() || IsReallyShown() || GetOutputSizePixel().Width() )
        pFrame->Resize();
}




void SfxViewFrame::UpdateTitle()

/*  [Description]

    With this method, can the SfxViewFrame be forced to immediately provide
    the new title from the <SfxObjectShell>.

    [Note]

    This is for example necessary if one listens to the SfxObjectShell as
    SfxListener and then react on the <SfxSimpleHint> SFX_HINT_TITLECHANGED,
    then query the title of his views. However these views (SfxTopViewFrames)
    are  also SfxListener and because the order of notifications might not be
    fixed, the title update will be enforced in advance.

    [Example]

    void SwDocShell::Notify( SfxBroadcaster& rBC, const SfxHint& rHint )
    {
        if ( rHint.IsA(TYPE(SfxSimpleHint)) )
        {
            switch( ( (SfxSimpleHint&) rHint ).GetId() )
            {
                case SFX_HINT_TITLECHANGED:
                    for ( SfxViewFrame *pTop = SfxViewFrame::GetFirst( this );
                          pTop;
                          pTop = SfxViewFrame::GetNext( this );
                    {
                        pTop->UpdateTitle();
                        ... pTop->GetName() ...
                    }
                    break;
                ...
            }
        }
    }
*/

{

    const SfxObjectFactory &rFact = GetObjectShell()->GetFactory();
    pImp->aFactoryName = OUString::createFromAscii(rFact.GetShortName());

    SfxObjectShell *pObjSh = GetObjectShell();
    if ( !pObjSh )
        return;


    const SfxMedium *pMedium = pObjSh->GetMedium();
    OUString aURL;
    GetFrame();  // -Wall required??
    if ( pObjSh->HasName() )
    {
        INetURLObject aTmp( pMedium->GetName() );
        aURL = aTmp.getName( INetURLObject::LAST_SEGMENT, true, INetURLObject::DECODE_WITH_CHARSET );
    }

    if ( aURL != pImp->aActualURL )
        // URL has changed
        pImp->aActualURL = aURL;

    // SbxObjects name
    OUString aSbxName = pObjSh->SfxShell::GetName();
    if ( IsVisible() )
    {
        aSbxName += ":";
        aSbxName += OUString::number(pImp->nDocViewNo);
    }

    SetName( aSbxName );
    GetBindings().Invalidate( SID_CURRENT_URL );
    GetBindings().Invalidate( SID_NEWDOCDIRECT );

#if defined USE_JAVA && defined MACOSX
    SystemWindow *pWindow = GetFrame().GetTopWindow_Impl();
    if ( pWindow )
    {
        NSView *pView = pWindow->GetSystemData()->mpNSView;
        if ( pView )
        {
            pImp->bNeedsUpdateTitle = sal_False;

            OUString aPath;
            SfxObjectShell *pDoc = GetObjectShell();
            if ( pDoc )
            {
                SfxMedium *pMedium = pDoc->GetMedium();
                if ( pMedium )
                {
                    OUString aBaseURL( pMedium->GetBaseURL( true ) );
                    if ( aBaseURL.getLength() )
                        osl::File::getSystemPathFromFileURL( aBaseURL, aPath );
                }
            }

            CFURLRef aDocURL = NULL;
            if ( aPath.getLength() )
            {
                CFStringRef aString = CFStringCreateWithCharactersNoCopy( NULL, aPath.getStr(), aPath.getLength(), kCFAllocatorNull );
                if ( aString )
                {
                    aDocURL = CFURLCreateWithFileSystemPath( NULL, aString, kCFURLPOSIXPathStyle, false );
                    CFRelease( aString );
                }
            }

            // Invoke even if URL is NULL
            SFXDocument_createDocument( this, pView, aDocURL, pDoc->IsReadOnlyMedium() );
            if ( aDocURL )
                CFRelease( aDocURL );
        }
    }
#endif	// USE_JAVA && MACOSX
}

void SfxViewFrame::Exec_Impl(SfxRequest &rReq )
{
    // If presently the shells are replaced...
    if ( !GetObjectShell() || !GetViewShell() )
        return;

    switch ( rReq.GetSlot() )
    {
        case SID_SHOWPOPUPS :
        {
            SFX_REQUEST_ARG(rReq, pShowItem, SfxBoolItem, SID_SHOWPOPUPS, false);
            bool bShow = pShowItem ? pShowItem->GetValue() : sal_True;
            SFX_REQUEST_ARG(rReq, pIdItem, SfxUInt16Item, SID_CONFIGITEMID, false);
            sal_uInt16 nId = pIdItem ? pIdItem->GetValue() : 0;

            SfxWorkWindow *pWorkWin = GetFrame().GetWorkWindow_Impl();
            if ( bShow )
            {
                // First, make the floats viewable
                pWorkWin->MakeChildrenVisible_Impl( bShow );
                GetDispatcher()->Update_Impl( true );

                // Then view it
                GetBindings().HidePopups( !bShow );
            }
            else
            {
                // Hide all
                SfxBindings *pBind = &GetBindings();
                while ( pBind )
                {
                    pBind->HidePopupCtrls_Impl( !bShow );
                    pBind = pBind->GetSubBindings_Impl();
                }

                pWorkWin->HidePopups_Impl( !bShow, true, nId );
                pWorkWin->MakeChildrenVisible_Impl( bShow );
            }

            Invalidate( rReq.GetSlot() );
            rReq.Done();
            break;
        }

        case SID_ACTIVATE:
        {
            MakeActive_Impl( true );
            rReq.SetReturnValue( SfxObjectItem( 0, this ) );
            break;
        }

        case SID_NEWDOCDIRECT :
        {
            SFX_REQUEST_ARG( rReq, pFactoryItem, SfxStringItem, SID_NEWDOCDIRECT, false);
            OUString aFactName;
            if ( pFactoryItem )
                aFactName = pFactoryItem->GetValue();
            else if ( !pImp->aFactoryName.isEmpty() )
                aFactName = pImp->aFactoryName;
            else
            {
                OSL_FAIL("Missing argument!");
                break;
            }

            SfxRequest aReq( SID_OPENDOC, SfxCallMode::SYNCHRON, GetPool() );
            OUString aFact("private:factory/");
            aFact += aFactName;
            aReq.AppendItem( SfxStringItem( SID_FILE_NAME, aFact ) );
            aReq.AppendItem( SfxFrameItem( SID_DOCFRAME, &GetFrame() ) );
            aReq.AppendItem( SfxStringItem( SID_TARGETNAME, OUString( "_blank" ) ) );
            SfxGetpApp()->ExecuteSlot( aReq );
            const SfxViewFrameItem* pItem = PTR_CAST( SfxViewFrameItem, aReq.GetReturnValue() );
            if ( pItem )
                rReq.SetReturnValue( SfxFrameItem( 0, pItem->GetFrame() ) );
            break;
        }

        case SID_CLOSEWIN:
        {
            // disable CloseWin, if frame is not a task
            Reference < XCloseable > xTask( GetFrame().GetFrameInterface(),  UNO_QUERY );
            if ( !xTask.is() )
                break;

            if ( GetViewShell()->PrepareClose() )
            {
                // More Views on the same Document?
                SfxObjectShell *pDocSh = GetObjectShell();
                bool bOther = false;
                for ( const SfxViewFrame* pFrame = SfxViewFrame::GetFirst( pDocSh );
                      !bOther && pFrame;
                      pFrame = SfxViewFrame::GetNext( *pFrame, pDocSh ) )
                    bOther = (pFrame != this);

                // Document only needs to be queried, if no other View present.
                bool bClosed = false;
                bool bUI = true;
                if ( ( bOther || pDocSh->PrepareClose( bUI ) ) )
                {
                    if ( !bOther )
                        pDocSh->SetModified( false );
                    rReq.Done(); // Must call this before Close()!
                    bClosed = false;
                    try
                    {
                        xTask->close(sal_True);
                        bClosed = true;
                    }
                    catch( CloseVetoException& )
                    {
                        bClosed = false;
                    }
                }

                rReq.SetReturnValue( SfxBoolItem( rReq.GetSlot(), bClosed ));
            }
            return;
        }
    }

    rReq.Done();
}

void SfxViewFrame::GetState_Impl( SfxItemSet &rSet )
{
    SfxObjectShell *pDocSh = GetObjectShell();

    if ( !pDocSh )
        return;

    const sal_uInt16 *pRanges = rSet.GetRanges();
    DBG_ASSERT(pRanges, "Set without Range");
    while ( *pRanges )
    {
        for ( sal_uInt16 nWhich = *pRanges++; nWhich <= *pRanges; ++nWhich )
        {
            switch(nWhich)
            {
            case SID_NEWDOCDIRECT :
            {
                if ( !pImp->aFactoryName.isEmpty() )
                {
                    OUString aFact("private:factory/");
                    aFact += pImp->aFactoryName;
                    rSet.Put( SfxStringItem( nWhich, aFact ) );
                }
                break;
            }

            case SID_NEWWINDOW:
                rSet.DisableItem(nWhich);
                break;

            case SID_CLOSEWIN:
            {
                // disable CloseWin, if frame is not a task
                Reference < XCloseable > xTask( GetFrame().GetFrameInterface(),  UNO_QUERY );
                if ( !xTask.is() )
                    rSet.DisableItem(nWhich);
                break;
            }

            case SID_SHOWPOPUPS :
                break;

            case SID_OBJECT:
                if ( GetViewShell() && GetViewShell()->GetVerbs().getLength() && !GetObjectShell()->IsInPlaceActive() )
                {
                    uno::Any aAny;
                    aAny <<= GetViewShell()->GetVerbs();
                    rSet.Put( SfxUnoAnyItem( sal_uInt16( SID_OBJECT ), aAny ) );
                }
                else
                    rSet.DisableItem( SID_OBJECT );
                break;

            default:
                OSL_FAIL( "invalid message-id" );
            }
        }
        ++pRanges;
    }
}

void SfxViewFrame::INetExecute_Impl( SfxRequest &rRequest )
{
    sal_uInt16 nSlotId = rRequest.GetSlot();
    switch( nSlotId )
    {
        case SID_BROWSE_FORWARD:
        case SID_BROWSE_BACKWARD:
            OSL_FAIL( "SfxViewFrame::INetExecute_Impl: SID_BROWSE_FORWARD/BACKWARD are dead!" );
            break;
        case SID_CREATELINK:
        {
/*! (pb) we need new implementation to create a link
*/
            break;
        }
        case SID_FOCUSURLBOX:
        {
            SfxStateCache *pCache = GetBindings().GetAnyStateCache_Impl( SID_OPENURL );
            if( pCache )
            {
                SfxControllerItem* pCtrl = pCache->GetItemLink();
                while( pCtrl )
                {
                    pCtrl->StateChanged( SID_FOCUSURLBOX, SfxItemState::UNKNOWN, 0 );
                    pCtrl = pCtrl->GetItemLink();
                }
            }
        }
    }

    // Recording
    rRequest.Done();
}

void SfxViewFrame::INetState_Impl( SfxItemSet &rItemSet )
{
    rItemSet.DisableItem( SID_BROWSE_FORWARD );
    rItemSet.DisableItem( SID_BROWSE_BACKWARD );

    // Add/SaveToBookmark at BASIC-IDE, QUERY-EDITOR etc. disable
    SfxObjectShell *pDocSh = GetObjectShell();
    bool bPseudo = pDocSh && !( pDocSh->GetFactory().GetFlags() & SFXOBJECTSHELL_HASOPENDOC );
    bool bEmbedded = pDocSh && pDocSh->GetCreateMode() == SFX_CREATE_MODE_EMBEDDED;
    if ( !pDocSh || bPseudo || bEmbedded || !pDocSh->HasName() )
        rItemSet.DisableItem( SID_CREATELINK );
}

void SfxViewFrame::SetZoomFactor( const Fraction &rZoomX, const Fraction &rZoomY )
{
    GetViewShell()->SetZoomFactor( rZoomX, rZoomY );
}

void SfxViewFrame::Activate( bool bMDI )
{
    DBG_ASSERT(GetViewShell(), "No Shell");
    if ( bMDI )
        pImp->bActive = true;
//(mba): here maybe as in Beanframe NotifyEvent ?!
}

void SfxViewFrame::Deactivate( bool bMDI )
{
    DBG_ASSERT(GetViewShell(), "No Shell");
    if ( bMDI )
        pImp->bActive = false;
//(mba): here maybe as in Beanframe NotifyEvent ?!
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
