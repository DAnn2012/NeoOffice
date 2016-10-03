/**************************************************************
 * 
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 * 
 * This file incorporates work covered by the following license notice:
 * 
 *   Modified May 2016 by Patrick Luby. NeoOffice is only distributed
 *   under the GNU General Public License, Version 3 as allowed by Section 4
 *   of the Apache License, Version 2.0.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 *************************************************************/



// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_sw.hxx"

#include <com/sun/star/text/NotePrintMode.hpp>

#include <cstdarg>

#ifndef _CMDID_H
#include <cmdid.h>
#endif

#ifndef _SVSTDARR_HXX
#define _SVSTDARR_STRINGSDTOR
#include <svl/svstdarr.hxx>
#endif
#include <sfx2/request.hxx>

#include <sfx2/progress.hxx>
#include <sfx2/app.hxx>
#include <svl/flagitem.hxx>
#include <vcl/msgbox.hxx>
#include <vcl/oldprintadaptor.hxx>
#include <sfx2/printer.hxx>
#include <sfx2/prnmon.hxx>
#include <editeng/paperinf.hxx>
#include <sfx2/dispatch.hxx>
#include <unotools/misccfg.hxx>
#include <svx/prtqry.hxx>
#include <svx/svdview.hxx>
#include <svl/eitem.hxx>
#include <svl/stritem.hxx>
#include <svl/intitem.hxx>
#include <svl/flagitem.hxx>
#include <sfx2/linkmgr.hxx>

#include <modcfg.hxx>
#include <edtwin.hxx>
#include <view.hxx>
#include <wrtsh.hxx>
#include <docsh.hxx>
#include <viewopt.hxx>
#include <prtopt.hxx>
#include <fontcfg.hxx>
#include <cfgitems.hxx>
#include <dbmgr.hxx>
#include <docstat.hxx>
#include <viewfunc.hxx>
#include <swmodule.hxx>
#include <wview.hxx>
#include <doc.hxx>
#include <fldbas.hxx>

#include <globals.hrc>
#include <view.hrc>
#include <app.hrc>
#include <svl/eitem.hxx>
#include <swwrtshitem.hxx>
#include "swabstdlg.hxx"
#include <svl/slstitm.hxx>

#include <unomid.h>

using namespace ::com::sun::star;


/*--------------------------------------------------------------------
	Beschreibung:	Drucker an Sfx uebergeben
 --------------------------------------------------------------------*/


SfxPrinter* __EXPORT SwView::GetPrinter( sal_Bool bCreate )
{
    const IDocumentDeviceAccess* pIDDA = GetWrtShell().getIDocumentDeviceAccess();
    SfxPrinter *pOld = pIDDA->getPrinter( false );
    SfxPrinter *pPrt = pIDDA->getPrinter( bCreate );
	if ( pOld != pPrt )
	{
		sal_Bool bWeb = 0 != PTR_CAST(SwWebView, this);
		::SetAppPrintOptions( &GetWrtShell(), bWeb );
	}
	return pPrt;
}

/*--------------------------------------------------------------------
	Beschreibung:	Druckerwechsel weitermelden
 --------------------------------------------------------------------*/

void SetPrinter( IDocumentDeviceAccess* pIDDA, SfxPrinter* pNew, sal_Bool bWeb )
{
	SwPrintOptions* pOpt = SW_MOD()->GetPrtOptions(bWeb);
	if( !pOpt)
		return;

	// Applikationseigene Druckoptionen aus SfxPrinter auslesen
	const SfxItemSet& rSet = pNew->GetOptions();

	const SwAddPrinterItem* pAddPrinterAttr;
	if( SFX_ITEM_SET == rSet.GetItemState( FN_PARAM_ADDPRINTER, sal_False,
		(const SfxPoolItem**)&pAddPrinterAttr ) )
	{
        if( pIDDA )
            pIDDA->setPrintData( *pAddPrinterAttr );
        if( pAddPrinterAttr->GetFax().getLength() )
			pOpt->SetFaxName(pAddPrinterAttr->GetFax());
	}
}


sal_uInt16 __EXPORT SwView::SetPrinter(SfxPrinter* pNew, sal_uInt16 nDiffFlags, bool  )
{
	SwWrtShell &rSh = GetWrtShell();
    SfxPrinter* pOld = rSh.getIDocumentDeviceAccess()->getPrinter( false );
    if ( pOld && pOld->IsPrinting() )
        return SFX_PRINTERROR_BUSY;

	if ( (SFX_PRINTER_JOBSETUP | SFX_PRINTER_PRINTER) & nDiffFlags )
	{
        rSh.getIDocumentDeviceAccess()->setPrinter( pNew, true, true );
        if ( nDiffFlags & SFX_PRINTER_PRINTER )
			rSh.SetModified();
	}
	sal_Bool bWeb = 0 != PTR_CAST(SwWebView, this);
	if ( nDiffFlags & SFX_PRINTER_OPTIONS )
		::SetPrinter( rSh.getIDocumentDeviceAccess(), pNew, bWeb );

	const sal_Bool bChgOri = nDiffFlags & SFX_PRINTER_CHG_ORIENTATION ? sal_True : sal_False;
	const sal_Bool bChgSize= nDiffFlags & SFX_PRINTER_CHG_SIZE ? sal_True : sal_False;
	if ( bChgOri || bChgSize )
	{
		rSh.StartAllAction();
		if ( bChgOri )
			rSh.ChgAllPageOrientation( sal_uInt16(pNew->GetOrientation()) );
		if ( bChgSize )
		{
			Size aSz( SvxPaperInfo::GetPaperSize( pNew ) );
			rSh.ChgAllPageSize( aSz );
		}
		rSh.SetModified();
		rSh.EndAllAction();
		InvalidateRulerPos();
	}
	return 0;
}

/*--------------------------------------------------------------------
	Beschreibung:	TabPage fuer applikationsspezifische Druckoptionen
 --------------------------------------------------------------------*/

SfxTabPage* __EXPORT SwView::CreatePrintOptionsPage(Window* pParent,
													const SfxItemSet& rSet)
{
#ifdef USE_JAVA
	// Fix bug 3636 by fetching the actual attributes that the printer options
	// page uses
	sal_Bool bWeb = 0 != PTR_CAST(SwWebView, this);
	::SetAppPrintOptions( &GetWrtShell(), bWeb );
	const IDocumentDeviceAccess* pIDDA = ((ViewShell*)&GetWrtShell())->getIDocumentDeviceAccess();
	SfxPrinter *pPrinter = pIDDA->getPrinter( false );
	if ( pPrinter )
	{
		SFX_ITEMSET_GET( pPrinter->GetOptions(), pAddPrinterAttr, SwAddPrinterItem, FN_PARAM_ADDPRINTER, sal_False );
		if ( pAddPrinterAttr )
		{
    		SfxItemSet aSet( rSet );
    		aSet.Put( *pAddPrinterAttr );
			return ::CreatePrintOptionsPage( pParent, aSet, sal_False );
		}
	}
#endif	 // USE_JAVA

	return ::CreatePrintOptionsPage( pParent, rSet, sal_False );
}

/*--------------------------------------------------------------------
	Beschreibung:	Print-Dispatcher
 --------------------------------------------------------------------*/

void __EXPORT SwView::ExecutePrint(SfxRequest& rReq)
{
	sal_Bool bWeb = 0 != PTR_CAST(SwWebView, this);
	::SetAppPrintOptions( &GetWrtShell(), bWeb );
	switch (rReq.GetSlot())
	{
		case FN_FAX:
		{
            SwPrintOptions* pPrintOptions = SW_MOD()->GetPrtOptions(bWeb);
            String sFaxName(pPrintOptions->GetFaxName());
			if (sFaxName.Len())
			{
				SfxStringItem aPrinterName(SID_PRINTER_NAME, sFaxName);
				SfxBoolItem aSilent( SID_SILENT, sal_True );
				GetViewFrame()->GetDispatcher()->Execute( SID_PRINTDOC,
							SFX_CALLMODE_SYNCHRON|SFX_CALLMODE_RECORD,
							&aPrinterName, &aSilent, 0L );
			}
			else
			{
				InfoBox aInfoBox(&GetEditWin(), SW_RES(MSG_ERR_NO_FAX));
				String sMsg = aInfoBox.GetMessText();
				sal_uInt16 nResNo = bWeb ? STR_WEBOPTIONS : STR_TEXTOPTIONS;
				sMsg.SearchAndReplace(String::CreateFromAscii("%1"), String(SW_RES(nResNo)));
				aInfoBox.SetMessText(sMsg);
				aInfoBox.Execute();
				SfxUInt16Item aDefPage(SID_SW_EDITOPTIONS, TP_OPTPRINT_PAGE);
				GetViewFrame()->GetDispatcher()->Execute(SID_SW_EDITOPTIONS,
							SFX_CALLMODE_SYNCHRON|SFX_CALLMODE_RECORD,
							&aDefPage, 0L );
			}
		}
		break;
		case SID_PRINTDOC:
		case SID_PRINTDOCDIRECT:
		{
			SwWrtShell* pSh = &GetWrtShell();
            SFX_REQUEST_ARG(rReq, pSilentItem, SfxBoolItem, SID_SILENT, sal_False);
            sal_Bool bSilent = pSilentItem ? pSilentItem->GetValue() : sal_False;
            SFX_REQUEST_ARG(rReq, pPrintFromMergeItem, SfxBoolItem, FN_QRY_MERGE, sal_False);
			if(pPrintFromMergeItem)
				rReq.RemoveItem(FN_QRY_MERGE);
            sal_Bool bFromMerge = pPrintFromMergeItem ? pPrintFromMergeItem->GetValue() : sal_False;
            SwMiscConfig aMiscConfig;
            bool bPrintSelection = false;
            if(!bSilent && !bFromMerge &&
                    SW_MOD()->GetModuleConfig()->IsAskForMailMerge() && pSh->IsAnyDatabaseFieldInDoc())
            {
                QueryBox aBox( &GetEditWin(), SW_RES( MSG_PRINT_AS_MERGE ));
                short nRet = aBox.Execute();
                if(RET_YES == nRet)
                {
                    SfxBoolItem aBool(FN_QRY_MERGE, sal_True);
                    GetViewFrame()->GetDispatcher()->Execute(
                                FN_QRY_MERGE, SFX_CALLMODE_ASYNCHRON, &aBool, 0L);
                    rReq.Ignore();
					return;
                }
            }
#if defined USE_JAVA && defined MACOSX
            // Don't do anything special when printing directly as we always
            // display a print dialog before printing
#else	// USE_JAVA && MACOSX
            else if( rReq.GetSlot() == SID_PRINTDOCDIRECT && ! bSilent )
            {
                if( /*!bIsAPI && */
                   ( pSh->IsSelection() || pSh->IsFrmSelected() || pSh->IsObjSelected() ) )
                {
                    short nBtn = SvxPrtQryBox(&GetEditWin()).Execute();
                    if( RET_CANCEL == nBtn )
                        return;;
                    
                    if( RET_OK == nBtn )
                        bPrintSelection = true;
                }
            }
#endif	// USE_JAVA && MACOSX

            //#i61455# if master documentes are printed silently without loaded links then update the links now
            if( bSilent && pSh->IsGlobalDoc() && !pSh->IsGlblDocSaveLinks() )
            {
                pSh->GetLinkManager().UpdateAllLinks( sal_False, sal_False, sal_False, 0 );
            }
            SfxRequest aReq( rReq );
            SfxBoolItem aBool(SID_SELECTION, bPrintSelection);
            aReq.AppendItem( aBool );
			SfxViewShell::ExecuteSlot( aReq, SfxViewShell::GetInterface() );
			return;
		}
		default:
			ASSERT(!this, falscher Dispatcher);
			return;
	}
}

/*--------------------------------------------------------------------
	Beschreibung:	Page Drucker/Zusaetze erzeugen fuer SwView und
					SwPagePreview
 --------------------------------------------------------------------*/

SfxTabPage* CreatePrintOptionsPage( Window *pParent,
								const SfxItemSet &rOptions, sal_Bool bPreview )
{
	SfxTabPage* pPage = NULL;
	SwAbstractDialogFactory* pFact = SwAbstractDialogFactory::Create();
	if ( pFact )
	{
		::CreateTabPage fnCreatePage = pFact->GetTabPageCreatorFunc( TP_OPTPRINT_PAGE );
		if ( fnCreatePage )
			pPage = (*fnCreatePage)( pParent, rOptions );
	}
	SfxAllItemSet aSet(*(rOptions.GetPool()));
	aSet.Put (SfxBoolItem(SID_PREVIEWFLAG_TYPE, bPreview));
	aSet.Put (SfxBoolItem(SID_FAX_LIST, sal_True));
	pPage->PageCreated(aSet);
	return pPage;
}


void SetAppPrintOptions( ViewShell* pSh, sal_Bool bWeb )
{
    const IDocumentDeviceAccess* pIDDA = pSh->getIDocumentDeviceAccess();
    SwPrintData aPrtData = pIDDA->getPrintData();

    if( pIDDA->getPrinter( false ) )
	{
		// Applikationseigene Druckoptionen in SfxPrinter schiessen
        SwAddPrinterItem aAddPrinterItem (FN_PARAM_ADDPRINTER, aPrtData);
		SfxItemSet aSet( pSh->GetAttrPool(),
					FN_PARAM_ADDPRINTER, 		FN_PARAM_ADDPRINTER,
					SID_HTML_MODE,				SID_HTML_MODE,
					SID_PRINTER_NOTFOUND_WARN, 	SID_PRINTER_NOTFOUND_WARN,
					SID_PRINTER_CHANGESTODOC, 	SID_PRINTER_CHANGESTODOC,
					0 );

        utl::MiscCfg aMisc;

		if(bWeb)
			aSet.Put(SfxUInt16Item(SID_HTML_MODE,
					::GetHtmlMode(((SwWrtShell*)pSh)->GetView().GetDocShell())));
		aSet.Put(SfxBoolItem(SID_PRINTER_NOTFOUND_WARN,
						aMisc.IsNotFoundWarning() ));
		aSet.Put(aAddPrinterItem);
		aSet.Put( SfxFlagItem( SID_PRINTER_CHANGESTODOC,
			(aMisc.IsPaperSizeWarning() ? SFX_PRINTER_CHG_SIZE : 0)   |
            (aMisc.IsPaperOrientationWarning()  ? SFX_PRINTER_CHG_ORIENTATION : 0 )));

        pIDDA->getPrinter( true )->SetOptions( aSet );
	}

}
