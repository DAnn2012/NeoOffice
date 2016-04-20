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
 *   Modified April 2016 by Patrick Luby. NeoOffice is only distributed
 *   under the GNU General Public License, Version 3 as allowed by Section 4
 *   of the Apache License, Version 2.0.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 *************************************************************/



// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_sc.hxx"

#include "scitems.hxx"

#include <memory> //auto_ptr
#include <editeng/adjitem.hxx>
#include <svx/algitem.hxx>
#include <svx/dbexch.hrc>
#include <editeng/editview.hxx>
#include <editeng/editstat.hxx>
#include <editeng/flditem.hxx>
#include <svx/svdetc.hxx>
#include <editeng/editobj.hxx>
#include <sfx2/dispatch.hxx>
#include <sfx2/viewfrm.hxx>
#include <sfx2/docfile.hxx>
#include <svl/stritem.hxx>
#include <svtools/svlbox.hxx>
#include <svtools/svtabbx.hxx>
#include <svl/urlbmk.hxx>
#include <tools/urlobj.hxx>
#include <vcl/cursor.hxx>
#include <vcl/sound.hxx>
#include <vcl/graph.hxx>
#include <vcl/hatch.hxx>
#include <sot/formats.hxx>
#include <sot/clsids.hxx>

#include <svx/svdview.hxx>		// fuer Command-Handler (COMMAND_INSERTTEXT)
#include <editeng/outliner.hxx>		// fuer Command-Handler (COMMAND_INSERTTEXT)
#include <svx/svditer.hxx>
#include <svx/svdocapt.hxx>
#include <svx/svdpagv.hxx>

#include <com/sun/star/sheet/DataPilotFieldFilter.hpp>
#include <com/sun/star/sheet/DataPilotFieldOrientation.hpp>
#include <com/sun/star/sheet/DataPilotTableHeaderData.hpp>
#include <com/sun/star/sheet/DataPilotTableResultData.hpp>
#include <com/sun/star/sheet/DataPilotTablePositionData.hpp>
#include <com/sun/star/sheet/DataPilotTablePositionType.hpp>
#include <com/sun/star/sheet/MemberResultFlags.hpp>
#include <com/sun/star/awt/KeyModifier.hpp>
#include <com/sun/star/awt/MouseButton.hpp>
#include <com/sun/star/script/vba/VBAEventId.hpp>
#include <com/sun/star/script/vba/XVBAEventProcessor.hpp>

#include "gridwin.hxx"
#include "tabvwsh.hxx"
#include "docsh.hxx"
#include "viewdata.hxx"
#include "tabview.hxx"
#include "select.hxx"
#include "scmod.hxx"
#include "document.hxx"
#include "attrib.hxx"
#include "dbcolect.hxx"
#include "stlpool.hxx"
#include "printfun.hxx"
#include "cbutton.hxx"
#include "sc.hrc"
#include "globstr.hrc"
#include "editutil.hxx"
#include "scresid.hxx"
#include "inputhdl.hxx"
#include "uiitems.hxx"			// Filter-Dialog - auslagern !!!
#include "filtdlg.hxx"
#include "impex.hxx"			// Sylk-ID fuer CB
#include "cell.hxx"				// fuer Edit-Felder
#include "patattr.hxx"
#include "notemark.hxx"
#include "rfindlst.hxx"
#include "docpool.hxx"
#include "output.hxx"
#include "docfunc.hxx"
#include "dbdocfun.hxx"
#include "dpobject.hxx"
#include "dpoutput.hxx"
#include "transobj.hxx"
#include "drwtrans.hxx"
#include "seltrans.hxx"
#include "sizedev.hxx"
#include "AccessibilityHints.hxx"
#include "dpsave.hxx"
#include "viewuno.hxx"
#include "compiler.hxx"
#include "editable.hxx"
#include "fillinfo.hxx"
#include "scitems.hxx"
#include "userdat.hxx"
#include "drwlayer.hxx"
#include "attrib.hxx"
#include "validat.hxx"
#include "tabprotection.hxx"
#include "postit.hxx"
#include "dpcontrol.hxx"
#include "cellsuno.hxx"

#include "drawview.hxx"
#include <svx/sdrpagewindow.hxx>
#include <svx/sdr/overlay/overlaymanager.hxx>
#include <vcl/svapp.hxx>
#include <svx/sdr/overlay/overlayselection.hxx>

#if defined USE_JAVA && defined MACOSX
#include <premac.h>
#include <CoreFoundation/CoreFoundation.h>
#include <postmac.h>
#endif	// USE_JAVA && MACOSX

using namespace com::sun::star;
using ::com::sun::star::uno::Sequence;
using ::com::sun::star::uno::Any;

const sal_uInt8 SC_NESTEDBUTTON_NONE = 0;
const sal_uInt8 SC_NESTEDBUTTON_DOWN = 1;
const sal_uInt8 SC_NESTEDBUTTON_UP   = 2;

#define SC_AUTOFILTER_ALL		0
#define	SC_AUTOFILTER_TOP10     1
#define	SC_AUTOFILTER_CUSTOM    2

//	Modi fuer die FilterListBox
enum ScFilterBoxMode
{
	SC_FILTERBOX_FILTER,
	SC_FILTERBOX_DATASELECT,
	SC_FILTERBOX_SCENARIO,
	SC_FILTERBOX_PAGEFIELD
};

extern SfxViewShell* pScActiveViewShell;			// global.cxx
extern sal_uInt16 nScClickMouseModifier;				// global.cxx
extern sal_uInt16 nScFillModeMouseModifier;				// global.cxx

#define SC_FILTERLISTBOX_LINES	12

#ifdef USE_JAVA

// Comment out the following line to disable our custom native highlighting code
#define USE_NATIVE_HIGHLIGHT_COLOR

static bool UseNativeHighlightColor()
{
    bool bUseNativeHighlightColor = true;

#ifdef MACOSX
    CFPropertyListRef aPref = CFPreferencesCopyAppValue( CFSTR( "UseNativeHighlightColor" ), kCFPreferencesCurrentApplication );
    if( aPref ) 
    {
        if ( CFGetTypeID( aPref ) == CFBooleanGetTypeID() && (CFBooleanRef)aPref == kCFBooleanFalse )
            bUseNativeHighlightColor = false;
        CFRelease( aPref );
    }
#endif	// MACOSX

    return bUseNativeHighlightColor;
}
 
#endif	// USE_JAVA

// ============================================================================

ScGridWindow::VisibleRange::VisibleRange() :
    mnCol1(0), mnCol2(MAXCOL), mnRow1(0), mnRow2(MAXROW)
{
}

bool ScGridWindow::VisibleRange::isInside(SCCOL nCol, SCROW nRow) const
{
    return mnCol1 <= nCol && nCol <= mnCol2 && mnRow1 <= nRow && nRow <= mnRow2;
}

// ============================================================================

class ScFilterListBox : public ListBox
{
private:
	ScGridWindow*	pGridWin;
	SCCOL			nCol;
	SCROW			nRow;
	sal_Bool			bButtonDown;
	sal_Bool			bInit;
	sal_Bool			bCancelled;
    sal_Bool            bInSelect;
    bool            mbListHasDates;
	sal_uLong			nSel;
	ScFilterBoxMode	eMode;

protected:
	virtual void	LoseFocus();
	void			SelectHdl();

public:
				ScFilterListBox( Window* pParent, ScGridWindow* pGrid,
								 SCCOL nNewCol, SCROW nNewRow, ScFilterBoxMode eNewMode );
				~ScFilterListBox();

	virtual long	PreNotify( NotifyEvent& rNEvt );
	virtual void	Select();

	SCCOL			GetCol() const			{ return nCol; }
	SCROW			GetRow() const			{ return nRow; }
	ScFilterBoxMode	GetMode() const			{ return eMode; }
	sal_Bool			IsDataSelect() const	{ return (eMode == SC_FILTERBOX_DATASELECT); }
	void			EndInit();
    sal_Bool            IsInInit() const        { return bInit; }
	void			SetCancelled()			{ bCancelled = sal_True; }
    sal_Bool            IsInSelect() const      { return bInSelect; }
    void            SetListHasDates(bool b) { mbListHasDates = b; }
    bool            HasDates() const        { return mbListHasDates; }
};

//-------------------------------------------------------------------

//	ListBox in einem FloatingWindow (pParent)
ScFilterListBox::ScFilterListBox( Window* pParent, ScGridWindow* pGrid,
								  SCCOL nNewCol, SCROW nNewRow, ScFilterBoxMode eNewMode ) :
	ListBox( pParent, WB_AUTOHSCROLL ),
	pGridWin( pGrid ),
	nCol( nNewCol ),
	nRow( nNewRow ),
	bButtonDown( sal_False ),
	bInit( sal_True ),
	bCancelled( sal_False ),
    bInSelect( sal_False ),
    mbListHasDates(false),
	nSel( 0 ),
	eMode( eNewMode )
{
}

__EXPORT ScFilterListBox::~ScFilterListBox()
{
	if (IsMouseCaptured())
		ReleaseMouse();
}

void ScFilterListBox::EndInit()
{
	sal_uInt16 nPos = GetSelectEntryPos();
	if ( LISTBOX_ENTRY_NOTFOUND == nPos )
		nSel = 0;
	else
		nSel = nPos;

	bInit = sal_False;
}

void __EXPORT ScFilterListBox::LoseFocus()
{
#ifndef UNX
	Hide();
#endif
}

// -----------------------------------------------------------------------

long ScFilterListBox::PreNotify( NotifyEvent& rNEvt )
{
	long nDone = 0;
	if ( rNEvt.GetType() == EVENT_KEYINPUT )
	{
		KeyEvent aKeyEvt = *rNEvt.GetKeyEvent();
		KeyCode aCode = aKeyEvt.GetKeyCode();
		if ( !aCode.GetModifier() )				// ohne alle Modifiers
		{
			sal_uInt16 nKey = aCode.GetCode();
			if ( nKey == KEY_RETURN )
			{
				SelectHdl();					// auswaehlen
				nDone = 1;
			}
			else if ( nKey == KEY_ESCAPE )
			{
				pGridWin->ClickExtern();		// loescht die List-Box !!!
				nDone = 1;
			}
		}
	}

	return nDone ? nDone : ListBox::PreNotify( rNEvt );
}

void __EXPORT ScFilterListBox::Select()
{
	ListBox::Select();
	SelectHdl();
}

void __EXPORT ScFilterListBox::SelectHdl()
{
	if ( !IsTravelSelect() && !bInit && !bCancelled )
	{
		sal_uInt16 nPos = GetSelectEntryPos();
		if ( LISTBOX_ENTRY_NOTFOUND != nPos )
		{
			nSel = nPos;
			if (!bButtonDown)
            {
                // #i81298# set bInSelect flag, so the box isn't deleted from modifications within FilterSelect
                bInSelect = sal_True;
				pGridWin->FilterSelect( nSel );
                bInSelect = sal_False;
            }
		}
	}
}

// ============================================================================

// use a System floating window for the above filter listbox
class ScFilterFloatingWindow : public FloatingWindow
{
public:
	ScFilterFloatingWindow( Window* pParent, WinBits nStyle = WB_STDFLOATWIN );
    virtual ~ScFilterFloatingWindow();
    // required for System FloatingWindows that will not process KeyInput by themselves
    virtual Window* GetPreferredKeyInputWindow();
};

ScFilterFloatingWindow::ScFilterFloatingWindow( Window* pParent, WinBits nStyle ) :
	FloatingWindow( pParent, nStyle|WB_SYSTEMWINDOW ) // make it a system floater
    {}

ScFilterFloatingWindow::~ScFilterFloatingWindow()
{
    EndPopupMode();
}

Window* ScFilterFloatingWindow::GetPreferredKeyInputWindow()
{
    // redirect keyinput in the child window
    return GetWindow(WINDOW_FIRSTCHILD) ? GetWindow(WINDOW_FIRSTCHILD)->GetPreferredKeyInputWindow() : NULL;    // will be the FilterBox
}

// ============================================================================

sal_Bool lcl_IsEditableMatrix( ScDocument* pDoc, const ScRange& rRange )
{
	//	wenn es ein editierbarer Bereich ist, und rechts unten eine Matrix-Zelle
	//	mit Origin links oben liegt, enthaelt der Bereich genau die Matrix.
	//!	Direkt die MatrixEdges Funktionen von der Column herausreichen ???

	if ( !pDoc->IsBlockEditable( rRange.aStart.Tab(), rRange.aStart.Col(),rRange.aStart.Row(),
									rRange.aEnd.Col(),rRange.aEnd.Row() ) )
		return sal_False;

	ScAddress aPos;
	const ScBaseCell* pCell = pDoc->GetCell( rRange.aEnd );
	return ( pCell && pCell->GetCellType() == CELLTYPE_FORMULA &&
			((ScFormulaCell*)pCell)->GetMatrixOrigin(aPos) && aPos == rRange.aStart );

}

void lcl_UnLockComment( ScDrawView* pView, SdrPageView* pPV, SdrModel* pDrDoc, const Point& rPos, ScViewData* pViewData )
{
    if (!pView && !pPV && !pDrDoc && !pViewData)
        return;

    ScDocument& rDoc = *pViewData->GetDocument();
    ScAddress aCellPos( pViewData->GetCurX(), pViewData->GetCurY(), pViewData->GetTabNo() );
    ScPostIt* pNote = rDoc.GetNote( aCellPos );
    SdrObject* pObj = pNote ? pNote->GetCaption() : 0;
    if( pObj && pObj->GetLogicRect().IsInside( rPos ) && ScDrawLayer::IsNoteCaption( pObj ) )
    {
        const ScProtectionAttr* pProtAttr =  static_cast< const ScProtectionAttr* > (rDoc.GetAttr( aCellPos.Col(), aCellPos.Row(), aCellPos.Tab(), ATTR_PROTECTION ) );
        bool bProtectAttr = pProtAttr->GetProtection() || pProtAttr->GetHideCell() ;
        bool bProtectDoc =  rDoc.IsTabProtected( aCellPos.Tab() ) || pViewData->GetSfxDocShell()->IsReadOnly() ;
        // unlock internal layer (if not protected), will be relocked in ScDrawView::MarkListHasChanged()
        pView->LockInternalLayer( bProtectDoc && bProtectAttr );
    }
}

sal_Bool lcl_GetHyperlinkCell(ScDocument* pDoc, SCCOL& rPosX, SCROW& rPosY, SCTAB nTab, ScBaseCell*& rpCell )
{
	sal_Bool bFound = sal_False;
	do
	{
		pDoc->GetCell( rPosX, rPosY, nTab, rpCell );
		if ( !rpCell || rpCell->GetCellType() == CELLTYPE_NOTE )
		{
			if ( rPosX <= 0 )
				return sal_False;							// alles leer bis links
			else
				--rPosX;								// weitersuchen
		}
                else if ( rpCell->GetCellType() == CELLTYPE_EDIT)
                    bFound = sal_True;
                else if (rpCell->GetCellType() == CELLTYPE_FORMULA &&
                  static_cast<ScFormulaCell*>(rpCell)->IsHyperLinkCell())
                    bFound = sal_True;
	    else
			return sal_False;								// andere Zelle
	}
	while ( !bFound );

	return bFound;
}

// ---------------------------------------------------------------------------
//	WB_DIALOGCONTROL noetig fuer UNO-Controls
ScGridWindow::ScGridWindow( Window* pParent, ScViewData* pData, ScSplitPos eWhichPos )
:			Window( pParent, WB_CLIPCHILDREN | WB_DIALOGCONTROL ),
			DropTargetHelper( this ),
			DragSourceHelper( this ),
            mpOOCursors( NULL ),
            mpOOSelection( NULL ),
            mpOOAutoFill( NULL ),
            mpOODragRect( NULL ),
            mpOOHeader( NULL ),
            mpOOShrink( NULL ),
            mpAutoFillRect(static_cast<Rectangle*>(NULL)),
			pViewData( pData ),
			eWhich( eWhichPos ),
			pNoteMarker( NULL ),
			pFilterBox( NULL ),
			pFilterFloat( NULL ),
            mpDPFieldPopup(NULL),
            mpFilterButton(NULL), 
			nCursorHideCount( 0 ),
			bMarking( sal_False ),
			nButtonDown( 0 ),
			bEEMouse( sal_False ),
			nMouseStatus( SC_GM_NONE ),
            nNestedButtonState( SC_NESTEDBUTTON_NONE ),
			bDPMouse( sal_False ),
			bRFMouse( sal_False ),
			nPagebreakMouse( SC_PD_NONE ),
            bPagebreakDrawn( sal_False ),
			nPageScript( 0 ),
			bDragRect( sal_False ),
            meDragInsertMode( INS_NONE ),
			nCurrentPointer( 0 ),
			bIsInScroll( sal_False ),
			bIsInPaint( sal_False ),
			aComboButton( this ),
			aCurMousePos( 0,0 ),
			nPaintCount( 0 ),
			bNeedsRepaint( sal_False ),
			bAutoMarkVisible( sal_False ),
			bListValButton( sal_False )
{
	switch(eWhich)
	{
		case SC_SPLIT_TOPLEFT:
			eHWhich = SC_SPLIT_LEFT;
			eVWhich = SC_SPLIT_TOP;
			break;
		case SC_SPLIT_TOPRIGHT:
			eHWhich = SC_SPLIT_RIGHT;
			eVWhich = SC_SPLIT_TOP;
			break;
		case SC_SPLIT_BOTTOMLEFT:
			eHWhich = SC_SPLIT_LEFT;
			eVWhich = SC_SPLIT_BOTTOM;
			break;
		case SC_SPLIT_BOTTOMRIGHT:
			eHWhich = SC_SPLIT_RIGHT;
			eVWhich = SC_SPLIT_BOTTOM;
			break;
		default:
			DBG_ERROR("GridWindow: falsche Position");
	}

	SetBackground();

	SetMapMode(pViewData->GetLogicMode(eWhich));
//	EnableDrop();
	EnableChildTransparentMode();
	SetDialogControlFlags( WINDOW_DLGCTRL_RETURN | WINDOW_DLGCTRL_WANTFOCUS );

	SetHelpId( HID_SC_WIN_GRIDWIN );
	SetUniqueId( HID_SC_WIN_GRIDWIN );

	SetDigitLanguage( SC_MOD()->GetOptDigitLanguage() );
    EnableRTL( sal_False );
}

__EXPORT ScGridWindow::~ScGridWindow()
{
	// #114409#
	ImpDestroyOverlayObjects();

	delete pFilterBox;
	delete pFilterFloat;
	delete pNoteMarker;
}

void __EXPORT ScGridWindow::Resize( const Size& )
{
	//	gar nix
}

void ScGridWindow::ClickExtern()
{
    do
    {
        // #i81298# don't delete the filter box when called from its select handler
        // (possible through row header size update)
        // #i84277# when initializing the filter box, a Basic error can deactivate the view
        if ( pFilterBox && ( pFilterBox->IsInSelect() || pFilterBox->IsInInit() ) )
        {
            break;
        }
    
        DELETEZ(pFilterBox);
        DELETEZ(pFilterFloat);
    }
    while (false);

    if (mpDPFieldPopup.get())
    {
        mpDPFieldPopup->close(false);
        mpDPFieldPopup.reset();
    }
}

IMPL_LINK( ScGridWindow, PopupModeEndHdl, FloatingWindow*, EMPTYARG )
{
	if (pFilterBox)
		pFilterBox->SetCancelled();		// nicht mehr auswaehlen
	GrabFocus();
	return 0;
}

IMPL_LINK( ScGridWindow, PopupSpellingHdl, SpellCallbackInfo*, pInfo )
{
    if( pInfo->nCommand == SPELLCMD_STARTSPELLDLG )
        pViewData->GetDispatcher().Execute( SID_SPELL_DIALOG, SFX_CALLMODE_ASYNCHRON );
    return 0;
}

void ScGridWindow::ExecPageFieldSelect( SCCOL nCol, SCROW nRow, sal_Bool bHasSelection, const String& rStr )
{
	//!	gridwin2 ?

	ScDocument* pDoc = pViewData->GetDocument();
	SCTAB nTab = pViewData->GetTabNo();
	ScDPObject*	pDPObj = pDoc->GetDPAtCursor(nCol, nRow, nTab);
	if ( pDPObj && nCol > 0 )
	{
		// look for the dimension header left of the drop-down arrow
		sal_uInt16 nOrient = sheet::DataPilotFieldOrientation_HIDDEN;
		long nField = pDPObj->GetHeaderDim( ScAddress( nCol-1, nRow, nTab ), nOrient );
		if ( nField >= 0 && nOrient == sheet::DataPilotFieldOrientation_PAGE )
		{
			ScDPSaveData aSaveData( *pDPObj->GetSaveData() );

			sal_Bool bIsDataLayout;
			String aDimName = pDPObj->GetDimName( nField, bIsDataLayout );
			if ( !bIsDataLayout )
			{
				ScDPSaveDimension* pDim = aSaveData.GetDimensionByName(aDimName);

				if ( bHasSelection )
					pDim->SetCurrentPage( &rStr );
				else
					pDim->SetCurrentPage( NULL );

				ScDPObject aNewObj( *pDPObj );
				aNewObj.SetSaveData( aSaveData );
				ScDBDocFunc aFunc( *pViewData->GetDocShell() );
				aFunc.DataPilotUpdate( pDPObj, &aNewObj, sal_True, sal_False );
				pViewData->GetView()->CursorPosChanged();		// shells may be switched
			}
		}
	}
}

void ScGridWindow::LaunchPageFieldMenu( SCCOL nCol, SCROW nRow )
{
	//!	merge position/size handling with DoAutoFilterMenue

	delete pFilterBox;
	delete pFilterFloat;

	sal_uInt16 i;
	ScDocument* pDoc = pViewData->GetDocument();
	SCTAB nTab = pViewData->GetTabNo();
	sal_Bool bLayoutRTL = pDoc->IsLayoutRTL( nTab );

	long nSizeX  = 0;
	long nSizeY  = 0;
	long nHeight = 0;
	pViewData->GetMergeSizePixel( nCol, nRow, nSizeX, nSizeY );
	Point aPos = pViewData->GetScrPos( nCol, nRow, eWhich );
	if ( bLayoutRTL )
		aPos.X() -= nSizeX;

	Rectangle aCellRect( OutputToScreenPixel(aPos), Size(nSizeX,nSizeY) );

	aPos.X() -= 1;
	aPos.Y() += nSizeY - 1;

	pFilterFloat = new ScFilterFloatingWindow( this, WinBits(WB_BORDER) );		// not resizable etc.
	pFilterFloat->SetPopupModeEndHdl( LINK( this, ScGridWindow, PopupModeEndHdl ) );
	pFilterBox = new ScFilterListBox( pFilterFloat, this, nCol, nRow, SC_FILTERBOX_PAGEFIELD );
	if ( bLayoutRTL )
		pFilterBox->EnableMirroring();

	nSizeX += 1;

	{
		Font 	aOldFont = GetFont(); SetFont( pFilterBox->GetFont() );
		MapMode aOldMode = GetMapMode(); SetMapMode( MAP_PIXEL );

		nHeight  = GetTextHeight();
		nHeight *= SC_FILTERLISTBOX_LINES;

		SetMapMode( aOldMode );
		SetFont( aOldFont );
	}

	//	SetSize comes later

	TypedScStrCollection aStrings( 128, 128 );

	//	get list box entries and selection
	sal_Bool bHasCurrentPage = sal_False;
	String aCurrentPage;
	ScDPObject*	pDPObj = pDoc->GetDPAtCursor(nCol, nRow, nTab);
	if ( pDPObj && nCol > 0 )
	{
		// look for the dimension header left of the drop-down arrow
		sal_uInt16 nOrient = sheet::DataPilotFieldOrientation_HIDDEN;
		long nField = pDPObj->GetHeaderDim( ScAddress( nCol-1, nRow, nTab ), nOrient );
		if ( nField >= 0 && nOrient == sheet::DataPilotFieldOrientation_PAGE )
		{
			pDPObj->FillPageList( aStrings, nField );

			// get current page from SaveData

			ScDPSaveData* pSaveData = pDPObj->GetSaveData();
			sal_Bool bIsDataLayout;
			String aDimName = pDPObj->GetDimName( nField, bIsDataLayout );
			if ( pSaveData && !bIsDataLayout )
			{
				ScDPSaveDimension* pDim = pSaveData->GetExistingDimensionByName(aDimName);
				if ( pDim && pDim->HasCurrentPage() )
				{
					aCurrentPage = pDim->GetCurrentPage();
					bHasCurrentPage = sal_True;
				}
			}
		}
	}

	//	include all entry widths for the size of the drop-down
	long nMaxText = 0;
	sal_uInt16 nCount = aStrings.GetCount();
	for (i=0; i<nCount; i++)
	{
		TypedStrData* pData = aStrings[i];
		long nTextWidth = pFilterBox->GetTextWidth( pData->GetString() );
		if ( nTextWidth > nMaxText )
			nMaxText = nTextWidth;
	}

	//	add scrollbar width if needed (string entries are counted here)
	//	(scrollbar is shown if the box is exactly full?)
	if ( nCount >= SC_FILTERLISTBOX_LINES )
		nMaxText += GetSettings().GetStyleSettings().GetScrollBarSize();

	nMaxText += 4;				// for borders

	if ( nMaxText > nSizeX )
		nSizeX = nMaxText;		// just modify width - starting position is unchanged

	//	adjust position and size to window

	Size aParentSize = GetParent()->GetOutputSizePixel();
	Size aSize( nSizeX, nHeight );

	if ( aSize.Height() > aParentSize.Height() )
		aSize.Height() = aParentSize.Height();
	if ( aPos.Y() + aSize.Height() > aParentSize.Height() )
		aPos.Y() = aParentSize.Height() - aSize.Height();

	pFilterBox->SetSizePixel( aSize );
	pFilterBox->Show();					// Show must be called before SetUpdateMode
	pFilterBox->SetUpdateMode(sal_False);

	pFilterFloat->SetOutputSizePixel( aSize );
	pFilterFloat->StartPopupMode( aCellRect, FLOATWIN_POPUPMODE_DOWN|FLOATWIN_POPUPMODE_GRABFOCUS);

	//	fill the list box
	sal_Bool bWait = ( nCount > 100 );

	if (bWait)
		EnterWait();

	for (i=0; i<nCount; i++)
		pFilterBox->InsertEntry( aStrings[i]->GetString() );

    pFilterBox->SetSeparatorPos( 0 );

	if (bWait)
		LeaveWait();

	pFilterBox->SetUpdateMode(sal_True);

    sal_uInt16 nSelPos = LISTBOX_ENTRY_NOTFOUND;
	if (bHasCurrentPage)
		nSelPos = pFilterBox->GetEntryPos( aCurrentPage );

	if ( nSelPos == LISTBOX_ENTRY_NOTFOUND )
		nSelPos = 0;                            // first entry

	pFilterBox->GrabFocus();

	//	call Select after GrabFocus, so the focus rectangle ends up in the right position
	if ( nSelPos != LISTBOX_ENTRY_NOTFOUND )
		pFilterBox->SelectEntryPos( nSelPos );

	pFilterBox->EndInit();

	nMouseStatus = SC_GM_FILTER;
	CaptureMouse();
}

void ScGridWindow::LaunchDPFieldMenu( SCCOL nCol, SCROW nRow )
{
    SCTAB nTab = pViewData->GetTabNo();
    ScDPObject* pDPObj = pViewData->GetDocument()->GetDPAtCursor(nCol, nRow, nTab);
    if (!pDPObj)
        return;

    // Get the geometry of the cell.
    Point aScrPos = pViewData->GetScrPos(nCol, nRow, eWhich);
    long nSizeX, nSizeY;
    pViewData->GetMergeSizePixel(nCol, nRow, nSizeX, nSizeY);
    Size aScrSize(nSizeX-1, nSizeY-1);

    DPLaunchFieldPopupMenu(OutputToScreenPixel(aScrPos), aScrSize, ScAddress(nCol, nRow, nTab), pDPObj);
}

void ScGridWindow::DoScenarioMenue( const ScRange& rScenRange )
{
	delete pFilterBox;
	delete pFilterFloat;

	SCCOL nCol = rScenRange.aEnd.Col();		// Zelle unterhalb des Buttons
	SCROW nRow = rScenRange.aStart.Row();
	if (nRow == 0)
	{
		nRow = rScenRange.aEnd.Row() + 1;		// Bereich ganz oben -> Button unterhalb
		if (nRow>MAXROW) nRow = MAXROW;
		//!	Texthoehe addieren (wenn sie an der View gespeichert ist...)
	}

	ScDocument* pDoc = pViewData->GetDocument();
	SCTAB nTab = pViewData->GetTabNo();
	sal_Bool bLayoutRTL = pDoc->IsLayoutRTL( nTab );

	long nSizeX  = 0;
	long nSizeY  = 0;
	long nHeight = 0;
	pViewData->GetMergeSizePixel( nCol, nRow, nSizeX, nSizeY );
	Point aPos = pViewData->GetScrPos( nCol, nRow, eWhich );
	if ( bLayoutRTL )
		aPos.X() -= nSizeX;
	Rectangle aCellRect( OutputToScreenPixel(aPos), Size(nSizeX,nSizeY) );
	aCellRect.Top()    -= nSizeY;
	aCellRect.Bottom() -= nSizeY - 1;
	//	Die ListBox direkt unter der schwarzen Linie auf dem Zellgitter
	//	(wenn die Linie verdeckt wird, sieht es komisch aus...)

	pFilterFloat = new ScFilterFloatingWindow( this, WinBits(WB_BORDER) );		// nicht resizable etc.
	pFilterFloat->SetPopupModeEndHdl( LINK( this, ScGridWindow, PopupModeEndHdl ) );
	pFilterBox = new ScFilterListBox( pFilterFloat, this, nCol, nRow, SC_FILTERBOX_SCENARIO );
	if ( bLayoutRTL )
		pFilterBox->EnableMirroring();

	nSizeX += 1;

	{
		Font 	aOldFont = GetFont(); SetFont( pFilterBox->GetFont() );
		MapMode aOldMode = GetMapMode(); SetMapMode( MAP_PIXEL );

		nHeight  = GetTextHeight();
		nHeight *= SC_FILTERLISTBOX_LINES;

		SetMapMode( aOldMode );
		SetFont( aOldFont );
	}

	//	SetSize spaeter
/*
	pFilterBox->SetSelectionMode( SINGLE_SELECTION );
	pFilterBox->SetTabs( nFilterBoxTabs, MapUnit( MAP_APPFONT ));
	pFilterBox->SetTabJustify( 1, bLayoutRTL ? AdjustRight : AdjustLeft );
*/

	//	ParentSize Abfrage fehlt
	Size aSize( nSizeX, nHeight );
	pFilterBox->SetSizePixel( aSize );
	pFilterBox->Show();					// Show muss vor SetUpdateMode kommen !!!
	pFilterBox->SetUpdateMode(sal_False);

	//	SetOutputSizePixel/StartPopupMode erst unten, wenn die Groesse feststeht

	//	Listbox fuellen

	long nMaxText = 0;
	String aCurrent;
	String aTabName;
	SCTAB nTabCount = pDoc->GetTableCount();
	SCTAB nEntryCount = 0;
	for (SCTAB i=nTab+1; i<nTabCount && pDoc->IsScenario(i); i++)
	{
		if (pDoc->HasScenarioRange( i, rScenRange ))
			if (pDoc->GetName( i, aTabName ))
			{
				pFilterBox->InsertEntry( aTabName );
				if (pDoc->IsActiveScenario(i))
					aCurrent = aTabName;
				long nTextWidth = pFilterBox->GetTextWidth( aTabName );
				if ( nTextWidth > nMaxText )
					nMaxText = nTextWidth;
				++nEntryCount;
			}
	}
	if (nEntryCount > SC_FILTERLISTBOX_LINES)
		nMaxText += GetSettings().GetStyleSettings().GetScrollBarSize();
	nMaxText += 4;			// fuer Rand
	if ( nMaxText > 300 )
		nMaxText = 300;		// auch nicht uebertreiben (Pixel)

	if (nMaxText > nSizeX)	// Groesse auf benoetigte Groesse anpassen
	{
		long nDiff = nMaxText - nSizeX;
		aSize = Size( nMaxText, nHeight );
		pFilterBox->SetSizePixel( aSize );
		pFilterFloat->SetOutputSizePixel( aSize );

		if ( !bLayoutRTL )
		{
			//	also move popup position
			long nNewX = aCellRect.Left() - nDiff;
			if ( nNewX < 0 )
				nNewX = 0;
			aCellRect.Left() = nNewX;
		}
	}

	pFilterFloat->SetOutputSizePixel( aSize );
	pFilterFloat->StartPopupMode( aCellRect, FLOATWIN_POPUPMODE_DOWN|FLOATWIN_POPUPMODE_GRABFOCUS );

	pFilterBox->SetUpdateMode(sal_True);
	pFilterBox->GrabFocus();

	//	Select erst nach GrabFocus, damit das Focus-Rechteck richtig landet
//!	SvLBoxEntry* pSelect = NULL;
	sal_uInt16 nPos = LISTBOX_ENTRY_NOTFOUND;
	if (aCurrent.Len())
	{
		nPos = pFilterBox->GetEntryPos( aCurrent );
//!		pSelect = pFilterBox->GetEntry( nPos );
	}
	if (/*!pSelect*/ LISTBOX_ENTRY_NOTFOUND == nPos && pFilterBox->GetEntryCount() > 0 )
		nPos = 0;
//!		pSelect = pFilterBox->GetEntry(0);			// einer sollte immer selektiert sein
	if (/*pSelect*/ LISTBOX_ENTRY_NOTFOUND != nPos )
		pFilterBox->SelectEntryPos(nPos);

	pFilterBox->EndInit();

	// Szenario-Auswahl kommt aus MouseButtonDown:
	//	der naechste MouseMove auf die Filterbox ist wie ein ButtonDown

	nMouseStatus = SC_GM_FILTER;
	CaptureMouse();
}

sal_Bool ScGridWindow::HasScenarioRange( sal_uInt16 nCol, sal_Int32 nRow, ScRange& rScenRange )
{
	ScDocument* pDoc = pViewData->GetDocument();
	sal_uInt16 nTab = pViewData->GetTabNo();
	sal_uInt16 nTabCount = pDoc->GetTableCount();
	if ( nTab+1<nTabCount && pDoc->IsScenario(nTab+1) && !pDoc->IsScenario(nTab) )
	{
		sal_uInt16 i;
		ScMarkData aMarks;
		for (i=nTab+1; i<nTabCount && pDoc->IsScenario(i); i++)
			pDoc->MarkScenario( i, nTab, aMarks, sal_False, SC_SCENARIO_SHOWFRAME );
		ScRangeList aRanges;
		aMarks.FillRangeListWithMarks( &aRanges, sal_False );
		sal_uInt16 nRangeCount = (sal_uInt16)aRanges.Count();
		for (i=0; i<nRangeCount; i++)
		{
			ScRange aRange = *aRanges.GetObject(i);
			pDoc->ExtendTotalMerge( aRange );
			sal_Bool bTextBelow = ( aRange.aStart.Row() == 0 );
			sal_Bool bIsInScen = sal_False;
			if ( bTextBelow )
			{
				bIsInScen = (aRange.aStart.Col() == nCol && aRange.aEnd.Row() == nRow-1);
			}
			else
			{
				bIsInScen = (aRange.aStart.Col() == nCol && aRange.aStart.Row() == nRow+1);
			}
			if (bIsInScen)
			{
				rScenRange = aRange;
				return sal_True;
			}
		}
	}
	return sal_False;
}
void ScGridWindow::DoAutoFilterMenue( SCCOL nCol, SCROW nRow, sal_Bool bDataSelect )
{
	delete pFilterBox;
	delete pFilterFloat;

	sal_uInt16 i;
	ScDocument* pDoc = pViewData->GetDocument();
	SCTAB nTab = pViewData->GetTabNo();
	sal_Bool bLayoutRTL = pDoc->IsLayoutRTL( nTab );

	long nSizeX  = 0;
	long nSizeY  = 0;
	long nHeight = 0;
	pViewData->GetMergeSizePixel( nCol, nRow, nSizeX, nSizeY );
	// The button height should not use the merged cell height, should still use single row height
	nSizeY = pViewData->ToPixel(pDoc->GetRowHeight(nRow, nTab), pViewData->GetPPTY());
	Point aPos = pViewData->GetScrPos( nCol, nRow, eWhich );
	if ( bLayoutRTL )
		aPos.X() -= nSizeX;

	Rectangle aCellRect( OutputToScreenPixel(aPos), Size(nSizeX,nSizeY) );

	aPos.X() -= 1;
	aPos.Y() += nSizeY - 1;

	pFilterFloat = new ScFilterFloatingWindow( this, WinBits(WB_BORDER) );		// nicht resizable etc.
	pFilterFloat->SetPopupModeEndHdl( LINK( this, ScGridWindow, PopupModeEndHdl ) );
	pFilterBox = new ScFilterListBox(
		pFilterFloat, this, nCol, nRow, bDataSelect ? SC_FILTERBOX_DATASELECT : SC_FILTERBOX_FILTER );
	if ( bLayoutRTL )
		pFilterBox->EnableMirroring();

	nSizeX += 1;

	{
		Font 	aOldFont = GetFont(); SetFont( pFilterBox->GetFont() );
		MapMode aOldMode = GetMapMode(); SetMapMode( MAP_PIXEL );

		nHeight  = GetTextHeight();
		nHeight *= SC_FILTERLISTBOX_LINES;

		SetMapMode( aOldMode );
		SetFont( aOldFont );
	}

	//	SetSize spaeter
/*
	pFilterBox->SetSelectionMode( SINGLE_SELECTION );
	pFilterBox->SetTabs( nFilterBoxTabs, MapUnit( MAP_APPFONT ));
	pFilterBox->SetTabJustify( 1, bLayoutRTL ? AdjustRight : AdjustLeft );
*/

	sal_Bool bEmpty = sal_False;
	TypedScStrCollection aStrings( 128, 128 );
	if ( bDataSelect )									// Auswahl-Liste
	{
		//	Liste fuellen
		aStrings.SetCaseSensitive( sal_True );
		pDoc->GetDataEntries( nCol, nRow, nTab, aStrings );
		if ( aStrings.GetCount() == 0 )
			bEmpty = sal_True;
	}
	else												// AutoFilter
	{
		//!	wird der Titel ueberhaupt ausgewertet ???
		String aString;
		pDoc->GetString( nCol, nRow, nTab, aString );
		pFilterBox->SetText( aString );

		long nMaxText = 0;

		//	default entries
        static const sal_uInt16 nDefIDs[] = { SCSTR_ALLFILTER, SCSTR_TOP10FILTER, SCSTR_STDFILTER };
		const sal_uInt16 nDefCount = sizeof(nDefIDs) / sizeof(sal_uInt16);
		for (i=0; i<nDefCount; i++)
		{
			String aEntry( (ScResId) nDefIDs[i] );
			pFilterBox->InsertEntry( aEntry );
			long nTextWidth = pFilterBox->GetTextWidth( aEntry );
			if ( nTextWidth > nMaxText )
				nMaxText = nTextWidth;
		}
        pFilterBox->SetSeparatorPos( nDefCount - 1 );

		//	get list entries
        bool bHasDates = false;
        pDoc->GetFilterEntries( nCol, nRow, nTab, true, aStrings, bHasDates);
        pFilterBox->SetListHasDates(bHasDates);

		//	check widths of numerical entries (string entries are not included)
		//	so all numbers are completely visible
		sal_uInt16 nCount = aStrings.GetCount();
		for (i=0; i<nCount; i++)
		{
			TypedStrData* pData = aStrings[i];
			if ( !pData->IsStrData() )				// only numerical entries
			{
				long nTextWidth = pFilterBox->GetTextWidth( pData->GetString() );
				if ( nTextWidth > nMaxText )
					nMaxText = nTextWidth;
			}
		}

		//	add scrollbar width if needed (string entries are counted here)
		//	(scrollbar is shown if the box is exactly full?)
		if ( nCount + nDefCount >= SC_FILTERLISTBOX_LINES )
			nMaxText += GetSettings().GetStyleSettings().GetScrollBarSize();

		nMaxText += 4;				// for borders

		if ( nMaxText > nSizeX )
			nSizeX = nMaxText;		// just modify width - starting position is unchanged
	}

	if (!bEmpty)
	{
		//	Position und Groesse an Fenster anpassen
		//!	vorher Abfrage, ob die Eintraege hineinpassen (Breite)

		Size aParentSize = GetParent()->GetOutputSizePixel();
		Size aSize( nSizeX, nHeight );

		if ( aSize.Height() > aParentSize.Height() )
			aSize.Height() = aParentSize.Height();
		if ( aPos.Y() + aSize.Height() > aParentSize.Height() )
			aPos.Y() = aParentSize.Height() - aSize.Height();

		pFilterBox->SetSizePixel( aSize );
		pFilterBox->Show();					// Show muss vor SetUpdateMode kommen !!!
		pFilterBox->SetUpdateMode(sal_False);

		pFilterFloat->SetOutputSizePixel( aSize );
		pFilterFloat->StartPopupMode( aCellRect, FLOATWIN_POPUPMODE_DOWN|FLOATWIN_POPUPMODE_GRABFOCUS);

		//	Listbox fuellen
		sal_uInt16 nCount = aStrings.GetCount();
		sal_Bool bWait = ( nCount > 100 );

		if (bWait)
			EnterWait();

		for (i=0; i<nCount; i++)
			pFilterBox->InsertEntry( aStrings[i]->GetString() );

		if (bWait)
			LeaveWait();

		pFilterBox->SetUpdateMode(sal_True);
	}

//!	SvLBoxEntry* pSelect = NULL;
	sal_uInt16 nSelPos = LISTBOX_ENTRY_NOTFOUND;

	if (!bDataSelect)						// AutoFilter: aktiven Eintrag selektieren
	{
		ScDBData* pDBData = pDoc->GetDBAtCursor( nCol, nRow, nTab );
		if (pDBData)
		{
			ScQueryParam aParam;
			pDBData->GetQueryParam( aParam );		// kann nur MAXQUERY Eintraege ergeben

			sal_Bool bValid = sal_True;
			for (SCSIZE j=0; j<MAXQUERY && bValid; j++)			// bisherige Filter-Einstellungen
				if (aParam.GetEntry(j).bDoQuery)
				{
					//!			Abfrage mit DrawButtons zusammenfassen!

					ScQueryEntry& rEntry = aParam.GetEntry(j);
					if (j>0)
						if (rEntry.eConnect != SC_AND)
							bValid = sal_False;
					if (rEntry.nField == nCol)
					{
						if (rEntry.eOp == SC_EQUAL)
						{
							String* pStr = rEntry.pStr;
							if (pStr)
							{
								nSelPos = pFilterBox->GetEntryPos( *pStr );
//!								pSelect = pFilterBox->GetEntry( nPos );
							}
						}
						else if (rEntry.eOp == SC_TOPVAL && rEntry.pStr &&
									rEntry.pStr->EqualsAscii("10"))
							nSelPos = SC_AUTOFILTER_TOP10;
						else
							nSelPos = SC_AUTOFILTER_CUSTOM;
					}
				}

			if (!bValid)
				nSelPos = SC_AUTOFILTER_CUSTOM;
		}
	}
	else
	{

		sal_uLong nIndex = ((SfxUInt32Item*)pDoc->GetAttr(
								nCol, nRow, nTab, ATTR_VALIDDATA ))->GetValue();
		if ( nIndex )
		{
			const ScValidationData*	pData = pDoc->GetValidationEntry( nIndex );
			if (pData)
			{
				TypedStrData* pNew = NULL;
				String aDocStr;
				pDoc->GetString( nCol, nRow, nTab, aDocStr );
				if ( pDoc->HasValueData( nCol, nRow, nTab ) )
				{
					double fVal = pDoc->GetValue(ScAddress(nCol, nRow, nTab));
					pNew = new TypedStrData( aDocStr, fVal, SC_STRTYPE_VALUE );
				}
				else
					pNew = new TypedStrData( aDocStr, 0.0, SC_STRTYPE_STANDARD );

				bool bSortList = ( pData->GetListType() == ValidListType::SORTEDASCENDING);
				if ( bSortList )
				{
					sal_uInt16 nStrIndex;
					if (aStrings.Search(pNew,nStrIndex))
						nSelPos = nStrIndex;
				}
				else
				{
					sal_uInt16 nCount = aStrings.GetCount();
					for (i = 0; ((i < nCount) && ( LISTBOX_ENTRY_NOTFOUND == nSelPos)); i++)
					{
						if ( aStrings.Compare(aStrings[i], pNew)==0 ) 
							nSelPos = i;
					}
				}
				delete pNew;
			}
		}
	}

		//	neu (309): irgendwas muss immer selektiert sein:
	if ( LISTBOX_ENTRY_NOTFOUND == nSelPos && pFilterBox->GetEntryCount() > 0 && !bDataSelect)
		nSelPos = 0;

	//	keine leere Auswahl-Liste anzeigen:

	if ( bEmpty )
	{
		DELETEZ(pFilterBox);				// war nix
		DELETEZ(pFilterFloat);
		Sound::Beep();						// bemerkbar machen
	}
	else
	{
//		pFilterBox->Show();					// schon vorne
		pFilterBox->GrabFocus();

			//	Select erst nach GrabFocus, damit das Focus-Rechteck richtig landet
		if ( LISTBOX_ENTRY_NOTFOUND != nSelPos )
			pFilterBox->SelectEntryPos( nSelPos );
		else
		{
			if (bDataSelect)
				pFilterBox->SetNoSelection();
		}

		pFilterBox->EndInit();

		if (!bDataSelect)
		{
			// AutoFilter (aus MouseButtonDown):
			//	der naechste MouseMove auf die Filterbox ist wie ein ButtonDown

			nMouseStatus = SC_GM_FILTER;
			CaptureMouse();
		}
	}
}

void ScGridWindow::FilterSelect( sal_uLong nSel )
{
	String aString;
/*
	SvLBoxEntry* pEntry = pFilterBox->GetEntry( nSel );
	if (pEntry)
	{
		SvLBoxString* pStringEntry = (SvLBoxString*) pEntry->GetFirstItem( SV_ITEM_ID_LBOXSTRING );
		if ( pStringEntry )
			aString = pStringEntry->GetText();
	}
*/
	aString = pFilterBox->GetEntry( static_cast< sal_uInt16 >( nSel ) );

	SCCOL nCol = pFilterBox->GetCol();
	SCROW nRow = pFilterBox->GetRow();
	switch ( pFilterBox->GetMode() )
	{
		case SC_FILTERBOX_DATASELECT:
			ExecDataSelect( nCol, nRow, aString );
			break;
		case SC_FILTERBOX_FILTER:
            ExecFilter( nSel, nCol, nRow, aString, pFilterBox->HasDates() );
			break;
		case SC_FILTERBOX_SCENARIO:
			pViewData->GetView()->UseScenario( aString );
			break;
		case SC_FILTERBOX_PAGEFIELD:
			// first entry is "all"
			ExecPageFieldSelect( nCol, nRow, (nSel != 0), aString );
			break;
	}

	if (pFilterFloat)
		pFilterFloat->EndPopupMode();

	GrabFocus();		// unter OS/2 stimmt der Focus sonst nicht
}

void ScGridWindow::ExecDataSelect( SCCOL nCol, SCROW nRow, const String& rStr )
{
    if ( rStr.Len() )
    {
        SCTAB nTab = pViewData->GetTabNo();
        ScViewFunc* pView = pViewData->GetView();
        pView->EnterData( nCol, nRow, nTab, rStr );

        // #i52307# CellContentChanged is not in EnterData so it isn't called twice
        // if the cursor is moved afterwards.
        pView->CellContentChanged();
    }
}

void ScGridWindow::ExecFilter( sal_uLong nSel,
							   SCCOL nCol, SCROW nRow,
                               const String& aValue, bool bCheckForDates )
{
	SCTAB nTab = pViewData->GetTabNo();
	ScDocument* pDoc = pViewData->GetDocument();

	ScDBData* pDBData = pDoc->GetDBAtCursor( nCol, nRow, nTab );
	if (pDBData)
	{
		ScQueryParam aParam;
		pDBData->GetQueryParam( aParam );		// kann nur MAXQUERY Eintraege ergeben

		if (SC_AUTOFILTER_CUSTOM == nSel)
		{
			SCTAB nAreaTab;
			SCCOL nStartCol;
			SCROW nStartRow;
			SCCOL nEndCol;
			SCROW nEndRow;
			pDBData->GetArea( nAreaTab, nStartCol,nStartRow,nEndCol,nEndRow );
			pViewData->GetView()->MarkRange( ScRange( nStartCol,nStartRow,nAreaTab,nEndCol,nEndRow,nAreaTab));
			pViewData->GetView()->SetCursor(nCol,nRow);		//! auch ueber Slot ??
			pViewData->GetDispatcher().Execute( SID_FILTER, SFX_CALLMODE_SLOT | SFX_CALLMODE_RECORD );
		}
		else
		{
			sal_Bool bDeleteOld = sal_False;
			SCSIZE nQueryPos = 0;
			sal_Bool bFound = sal_False;
			if (!aParam.bInplace)
				bDeleteOld = sal_True;
			if (aParam.bRegExp)
				bDeleteOld = sal_True;
			for (SCSIZE i=0; i<MAXQUERY && !bDeleteOld; i++)	// bisherige Filter-Einstellungen
				if (aParam.GetEntry(i).bDoQuery)
				{
					//!			Abfrage mit DrawButtons zusammenfassen!

					ScQueryEntry& rEntry = aParam.GetEntry(i);
					if (i>0)
						if (rEntry.eConnect != SC_AND)
							bDeleteOld = sal_True;

					if (rEntry.nField == nCol)
					{
						if (bFound)							// diese Spalte zweimal?
							bDeleteOld = sal_True;
						nQueryPos = i;
						bFound = sal_True;
					}
					if (!bFound)
						nQueryPos = i + 1;
				}

			if (bDeleteOld)
			{
				SCSIZE nEC = aParam.GetEntryCount();
				for (SCSIZE i=0; i<nEC; i++)
                    aParam.GetEntry(i).Clear();
				nQueryPos = 0;
				aParam.bInplace = sal_True;
				aParam.bRegExp = sal_False;
			}

			if ( nQueryPos < MAXQUERY || SC_AUTOFILTER_ALL == nSel )	// loeschen geht immer
			{
				if (nSel)
				{
					ScQueryEntry& rNewEntry = aParam.GetEntry(nQueryPos);

					rNewEntry.bDoQuery		 = sal_True;
					rNewEntry.bQueryByString = sal_True;
					rNewEntry.nField		 = nCol;
                    rNewEntry.bQueryByDate   = bCheckForDates;
					if ( nSel == SC_AUTOFILTER_TOP10 )
					{
						rNewEntry.eOp	= SC_TOPVAL;
						*rNewEntry.pStr	= String::CreateFromAscii(RTL_CONSTASCII_STRINGPARAM("10"));
					}
					else
					{
						rNewEntry.eOp	= SC_EQUAL;
						*rNewEntry.pStr	= aValue;
					}
					if (nQueryPos > 0)
						rNewEntry.eConnect   = SC_AND;
				}
				else
				{
					if (bFound)
						aParam.DeleteQuery(nQueryPos);
				}

				//	#100597# end edit mode - like in ScCellShell::ExecuteDB
				if ( pViewData->HasEditView( pViewData->GetActivePart() ) )
				{
					SC_MOD()->InputEnterHandler();
					pViewData->GetViewShell()->UpdateInputHandler();
				}

				pViewData->GetView()->Query( aParam, NULL, sal_True );
				pDBData->SetQueryParam( aParam );							// speichern
			}
			else					//	"Zuviele Bedingungen"
				pViewData->GetView()->ErrorMessage( STR_FILTER_TOOMANY );
		}
	}
	else
	{
		DBG_ERROR("Wo ist der Datenbankbereich?");
	}
}

void ScGridWindow::SetPointer( const Pointer& rPointer )
{
	nCurrentPointer = 0;
	Window::SetPointer( rPointer );
}

void ScGridWindow::MoveMouseStatus( ScGridWindow& rDestWin )
{
	if (nButtonDown)
	{
		rDestWin.nButtonDown = nButtonDown;
		rDestWin.nMouseStatus = nMouseStatus;
	}

	if (bRFMouse)
	{
		rDestWin.bRFMouse = bRFMouse;
		rDestWin.bRFSize  = bRFSize;
		rDestWin.nRFIndex = nRFIndex;
		rDestWin.nRFAddX  = nRFAddX;
		rDestWin.nRFAddY  = nRFAddY;
		bRFMouse = sal_False;
	}

	if (nPagebreakMouse)
	{
		rDestWin.nPagebreakMouse  = nPagebreakMouse;
		rDestWin.nPagebreakBreak  = nPagebreakBreak;
		rDestWin.nPagebreakPrev   = nPagebreakPrev;
		rDestWin.aPagebreakSource = aPagebreakSource;
		rDestWin.aPagebreakDrag   = aPagebreakDrag;
		nPagebreakMouse = SC_PD_NONE;
	}
}

sal_Bool ScGridWindow::TestMouse( const MouseEvent& rMEvt, sal_Bool bAction )
{
	//	MouseEvent buttons must only be checked if bAction==TRUE
	//	to allow changing the mouse pointer in MouseMove,
	//	but not start AutoFill with right button (#74229#).
	//	with bAction==sal_True, SetFillMode / SetDragMode is called

	if ( bAction && !rMEvt.IsLeft() )
		return sal_False;

	sal_Bool bNewPointer = sal_False;

	SfxInPlaceClient* pClient = pViewData->GetViewShell()->GetIPClient();
    sal_Bool bOleActive = ( pClient && pClient->IsObjectInPlaceActive() );

	if ( pViewData->IsActive() && !bOleActive )
	{
		ScDocument* pDoc = pViewData->GetDocument();
		SCTAB nTab = pViewData->GetTabNo();
		sal_Bool bLayoutRTL = pDoc->IsLayoutRTL( nTab );

		//	Auto-Fill

		ScRange aMarkRange;
		if (pViewData->GetSimpleArea( aMarkRange ) == SC_MARK_SIMPLE)
		{
            if (aMarkRange.aStart.Tab() == pViewData->GetTabNo() && mpAutoFillRect)
			{
				Point aMousePos = rMEvt.GetPosPixel();
                if (mpAutoFillRect->IsInside(aMousePos))
				{
                    SetPointer( Pointer( POINTER_CROSS ) );     //! dickeres Kreuz ?
					if (bAction)
					{
                        SCCOL nX = aMarkRange.aEnd.Col();
                        SCROW nY = aMarkRange.aEnd.Row();

						if ( lcl_IsEditableMatrix( pViewData->GetDocument(), aMarkRange ) )
							pViewData->SetDragMode(
								aMarkRange.aStart.Col(), aMarkRange.aStart.Row(), nX, nY, SC_FILL_MATRIX );
						else
							pViewData->SetFillMode(
								aMarkRange.aStart.Col(), aMarkRange.aStart.Row(), nX, nY );

						//	#108266# The simple selection must also be recognized when dragging,
						//	where the Marking flag is set and MarkToSimple won't work anymore.
						pViewData->GetMarkData().MarkToSimple();
					}
					bNewPointer = sal_True;
				}
			}
		}

		//	Embedded-Rechteck

		if (pDoc->IsEmbedded())
		{
            ScRange aRange;
			pDoc->GetEmbedded( aRange );
			if ( pViewData->GetTabNo() == aRange.aStart.Tab() )
			{
				Point aStartPos = pViewData->GetScrPos( aRange.aStart.Col(), aRange.aStart.Row(), eWhich );
				Point aEndPos   = pViewData->GetScrPos( aRange.aEnd.Col()+1, aRange.aEnd.Row()+1, eWhich );
				Point aMousePos = rMEvt.GetPosPixel();
				if ( bLayoutRTL )
				{
					aStartPos.X() += 2;
					aEndPos.X()   += 2;
				}
				sal_Bool bTop = ( aMousePos.X() >= aStartPos.X()-3 && aMousePos.X() <= aStartPos.X()+1 &&
							  aMousePos.Y() >= aStartPos.Y()-3 && aMousePos.Y() <= aStartPos.Y()+1 );
				sal_Bool bBottom = ( aMousePos.X() >= aEndPos.X()-3 && aMousePos.X() <= aEndPos.X()+1 &&
								 aMousePos.Y() >= aEndPos.Y()-3 && aMousePos.Y() <= aEndPos.Y()+1 );
				if ( bTop || bBottom )
				{
					SetPointer( Pointer( POINTER_CROSS ) );
					if (bAction)
					{
						sal_uInt8 nMode = bTop ? SC_FILL_EMBED_LT : SC_FILL_EMBED_RB;
						pViewData->SetDragMode(
									aRange.aStart.Col(), aRange.aStart.Row(),
									aRange.aEnd.Col(), aRange.aEnd.Row(), nMode );
					}
					bNewPointer = sal_True;
				}
			}
		}
	}

	if (!bNewPointer && bAction)
	{
//		SetPointer( POINTER_ARROW );			// in Fu...
		pViewData->ResetFillMode();
	}

	return bNewPointer;
}

void __EXPORT ScGridWindow::MouseButtonDown( const MouseEvent& rMEvt )
{
    nNestedButtonState = SC_NESTEDBUTTON_DOWN;

    HandleMouseButtonDown( rMEvt );

    if ( nNestedButtonState == SC_NESTEDBUTTON_UP )
    {
        // #i41690# If an object is deactivated from MouseButtonDown, it might reschedule,
        // so MouseButtonUp comes before the MouseButtonDown call is finished. In this case,
        // simulate another MouseButtonUp call, so the selection state is consistent.

        nButtonDown = rMEvt.GetButtons();
        FakeButtonUp();

        if ( IsTracking() )
            EndTracking();      // normally done in VCL as part of MouseButtonUp handling
    }
    nNestedButtonState = SC_NESTEDBUTTON_NONE;
}

void ScGridWindow::HandleMouseButtonDown( const MouseEvent& rMEvt )
{
    // We have to check if a context menu is shown and we have an UI
    // active inplace client. In that case we have to ignore the event.
    // Otherwise we would crash (context menu has been
    // opened by inplace client and we would deactivate the inplace client,
    // the contex menu is closed by VCL asynchronously which in the end
    // would work on deleted objects or the context menu has no parent anymore)
    // See #126086# and #128122#
	SfxViewShell* pViewSh = pViewData->GetViewShell();
	SfxInPlaceClient* pClient = pViewSh->GetIPClient();
    if ( pClient &&
         pClient->IsObjectInPlaceActive() &&
         PopupMenu::IsInExecute() )
        return;

    aCurMousePos = rMEvt.GetPosPixel();

	//	Filter-Popup beendet sich mit eigenem Mausklick, nicht erst beim Klick
	//	in das GridWindow, darum ist die folgende Abfrage nicht mehr noetig:
#if 0
	// merken, dass FilterBox geloescht wird, damit sichergestellt
	// ist, dass in diesem Handler nicht an gleicher Stelle wieder
	// eine neue geoeffnet wird.
	sal_Bool	bWasFilterBox = ( pFilterBox != NULL &&
								((Window*)pFilterBox)->IsVisible() &&
								!pFilterBox->IsDataSelect() );
	SCCOL	nOldColFBox	  = bWasFilterBox ? pFilterBox->GetCol() : 0;
	SCROW  nOldRowFBox	  = bWasFilterBox ? pFilterBox->GetRow() : 0;
#endif

	ClickExtern();	// loescht FilterBox, wenn vorhanden

	HideNoteMarker();	// Notiz-Anzeige

	bEEMouse = sal_False;

	ScModule* pScMod = SC_MOD();
	if (pScMod->IsModalMode(pViewData->GetSfxDocShell()))
	{
		Sound::Beep();
		return;
	}

	pScActiveViewShell = pViewData->GetViewShell();			// falls auf Link geklickt wird
	nScClickMouseModifier = rMEvt.GetModifier();			// um Control-Klick immer zu erkennen

	sal_Bool bDetective = pViewData->GetViewShell()->IsAuditShell();
	sal_Bool bRefMode =	pViewData->IsRefMode();					// Referenz angefangen
	sal_Bool bFormulaMode = pScMod->IsFormulaMode();			// naechster Klick -> Referenz
	sal_Bool bEditMode = pViewData->HasEditView(eWhich);		// auch bei Mode==SC_INPUT_TYPE
    sal_Bool bDouble = (rMEvt.GetClicks() == 2);

	//	DeactivateIP passiert nur noch bei MarkListHasChanged

	//	im GrabFocus Aufruf kann eine Fehlermeldung hochkommen
	//	(z.B. beim Umbenennen von Tabellen per Tab-Reiter)

    if ( !nButtonDown || !bDouble )             // single (first) click is always valid
        nButtonDown = rMEvt.GetButtons();       // set nButtonDown first, so StopMarking works

//	pViewData->GetViewShell()->GetViewFrame()->GetWindow().GrabFocus();
	if ( ( bEditMode && pViewData->GetActivePart() == eWhich ) || !bFormulaMode )
		GrabFocus();

    // #i31846# need to cancel a double click if the first click has set the "ignore" state,
    // but a single (first) click is always valid
    if ( nMouseStatus == SC_GM_IGNORE && bDouble )
	{
		nButtonDown = 0;
		nMouseStatus = SC_GM_NONE;
		return;
	}

	if ( bDetective )				// Detektiv-Fuell-Modus
	{
		if ( rMEvt.IsLeft() && !rMEvt.GetModifier() )
		{
			Point	aPos = rMEvt.GetPosPixel();
			SCsCOL	nPosX;
			SCsROW	nPosY;
			pViewData->GetPosFromPixel( aPos.X(), aPos.Y(), eWhich, nPosX, nPosY );

			SfxInt16Item aPosXItem( SID_RANGE_COL, nPosX );
			SfxInt32Item aPosYItem( SID_RANGE_ROW, nPosY );
			pViewData->GetDispatcher().Execute( SID_FILL_SELECT, SFX_CALLMODE_SLOT | SFX_CALLMODE_RECORD,
										&aPosXItem, &aPosYItem, (void*)0L );

		}
		nButtonDown = 0;
		nMouseStatus = SC_GM_NONE;
		return;
	}

	if (!bDouble)
		nMouseStatus = SC_GM_NONE;

	if (!bFormulaMode)
	{
		if ( pViewData->GetActivePart() != eWhich )
			pViewData->GetView()->ActivatePart( eWhich );
	}
	else
	{
		ScViewSelectionEngine* pSelEng = pViewData->GetView()->GetSelEngine();
		pSelEng->SetWindow(this);
		pSelEng->SetWhich(eWhich);
		pSelEng->SetVisibleArea( Rectangle(Point(), GetOutputSizePixel()) );
	}

	if (bEditMode && (pViewData->GetRefTabNo() == pViewData->GetTabNo()))
	{
		Point	aPos = rMEvt.GetPosPixel();
		SCsCOL	nPosX;
		SCsROW	nPosY;
		pViewData->GetPosFromPixel( aPos.X(), aPos.Y(), eWhich, nPosX, nPosY );

		EditView*	pEditView;
		SCCOL		nEditCol;
		SCROW		nEditRow;
		pViewData->GetEditView( eWhich, pEditView, nEditCol, nEditRow );
		SCCOL nEndCol = pViewData->GetEditEndCol();
		SCROW nEndRow = pViewData->GetEditEndRow();

		if ( nPosX >= (SCsCOL) nEditCol && nPosX <= (SCsCOL) nEndCol &&
			 nPosY >= (SCsROW) nEditRow && nPosY <= (SCsROW) nEndRow )
		{
			//	#53966# beim Klick in die Tabellen-EditView immer den Focus umsetzen
			if (bFormulaMode)	// sonst ist es oben schon passiert
				GrabFocus();

			pScMod->SetInputMode( SC_INPUT_TABLE );
			bEEMouse = sal_True;
			bEditMode = pEditView->MouseButtonDown( rMEvt );
			return;
		}
	}

	if (pScMod->GetIsWaterCan())
	{
		//!		was is mit'm Mac ???
		if ( rMEvt.GetModifier() + rMEvt.GetButtons() == MOUSE_RIGHT )
		{
			nMouseStatus = SC_GM_WATERUNDO;
			return;
		}
	}

	// Reihenfolge passend zum angezeigten Cursor:
	//	RangeFinder, AutoFill, PageBreak, Drawing

	if ( HitRangeFinder( rMEvt.GetPosPixel(), bRFSize, &nRFIndex, &nRFAddX, &nRFAddY ) )
	{
		bRFMouse = sal_True;		// die anderen Variablen sind oben initialisiert

		if ( pViewData->GetActivePart() != eWhich )
			pViewData->GetView()->ActivatePart( eWhich );	//! schon oben immer ???

		// CaptureMouse();
		StartTracking();
		return;
	}

	sal_Bool bCrossPointer = TestMouse( rMEvt, sal_True );
	if ( bCrossPointer )
	{
		if ( bDouble )
			pViewData->GetView()->FillCrossDblClick();
		else
		pScMod->InputEnterHandler();								// Autofill etc.
	}

	if ( !bCrossPointer )
	{
		nPagebreakMouse = HitPageBreak( rMEvt.GetPosPixel(), &aPagebreakSource,
											&nPagebreakBreak, &nPagebreakPrev );
		if (nPagebreakMouse)
		{
			bPagebreakDrawn = sal_False;
			// CaptureMouse();
			StartTracking();
			PagebreakMove( rMEvt, sal_False );
			return;
		}
	}

	if (!bFormulaMode && !bEditMode && rMEvt.IsLeft())
	{
		if ( !bCrossPointer && DrawMouseButtonDown(rMEvt) )
		{
			//if (DrawHasMarkedObj())
			//	pViewData->GetViewShell()->SetDrawShellOrSub();		// Draw-Objekt selektiert
			return;
		}

		pViewData->GetViewShell()->SetDrawShell( sal_False );				// kein Draw-Objekt selektiert

		//	TestMouse schon oben passiert
	}

	Point aPos = rMEvt.GetPosPixel();
	SCsCOL nPosX;
	SCsROW nPosY;
	pViewData->GetPosFromPixel( aPos.X(), aPos.Y(), eWhich, nPosX, nPosY );
	SCTAB nTab = pViewData->GetTabNo();
	ScDocument* pDoc = pViewData->GetDocument();


            //
            //      AutoFilter buttons
            //

    if ( !bDouble && !bFormulaMode && rMEvt.IsLeft() )
	{
		SCsCOL nRealPosX;
		SCsROW nRealPosY;
		pViewData->GetPosFromPixel( aPos.X(), aPos.Y(), eWhich, nRealPosX, nRealPosY, false );//the real row/col
		ScMergeFlagAttr* pRealPosAttr = (ScMergeFlagAttr*)
									pDoc->GetAttr( nRealPosX, nRealPosY, nTab, ATTR_MERGE_FLAG );
		ScMergeFlagAttr* pAttr = (ScMergeFlagAttr*)
									pDoc->GetAttr( nPosX, nPosY, nTab, ATTR_MERGE_FLAG );
		if( pRealPosAttr->HasAutoFilter() )
		{
			SC_MOD()->InputEnterHandler();	
			if (DoAutoFilterButton( nRealPosX, nRealPosY, rMEvt))
                return;
		}
		if( pAttr->HasAutoFilter() )
		{
			SC_MOD()->InputEnterHandler();	//Add for i85305
			if (DoAutoFilterButton( nPosX, nPosY, rMEvt))
                return;
		}
		if (pAttr->HasButton())
		{
			DoPushButton( nPosX, nPosY, rMEvt );	// setzt evtl. bPivotMouse / bDPMouse
			return;
		}

        //  List Validity drop-down button

        if ( bListValButton )
        {
            Rectangle aButtonRect = GetListValButtonRect( aListValPos );
            if ( aButtonRect.IsInside( aPos ) )
            {
                DoAutoFilterMenue( aListValPos.Col(), aListValPos.Row(), sal_True );

                nMouseStatus = SC_GM_FILTER;    // not set in DoAutoFilterMenue for bDataSelect
                CaptureMouse();
                return;
            }
        }
	}

            //
            //      scenario selection
            //

	ScRange aScenRange;
	if ( rMEvt.IsLeft() && HasScenarioButton( aPos, aScenRange ) )
	{
		DoScenarioMenue( aScenRange );
		return;
	}

			//
			//		Doppelklick angefangen ?
			//

	// StopMarking kann aus DrawMouseButtonDown gerufen werden

	if ( nMouseStatus != SC_GM_IGNORE && !bRefMode )
	{
		if ( bDouble && !bCrossPointer )
		{
			if (nMouseStatus == SC_GM_TABDOWN)
				nMouseStatus = SC_GM_DBLDOWN;
		}
		else
			nMouseStatus = SC_GM_TABDOWN;
	}

			//
			//		Links in Edit-Zellen
			//

	sal_Bool bAlt = rMEvt.IsMod2();
	if ( !bAlt && rMEvt.IsLeft() &&
			GetEditUrl(rMEvt.GetPosPixel()) )			// Klick auf Link: Cursor nicht bewegen
	{
		SetPointer( Pointer( POINTER_REFHAND ) );
		nMouseStatus = SC_GM_URLDOWN;					// auch nur dann beim ButtonUp ausfuehren
		return;
	}

			//
			//		Gridwin - SelectionEngine
			//

	if ( rMEvt.IsLeft() )
	{
		ScViewSelectionEngine* pSelEng = pViewData->GetView()->GetSelEngine();
		pSelEng->SetWindow(this);
		pSelEng->SetWhich(eWhich);
		pSelEng->SetVisibleArea( Rectangle(Point(), GetOutputSizePixel()) );

		//	SelMouseButtonDown an der View setzt noch das bMoveIsShift Flag
		if ( pViewData->GetView()->SelMouseButtonDown( rMEvt ) )
		{
			if (IsMouseCaptured())
			{
				//	Tracking statt CaptureMouse, damit sauber abgebrochen werden kann
				//!	Irgendwann sollte die SelectionEngine selber StartTracking rufen!?!
				ReleaseMouse();
				StartTracking();
			}
			pViewData->GetMarkData().SetMarking(sal_True);
			return;
		}
	}
}

void __EXPORT ScGridWindow::MouseButtonUp( const MouseEvent& rMEvt )
{
	aCurMousePos = rMEvt.GetPosPixel();
	ScDocument* pDoc = pViewData->GetDocument();
	ScMarkData& rMark = pViewData->GetMarkData();

    // #i41690# detect a MouseButtonUp call from within MouseButtonDown
    // (possible through Reschedule from storing an OLE object that is deselected)

    if ( nNestedButtonState == SC_NESTEDBUTTON_DOWN )
        nNestedButtonState = SC_NESTEDBUTTON_UP;

	if (nButtonDown != rMEvt.GetButtons())
		nMouseStatus = SC_GM_IGNORE;			// reset und return

	nButtonDown = 0;

	if (nMouseStatus == SC_GM_IGNORE)
	{
		nMouseStatus = SC_GM_NONE;
										// Selection-Engine: Markieren abbrechen
		pViewData->GetView()->GetSelEngine()->Reset();
		rMark.SetMarking(sal_False);
		if (pViewData->IsAnyFillMode())
		{
			pViewData->GetView()->StopRefMode();
			pViewData->ResetFillMode();
		}
		StopMarking();
		DrawEndAction();				// Markieren/Verschieben auf Drawing-Layer abbrechen
		ReleaseMouse();
		return;
	}

	if (nMouseStatus == SC_GM_FILTER)
	{
		if ( pFilterBox && pFilterBox->GetMode() == SC_FILTERBOX_FILTER )
		{
            if (mpFilterButton.get())
            {
                bool bFilterActive = IsAutoFilterActive(
                    pFilterBox->GetCol(), pFilterBox->GetRow(), pViewData->GetTabNo() );

                mpFilterButton->setHasHiddenMember(bFilterActive);
                mpFilterButton->setPopupPressed(false);
                HideCursor();
                mpFilterButton->draw();
                ShowCursor();
            }
		}
		nMouseStatus = SC_GM_NONE;
		ReleaseMouse();
		return;							// da muss nix mehr passieren
	}

	ScModule* pScMod = SC_MOD();
	if (pScMod->IsModalMode(pViewData->GetSfxDocShell()))
		return;

	SfxBindings& rBindings = pViewData->GetBindings();
    if (bEEMouse && pViewData->HasEditView( eWhich ))
	{
		EditView*	pEditView;
		SCCOL		nEditCol;
		SCROW		nEditRow;
		pViewData->GetEditView( eWhich, pEditView, nEditCol, nEditRow );
		pEditView->MouseButtonUp( rMEvt );

		if ( rMEvt.IsMiddle() &&
	         	GetSettings().GetMouseSettings().GetMiddleButtonAction() == MOUSE_MIDDLE_PASTESELECTION )
	    {
	    	//	EditView may have pasted from selection
	    	pScMod->InputChanged( pEditView );
	    }
		else
			pScMod->InputSelection( pEditView );			// parentheses etc.

		pViewData->GetView()->InvalidateAttribs();
		rBindings.Invalidate( SID_HYPERLINK_GETLINK );
		bEEMouse = sal_False;
		return;
	}

	if (bDPMouse)
	{
		DPMouseButtonUp( rMEvt );		// resets bDPMouse
		return;
	}

	if (bRFMouse)
	{
		RFMouseMove( rMEvt, sal_True );		// Range wieder richtigherum
		bRFMouse = sal_False;
		SetPointer( Pointer( POINTER_ARROW ) );
		ReleaseMouse();
		return;
	}

	if (nPagebreakMouse)
	{
		PagebreakMove( rMEvt, sal_True );
		nPagebreakMouse = SC_PD_NONE;
		SetPointer( Pointer( POINTER_ARROW ) );
		ReleaseMouse();
		return;
	}

	if (nMouseStatus == SC_GM_WATERUNDO)	// Undo im Giesskannenmodus
	{
		::svl::IUndoManager* pMgr = pViewData->GetDocShell()->GetUndoManager();
		if ( pMgr->GetUndoActionCount() && pMgr->GetUndoActionId() == STR_UNDO_APPLYCELLSTYLE )
			pMgr->Undo();
		else
			Sound::Beep();
		return;
	}

	if (DrawMouseButtonUp(rMEvt))       // includes format paint brush handling for drawing objects
    {
        ScTabViewShell* pViewShell = pViewData->GetViewShell();
        SfxBindings& rBindings=pViewShell->GetViewFrame()->GetBindings();
        rBindings.Invalidate(SID_ATTR_TRANSFORM_WIDTH);
        rBindings.Invalidate(SID_ATTR_TRANSFORM_HEIGHT);
        rBindings.Invalidate(SID_ATTR_TRANSFORM_POS_X);
        rBindings.Invalidate(SID_ATTR_TRANSFORM_POS_Y);
        rBindings.Invalidate(SID_ATTR_TRANSFORM_ANGLE);
        rBindings.Invalidate(SID_ATTR_TRANSFORM_ROT_X);
        rBindings.Invalidate(SID_ATTR_TRANSFORM_ROT_Y);
        rBindings.Invalidate(SID_ATTR_TRANSFORM_AUTOWIDTH);
        rBindings.Invalidate(SID_ATTR_TRANSFORM_AUTOHEIGHT);
        return;
    }

	rMark.SetMarking(sal_False);

	SetPointer( Pointer( POINTER_ARROW ) );

	if (pViewData->IsFillMode() ||
		( pViewData->GetFillMode() == SC_FILL_MATRIX && rMEvt.IsMod1() ))
	{
		nScFillModeMouseModifier = rMEvt.GetModifier();
		SCCOL nStartCol;
		SCROW nStartRow;
		SCCOL nEndCol;
		SCROW nEndRow;
		pViewData->GetFillData( nStartCol, nStartRow, nEndCol, nEndRow );
//		DBG_ASSERT( nStartCol==pViewData->GetRefStartX() && nStartRow==pViewData->GetRefStartY(),
//								"Block falsch fuer AutoFill" );
		ScRange aDelRange;
		sal_Bool bIsDel = pViewData->GetDelMark( aDelRange );

		ScViewFunc* pView = pViewData->GetView();
		pView->StopRefMode();
		pViewData->ResetFillMode();
		pView->GetFunctionSet()->SetAnchorFlag( sal_False );	// #i5819# don't use AutoFill anchor flag for selection

		if ( bIsDel )
		{
			pView->MarkRange( aDelRange, sal_False );
			pView->DeleteContents( IDF_CONTENTS );
			SCTAB nTab = pViewData->GetTabNo();
			ScRange aBlockRange( nStartCol, nStartRow, nTab, nEndCol, nEndRow, nTab );
			if ( aBlockRange != aDelRange )
			{
				if ( aDelRange.aStart.Row() == nStartRow )
					aBlockRange.aEnd.SetCol( aDelRange.aStart.Col() - 1 );
				else
					aBlockRange.aEnd.SetRow( aDelRange.aStart.Row() - 1 );
				pView->MarkRange( aBlockRange, sal_False );
			}
		}
		else
			pViewData->GetDispatcher().Execute( FID_FILL_AUTO, SFX_CALLMODE_SLOT | SFX_CALLMODE_RECORD );
	}
	else if (pViewData->GetFillMode() == SC_FILL_MATRIX)
	{
		SCTAB nTab = pViewData->GetTabNo();
		SCCOL nStartCol;
		SCROW nStartRow;
		SCCOL nEndCol;
		SCROW nEndRow;
		pViewData->GetFillData( nStartCol, nStartRow, nEndCol, nEndRow );
		ScRange aBlockRange( nStartCol, nStartRow, nTab, nEndCol, nEndRow, nTab );
		SCCOL nFillCol = pViewData->GetRefEndX();
		SCROW nFillRow = pViewData->GetRefEndY();
		ScAddress aEndPos( nFillCol, nFillRow, nTab );

		ScTabView* pView = pViewData->GetView();
		pView->StopRefMode();
		pViewData->ResetFillMode();
		pView->GetFunctionSet()->SetAnchorFlag( sal_False );

		if ( aEndPos != aBlockRange.aEnd )
		{
			pViewData->GetDocShell()->GetDocFunc().ResizeMatrix( aBlockRange, aEndPos, sal_False );
			pViewData->GetView()->MarkRange( ScRange( aBlockRange.aStart, aEndPos ) );
		}
	}
	else if (pViewData->IsAnyFillMode())
	{
												// Embedded-Area has been changed
		ScTabView* pView = pViewData->GetView();
		pView->StopRefMode();
		pViewData->ResetFillMode();
		pView->GetFunctionSet()->SetAnchorFlag( sal_False );
		pViewData->GetDocShell()->UpdateOle(pViewData);
	}

	sal_Bool bRefMode =	pViewData->IsRefMode();
	if (bRefMode)
		pScMod->EndReference();

		//
		//	Giesskannen-Modus (Gestalter)
		//

	if (pScMod->GetIsWaterCan())
	{
		//	Abfrage auf Undo schon oben

		ScStyleSheetPool* pStylePool = (ScStyleSheetPool*)
									   (pViewData->GetDocument()->
											GetStyleSheetPool());
		if ( pStylePool )
		{
			SfxStyleSheet* pStyleSheet = (SfxStyleSheet*)
										 pStylePool->GetActualStyleSheet();

			if ( pStyleSheet )
			{
				SfxStyleFamily eFamily = pStyleSheet->GetFamily();

				switch ( eFamily )
				{
					case SFX_STYLE_FAMILY_PARA:
						pViewData->GetView()->SetStyleSheetToMarked( pStyleSheet );
						pViewData->GetView()->DoneBlockMode();
						break;

					case SFX_STYLE_FAMILY_PAGE:
						pViewData->GetDocument()->SetPageStyle( pViewData->GetTabNo(),
																pStyleSheet->GetName() );

						ScPrintFunc( pViewData->GetDocShell(),
									 pViewData->GetViewShell()->GetPrinter(sal_True),
									 pViewData->GetTabNo() ).UpdatePages();

						rBindings.Invalidate( SID_STATUS_PAGESTYLE );
						break;

					default:
						break;
				}
			}
		}
	}

    ScDBFunc* pView = pViewData->GetView();
    ScDocument* pBrushDoc = pView->GetBrushDocument();
    if ( pBrushDoc )
    {
        pView->PasteFromClip( IDF_ATTRIB, pBrushDoc );
        if ( !pView->IsPaintBrushLocked() )
            pView->ResetBrushDocument();            // invalidates pBrushDoc pointer
    }

			//
			//		double click (only left button)
			//

	sal_Bool bDouble = ( rMEvt.GetClicks() == 2 && rMEvt.IsLeft() );
	if ( bDouble && !bRefMode && nMouseStatus == SC_GM_DBLDOWN && !pScMod->IsRefDialogOpen() )
	{
		//	data pilot table
		Point aPos = rMEvt.GetPosPixel();
        SCsCOL nPosX;
        SCsROW nPosY;
        SCTAB nTab = pViewData->GetTabNo();
        pViewData->GetPosFromPixel( aPos.X(), aPos.Y(), eWhich, nPosX, nPosY );
		ScDPObject*	pDPObj	= pDoc->GetDPAtCursor( nPosX, nPosY, nTab );
		if ( pDPObj && pDPObj->GetSaveData()->GetDrillDown() )
		{
			ScAddress aCellPos( nPosX, nPosY, pViewData->GetTabNo() );

            // Check for header drill-down first.
            sheet::DataPilotTableHeaderData aData;
            pDPObj->GetHeaderPositionData(aCellPos, aData);

            if ( ( aData.Flags & sheet::MemberResultFlags::HASMEMBER ) &&
                 ! ( aData.Flags & sheet::MemberResultFlags::SUBTOTAL ) )
			{
                sal_uInt16 nDummy;
                if ( pView->HasSelectionForDrillDown( nDummy ) )
                {
                    // execute slot to show dialog
                    pViewData->GetDispatcher().Execute( SID_OUTLINE_SHOW, SFX_CALLMODE_SLOT | SFX_CALLMODE_RECORD );
                }
                else
                {
                    // toggle single entry
                    ScDPObject aNewObj( *pDPObj );
                    pDPObj->ToggleDetails( aData, &aNewObj );
                    ScDBDocFunc aFunc( *pViewData->GetDocShell() );
                    aFunc.DataPilotUpdate( pDPObj, &aNewObj, sal_True, sal_False );
                    pViewData->GetView()->CursorPosChanged();       // shells may be switched
                }
			}
			else
            {
                // Check if the data area is double-clicked.

                Sequence<sheet::DataPilotFieldFilter> aFilters;
                if ( pDPObj->GetDataFieldPositionData(aCellPos, aFilters) )
                    pViewData->GetView()->ShowDataPilotSourceData( *pDPObj, aFilters );
                else
                    Sound::Beep();  // nothing to expand/collapse/show
            }

			return;
		}

        // Check for cell protection attribute.
        ScTableProtection* pProtect = pDoc->GetTabProtection( nTab );
        bool bEditAllowed = true;
        if ( pProtect && pProtect->isProtected() )
        {
            bool bCellProtected = pDoc->HasAttrib(nPosX, nPosY, nTab, nPosX, nPosY, nTab, HASATTR_PROTECTED);
            bool bSkipProtected = !pProtect->isOptionEnabled(ScTableProtection::SELECT_LOCKED_CELLS);
            bool bSkipUnprotected = !pProtect->isOptionEnabled(ScTableProtection::SELECT_UNLOCKED_CELLS);

            if ( bSkipProtected && bSkipUnprotected )
                bEditAllowed = false;
            else if ( (bCellProtected && bSkipProtected) || (!bCellProtected && bSkipUnprotected) )
                bEditAllowed = false;
        }

        if ( bEditAllowed )
        {
            //  edit cell contents
            pViewData->GetViewShell()->UpdateInputHandler();
            pScMod->SetInputMode( SC_INPUT_TABLE );
            if (pViewData->HasEditView(eWhich))
            {
                //  Text-Cursor gleich an die geklickte Stelle setzen
                EditView* pEditView = pViewData->GetEditView( eWhich );
                MouseEvent aEditEvt( rMEvt.GetPosPixel(), 1, MOUSE_SYNTHETIC, MOUSE_LEFT, 0 );
                pEditView->MouseButtonDown( aEditEvt );
                pEditView->MouseButtonUp( aEditEvt );
            }
        }
        return;
    }

			//
			//		Links in edit cells
			//

	sal_Bool bAlt = rMEvt.IsMod2();
	if ( !bAlt && !bRefMode && !bDouble && nMouseStatus == SC_GM_URLDOWN )
	{
		//	beim ButtonUp nur ausfuehren, wenn ButtonDown auch ueber einer URL war

		String aName, aUrl, aTarget;
		if ( GetEditUrl( rMEvt.GetPosPixel(), &aName, &aUrl, &aTarget ) )
		{
			nMouseStatus = SC_GM_NONE;				// keinen Doppelklick anfangen
			ScGlobal::OpenURL( aUrl, aTarget );
			
			// fire worksheet_followhyperlink event
            uno::Reference< script::vba::XVBAEventProcessor > xVbaEvents = pDoc->GetVbaEventProcessor();
			if( xVbaEvents.is() ) try
			{
    			Point aPos = rMEvt.GetPosPixel();
    	        SCsCOL nPosX;
        	    SCsROW nPosY;
            	SCTAB nTab = pViewData->GetTabNo();
            	pViewData->GetPosFromPixel( aPos.X(), aPos.Y(), eWhich, nPosX, nPosY );
    			ScBaseCell* pCell = NULL;
    			if( lcl_GetHyperlinkCell( pDoc, nPosX, nPosY, nTab, pCell ) )
    			{
    				ScAddress aCellPos( nPosX, nPosY, nTab );
    				uno::Reference< table::XCell > xCell( new ScCellObj( pViewData->GetDocShell(), aCellPos ) );
    				uno::Sequence< uno::Any > aArgs(1);
    				aArgs[0] <<= xCell;
    			    xVbaEvents->processVbaEvent( script::vba::VBAEventId::WORKSHEET_FOLLOWHYPERLINK, aArgs );
    			}
			}
            catch( uno::Exception& )
            {
            }

			return;
		}
	}

			//
			//		Gridwin - SelectionEngine
			//

	//	SelMouseButtonDown is called only for left button, but SelMouseButtonUp would return
	//	sal_True for any call, so IsLeft must be checked here, too.

	if ( rMEvt.IsLeft() && pViewData->GetView()->GetSelEngine()->SelMouseButtonUp( rMEvt ) )
	{
//		rMark.MarkToSimple();
		pViewData->GetView()->UpdateAutoFillMark();

		SfxDispatcher* pDisp = pViewData->GetViewShell()->GetDispatcher();
        sal_Bool bFormulaMode = pScMod->IsFormulaMode();
		DBG_ASSERT( pDisp || bFormulaMode, "Cursor auf nicht aktiver View bewegen ?" );

		//	#i14927# execute SID_CURRENTCELL (for macro recording) only if there is no
		//	multiple selection, so the argument string completely describes the selection,
		//	and executing the slot won't change the existing selection (executing the slot
		//	here and from a recorded macro is treated equally)

		if ( pDisp && !bFormulaMode && !rMark.IsMultiMarked() )
		{
			String aAddr;								// CurrentCell
			if( rMark.IsMarked() )
			{
//				sal_Bool bKeep = rMark.IsMultiMarked();		//! wohin damit ???

				ScRange aScRange;
				rMark.GetMarkArea( aScRange );
				aScRange.Format( aAddr, SCR_ABS );
				if ( aScRange.aStart == aScRange.aEnd )
				{
					//	make sure there is a range selection string even for a single cell
					String aSingle = aAddr;
					aAddr.Append( (sal_Char) ':' );
					aAddr.Append( aSingle );
				}

				//!	SID_MARKAREA gibts nicht mehr ???
				//!	was passiert beim Markieren mit dem Cursor ???
			}
			else										// nur Cursor bewegen
			{
				ScAddress aScAddress( pViewData->GetCurX(), pViewData->GetCurY(), 0 );
				aScAddress.Format( aAddr, SCA_ABS );
			}

			SfxStringItem aPosItem( SID_CURRENTCELL, aAddr );
			pDisp->Execute( SID_CURRENTCELL, SFX_CALLMODE_SLOT | SFX_CALLMODE_RECORD,
										&aPosItem, (void*)0L );

			pViewData->GetView()->InvalidateAttribs();
		}
		pViewData->GetViewShell()->SelectionChanged();
		return;
	}
}

void ScGridWindow::FakeButtonUp()
{
	if ( nButtonDown )
	{
		MouseEvent aEvent( aCurMousePos );		// nButtons = 0 -> ignore
		MouseButtonUp( aEvent );
	}
}

void __EXPORT ScGridWindow::MouseMove( const MouseEvent& rMEvt )
{
	aCurMousePos = rMEvt.GetPosPixel();

	if ( rMEvt.IsLeaveWindow() && pNoteMarker && !pNoteMarker->IsByKeyboard() )
		HideNoteMarker();

	ScModule* pScMod = SC_MOD();
	if (pScMod->IsModalMode(pViewData->GetSfxDocShell()))
		return;

		//	Ob aus dem Edit-Modus Drag&Drop gestartet wurde, bekommt man leider
		//	nicht anders mit:

	if (bEEMouse && nButtonDown && !rMEvt.GetButtons())
	{
		bEEMouse = sal_False;
		nButtonDown = 0;
		nMouseStatus = SC_GM_NONE;
		return;
	}

	if (nMouseStatus == SC_GM_IGNORE)
		return;

	if (nMouseStatus == SC_GM_WATERUNDO)	// Undo im Giesskannenmodus -> nur auf Up warten
		return;

	if ( pViewData->GetViewShell()->IsAuditShell() )		// Detektiv-Fuell-Modus
	{
		SetPointer( Pointer( POINTER_FILL ) );
		return;
	}

	if (nMouseStatus == SC_GM_FILTER && pFilterBox)
	{
		Point aRelPos = pFilterBox->ScreenToOutputPixel( OutputToScreenPixel( rMEvt.GetPosPixel() ) );
		if ( Rectangle(Point(),pFilterBox->GetOutputSizePixel()).IsInside(aRelPos) )
		{
			nButtonDown = 0;
			nMouseStatus = SC_GM_NONE;
			if ( pFilterBox->GetMode() == SC_FILTERBOX_FILTER )
			{
                if (mpFilterButton.get())
                {
                    mpFilterButton->setHasHiddenMember(false);
                    mpFilterButton->setPopupPressed(false);
                    HideCursor();
                    mpFilterButton->draw();
                    ShowCursor();
                }
			}
			ReleaseMouse();
			pFilterBox->MouseButtonDown( MouseEvent( aRelPos, 1, MOUSE_SIMPLECLICK, MOUSE_LEFT ) );
			return;
		}
	}

	sal_Bool bFormulaMode = pScMod->IsFormulaMode();			// naechster Klick -> Referenz

    if (bEEMouse && pViewData->HasEditView( eWhich ))
	{
		EditView*	pEditView;
		SCCOL		nEditCol;
		SCROW		nEditRow;
        pViewData->GetEditView( eWhich, pEditView, nEditCol, nEditRow );
        pEditView->MouseMove( rMEvt );
        return;
	}

	if (bDPMouse)
	{
		DPMouseMove( rMEvt );
		return;
	}

	if (bRFMouse)
	{
		RFMouseMove( rMEvt, sal_False );
		return;
	}

	if (nPagebreakMouse)
	{
		PagebreakMove( rMEvt, sal_False );
		return;
	}

	//	anderen Mauszeiger anzeigen?

	sal_Bool bEditMode = pViewData->HasEditView(eWhich);

					//! Testen ob RefMode-Dragging !!!
	if ( bEditMode && (pViewData->GetRefTabNo() == pViewData->GetTabNo()) )
	{
		Point	aPos = rMEvt.GetPosPixel();
		SCsCOL	nPosX;
		SCsROW	nPosY;
		pViewData->GetPosFromPixel( aPos.X(), aPos.Y(), eWhich, nPosX, nPosY );

		EditView*	pEditView;
		SCCOL		nEditCol;
		SCROW		nEditRow;
		pViewData->GetEditView( eWhich, pEditView, nEditCol, nEditRow );
		SCCOL nEndCol = pViewData->GetEditEndCol();
		SCROW nEndRow = pViewData->GetEditEndRow();

		if ( nPosX >= (SCsCOL) nEditCol && nPosX <= (SCsCOL) nEndCol &&
			 nPosY >= (SCsROW) nEditRow && nPosY <= (SCsROW) nEndRow )
		{
			//	Field can only be URL field
			sal_Bool bAlt = rMEvt.IsMod2();
			if ( !bAlt && !nButtonDown && pEditView && pEditView->GetFieldUnderMousePointer() )
				SetPointer( Pointer( POINTER_REFHAND ) );
			else if ( pEditView && pEditView->GetEditEngine()->IsVertical() )
				SetPointer( Pointer( POINTER_TEXT_VERTICAL ) );
			else
				SetPointer( Pointer( POINTER_TEXT ) );
			return;
		}
	}

	sal_Bool bWater = SC_MOD()->GetIsWaterCan() || pViewData->GetView()->HasPaintBrush();
	if (bWater)
		SetPointer( Pointer(POINTER_FILL) );

	if (!bWater)
	{
		sal_Bool bCross = sal_False;

		//	Range-Finder

		sal_Bool bCorner;
		if ( HitRangeFinder( rMEvt.GetPosPixel(), bCorner ) )
		{
			if (bCorner)
				SetPointer( Pointer( POINTER_CROSS ) );
			else
				SetPointer( Pointer( POINTER_HAND ) );
			bCross = sal_True;
		}

		//	Page-Break-Modus

		sal_uInt16 nBreakType;
		if ( !nButtonDown && pViewData->IsPagebreakMode() &&
                ( nBreakType = HitPageBreak( rMEvt.GetPosPixel() ) ) != 0 )
		{
			PointerStyle eNew = POINTER_ARROW;
			switch ( nBreakType )
			{
				case SC_PD_RANGE_L:
				case SC_PD_RANGE_R:
				case SC_PD_BREAK_H:
					eNew = POINTER_ESIZE;
					break;
				case SC_PD_RANGE_T:
				case SC_PD_RANGE_B:
				case SC_PD_BREAK_V:
					eNew = POINTER_SSIZE;
					break;
				case SC_PD_RANGE_TL:
				case SC_PD_RANGE_BR:
					eNew = POINTER_SESIZE;
					break;
				case SC_PD_RANGE_TR:
				case SC_PD_RANGE_BL:
					eNew = POINTER_NESIZE;
					break;
			}
			SetPointer( Pointer( eNew ) );
			bCross = sal_True;
		}

		//	Fill-Cursor anzeigen ?

		if ( !bFormulaMode && !nButtonDown )
			if (TestMouse( rMEvt, sal_False ))
				bCross = sal_True;

		if ( nButtonDown && pViewData->IsAnyFillMode() )
		{
			SetPointer( Pointer( POINTER_CROSS ) );
			bCross = sal_True;
			nScFillModeMouseModifier = rMEvt.GetModifier();	// ausgewertet bei AutoFill und Matrix
		}

		if (!bCross)
		{
			sal_Bool bAlt = rMEvt.IsMod2();

			if (bEditMode)									// Edit-Mode muss zuerst kommen!
				SetPointer( Pointer( POINTER_ARROW ) );
			else if ( !bAlt && !nButtonDown &&
						GetEditUrl(rMEvt.GetPosPixel()) )
				SetPointer( Pointer( POINTER_REFHAND ) );
			else if ( DrawMouseMove(rMEvt) )				// setzt Pointer um
				return;
		}
	}

	if ( pViewData->GetView()->GetSelEngine()->SelMouseMove( rMEvt ) )
		return;
}

void lcl_InitMouseEvent( ::com::sun::star::awt::MouseEvent& rEvent, const MouseEvent& rEvt )
{
	rEvent.Modifiers = 0;
	if ( rEvt.IsShift() )
		rEvent.Modifiers |= ::com::sun::star::awt::KeyModifier::SHIFT;
	if ( rEvt.IsMod1() )
	rEvent.Modifiers |= ::com::sun::star::awt::KeyModifier::MOD1;
	if ( rEvt.IsMod2() )
		rEvent.Modifiers |= ::com::sun::star::awt::KeyModifier::MOD2;
        if ( rEvt.IsMod3() )
                rEvent.Modifiers |= ::com::sun::star::awt::KeyModifier::MOD3;

	rEvent.Buttons = 0;
	if ( rEvt.IsLeft() )
		rEvent.Buttons |= ::com::sun::star::awt::MouseButton::LEFT;
	if ( rEvt.IsRight() )
		rEvent.Buttons |= ::com::sun::star::awt::MouseButton::RIGHT;
	if ( rEvt.IsMiddle() )
		rEvent.Buttons |= ::com::sun::star::awt::MouseButton::MIDDLE;

	rEvent.X = rEvt.GetPosPixel().X();
	rEvent.Y = rEvt.GetPosPixel().Y();
	rEvent.ClickCount = rEvt.GetClicks();
	rEvent.PopupTrigger = sal_False;
}

long ScGridWindow::PreNotify( NotifyEvent& rNEvt )
{
    bool bDone = false;
	sal_uInt16 nType = rNEvt.GetType();
	if ( nType == EVENT_MOUSEBUTTONUP || nType == EVENT_MOUSEBUTTONDOWN )
    {
		Window* pWindow = rNEvt.GetWindow();
        if (pWindow == this && pViewData)
        {
	        SfxViewFrame* pViewFrame = pViewData->GetViewShell()->GetViewFrame();
	        if (pViewFrame)
	        {
		        com::sun::star::uno::Reference<com::sun::star::frame::XController> xController = pViewFrame->GetFrame().GetController();
		        if (xController.is())
		        {
			        ScTabViewObj* pImp = ScTabViewObj::getImplementation( xController );
			        if (pImp && pImp->IsMouseListening())
                    {
		                ::com::sun::star::awt::MouseEvent aEvent;
		                lcl_InitMouseEvent( aEvent, *rNEvt.GetMouseEvent() );
                        if ( rNEvt.GetWindow() )
	                        aEvent.Source = rNEvt.GetWindow()->GetComponentInterface();
                        if ( nType == EVENT_MOUSEBUTTONDOWN)
                            bDone = pImp->MousePressed( aEvent );
                        else
                            bDone = pImp->MouseReleased( aEvent );
                    }
		        }
	        }
        }
	}
    if (bDone)      // event consumed by a listener
    {
        if ( nType == EVENT_MOUSEBUTTONDOWN )
        {
            const MouseEvent* pMouseEvent = rNEvt.GetMouseEvent();
            if ( pMouseEvent->IsRight() && pMouseEvent->GetClicks() == 1 )
            {
                // If a listener returned true for a right-click call, also prevent opening the context menu
                // (this works only if the context menu is opened on mouse-down)
                nMouseStatus = SC_GM_IGNORE;
            }
        }

        return 1;
    }
    else
        return Window::PreNotify( rNEvt );
}

void ScGridWindow::Tracking( const TrackingEvent& rTEvt )
{
	//	Weil die SelectionEngine kein Tracking kennt, die Events nur auf
	//	die verschiedenen MouseHandler verteilen...

	const MouseEvent& rMEvt = rTEvt.GetMouseEvent();

	if ( rTEvt.IsTrackingCanceled() )		// alles abbrechen...
	{
		if (!pViewData->GetView()->IsInActivatePart())
		{
			if (bDPMouse)
				bDPMouse = sal_False;				// gezeichnet wird per bDragRect
			if (bDragRect)
			{
				// pViewData->GetView()->DrawDragRect( nDragStartX, nDragStartY, nDragEndX, nDragEndY, eWhich );
				bDragRect = sal_False;
                UpdateDragRectOverlay();
			}
			if (bRFMouse)
			{
				RFMouseMove( rMEvt, sal_True );		// richtig abbrechen geht dabei nicht...
				bRFMouse = sal_False;
			}
			if (nPagebreakMouse)
			{
				// if (bPagebreakDrawn)
				//	DrawDragRect( aPagebreakDrag.aStart.Col(), aPagebreakDrag.aStart.Row(),
				//					aPagebreakDrag.aEnd.Col(), aPagebreakDrag.aEnd.Row(), sal_False );
				bPagebreakDrawn = sal_False;
                UpdateDragRectOverlay();
				nPagebreakMouse = SC_PD_NONE;
			}

			SetPointer( Pointer( POINTER_ARROW ) );
			StopMarking();
			MouseButtonUp( rMEvt );		// mit Status SC_GM_IGNORE aus StopMarking

			sal_Bool bRefMode =	pViewData->IsRefMode();
			if (bRefMode)
				SC_MOD()->EndReference();		// #63148# Dialog nicht verkleinert lassen
		}
	}
	else if ( rTEvt.IsTrackingEnded() )
	{
		//	MouseButtonUp immer mit passenden Buttons (z.B. wegen Testtool, #63148#)
		//	Schliesslich behauptet der Tracking-Event ja, dass normal beendet und nicht
		//	abgebrochen wurde.

		MouseEvent aUpEvt( rMEvt.GetPosPixel(), rMEvt.GetClicks(),
							rMEvt.GetMode(), nButtonDown, rMEvt.GetModifier() );
		MouseButtonUp( aUpEvt );
	}
	else
		MouseMove( rMEvt );
}

void ScGridWindow::StartDrag( sal_Int8 /* nAction */, const Point& rPosPixel )
{
	if ( pFilterBox || nPagebreakMouse )
		return;

	HideNoteMarker();

	CommandEvent aDragEvent( rPosPixel, COMMAND_STARTDRAG, sal_True );

    if (bEEMouse && pViewData->HasEditView( eWhich ))
	{
		EditView*	pEditView;
		SCCOL		nEditCol;
		SCROW		nEditRow;
		pViewData->GetEditView( eWhich, pEditView, nEditCol, nEditRow );

		// #63263# don't remove the edit view while switching views
		ScModule* pScMod = SC_MOD();
		pScMod->SetInEditCommand( sal_True );

		pEditView->Command( aDragEvent );

		ScInputHandler* pHdl = pScMod->GetInputHdl();
		if (pHdl)
			pHdl->DataChanged();

		pScMod->SetInEditCommand( sal_False );
		if (!pViewData->IsActive())				// dropped to different view?
		{
			ScInputHandler* pViewHdl = pScMod->GetInputHdl( pViewData->GetViewShell() );
			if ( pViewHdl && pViewData->HasEditView( eWhich ) )
			{
				pViewHdl->CancelHandler();
				ShowCursor();	// missing from KillEditView
			}
		}
	}
	else
		if ( !DrawCommand(aDragEvent) )
			pViewData->GetView()->GetSelEngine()->Command( aDragEvent );
}

void lcl_SetTextCursorPos( ScViewData* pViewData, ScSplitPos eWhich, Window* pWin )
{
	SCCOL nCol = pViewData->GetCurX();
	SCROW nRow = pViewData->GetCurY();
	Rectangle aEditArea = pViewData->GetEditArea( eWhich, nCol, nRow, pWin, NULL, sal_True );
	aEditArea.Right() = aEditArea.Left();
	aEditArea = pWin->PixelToLogic( aEditArea );
	pWin->SetCursorRect( &aEditArea );
}

void __EXPORT ScGridWindow::Command( const CommandEvent& rCEvt )
{
    // The command event is send to the window after a possible context
    // menu from an inplace client is closed. Now we have the chance to
    // deactivate the inplace client without any problem regarding parent
    // windows and code on the stack.
    // For more information, see #126086# and #128122#
    sal_uInt16 nCmd = rCEvt.GetCommand();
    ScTabViewShell* pTabViewSh = pViewData->GetViewShell();
	SfxInPlaceClient* pClient = pTabViewSh->GetIPClient();
    if ( pClient &&
         pClient->IsObjectInPlaceActive() &&
         nCmd == COMMAND_CONTEXTMENU )
    {
        pTabViewSh->DeactivateOle();
        return;
    }

	ScModule* pScMod = SC_MOD();
	DBG_ASSERT( nCmd != COMMAND_STARTDRAG, "ScGridWindow::Command called with COMMAND_STARTDRAG" );

	if ( nCmd == COMMAND_STARTEXTTEXTINPUT ||
		 nCmd == COMMAND_ENDEXTTEXTINPUT ||
		 nCmd == COMMAND_EXTTEXTINPUT ||
		 nCmd == COMMAND_CURSORPOS )
	{
		sal_Bool bEditView = pViewData->HasEditView( eWhich );
		if (!bEditView)
		{
			//	only if no cell editview is active, look at drawview
			SdrView* pSdrView = pViewData->GetView()->GetSdrView();
			if ( pSdrView )
			{
				OutlinerView* pOlView = pSdrView->GetTextEditOutlinerView();
				if ( pOlView && pOlView->GetWindow() == this )
				{
					pOlView->Command( rCEvt );
					return;								// done
				}
			}
		}

		if ( nCmd == COMMAND_CURSORPOS && !bEditView )
		{
			//	#88458# CURSORPOS may be called without following text input,
			//	to set the input method window position
			//	-> input mode must not be started,
			//	manually calculate text insert position if not in input mode

			lcl_SetTextCursorPos( pViewData, eWhich, this );
			return;
		}

		ScInputHandler* pHdl = pScMod->GetInputHdl( pViewData->GetViewShell() );
		if ( pHdl )
		{
			pHdl->InputCommand( rCEvt, sal_True );
			return;										// done
		}

		Window::Command( rCEvt );
		return;
	}

	if ( nCmd == COMMAND_VOICE )
	{
		//	Der Handler wird nur gerufen, wenn ein Text-Cursor aktiv ist,
		//	also muss es eine EditView oder ein editiertes Zeichenobjekt geben

		ScInputHandler* pHdl = pScMod->GetInputHdl( pViewData->GetViewShell() );
		if ( pHdl && pViewData->HasEditView( eWhich ) )
		{
			EditView* pEditView = pViewData->GetEditView( eWhich );	// ist dann nicht 0
			pHdl->DataChanging();
			pEditView->Command( rCEvt );
			pHdl->DataChanged();
			return;										// erledigt
		}
		SdrView* pSdrView = pViewData->GetView()->GetSdrView();
		if ( pSdrView )
		{
			OutlinerView* pOlView = pSdrView->GetTextEditOutlinerView();
			if ( pOlView && pOlView->GetWindow() == this )
			{
				pOlView->Command( rCEvt );
				return;									// erledigt
			}
		}
		Window::Command(rCEvt);		//	sonst soll sich die Basisklasse drum kuemmern...
		return;
	}

	if ( nCmd == COMMAND_PASTESELECTION )
	{
		if ( bEEMouse )
		{
			//	EditEngine handles selection in MouseButtonUp - no action
			//	needed in command handler
		}
		else
		{
			PasteSelection( rCEvt.GetMousePosPixel() );
		}
		return;
	}

    if ( nCmd == COMMAND_INPUTLANGUAGECHANGE )
    {
        // #i55929# Font and font size state depends on input language if nothing is selected,
        // so the slots have to be invalidated when the input language is changed.

        SfxBindings& rBindings = pViewData->GetBindings();
        rBindings.Invalidate( SID_ATTR_CHAR_FONT );
        rBindings.Invalidate( SID_ATTR_CHAR_FONTHEIGHT );
        return;
    }

	if ( nCmd == COMMAND_WHEEL || nCmd == COMMAND_STARTAUTOSCROLL || nCmd == COMMAND_AUTOSCROLL )
	{
		sal_Bool bDone = pViewData->GetView()->ScrollCommand( rCEvt, eWhich );
		if (!bDone)
			Window::Command(rCEvt);
		return;
	}
    // #i7560# FormulaMode check is below scrolling - scrolling is allowed during formula input
	sal_Bool bDisable = pScMod->IsFormulaMode() ||
					pScMod->IsModalMode(pViewData->GetSfxDocShell());
	if (bDisable)
		return;

	if ( nCmd == COMMAND_CONTEXTMENU && !SC_MOD()->GetIsWaterCan() )
	{
        sal_Bool bMouse = rCEvt.IsMouseEvent();
        if ( bMouse && nMouseStatus == SC_GM_IGNORE )
            return;

		if (pViewData->IsAnyFillMode())
		{
			pViewData->GetView()->StopRefMode();
			pViewData->ResetFillMode();
		}
		ReleaseMouse();
		StopMarking();

		Point aPosPixel = rCEvt.GetMousePosPixel();
		Point aMenuPos = aPosPixel;

		if ( bMouse )
		{
            SCsCOL nCellX = -1;
            SCsROW nCellY = -1;
            pViewData->GetPosFromPixel(aPosPixel.X(), aPosPixel.Y(), eWhich, nCellX, nCellY);
            ScDocument* pDoc = pViewData->GetDocument();
            SCTAB nTab = pViewData->GetTabNo();
            const ScTableProtection* pProtect = pDoc->GetTabProtection(nTab);
            bool bSelectAllowed = true;
            if ( pProtect && pProtect->isProtected() )
            {
                // This sheet is protected.  Check if a context menu is allowed on this cell.
                bool bCellProtected = pDoc->HasAttrib(nCellX, nCellY, nTab, nCellX, nCellY, nTab, HASATTR_PROTECTED);
                bool bSelProtected   = pProtect->isOptionEnabled(ScTableProtection::SELECT_LOCKED_CELLS);
                bool bSelUnprotected = pProtect->isOptionEnabled(ScTableProtection::SELECT_UNLOCKED_CELLS);

                if (bCellProtected)
                    bSelectAllowed = bSelProtected;
                else
                    bSelectAllowed = bSelUnprotected;
            }
            if (!bSelectAllowed)
                // Selecting this cell is not allowed, neither is context menu.
                return;

			//	#i18735# First select the item under the mouse pointer.
			//	This can change the selection, and the view state (edit mode, etc).
            SelectForContextMenu( aPosPixel, nCellX, nCellY );
		}

		sal_Bool bDone = sal_False;
		sal_Bool bEdit = pViewData->HasEditView(eWhich);
		if ( !bEdit )
		{
				// Edit-Zelle mit Spelling-Errors ?
			if ( bMouse && GetEditUrlOrError( sal_True, aPosPixel ) )
			{
				//	GetEditUrlOrError hat den Cursor schon bewegt

				pScMod->SetInputMode( SC_INPUT_TABLE );
				bEdit = pViewData->HasEditView(eWhich);		// hat's geklappt ?

				DBG_ASSERT( bEdit, "kann nicht in Edit-Modus schalten" );
			}
		}
		if ( bEdit )
		{
			EditView* pEditView = pViewData->GetEditView( eWhich );		// ist dann nicht 0

			if ( !bMouse )
			{
				Cursor* pCur = pEditView->GetCursor();
				if ( pCur )
				{
					Point aLogicPos = pCur->GetPos();
					//	use the position right of the cursor (spell popup is opened if
					//	the cursor is before the word, but not if behind it)
					aLogicPos.X() += pCur->GetWidth();
					aLogicPos.Y() += pCur->GetHeight() / 2;		// center vertically
					aMenuPos = LogicToPixel( aLogicPos );
				}
			}

			//	if edit mode was just started above, online spelling may be incomplete
			pEditView->GetEditEngine()->CompleteOnlineSpelling();

			//	IsCursorAtWrongSpelledWord could be used for !bMouse
			//	if there was a corresponding ExecuteSpellPopup call

			if( pEditView->IsWrongSpelledWordAtPos( aMenuPos ) )
			{
				//	Wenn man unter OS/2 neben das Popupmenue klickt, kommt MouseButtonDown
				//	vor dem Ende des Menue-Execute, darum muss SetModified vorher kommen
				//	(Bug #40968#)
				ScInputHandler* pHdl = pScMod->GetInputHdl();
				if (pHdl)
					pHdl->SetModified();

                Link aLink = LINK( this, ScGridWindow, PopupSpellingHdl );
                pEditView->ExecuteSpellPopup( aMenuPos, &aLink );

				bDone = sal_True;
			}
		}
		else if ( !bMouse )
		{
			//	non-edit menu by keyboard -> use lower right of cell cursor position

			SCCOL nCurX = pViewData->GetCurX();
			SCROW nCurY = pViewData->GetCurY();
			aMenuPos = pViewData->GetScrPos( nCurX, nCurY, eWhich, sal_True );
			long nSizeXPix;
			long nSizeYPix;
			pViewData->GetMergeSizePixel( nCurX, nCurY, nSizeXPix, nSizeYPix );
			aMenuPos.X() += nSizeXPix;
			aMenuPos.Y() += nSizeYPix;

            if (pViewData)
            {
        	    ScTabViewShell* pViewSh = pViewData->GetViewShell();
	            if (pViewSh)
	            {
		            //	Is a draw object selected?

		            SdrView* pDrawView = pViewSh->GetSdrView();
		            if (pDrawView && pDrawView->AreObjectsMarked())
		            {
                        // #100442#; the conext menu should open in the middle of the selected objects
                        Rectangle aSelectRect(LogicToPixel(pDrawView->GetAllMarkedBoundRect()));
                        aMenuPos = aSelectRect.Center();
		            }
                }
            }
		}

		if (!bDone)
		{
			SfxDispatcher::ExecutePopup( 0, this, &aMenuPos );
		}
	}
}

void ScGridWindow::SelectForContextMenu( const Point& rPosPixel, SCsCOL nCellX, SCsROW nCellY )
{
    //  #i18735# if the click was outside of the current selection,
    //  the cursor is moved or an object at the click position selected.
    //  (see SwEditWin::SelectMenuPosition in Writer)

    ScTabView* pView = pViewData->GetView();
    ScDrawView* pDrawView = pView->GetScDrawView();

    //  check cell edit mode

    if ( pViewData->HasEditView(eWhich) )
    {
        ScModule* pScMod = SC_MOD();
        SCCOL nEditStartCol = pViewData->GetEditViewCol(); //! change to GetEditStartCol after calcrtl is integrated
        SCROW nEditStartRow = pViewData->GetEditViewRow();
        SCCOL nEditEndCol = pViewData->GetEditEndCol();
        SCROW nEditEndRow = pViewData->GetEditEndRow();

        if ( nCellX >= (SCsCOL) nEditStartCol && nCellX <= (SCsCOL) nEditEndCol &&
             nCellY >= (SCsROW) nEditStartRow && nCellY <= (SCsROW) nEditEndRow )
        {
            //  handle selection within the EditView

            EditView* pEditView = pViewData->GetEditView( eWhich );     // not NULL (HasEditView)
            EditEngine* pEditEngine = pEditView->GetEditEngine();
            Rectangle aOutputArea = pEditView->GetOutputArea();
            Rectangle aVisArea = pEditView->GetVisArea();

            Point aTextPos = PixelToLogic( rPosPixel );
            if ( pEditEngine->IsVertical() )            // have to manually transform position
            {
                aTextPos -= aOutputArea.TopRight();
                long nTemp = -aTextPos.X();
                aTextPos.X() = aTextPos.Y();
                aTextPos.Y() = nTemp;
            }
            else
                aTextPos -= aOutputArea.TopLeft();
            aTextPos += aVisArea.TopLeft();             // position in the edit document

            EPosition aDocPosition = pEditEngine->FindDocPosition(aTextPos);
            ESelection aCompare(aDocPosition.nPara, aDocPosition.nIndex);
            ESelection aSelection = pEditView->GetSelection();
            aSelection.Adjust();    // needed for IsLess/IsGreater
            if ( aCompare.IsLess(aSelection) || aCompare.IsGreater(aSelection) )
            {
                // clicked outside the selected text - deselect and move text cursor
                MouseEvent aEvent( rPosPixel );
                pEditView->MouseButtonDown( aEvent );
                pEditView->MouseButtonUp( aEvent );
                pScMod->InputSelection( pEditView );
            }

            return;     // clicked within the edit view - keep edit mode
        }
        else
        {
            // outside of the edit view - end edit mode, regardless of cell selection, then continue
            pScMod->InputEnterHandler();
        }
    }

    //  check draw text edit mode

    Point aLogicPos = PixelToLogic( rPosPixel );        // after cell edit mode is ended
    if ( pDrawView && pDrawView->GetTextEditObject() && pDrawView->GetTextEditOutlinerView() )
    {
        OutlinerView* pOlView = pDrawView->GetTextEditOutlinerView();
        Rectangle aOutputArea = pOlView->GetOutputArea();
        if ( aOutputArea.IsInside( aLogicPos ) )
        {
            //  handle selection within the OutlinerView

            Outliner* pOutliner = pOlView->GetOutliner();
            const EditEngine& rEditEngine = pOutliner->GetEditEngine();
            Rectangle aVisArea = pOlView->GetVisArea();

            Point aTextPos = aLogicPos;
            if ( pOutliner->IsVertical() )              // have to manually transform position
            {
                aTextPos -= aOutputArea.TopRight();
                long nTemp = -aTextPos.X();
                aTextPos.X() = aTextPos.Y();
                aTextPos.Y() = nTemp;
            }
            else
                aTextPos -= aOutputArea.TopLeft();
            aTextPos += aVisArea.TopLeft();             // position in the edit document

            EPosition aDocPosition = rEditEngine.FindDocPosition(aTextPos);
            ESelection aCompare(aDocPosition.nPara, aDocPosition.nIndex);
            ESelection aSelection = pOlView->GetSelection();
            aSelection.Adjust();    // needed for IsLess/IsGreater
            if ( aCompare.IsLess(aSelection) || aCompare.IsGreater(aSelection) )
            {
                // clicked outside the selected text - deselect and move text cursor
                // use DrawView to allow extra handling there (none currently)
                MouseEvent aEvent( rPosPixel );
                pDrawView->MouseButtonDown( aEvent, this );
                pDrawView->MouseButtonUp( aEvent, this );
            }

            return;     // clicked within the edit area - keep edit mode
        }
        else
        {
            // Outside of the edit area - end text edit mode, then continue.
            // DrawDeselectAll also ends text edit mode and updates the shells.
            // If the click was on the edited object, it will be selected again below.
            pView->DrawDeselectAll();
        }
    }

    //  look for existing selection

    sal_Bool bHitSelected = sal_False;
    if ( pDrawView && pDrawView->IsMarkedObjHit( aLogicPos ) )
    {
        //  clicked on selected object -> don't change anything
        bHitSelected = sal_True;
    }
    else if ( pViewData->GetMarkData().IsCellMarked(nCellX, nCellY) )
    {
        //  clicked on selected cell -> don't change anything
        bHitSelected = sal_True;
    }

    //  select drawing object or move cell cursor

    if ( !bHitSelected )
    {
        sal_Bool bWasDraw = ( pDrawView && pDrawView->AreObjectsMarked() );
        sal_Bool bHitDraw = sal_False;
        if ( pDrawView )
        {
            pDrawView->UnmarkAllObj();
            // Unlock the Internal Layer in order to activate the context menu.
            // re-lock in ScDrawView::MarkListHasChanged()
            lcl_UnLockComment( pDrawView, pDrawView->GetSdrPageView(), pDrawView->GetModel(), aLogicPos ,pViewData);
            bHitDraw = pDrawView->MarkObj( aLogicPos );
            // draw shell is activated in MarkListHasChanged
        }
        if ( !bHitDraw )
        {
            pView->Unmark();
            pView->SetCursor(nCellX, nCellY);
            if ( bWasDraw )
                pViewData->GetViewShell()->SetDrawShell( sal_False );   // switch shells
        }
    }
}

void __EXPORT ScGridWindow::KeyInput(const KeyEvent& rKEvt)
{
    // #96965# Cursor control for ref input dialog
    if( SC_MOD()->IsRefDialogOpen() )
    {
        const KeyCode& rKeyCode = rKEvt.GetKeyCode();
        if( !rKeyCode.GetModifier() && (rKeyCode.GetCode() == KEY_F2) )
        {
            SC_MOD()->EndReference();
        }
        else if( pViewData->GetViewShell()->MoveCursorKeyInput( rKEvt ) )
        {
            ScRange aRef(
                pViewData->GetRefStartX(), pViewData->GetRefStartY(), pViewData->GetRefStartZ(),
                pViewData->GetRefEndX(), pViewData->GetRefEndY(), pViewData->GetRefEndZ() );
            SC_MOD()->SetReference( aRef, pViewData->GetDocument() );
        }
		pViewData->GetViewShell()->SelectionChanged();
		return ;
    }
	// wenn semi-Modeless-SfxChildWindow-Dialog oben, keine KeyInputs:
    else if( !pViewData->IsAnyFillMode() )
	{
		//	query for existing note marker before calling ViewShell's keyboard handling
		//	which may remove the marker
		sal_Bool bHadKeyMarker = ( pNoteMarker && pNoteMarker->IsByKeyboard() );
		ScTabViewShell* pViewSh = pViewData->GetViewShell();

		if (pViewData->GetDocShell()->GetProgress())
			return;

        if (DrawKeyInput(rKEvt))
        {
            const KeyCode& rKeyCode = rKEvt.GetKeyCode();
            if (rKeyCode.GetCode() == KEY_DOWN
                || rKeyCode.GetCode() == KEY_UP
                || rKeyCode.GetCode() == KEY_LEFT
                || rKeyCode.GetCode() == KEY_RIGHT)
            {
                ScTabViewShell* pViewShell = pViewData->GetViewShell();
                SfxBindings& rBindings = pViewShell->GetViewFrame()->GetBindings();
                rBindings.Invalidate(SID_ATTR_TRANSFORM_POS_X);
                rBindings.Invalidate(SID_ATTR_TRANSFORM_POS_Y);
 			}
			return;
        }

		if (!pViewData->GetView()->IsDrawSelMode() && !DrawHasMarkedObj())	//	keine Eingaben im Zeichenmodus
		{															//! DrawShell abfragen !!!
			if (pViewSh->TabKeyInput(rKEvt))
				return;
		}
		else
			if (pViewSh->SfxViewShell::KeyInput(rKEvt))				// von SfxViewShell
				return;

		KeyCode aCode = rKEvt.GetKeyCode();
		if ( aCode.GetCode() == KEY_ESCAPE && aCode.GetModifier() == 0 )
		{
			if ( bHadKeyMarker )
				HideNoteMarker();
            else
                pViewSh->Escape();
			return;
		}
		if ( aCode.GetCode() == KEY_F1 && aCode.GetModifier() == KEY_MOD1 )
		{
			//	ctrl-F1 shows or hides the note or redlining info for the cursor position
			//	(hard-coded because F1 can't be configured)

			if ( bHadKeyMarker )
				HideNoteMarker();		// hide when previously visible
			else
				ShowNoteMarker( pViewData->GetCurX(), pViewData->GetCurY(), sal_True );
			return;
		}
	}

	Window::KeyInput(rKEvt);
}

void ScGridWindow::StopMarking()
{
	DrawEndAction();				// Markieren/Verschieben auf Drawing-Layer abbrechen

	if (nButtonDown)
	{
		pViewData->GetMarkData().SetMarking(sal_False);
		nMouseStatus = SC_GM_IGNORE;
	}
}

void ScGridWindow::UpdateInputContext()
{
	sal_Bool bReadOnly = pViewData->GetDocShell()->IsReadOnly();
	sal_uLong nOptions = bReadOnly ? 0 : ( INPUTCONTEXT_TEXT | INPUTCONTEXT_EXTTEXTINPUT );

	//	when font from InputContext is used,
	//	it must be taken from the cursor position's cell attributes

	InputContext aContext;
	aContext.SetOptions( nOptions );
	SetInputContext( aContext );
}

//--------------------------------------------------------

								// sensitiver Bereich (Pixel)
#define SCROLL_SENSITIVE 20

sal_Bool ScGridWindow::DropScroll( const Point& rMousePos )
{
/*	doch auch auf nicht aktiven Views...
	if ( !pViewData->IsActive() )
		return sal_False;
*/
	SCsCOL nDx = 0;
	SCsROW nDy = 0;
	Size aSize = GetOutputSizePixel();

	if (aSize.Width() > SCROLL_SENSITIVE * 3)
	{
		if ( rMousePos.X() < SCROLL_SENSITIVE && pViewData->GetPosX(WhichH(eWhich)) > 0 )
			nDx = -1;
		if ( rMousePos.X() >= aSize.Width() - SCROLL_SENSITIVE
				&& pViewData->GetPosX(WhichH(eWhich)) < MAXCOL )
			nDx = 1;
	}
	if (aSize.Height() > SCROLL_SENSITIVE * 3)
	{
		if ( rMousePos.Y() < SCROLL_SENSITIVE && pViewData->GetPosY(WhichV(eWhich)) > 0 )
			nDy = -1;
		if ( rMousePos.Y() >= aSize.Height() - SCROLL_SENSITIVE
				&& pViewData->GetPosY(WhichV(eWhich)) < MAXROW )
			nDy = 1;
	}

	if ( nDx != 0 || nDy != 0 )
	{
//		if (bDragRect)
//			pViewData->GetView()->DrawDragRect( nDragStartX, nDragStartY, nDragEndX, nDragEndY, eWhich );

		if ( nDx != 0 )
			pViewData->GetView()->ScrollX( nDx, WhichH(eWhich) );
		if ( nDy != 0 )
			pViewData->GetView()->ScrollY( nDy, WhichV(eWhich) );

//		if (bDragRect)
//			pViewData->GetView()->DrawDragRect( nDragStartX, nDragStartY, nDragEndX, nDragEndY, eWhich );
	}

	return sal_False;
}

sal_Bool lcl_TestScenarioRedliningDrop( ScDocument* pDoc, const ScRange& aDragRange)
{
	//	Testet, ob bei eingeschalteten RedLining,
	//  bei einem Drop ein Scenario betroffen ist.

	sal_Bool bReturn = sal_False;
	SCTAB nTab = aDragRange.aStart.Tab();
	SCTAB nTabCount = pDoc->GetTableCount();

	if(pDoc->GetChangeTrack()!=NULL)
	{
		if( pDoc->IsScenario(nTab) && pDoc->HasScenarioRange(nTab, aDragRange))
		{
			bReturn = sal_True;
		}
		else
		{
			for(SCTAB i=nTab+1; i<nTabCount && pDoc->IsScenario(i); i++)
			{
				if(pDoc->HasScenarioRange(i, aDragRange))
				{
					bReturn = sal_True;
					break;
				}
			}
		}
	}
	return bReturn;
}

ScRange lcl_MakeDropRange( SCCOL nPosX, SCROW nPosY, SCTAB nTab, const ScRange& rSource )
{
	SCCOL nCol1 = nPosX;
	SCCOL nCol2 = nCol1 + ( rSource.aEnd.Col() - rSource.aStart.Col() );
	if ( nCol2 > MAXCOL )
	{
		nCol1 -= nCol2 - MAXCOL;
		nCol2 = MAXCOL;
	}
	SCROW nRow1 = nPosY;
	SCROW nRow2 = nRow1 + ( rSource.aEnd.Row() - rSource.aStart.Row() );
	if ( nRow2 > MAXROW )
	{
		nRow1 -= nRow2 - MAXROW;
		nRow2 = MAXROW;
	}

	return ScRange( nCol1, nRow1, nTab, nCol2, nRow2, nTab );
}

//--------------------------------------------------------

extern sal_Bool bPasteIsDrop;		// viewfun4 -> move to header
extern sal_Bool bPasteIsMove;		// viewfun7 -> move to header

//--------------------------------------------------------

sal_Int8 ScGridWindow::AcceptPrivateDrop( const AcceptDropEvent& rEvt )
{
	if ( rEvt.mbLeaving )
	{
		// if (bDragRect)
		//	pViewData->GetView()->DrawDragRect( nDragStartX, nDragStartY, nDragEndX, nDragEndY, eWhich );
		bDragRect = sal_False;
		UpdateDragRectOverlay();
		return rEvt.mnAction;
	}

	const ScDragData& rData = SC_MOD()->GetDragData();
	if ( rData.pCellTransfer )
	{
        // Don't move source that would include filtered rows.
        if ((rEvt.mnAction & DND_ACTION_MOVE) && rData.pCellTransfer->HasFilteredRows())
        {
            if (bDragRect)
            {
                bDragRect = sal_False;
                UpdateDragRectOverlay();
            }
            return DND_ACTION_NONE;
        }

		Point aPos = rEvt.maPosPixel;

		ScDocument* pSourceDoc = rData.pCellTransfer->GetSourceDocument();
		ScDocument* pThisDoc   = pViewData->GetDocument();
		if (pSourceDoc == pThisDoc)
		{
			if ( pThisDoc->HasChartAtPoint(pViewData->GetTabNo(), PixelToLogic(aPos)) )
			{
				if (bDragRect)			// Rechteck loeschen
				{
					// pViewData->GetView()->DrawDragRect( nDragStartX, nDragStartY, nDragEndX, nDragEndY, eWhich );
					bDragRect = sal_False;
					UpdateDragRectOverlay();
				}

				//!	highlight chart? (selection border?)

				sal_Int8 nRet = rEvt.mnAction;
//!				if ( rEvt.GetAction() == DROP_LINK )
//!					bOk = rEvt.SetAction( DROP_COPY );			// can't link onto chart
				return nRet;
			}
		}
//!		else
//!			if ( rEvt.GetAction() == DROP_MOVE )
//!				rEvt.SetAction( DROP_COPY );					// different doc: default=COPY


		if ( rData.pCellTransfer->GetDragSourceFlags() & SC_DROP_TABLE )		// whole sheet?
		{
			sal_Bool bOk = pThisDoc->IsDocEditable();
			return bOk ? rEvt.mnAction : 0;						// don't draw selection frame
		}

		SCsCOL	nPosX;
		SCsROW	nPosY;
		pViewData->GetPosFromPixel( aPos.X(), aPos.Y(), eWhich, nPosX, nPosY );

		ScRange aSourceRange = rData.pCellTransfer->GetRange();
        SCCOL nSourceStartX = aSourceRange.aStart.Col();
        SCROW nSourceStartY = aSourceRange.aStart.Row();
        SCCOL nSourceEndX = aSourceRange.aEnd.Col();
        SCROW nSourceEndY = aSourceRange.aEnd.Row();
        SCCOL nSizeX = nSourceEndX - nSourceStartX + 1;
        SCROW nSizeY = nSourceEndY - nSourceStartY + 1;

		if ( rEvt.mnAction != DND_ACTION_MOVE )
			nSizeY = rData.pCellTransfer->GetNonFilteredRows();		// copy/link: no filtered rows

		SCsCOL nNewDragX = nPosX - rData.pCellTransfer->GetDragHandleX();
		if (nNewDragX<0) nNewDragX=0;
		if (nNewDragX+(nSizeX-1) > MAXCOL)
			nNewDragX = MAXCOL-(nSizeX-1);
		SCsROW nNewDragY = nPosY - rData.pCellTransfer->GetDragHandleY();
		if (nNewDragY<0) nNewDragY=0;
		if (nNewDragY+(nSizeY-1) > MAXROW)
			nNewDragY = MAXROW-(nSizeY-1);

		//	don't break scenario ranges, don't drop on filtered
		SCTAB nTab = pViewData->GetTabNo();
		ScRange aDropRange = lcl_MakeDropRange( nNewDragX, nNewDragY, nTab, aSourceRange );
		if ( lcl_TestScenarioRedliningDrop( pThisDoc, aDropRange ) ||
			 lcl_TestScenarioRedliningDrop( pSourceDoc, aSourceRange ) ||
             ScViewUtil::HasFiltered( aDropRange, pThisDoc) )
		{
			if (bDragRect)
			{
				// pViewData->GetView()->DrawDragRect( nDragStartX, nDragStartY, nDragEndX, nDragEndY, eWhich );
				bDragRect = sal_False;
                UpdateDragRectOverlay();
			}
			return DND_ACTION_NONE;
		}

        InsCellCmd eDragInsertMode = INS_NONE;
        Window::PointerState aState = GetPointerState();

        // check for datapilot item sorting
        ScDPObject* pDPObj = NULL;
        if ( pThisDoc == pSourceDoc && ( pDPObj = pThisDoc->GetDPAtCursor( nNewDragX, nNewDragY, nTab ) ) != NULL )
        {
            // drop on DataPilot table: sort or nothing

            bool bDPSort = false;
            if ( pThisDoc->GetDPAtCursor( nSourceStartX, nSourceStartY, aSourceRange.aStart.Tab() ) == pDPObj )
            {
                sheet::DataPilotTableHeaderData aDestData;
                pDPObj->GetHeaderPositionData( ScAddress(nNewDragX, nNewDragY, nTab), aDestData );
                bool bValid = ( aDestData.Dimension >= 0 );        // dropping onto a field

                // look through the source range
                for (SCROW nRow = aSourceRange.aStart.Row(); bValid && nRow <= aSourceRange.aEnd.Row(); ++nRow )
                    for (SCCOL nCol = aSourceRange.aStart.Col(); bValid && nCol <= aSourceRange.aEnd.Col(); ++nCol )
                    {
                        sheet::DataPilotTableHeaderData aSourceData;
                        pDPObj->GetHeaderPositionData( ScAddress( nCol, nRow, aSourceRange.aStart.Tab() ), aSourceData );
                        if ( aSourceData.Dimension != aDestData.Dimension || !aSourceData.MemberName.getLength() )
                            bValid = false;     // empty (subtotal) or different field
                    }

                if ( bValid )
                {
                    sal_Bool bIsDataLayout;
                    String aDimName = pDPObj->GetDimName( aDestData.Dimension, bIsDataLayout );
                    const ScDPSaveDimension* pDim = pDPObj->GetSaveData()->GetExistingDimensionByName( aDimName );
                    if ( pDim )
                    {
                        ScRange aOutRange = pDPObj->GetOutRange();

                        sal_uInt16 nOrient = pDim->GetOrientation();
                        if ( nOrient == sheet::DataPilotFieldOrientation_COLUMN )
                        {
                            eDragInsertMode = INS_CELLSRIGHT;
                            nSizeY = aOutRange.aEnd.Row() - nNewDragY + 1;
                            bDPSort = true;
                        }
                        else if ( nOrient == sheet::DataPilotFieldOrientation_ROW )
                        {
                            eDragInsertMode = INS_CELLSDOWN;
                            nSizeX = aOutRange.aEnd.Col() - nNewDragX + 1;
                            bDPSort = true;
                        }
                    }
                }
            }

            if ( !bDPSort )
            {
                // no valid sorting in a DataPilot table -> disallow
                if ( bDragRect )
                {
                    bDragRect = sal_False;
                    UpdateDragRectOverlay();
                }
                return DND_ACTION_NONE;
            }
        }
        else if ( aState.mnState & KEY_MOD2 )
        {
            if ( pThisDoc == pSourceDoc && nTab == aSourceRange.aStart.Tab() )
            {
                long nDeltaX = labs( static_cast< long >( nNewDragX - nSourceStartX ) );
                long nDeltaY = labs( static_cast< long >( nNewDragY - nSourceStartY ) );
                if ( nDeltaX <= nDeltaY )
                {
                    eDragInsertMode = INS_CELLSDOWN;
                }
                else
                {
                    eDragInsertMode = INS_CELLSRIGHT;
                }

                if ( ( eDragInsertMode == INS_CELLSDOWN && nNewDragY <= nSourceEndY &&
                       ( nNewDragX + nSizeX - 1 ) >= nSourceStartX && nNewDragX <= nSourceEndX &&
                       ( nNewDragX != nSourceStartX || nNewDragY >= nSourceStartY ) ) ||
                     ( eDragInsertMode == INS_CELLSRIGHT && nNewDragX <= nSourceEndX &&
                       ( nNewDragY + nSizeY - 1 ) >= nSourceStartY && nNewDragY <= nSourceEndY &&
                       ( nNewDragY != nSourceStartY || nNewDragX >= nSourceStartX ) ) )
                {
                    if ( bDragRect )
                    {
                        bDragRect = sal_False;
                        UpdateDragRectOverlay();
                    }
                    return DND_ACTION_NONE;
                }
            }
            else
            {
                if ( static_cast< long >( nSizeX ) >= static_cast< long >( nSizeY ) )
                {
                    eDragInsertMode = INS_CELLSDOWN;

                }
                else
                {
                    eDragInsertMode = INS_CELLSRIGHT;
                }
            }
        }

		if ( nNewDragX != (SCsCOL) nDragStartX || nNewDragY != (SCsROW) nDragStartY ||
			 nDragStartX+nSizeX-1 != nDragEndX || nDragStartY+nSizeY-1 != nDragEndY ||
			 !bDragRect || eDragInsertMode != meDragInsertMode )
		{
			// if (bDragRect)
			//	pViewData->GetView()->DrawDragRect( nDragStartX, nDragStartY, nDragEndX, nDragEndY, eWhich );

			nDragStartX = nNewDragX;
			nDragStartY = nNewDragY;
			nDragEndX = nDragStartX+nSizeX-1;
			nDragEndY = nDragStartY+nSizeY-1;
			bDragRect = sal_True;
            meDragInsertMode = eDragInsertMode;

			// pViewData->GetView()->DrawDragRect( nDragStartX, nDragStartY, nDragEndX, nDragEndY, eWhich );

            UpdateDragRectOverlay();

			//	show target position as tip help
#if 0
			if (Help::IsQuickHelpEnabled())
			{
				ScRange aRange( nDragStartX, nDragStartY, nTab, nDragEndX, nDragEndY, nTab );
				String aHelpStr;
				aRange.Format( aHelpStr, SCA_VALID );	// non-3D

				Point aPos = Pointer::GetPosPixel();
				sal_uInt16 nAlign = QUICKHELP_BOTTOM|QUICKHELP_RIGHT;
				Rectangle aRect( aPos, aPos );
				Help::ShowQuickHelp(aRect, aHelpStr, nAlign);
			}
#endif
		}
	}

	return rEvt.mnAction;
}

sal_Int8 ScGridWindow::AcceptDrop( const AcceptDropEvent& rEvt )
{
	const ScDragData& rData = SC_MOD()->GetDragData();
	if ( rEvt.mbLeaving )
	{
		DrawMarkDropObj( NULL );
		if ( rData.pCellTransfer )
			return AcceptPrivateDrop( rEvt );	// hide drop marker for internal D&D
		else
			return rEvt.mnAction;
	}

	if ( pViewData->GetDocShell()->IsReadOnly() )
		return DND_ACTION_NONE;


	sal_Int8 nRet = DND_ACTION_NONE;

	if (rData.pCellTransfer)
	{
		ScRange aSource = rData.pCellTransfer->GetRange();
		if ( aSource.aStart.Col() != 0 || aSource.aEnd.Col() != MAXCOL ||
			 aSource.aStart.Row() != 0 || aSource.aEnd.Row() != MAXROW )
			DropScroll( rEvt.maPosPixel );

		nRet = AcceptPrivateDrop( rEvt );
	}
	else
	{
		if ( rData.aLinkDoc.Len() )
		{
			String aThisName;
			ScDocShell* pDocSh = pViewData->GetDocShell();
			if (pDocSh && pDocSh->HasName())
				aThisName = pDocSh->GetMedium()->GetName();

			if ( rData.aLinkDoc != aThisName )
				nRet = rEvt.mnAction;
		}
		else if (rData.aJumpTarget.Len())
		{
			//	internal bookmarks (from Navigator)
			//	local jumps from an unnamed document are possible only within a document

			if ( !rData.pJumpLocalDoc || rData.pJumpLocalDoc == pViewData->GetDocument() )
				nRet = rEvt.mnAction;
		}
		else
		{
			sal_Int8 nMyAction = rEvt.mnAction;
			
			// clear DND_ACTION_LINK when other actions are set. The usage below cannot handle
			// multiple set values
			if((nMyAction & DND_ACTION_LINK) && (nMyAction & (DND_ACTION_COPYMOVE)))
			{
			    nMyAction &= ~DND_ACTION_LINK;
			}

			if ( !rData.pDrawTransfer ||
					!IsMyModel(rData.pDrawTransfer->GetDragSourceView()) )		// drawing within the document
				if ( rEvt.mbDefault && nMyAction == DND_ACTION_MOVE )
					nMyAction = DND_ACTION_COPY;

			ScDocument* pThisDoc = pViewData->GetDocument();
			SdrObject* pHitObj = pThisDoc->GetObjectAtPoint(
						pViewData->GetTabNo(), PixelToLogic(rEvt.maPosPixel) );
			if ( pHitObj && nMyAction == DND_ACTION_LINK ) // && !rData.pDrawTransfer )
			{
				if ( IsDropFormatSupported(SOT_FORMATSTR_ID_SVXB)
					|| IsDropFormatSupported(SOT_FORMAT_GDIMETAFILE)
					|| IsDropFormatSupported(SOT_FORMATSTR_ID_PNG)
					|| IsDropFormatSupported(SOT_FORMAT_BITMAP) )
				{
					//	graphic dragged onto drawing object
					DrawMarkDropObj( pHitObj );
					nRet = nMyAction;
				}
			}
			if (!nRet)
				DrawMarkDropObj( NULL );

			if (!nRet)
			{
				switch ( nMyAction )
				{
					case DND_ACTION_COPY:
					case DND_ACTION_MOVE:
					case DND_ACTION_COPYMOVE:
						{
							sal_Bool bMove = ( nMyAction == DND_ACTION_MOVE );
							if ( IsDropFormatSupported( SOT_FORMATSTR_ID_EMBED_SOURCE ) ||
								 IsDropFormatSupported( SOT_FORMATSTR_ID_LINK_SOURCE ) ||
								 IsDropFormatSupported( SOT_FORMATSTR_ID_EMBED_SOURCE_OLE ) ||
								 IsDropFormatSupported( SOT_FORMATSTR_ID_LINK_SOURCE_OLE ) ||
								 IsDropFormatSupported( SOT_FORMATSTR_ID_EMBEDDED_OBJ_OLE ) ||
								 IsDropFormatSupported( SOT_FORMAT_STRING ) ||
								 IsDropFormatSupported( SOT_FORMATSTR_ID_SYLK ) ||
								 IsDropFormatSupported( SOT_FORMATSTR_ID_LINK ) ||
								 IsDropFormatSupported( SOT_FORMATSTR_ID_HTML ) ||
								 IsDropFormatSupported( SOT_FORMATSTR_ID_HTML_SIMPLE ) ||
								 IsDropFormatSupported( SOT_FORMATSTR_ID_DIF ) ||
								 IsDropFormatSupported( SOT_FORMATSTR_ID_DRAWING ) ||
								 IsDropFormatSupported( SOT_FORMATSTR_ID_SVXB ) ||
								 IsDropFormatSupported( SOT_FORMAT_RTF ) ||
								 IsDropFormatSupported( SOT_FORMAT_GDIMETAFILE ) ||
								 IsDropFormatSupported( SOT_FORMATSTR_ID_PNG ) ||
								 IsDropFormatSupported( SOT_FORMAT_BITMAP ) ||
								 IsDropFormatSupported( SOT_FORMATSTR_ID_SBA_DATAEXCHANGE ) ||
								 IsDropFormatSupported( SOT_FORMATSTR_ID_SBA_FIELDDATAEXCHANGE ) ||
								 ( !bMove && (
                                    IsDropFormatSupported( SOT_FORMAT_FILE_LIST ) ||
								 	IsDropFormatSupported( SOT_FORMAT_FILE ) ||
								 	IsDropFormatSupported( SOT_FORMATSTR_ID_SOLK ) ||
								 	IsDropFormatSupported( SOT_FORMATSTR_ID_UNIFORMRESOURCELOCATOR ) ||
								 	IsDropFormatSupported( SOT_FORMATSTR_ID_NETSCAPE_BOOKMARK ) ||
								 	IsDropFormatSupported( SOT_FORMATSTR_ID_FILEGRPDESCRIPTOR ) ) ) )
							{
								nRet = nMyAction;
							}
						}
						break;
					case DND_ACTION_LINK:
						if ( IsDropFormatSupported( SOT_FORMATSTR_ID_LINK_SOURCE ) ||
							 IsDropFormatSupported( SOT_FORMATSTR_ID_LINK_SOURCE_OLE ) ||
							 IsDropFormatSupported( SOT_FORMATSTR_ID_LINK ) ||
                             IsDropFormatSupported( SOT_FORMAT_FILE_LIST ) ||
							 IsDropFormatSupported( SOT_FORMAT_FILE ) ||
							 IsDropFormatSupported( SOT_FORMATSTR_ID_SOLK ) ||
							 IsDropFormatSupported( SOT_FORMATSTR_ID_UNIFORMRESOURCELOCATOR ) ||
							 IsDropFormatSupported( SOT_FORMATSTR_ID_NETSCAPE_BOOKMARK ) ||
							 IsDropFormatSupported( SOT_FORMATSTR_ID_FILEGRPDESCRIPTOR ) )
						{
							nRet = nMyAction;
						}
						break;
				}

                if ( nRet )
                {
                    // Simple check for protection: It's not known here if the drop will result
                    // in cells or drawing objects (some formats can be both) and how many cells
                    // the result will be. But if IsFormatEditable for the drop cell position
                    // is sal_False (ignores matrix formulas), nothing can be pasted, so the drop
                    // can already be rejected here.

                    Point aPos = rEvt.maPosPixel;
                    SCsCOL nPosX;
                    SCsROW nPosY;
                    pViewData->GetPosFromPixel( aPos.X(), aPos.Y(), eWhich, nPosX, nPosY );
                    SCTAB nTab = pViewData->GetTabNo();
                    ScDocument* pDoc = pViewData->GetDocument();

                    ScEditableTester aTester( pDoc, nTab, nPosX,nPosY, nPosX,nPosY );
                    if ( !aTester.IsFormatEditable() )
                        nRet = DND_ACTION_NONE;             // forbidden
                }
			}
		}

		//	scroll only for accepted formats
		if (nRet)
			DropScroll( rEvt.maPosPixel );
	}

	return nRet;
}

sal_uLong lcl_GetDropFormatId( const uno::Reference<datatransfer::XTransferable>& xTransfer, bool bPreferText = false )
{
	TransferableDataHelper aDataHelper( xTransfer );

	if ( !aDataHelper.HasFormat( SOT_FORMATSTR_ID_SBA_DATAEXCHANGE ) )
	{
		//	use bookmark formats if no sba is present

		if ( aDataHelper.HasFormat( SOT_FORMATSTR_ID_SOLK ) )
			return SOT_FORMATSTR_ID_SOLK;
		else if ( aDataHelper.HasFormat( SOT_FORMATSTR_ID_UNIFORMRESOURCELOCATOR ) )
			return SOT_FORMATSTR_ID_UNIFORMRESOURCELOCATOR;
		else if ( aDataHelper.HasFormat( SOT_FORMATSTR_ID_NETSCAPE_BOOKMARK ) )
			return SOT_FORMATSTR_ID_NETSCAPE_BOOKMARK;
		else if ( aDataHelper.HasFormat( SOT_FORMATSTR_ID_FILEGRPDESCRIPTOR ) )
			return SOT_FORMATSTR_ID_FILEGRPDESCRIPTOR;
	}

	sal_uLong nFormatId = 0;
	if ( aDataHelper.HasFormat( SOT_FORMATSTR_ID_DRAWING ) )
		nFormatId = SOT_FORMATSTR_ID_DRAWING;
	else if ( aDataHelper.HasFormat( SOT_FORMATSTR_ID_SVXB ) )
		nFormatId = SOT_FORMATSTR_ID_SVXB;
	else if ( aDataHelper.HasFormat( SOT_FORMATSTR_ID_EMBED_SOURCE ) )
	{
		//	If it's a Writer object, insert RTF instead of OLE

		sal_Bool bDoRtf = sal_False;
		SotStorageStreamRef xStm;
		TransferableObjectDescriptor aObjDesc;
		if( aDataHelper.GetTransferableObjectDescriptor( SOT_FORMATSTR_ID_OBJECTDESCRIPTOR, aObjDesc ) &&
			aDataHelper.GetSotStorageStream( SOT_FORMATSTR_ID_EMBED_SOURCE, xStm ) )
		{
			SotStorageRef xStore( new SotStorage( *xStm ) );
			bDoRtf = ( ( aObjDesc.maClassName == SvGlobalName( SO3_SW_CLASSID ) ||
						 aObjDesc.maClassName == SvGlobalName( SO3_SWWEB_CLASSID ) )
					   && aDataHelper.HasFormat( SOT_FORMAT_RTF ) );
		}
		if ( bDoRtf )
			nFormatId = FORMAT_RTF;
		else
			nFormatId = SOT_FORMATSTR_ID_EMBED_SOURCE;
	}
	else if ( aDataHelper.HasFormat( SOT_FORMATSTR_ID_LINK_SOURCE ) )
		nFormatId = SOT_FORMATSTR_ID_LINK_SOURCE;
	else if ( aDataHelper.HasFormat( SOT_FORMATSTR_ID_SBA_DATAEXCHANGE ) )
		nFormatId = SOT_FORMATSTR_ID_SBA_DATAEXCHANGE;
	else if ( aDataHelper.HasFormat( SOT_FORMATSTR_ID_SBA_FIELDDATAEXCHANGE ) )
		nFormatId = SOT_FORMATSTR_ID_SBA_FIELDDATAEXCHANGE;
    else if ( aDataHelper.HasFormat( SOT_FORMATSTR_ID_BIFF_8 ) )
        nFormatId = SOT_FORMATSTR_ID_BIFF_8;
	else if ( aDataHelper.HasFormat( SOT_FORMATSTR_ID_BIFF_5 ) )
		nFormatId = SOT_FORMATSTR_ID_BIFF_5;
	else if ( aDataHelper.HasFormat( SOT_FORMATSTR_ID_EMBED_SOURCE_OLE ) )
		nFormatId = SOT_FORMATSTR_ID_EMBED_SOURCE_OLE;
	else if ( aDataHelper.HasFormat( SOT_FORMATSTR_ID_EMBEDDED_OBJ_OLE ) )
		nFormatId = SOT_FORMATSTR_ID_EMBEDDED_OBJ_OLE;
	else if ( aDataHelper.HasFormat( SOT_FORMATSTR_ID_LINK_SOURCE_OLE ) )
		nFormatId = SOT_FORMATSTR_ID_LINK_SOURCE_OLE;
	else if ( aDataHelper.HasFormat( SOT_FORMAT_RTF ) )
		nFormatId = SOT_FORMAT_RTF;
	else if ( aDataHelper.HasFormat( SOT_FORMATSTR_ID_HTML ) )
		nFormatId = SOT_FORMATSTR_ID_HTML;
	else if ( aDataHelper.HasFormat( SOT_FORMATSTR_ID_HTML_SIMPLE ) )
		nFormatId = SOT_FORMATSTR_ID_HTML_SIMPLE;
	else if ( aDataHelper.HasFormat( SOT_FORMATSTR_ID_SYLK ) )
		nFormatId = SOT_FORMATSTR_ID_SYLK;
	else if ( aDataHelper.HasFormat( SOT_FORMATSTR_ID_LINK ) )
		nFormatId = SOT_FORMATSTR_ID_LINK;
	else if ( bPreferText && aDataHelper.HasFormat( SOT_FORMAT_STRING ) ) // #i86734# the behaviour introduced in #i62773# is wrong when pasting
		nFormatId = SOT_FORMAT_STRING;
    else if ( aDataHelper.HasFormat( SOT_FORMAT_FILE_LIST ) )
        nFormatId = SOT_FORMAT_FILE_LIST;
    else if ( aDataHelper.HasFormat( SOT_FORMAT_FILE ) )    // #i62773# FILE_LIST/FILE before STRING (Unix file managers)
        nFormatId = SOT_FORMAT_FILE;
	else if ( aDataHelper.HasFormat( SOT_FORMAT_STRING ) )
		nFormatId = SOT_FORMAT_STRING;
	else if ( aDataHelper.HasFormat( SOT_FORMAT_GDIMETAFILE ) )
		nFormatId = SOT_FORMAT_GDIMETAFILE;
	else if ( aDataHelper.HasFormat( SOT_FORMATSTR_ID_PNG ) )
		nFormatId = SOT_FORMATSTR_ID_PNG;
	else if ( aDataHelper.HasFormat( SOT_FORMAT_BITMAP ) )
		nFormatId = SOT_FORMAT_BITMAP;

	return nFormatId;
}

sal_uLong lcl_GetDropLinkId( const uno::Reference<datatransfer::XTransferable>& xTransfer )
{
	TransferableDataHelper aDataHelper( xTransfer );

	sal_uLong nFormatId = 0;
	if ( aDataHelper.HasFormat( SOT_FORMATSTR_ID_LINK_SOURCE ) )
		nFormatId = SOT_FORMATSTR_ID_LINK_SOURCE;
	else if ( aDataHelper.HasFormat( SOT_FORMATSTR_ID_LINK_SOURCE_OLE ) )
		nFormatId = SOT_FORMATSTR_ID_LINK_SOURCE_OLE;
	else if ( aDataHelper.HasFormat( SOT_FORMATSTR_ID_LINK ) )
		nFormatId = SOT_FORMATSTR_ID_LINK;
    else if ( aDataHelper.HasFormat( SOT_FORMAT_FILE_LIST ) )
        nFormatId = SOT_FORMAT_FILE_LIST;
	else if ( aDataHelper.HasFormat( SOT_FORMAT_FILE ) )
		nFormatId = SOT_FORMAT_FILE;
	else if ( aDataHelper.HasFormat( SOT_FORMATSTR_ID_SOLK ) )
		nFormatId = SOT_FORMATSTR_ID_SOLK;
	else if ( aDataHelper.HasFormat( SOT_FORMATSTR_ID_UNIFORMRESOURCELOCATOR ) )
		nFormatId = SOT_FORMATSTR_ID_UNIFORMRESOURCELOCATOR;
	else if ( aDataHelper.HasFormat( SOT_FORMATSTR_ID_NETSCAPE_BOOKMARK ) )
		nFormatId = SOT_FORMATSTR_ID_NETSCAPE_BOOKMARK;
	else if ( aDataHelper.HasFormat( SOT_FORMATSTR_ID_FILEGRPDESCRIPTOR ) )
		nFormatId = SOT_FORMATSTR_ID_FILEGRPDESCRIPTOR;

	return nFormatId;
}


sal_Int8 ScGridWindow::ExecutePrivateDrop( const ExecuteDropEvent& rEvt )
{
	// hide drop marker
	// if (bDragRect)
	//	pViewData->GetView()->DrawDragRect( nDragStartX, nDragStartY, nDragEndX, nDragEndY, eWhich );
	bDragRect = sal_False;
    UpdateDragRectOverlay();

	ScModule* pScMod = SC_MOD();
	const ScDragData& rData = pScMod->GetDragData();

	return DropTransferObj( rData.pCellTransfer, nDragStartX, nDragStartY,
								PixelToLogic(rEvt.maPosPixel), rEvt.mnAction );
}

sal_Int8 ScGridWindow::DropTransferObj( ScTransferObj* pTransObj, SCCOL nDestPosX, SCROW nDestPosY,
										const Point& rLogicPos, sal_Int8 nDndAction )
{
	if ( !pTransObj )
		return 0;

	ScDocument* pSourceDoc = pTransObj->GetSourceDocument();
    ScDocShell* pDocSh     = pViewData->GetDocShell();
	ScDocument* pThisDoc   = pViewData->GetDocument();
	ScViewFunc* pView	   = pViewData->GetView();
	SCTAB       nThisTab   = pViewData->GetTabNo();
	sal_uInt16 nFlags = pTransObj->GetDragSourceFlags();

	sal_Bool bIsNavi = ( nFlags & SC_DROP_NAVIGATOR ) != 0;
	sal_Bool bIsMove = ( nDndAction == DND_ACTION_MOVE && !bIsNavi );

    // workaround for wrong nDndAction on Windows when pressing solely
    // the Alt key during drag and drop;
    // can be removed after #i79215# has been fixed
    if ( meDragInsertMode != INS_NONE )
    {
        bIsMove = ( nDndAction & DND_ACTION_MOVE && !bIsNavi );
    }

	sal_Bool bIsLink = ( nDndAction == DND_ACTION_LINK );

	ScRange aSource = pTransObj->GetRange();

	//	only use visible tab from source range - when dragging within one table,
	//	all selected tables at the time of dropping are used (handled in MoveBlockTo)
	SCTAB nSourceTab = pTransObj->GetVisibleTab();
	aSource.aStart.SetTab( nSourceTab );
	aSource.aEnd.SetTab( nSourceTab );

    SCCOL nSizeX = aSource.aEnd.Col() - aSource.aStart.Col() + 1;
    SCROW nSizeY = (bIsMove ? (aSource.aEnd.Row() - aSource.aStart.Row() + 1) :
            pTransObj->GetNonFilteredRows());   // copy/link: no filtered rows
    ScRange aDest( nDestPosX, nDestPosY, nThisTab,
                   nDestPosX + nSizeX - 1, nDestPosY + nSizeY - 1, nThisTab );


    /* NOTE: AcceptPrivateDrop() already checked for filtered conditions during
     * dragging and adapted drawing of the selection frame. We check here
     * (again) because this may actually also be called from PasteSelection(),
     * we would have to duplicate determination of flags and destination range
     * and would lose the context of the "filtered destination is OK" cases
     * below, which is already awkward enough as is. */

    // Don't move filtered source.
    bool bFiltered = (bIsMove && pTransObj->HasFilteredRows());
    if (!bFiltered)
    {
        if (pSourceDoc != pThisDoc && ((nFlags & SC_DROP_TABLE) ||
                    (!bIsLink && meDragInsertMode == INS_NONE)))
        {
            // Nothing. Either entire sheet to be dropped, or the one case
            // where PasteFromClip() is to be called that handles a filtered
            // destination itself. Drag-copy from another document without
            // inserting cells.
        }
        else
            // Don't copy or move to filtered destination.
            bFiltered = ScViewUtil::HasFiltered( aDest, pThisDoc);
    }

	sal_Bool bDone = sal_False;

	if (!bFiltered && pSourceDoc == pThisDoc)
	{
		if ( nFlags & SC_DROP_TABLE )			// whole sheet?
		{
			if ( pThisDoc->IsDocEditable() )
			{
				SCTAB nSrcTab = aSource.aStart.Tab();
				pViewData->GetDocShell()->MoveTable( nSrcTab, nThisTab, !bIsMove, sal_True );	// with Undo
				pView->SetTabNo( nThisTab, sal_True );
				bDone = sal_True;
			}
		}
		else										// move/copy block
		{
			String aChartName;
			if (pThisDoc->HasChartAtPoint( nThisTab, rLogicPos, &aChartName ))
			{
				String aRangeName;
				aSource.Format( aRangeName, SCR_ABS_3D, pThisDoc );
				SfxStringItem aNameItem( SID_CHART_NAME, aChartName );
				SfxStringItem aRangeItem( SID_CHART_SOURCE, aRangeName );
				sal_uInt16 nId = bIsMove ? SID_CHART_SOURCE : SID_CHART_ADDSOURCE;
				pViewData->GetDispatcher().Execute( nId, SFX_CALLMODE_ASYNCHRON | SFX_CALLMODE_RECORD,
											&aRangeItem, &aNameItem, (void*) NULL );
				bDone = sal_True;
			}
            else if ( pThisDoc->GetDPAtCursor( nDestPosX, nDestPosY, nThisTab ) )
            {
                // drop on DataPilot table: try to sort, fail if that isn't possible

                ScAddress aDestPos( nDestPosX, nDestPosY, nThisTab );
                if ( aDestPos != aSource.aStart )
                    bDone = pViewData->GetView()->DataPilotMove( aSource, aDestPos );
                else
                    bDone = sal_True;   // same position: nothing
            }
			else if ( nDestPosX != aSource.aStart.Col() || nDestPosY != aSource.aStart.Row() ||
						nSourceTab != nThisTab )
			{
                String aUndo = ScGlobal::GetRscString( bIsMove ? STR_UNDO_MOVE : STR_UNDO_COPY );
                pDocSh->GetUndoManager()->EnterListAction( aUndo, aUndo );

                bDone = sal_True;
                if ( meDragInsertMode != INS_NONE )
                {
                    // call with bApi = sal_True to avoid error messages in drop handler
                    bDone = pDocSh->GetDocFunc().InsertCells( aDest, NULL, meDragInsertMode, sal_True /*bRecord*/, sal_True /*bApi*/, sal_True /*bPartOfPaste*/ );
                    if ( bDone )
                    {
                        if ( nThisTab == nSourceTab )
                        {
                            if ( meDragInsertMode == INS_CELLSDOWN &&
                                 nDestPosX == aSource.aStart.Col() && nDestPosY < aSource.aStart.Row() )
                            {
                                bDone = aSource.Move( 0, nSizeY, 0, pSourceDoc );
                            }
                            else if ( meDragInsertMode == INS_CELLSRIGHT &&
                                      nDestPosY == aSource.aStart.Row() && nDestPosX < aSource.aStart.Col() )
                            {
                                bDone = aSource.Move( nSizeX, 0, 0, pSourceDoc );
                            }
                        }
                        pDocSh->UpdateOle( pViewData );
                        pView->CellContentChanged();
                    }
                }

                if ( bDone )
                {
                    if ( bIsLink )
                    {
                        // call with bApi = sal_True to avoid error messages in drop handler
                        bDone = pView->LinkBlock( aSource, aDest.aStart, sal_True /*bApi*/ );
                    }
                    else
                    {
                        // call with bApi = sal_True to avoid error messages in drop handler
                        bDone = pView->MoveBlockTo( aSource, aDest.aStart, bIsMove, sal_True /*bRecord*/, sal_True /*bPaint*/, sal_True /*bApi*/ );
                    }
                }

                if ( bDone && meDragInsertMode != INS_NONE && bIsMove && nThisTab == nSourceTab )
                {
                    DelCellCmd eCmd = DEL_NONE;
                    if ( meDragInsertMode == INS_CELLSDOWN )
                    {
                        eCmd = DEL_CELLSUP;
                    }
                    else if ( meDragInsertMode == INS_CELLSRIGHT )
                    {
                        eCmd = DEL_CELLSLEFT;
                    }

                    if ( ( eCmd == DEL_CELLSUP  && nDestPosX == aSource.aStart.Col() ) ||
                         ( eCmd == DEL_CELLSLEFT && nDestPosY == aSource.aStart.Row() ) )
                    {
                        // call with bApi = sal_True to avoid error messages in drop handler
                        bDone = pDocSh->GetDocFunc().DeleteCells( aSource, NULL, eCmd, sal_True /*bRecord*/, sal_True /*bApi*/ );
                        if ( bDone )
                        {
                            if ( eCmd == DEL_CELLSUP && nDestPosY > aSource.aEnd.Row() )
                            {
                                bDone = aDest.Move( 0, -nSizeY, 0, pThisDoc );
                            }
                            else if ( eCmd == DEL_CELLSLEFT && nDestPosX > aSource.aEnd.Col() )
                            {
                                bDone = aDest.Move( -nSizeX, 0, 0, pThisDoc );
                            }
                            pDocSh->UpdateOle( pViewData );
                            pView->CellContentChanged();
                        }
                    }
                }

                if ( bDone )
                {
                    pView->MarkRange( aDest, sal_False, sal_False );
                    pView->SetCursor( aDest.aEnd.Col(), aDest.aEnd.Row() );
                }

                pDocSh->GetUndoManager()->LeaveListAction();

				if (!bDone)
					Sound::Beep();	// instead of error message in drop handler
			}
			else
				bDone = sal_True;		// nothing to do
		}

		if (bDone)
			pTransObj->SetDragWasInternal();	// don't delete source in DragFinished
	}
	else if ( !bFiltered && pSourceDoc )						// between documents
	{
		if ( nFlags & SC_DROP_TABLE )			// copy/link sheets between documents
		{
			if ( pThisDoc->IsDocEditable() )
			{
				ScDocShell* pSrcShell = pTransObj->GetSourceDocShell();

				SCTAB nTabs[MAXTABCOUNT];

				ScMarkData	aMark		= pTransObj->GetSourceMarkData();
				SCTAB		nTabCount	= pSourceDoc->GetTableCount();
				SCTAB		nTabSelCount = 0;

				for(SCTAB i=0; i<nTabCount; i++)
				{
					if(aMark.GetTableSelect(i))
					{
						nTabs[nTabSelCount++]=i;
						for(SCTAB j=i+1;j<nTabCount;j++)
						{
							if((!pSourceDoc->IsVisible(j))&&(pSourceDoc->IsScenario(j)))
							{
								nTabs[nTabSelCount++]=j;
								i=j;
							}
							else break;
						}
					}
				}

				pView->ImportTables( pSrcShell,nTabSelCount, nTabs, bIsLink, nThisTab );
				bDone = sal_True;
			}
		}
		else if ( bIsLink )
		{
			//	as in PasteDDE
			//	(external references might be used instead?)

			SfxObjectShell* pSourceSh = pSourceDoc->GetDocumentShell();
			DBG_ASSERT(pSourceSh, "drag document has no shell");
			if (pSourceSh)
			{
                String aUndo = ScGlobal::GetRscString( STR_UNDO_COPY );
                pDocSh->GetUndoManager()->EnterListAction( aUndo, aUndo );

                bDone = sal_True;
                if ( meDragInsertMode != INS_NONE )
                {
                    // call with bApi = sal_True to avoid error messages in drop handler
                    bDone = pDocSh->GetDocFunc().InsertCells( aDest, NULL, meDragInsertMode, sal_True /*bRecord*/, sal_True /*bApi*/, sal_True /*bPartOfPaste*/ );
                    if ( bDone )
                    {
                        pDocSh->UpdateOle( pViewData );
                        pView->CellContentChanged();
                    }
                }

                if ( bDone )
                {
                    String aApp = Application::GetAppName();
                    String aTopic = pSourceSh->GetTitle( SFX_TITLE_FULLNAME );
                    String aItem;
                    aSource.Format( aItem, SCA_VALID | SCA_TAB_3D, pSourceDoc );

                    // TODO: we could define ocQuote for "
                    const String aQuote( '"' );
                    const String& sSep = ScCompiler::GetNativeSymbol( ocSep);
                    String aFormula( '=' );
                    aFormula += ScCompiler::GetNativeSymbol( ocDde);
                    aFormula += ScCompiler::GetNativeSymbol( ocOpen);
                    aFormula += aQuote;
                    aFormula += aApp;
                    aFormula += aQuote;
                    aFormula += sSep;
                    aFormula += aQuote;
                    aFormula += aTopic;
                    aFormula += aQuote;
                    aFormula += sSep;
                    aFormula += aQuote;
                    aFormula += aItem;
                    aFormula += aQuote;
                    aFormula += ScCompiler::GetNativeSymbol( ocClose);

                    pView->DoneBlockMode();
                    pView->InitBlockMode( nDestPosX, nDestPosY, nThisTab );
                    pView->MarkCursor( nDestPosX + nSizeX - 1,
                                       nDestPosY + nSizeY - 1, nThisTab );

                    pView->EnterMatrix( aFormula );

                    pView->MarkRange( aDest, sal_False, sal_False );
                    pView->SetCursor( aDest.aEnd.Col(), aDest.aEnd.Row() );
                }

                pDocSh->GetUndoManager()->LeaveListAction();
			}
		}
		else
		{
			//!	HasSelectedBlockMatrixFragment without selected sheet?
			//!	or don't start dragging on a part of a matrix

            String aUndo = ScGlobal::GetRscString( bIsMove ? STR_UNDO_MOVE : STR_UNDO_COPY );
            pDocSh->GetUndoManager()->EnterListAction( aUndo, aUndo );

            bDone = sal_True;
            if ( meDragInsertMode != INS_NONE )
            {
                // call with bApi = sal_True to avoid error messages in drop handler
                bDone = pDocSh->GetDocFunc().InsertCells( aDest, NULL, meDragInsertMode, sal_True /*bRecord*/, sal_True /*bApi*/, sal_True /*bPartOfPaste*/ );
                if ( bDone )
                {
                    pDocSh->UpdateOle( pViewData );
                    pView->CellContentChanged();
                }
            }

            if ( bDone )
            {
                pView->Unmark();  // before SetCursor, so CheckSelectionTransfer isn't called with a selection
                pView->SetCursor( nDestPosX, nDestPosY );
                bDone = pView->PasteFromClip( IDF_ALL, pTransObj->GetDocument() );  // clip-doc
                if ( bDone )
                {
                    pView->MarkRange( aDest, sal_False, sal_False );
                    pView->SetCursor( aDest.aEnd.Col(), aDest.aEnd.Row() );
                }
            }

            pDocSh->GetUndoManager()->LeaveListAction();

			//	no longer call ResetMark here - the inserted block has been selected
			//	and may have been copied to primary selection
		}
	}

	sal_Int8 nRet = bDone ? nDndAction : DND_ACTION_NONE;
	return nRet;
}

sal_Int8 ScGridWindow::ExecuteDrop( const ExecuteDropEvent& rEvt )
{
	DrawMarkDropObj( NULL );	// drawing layer

	ScModule* pScMod = SC_MOD();
	const ScDragData& rData = pScMod->GetDragData();
	if (rData.pCellTransfer)
		return ExecutePrivateDrop( rEvt );

	Point aPos = rEvt.maPosPixel;

	if ( rData.aLinkDoc.Len() )
	{
		//	try to insert a link

		sal_Bool bOk = sal_True;
		String aThisName;
		ScDocShell* pDocSh = pViewData->GetDocShell();
		if (pDocSh && pDocSh->HasName())
			aThisName = pDocSh->GetMedium()->GetName();

		if ( rData.aLinkDoc == aThisName )				// error - no link within a document
			bOk = sal_False;
		else
		{
			ScViewFunc* pView = pViewData->GetView();
			if ( rData.aLinkTable.Len() )
				pView->InsertTableLink( rData.aLinkDoc, EMPTY_STRING, EMPTY_STRING,
										rData.aLinkTable );
			else if ( rData.aLinkArea.Len() )
			{
				SCsCOL	nPosX;
				SCsROW	nPosY;
				pViewData->GetPosFromPixel( aPos.X(), aPos.Y(), eWhich, nPosX, nPosY );
				pView->MoveCursorAbs( nPosX, nPosY, SC_FOLLOW_NONE, sal_False, sal_False );

				pView->InsertAreaLink( rData.aLinkDoc, EMPTY_STRING, EMPTY_STRING,
										rData.aLinkArea, 0 );
			}
			else
			{
				DBG_ERROR("drop with link: no sheet nor area");
				bOk = sal_False;
			}
		}

		return bOk ? rEvt.mnAction : DND_ACTION_NONE;			// don't try anything else
	}

	Point aLogicPos = PixelToLogic(aPos);
	sal_Bool bIsLink = ( rEvt.mnAction == DND_ACTION_LINK );

	if (!bIsLink && rData.pDrawTransfer)
	{
		sal_uInt16 nFlags = rData.pDrawTransfer->GetDragSourceFlags();

		sal_Bool bIsNavi = ( nFlags & SC_DROP_NAVIGATOR ) != 0;
		sal_Bool bIsMove = ( rEvt.mnAction == DND_ACTION_MOVE && !bIsNavi );

		bPasteIsMove = bIsMove;

		pViewData->GetView()->PasteDraw( aLogicPos, rData.pDrawTransfer->GetModel() );

		if (bPasteIsMove)
			rData.pDrawTransfer->SetDragWasInternal();
		bPasteIsMove = sal_False;

		return rEvt.mnAction;
	}


	SCsCOL	nPosX;
	SCsROW	nPosY;
	pViewData->GetPosFromPixel( aPos.X(), aPos.Y(), eWhich, nPosX, nPosY );

	if (rData.aJumpTarget.Len())
	{
		//	internal bookmark (from Navigator)
		//	bookmark clipboard formats are in PasteScDataObject

		if ( !rData.pJumpLocalDoc || rData.pJumpLocalDoc == pViewData->GetDocument() )
		{
			pViewData->GetViewShell()->InsertBookmark( rData.aJumpText, rData.aJumpTarget,
														nPosX, nPosY );
			return rEvt.mnAction;
		}
	}

	ScDocument* pThisDoc = pViewData->GetDocument();
	SdrObject* pHitObj = pThisDoc->GetObjectAtPoint( pViewData->GetTabNo(), PixelToLogic(aPos) );
	if ( pHitObj && bIsLink )
	{
		//	dropped on drawing object
		//	PasteOnDrawObjectLinked checks for valid formats
		if ( pViewData->GetView()->PasteOnDrawObjectLinked( rEvt.maDropEvent.Transferable, *pHitObj ) )
			return rEvt.mnAction;
	}

	sal_Bool bDone = sal_False;

	sal_uLong nFormatId = bIsLink ?
						lcl_GetDropLinkId( rEvt.maDropEvent.Transferable ) :
						lcl_GetDropFormatId( rEvt.maDropEvent.Transferable );
	if ( nFormatId )
	{
        pScMod->SetInExecuteDrop( sal_True );   // #i28468# prevent error messages from PasteDataFormat
		bPasteIsDrop = sal_True;
		bDone = pViewData->GetView()->PasteDataFormat(
					nFormatId, rEvt.maDropEvent.Transferable, nPosX, nPosY, &aLogicPos, bIsLink );
		bPasteIsDrop = sal_False;
        pScMod->SetInExecuteDrop( sal_False );
	}

	sal_Int8 nRet = bDone ? rEvt.mnAction : DND_ACTION_NONE;
	return nRet;
}

//--------------------------------------------------------

void ScGridWindow::PasteSelection( const Point& rPosPixel )
{
	Point aLogicPos = PixelToLogic( rPosPixel );

	SCsCOL	nPosX;
	SCsROW	nPosY;
	pViewData->GetPosFromPixel( rPosPixel.X(), rPosPixel.Y(), eWhich, nPosX, nPosY );

	ScSelectionTransferObj* pOwnSelection = SC_MOD()->GetSelectionTransfer();
	if ( pOwnSelection )
	{
		//	within Calc

		ScTransferObj* pCellTransfer = pOwnSelection->GetCellData();
		if ( pCellTransfer )
		{
			// keep a reference to the data in case the selection is changed during paste
			uno::Reference<datatransfer::XTransferable> xRef( pCellTransfer );
			DropTransferObj( pCellTransfer, nPosX, nPosY, aLogicPos, DND_ACTION_COPY );
		}
		else
		{
			ScDrawTransferObj* pDrawTransfer = pOwnSelection->GetDrawData();
			if ( pDrawTransfer )
			{
				// keep a reference to the data in case the selection is changed during paste
				uno::Reference<datatransfer::XTransferable> xRef( pDrawTransfer );

				//	#96821# bSameDocClipboard argument for PasteDraw is needed
				//	because only DragData is checked directly inside PasteDraw
				pViewData->GetView()->PasteDraw( aLogicPos, pDrawTransfer->GetModel(), sal_False,
							pDrawTransfer->GetSourceDocID() == pViewData->GetDocument()->GetDocumentID() );
			}
		}
	}
	else
	{
		//	get selection from system

		TransferableDataHelper aDataHelper( TransferableDataHelper::CreateFromSelection( this ) );
		uno::Reference<datatransfer::XTransferable> xTransferable = aDataHelper.GetTransferable();
		if ( xTransferable.is() )
		{
			sal_uLong nFormatId = lcl_GetDropFormatId( xTransferable, true );
			if ( nFormatId )
			{
				bPasteIsDrop = sal_True;
				pViewData->GetView()->PasteDataFormat( nFormatId, xTransferable, nPosX, nPosY, &aLogicPos );
				bPasteIsDrop = sal_False;
			}
		}
	}
}

//--------------------------------------------------------

void ScGridWindow::UpdateEditViewPos()
{
	if (pViewData->HasEditView(eWhich))
	{
		EditView* pView;
		SCCOL nCol;
		SCROW nRow;
		pViewData->GetEditView( eWhich, pView, nCol, nRow );
		SCCOL nEndCol = pViewData->GetEditEndCol();
		SCROW nEndRow = pViewData->GetEditEndRow();

		//	hide EditView?

		sal_Bool bHide = ( nEndCol<pViewData->GetPosX(eHWhich) || nEndRow<pViewData->GetPosY(eVWhich) );
		if ( SC_MOD()->IsFormulaMode() )
			if ( pViewData->GetTabNo() != pViewData->GetRefTabNo() )
				bHide = sal_True;

		if (bHide)
		{
			Rectangle aRect = pView->GetOutputArea();
			long nHeight = aRect.Bottom() - aRect.Top();
			aRect.Top() = PixelToLogic(GetOutputSizePixel(), pViewData->GetLogicMode()).
							Height() * 2;
			aRect.Bottom() = aRect.Top() + nHeight;
			pView->SetOutputArea( aRect );
			pView->HideCursor();
		}
		else
		{
			// bForceToTop = sal_True for editing
			Rectangle aPixRect = pViewData->GetEditArea( eWhich, nCol, nRow, this, NULL, sal_True );
			Point aScrPos = PixelToLogic( aPixRect.TopLeft(), pViewData->GetLogicMode() );

			Rectangle aRect = pView->GetOutputArea();
			aRect.SetPos( aScrPos );
			pView->SetOutputArea( aRect );
			pView->ShowCursor();
		}
	}
}

void ScGridWindow::ScrollPixel( long nDifX, long nDifY )
{
	ClickExtern();
	HideNoteMarker();

	bIsInScroll = sal_True;
	//sal_Bool bXor=DrawBeforeScroll();

	SetMapMode(MAP_PIXEL);
	Scroll( nDifX, nDifY, SCROLL_CHILDREN );
	SetMapMode( GetDrawMapMode() );				// verschobenen MapMode erzeugen

	UpdateEditViewPos();

	DrawAfterScroll(); //bXor);
	bIsInScroll = sal_False;
}

// 	Formeln neu zeichnen -------------------------------------------------

void ScGridWindow::UpdateFormulas()
{
	if (pViewData->GetView()->IsMinimized())
		return;

	if ( nPaintCount )
	{
		//	nicht anfangen, verschachtelt zu painten
		//	(dann wuerde zumindest der MapMode nicht mehr stimmen)

		bNeedsRepaint = sal_True;			// -> am Ende vom Paint nochmal Invalidate auf alles
		aRepaintPixel = Rectangle();	// alles
		return;
	}

	SCCOL	nX1 = pViewData->GetPosX( eHWhich );
	SCROW	nY1 = pViewData->GetPosY( eVWhich );
	SCCOL	nX2 = nX1 + pViewData->VisibleCellsX( eHWhich );
	SCROW	nY2 = nY1 + pViewData->VisibleCellsY( eVWhich );

	if (nX2 > MAXCOL) nX2 = MAXCOL;
	if (nY2 > MAXROW) nY2 = MAXROW;

    // Draw( nX1, nY1, nX2, nY2, SC_UPDATE_CHANGED );

    // don't draw directly - instead use OutputData to find changed area and invalidate

    SCROW nPosY = nY1;

    ScDocShell* pDocSh = pViewData->GetDocShell();
    ScDocument* pDoc = pDocSh->GetDocument();
    SCTAB nTab = pViewData->GetTabNo();

    pDoc->ExtendHidden( nX1, nY1, nX2, nY2, nTab );

    Point aScrPos = pViewData->GetScrPos( nX1, nY1, eWhich );
    long nMirrorWidth = GetSizePixel().Width();
    sal_Bool bLayoutRTL = pDoc->IsLayoutRTL( nTab );
    // unused variable long nLayoutSign = bLayoutRTL ? -1 : 1;
    if ( bLayoutRTL )
    {
        long nEndPixel = pViewData->GetScrPos( nX2+1, nPosY, eWhich ).X();
        nMirrorWidth = aScrPos.X() - nEndPixel;
        aScrPos.X() = nEndPixel + 1;
    }

    long nScrX = aScrPos.X();
    long nScrY = aScrPos.Y();

    double nPPTX = pViewData->GetPPTX();
    double nPPTY = pViewData->GetPPTY();

    ScTableInfo aTabInfo;
    pDoc->FillInfo( aTabInfo, nX1, nY1, nX2, nY2, nTab, nPPTX, nPPTY, sal_False, sal_False );

    Fraction aZoomX = pViewData->GetZoomX();
    Fraction aZoomY = pViewData->GetZoomY();
    ScOutputData aOutputData( this, OUTTYPE_WINDOW, aTabInfo, pDoc, nTab,
                                nScrX, nScrY, nX1, nY1, nX2, nY2, nPPTX, nPPTY,
                                &aZoomX, &aZoomY );
    aOutputData.SetMirrorWidth( nMirrorWidth );

    aOutputData.FindChanged();

    // #122149# do not use old GetChangedArea() which used polygon-based Regions, but use
    // the region-band based new version; anyways, only rectangles are added
    Region aChangedRegion( aOutputData.GetChangedAreaRegion() );   // logic (PixelToLogic)
    if(!aChangedRegion.IsEmpty())
    {
        Invalidate(aChangedRegion);
    }

    CheckNeedsRepaint();    // #i90362# used to be called via Draw() - still needed here
}

void ScGridWindow::UpdateAutoFillMark(sal_Bool bMarked, const ScRange& rMarkRange)
{
	if ( bMarked != bAutoMarkVisible || ( bMarked && rMarkRange.aEnd != aAutoMarkPos ) )
	{
		HideCursor();
		bAutoMarkVisible = bMarked;
		if ( bMarked )
			aAutoMarkPos = rMarkRange.aEnd;
		ShowCursor();

        UpdateAutoFillOverlay();
	}
}

void ScGridWindow::UpdateListValPos( sal_Bool bVisible, const ScAddress& rPos )
{
    sal_Bool bOldButton = bListValButton;
    ScAddress aOldPos = aListValPos;

    bListValButton = bVisible;
    aListValPos = rPos;

    if ( bListValButton )
    {
        if ( !bOldButton || aListValPos != aOldPos )
        {
            // paint area of new button
            Invalidate( PixelToLogic( GetListValButtonRect( aListValPos ) ) );
        }
    }
    if ( bOldButton )
    {
        if ( !bListValButton || aListValPos != aOldPos )
        {
            // paint area of old button
            Invalidate( PixelToLogic( GetListValButtonRect( aOldPos ) ) );
        }
    }
}

void ScGridWindow::HideCursor()
{
	++nCursorHideCount;
	if (nCursorHideCount==1)
	{
		DrawCursor();
		DrawAutoFillMark();
	}
}

void ScGridWindow::ShowCursor()
{
	if (nCursorHideCount==0)
	{
		DBG_ERROR("zuviel ShowCursor");
		return;
	}

    if (nCursorHideCount==1)
    {
        // #i57745# Draw the cursor before setting the variable, in case the
        // GetSizePixel call from drawing causes a repaint (resize handler is called)
        DrawAutoFillMark();
        DrawCursor();
    }

	--nCursorHideCount;
}

void __EXPORT ScGridWindow::GetFocus()
{
	ScTabViewShell* pViewShell = pViewData->GetViewShell();
	pViewShell->GotFocus();
    pViewShell->SetFormShellAtTop( sal_False );     // focus in GridWindow -> FormShell no longer on top

    if (pViewShell->HasAccessibilityObjects())
		pViewShell->BroadcastAccessibility(ScAccGridWinFocusGotHint(eWhich, GetAccessible()));


	if ( !SC_MOD()->IsFormulaMode() )
	{
		pViewShell->UpdateInputHandler();
//		StopMarking();		// falls Dialog (Fehler), weil dann kein ButtonUp
							// MO: nur wenn nicht im RefInput-Modus
							//     -> GetFocus/MouseButtonDown-Reihenfolge
							//		  auf dem Mac
	}

	Window::GetFocus();
}

void __EXPORT ScGridWindow::LoseFocus()
{
	ScTabViewShell* pViewShell = pViewData->GetViewShell();
	pViewShell->LostFocus();

    if (pViewShell->HasAccessibilityObjects())
		pViewShell->BroadcastAccessibility(ScAccGridWinFocusLostHint(eWhich, GetAccessible()));

	Window::LoseFocus();
}

Point ScGridWindow::GetMousePosPixel() const  { return aCurMousePos; }

//------------------------------------------------------------------------

sal_Bool ScGridWindow::HitRangeFinder( const Point& rMouse, sal_Bool& rCorner,
								sal_uInt16* pIndex, SCsCOL* pAddX, SCsROW* pAddY )
{
	sal_Bool bFound = sal_False;
	ScInputHandler* pHdl = SC_MOD()->GetInputHdl( pViewData->GetViewShell() );
	if (pHdl)
	{
		ScRangeFindList* pRangeFinder = pHdl->GetRangeFindList();
		if ( pRangeFinder && !pRangeFinder->IsHidden() &&
				pRangeFinder->GetDocName() == pViewData->GetDocShell()->GetTitle() )
		{
			ScDocument* pDoc = pViewData->GetDocument();
			SCTAB nTab = pViewData->GetTabNo();
			sal_Bool bLayoutRTL = pDoc->IsLayoutRTL( nTab );
			long nLayoutSign = bLayoutRTL ? -1 : 1;

            SCsCOL nPosX;
            SCsROW nPosY;
			pViewData->GetPosFromPixel( rMouse.X(), rMouse.Y(), eWhich, nPosX, nPosY );
			//	zusammengefasste (einzeln/Bereich) ???
			ScAddress aAddr( nPosX, nPosY, nTab );

//			Point aNext = pViewData->GetScrPos( nPosX+1, nPosY+1, eWhich );

			Point aNext = pViewData->GetScrPos( nPosX, nPosY, eWhich, sal_True );
			long nSizeXPix;
			long nSizeYPix;
			pViewData->GetMergeSizePixel( nPosX, nPosY, nSizeXPix, nSizeYPix );
			aNext.X() += nSizeXPix * nLayoutSign;
			aNext.Y() += nSizeYPix;

			sal_Bool bCornerHor;
			if ( bLayoutRTL )
				bCornerHor = ( rMouse.X() >= aNext.X() && rMouse.X() <= aNext.X() + 8 );
			else
				bCornerHor = ( rMouse.X() >= aNext.X() - 8 && rMouse.X() <= aNext.X() );

			sal_Bool bCellCorner = ( bCornerHor &&
								 rMouse.Y() >= aNext.Y() - 8 && rMouse.Y() <= aNext.Y() );
			//	corner is hit only if the mouse is within the cell

			sal_uInt16 nCount = (sal_uInt16)pRangeFinder->Count();
			for (sal_uInt16 i=nCount; i;)
			{
				//	rueckwaerts suchen, damit der zuletzt gepaintete Rahmen gefunden wird
				--i;
				ScRangeFindData* pData = pRangeFinder->GetObject(i);
				if ( pData && pData->aRef.In(aAddr) )
				{
					if (pIndex)	*pIndex = i;
					if (pAddX)	*pAddX = nPosX - pData->aRef.aStart.Col();
					if (pAddY)	*pAddY = nPosY - pData->aRef.aStart.Row();
					bFound = sal_True;
					rCorner = ( bCellCorner && aAddr == pData->aRef.aEnd );
					break;
				}
			}
		}
	}
	return bFound;
}

#define SCE_TOP		1
#define SCE_BOTTOM	2
#define SCE_LEFT	4
#define SCE_RIGHT	8
#define SCE_ALL		15

void lcl_PaintOneRange( ScDocShell* pDocSh, const ScRange& rRange, sal_uInt16 nEdges )
{
	//	der Range ist immer richtigherum

	SCCOL nCol1 = rRange.aStart.Col();
	SCROW nRow1 = rRange.aStart.Row();
	SCTAB nTab1 = rRange.aStart.Tab();
	SCCOL nCol2 = rRange.aEnd.Col();
	SCROW nRow2 = rRange.aEnd.Row();
	SCTAB nTab2 = rRange.aEnd.Tab();
	sal_Bool bHiddenEdge = sal_False;
    SCROW nTmp;

	ScDocument* pDoc = pDocSh->GetDocument();
    while ( nCol1 > 0 && pDoc->ColHidden(nCol1, nTab1) )
	{
		--nCol1;
		bHiddenEdge = sal_True;
	}
    while ( nCol2 < MAXCOL && pDoc->ColHidden(nCol2, nTab1) )
	{
		++nCol2;
		bHiddenEdge = sal_True;
	}
    nTmp = pDoc->FirstVisibleRow(0, nRow1, nTab1);
    if (!ValidRow(nTmp))
        nTmp = 0;
    if (nTmp < nRow1)
    {
        nRow1 = nTmp;
        bHiddenEdge = sal_True;
    }
    nTmp = pDoc->FirstVisibleRow(nRow2, MAXROW, nTab1);
    if (!ValidRow(nTmp))
        nTmp = MAXROW;
    if (nTmp > nRow2)
    {
        nRow2 = nTmp;
        bHiddenEdge = sal_True;
    }

	if ( nCol2 > nCol1 + 1 && nRow2 > nRow1 + 1 && !bHiddenEdge )
	{
		//	nur an den Raendern entlang
		//	(die Ecken werden evtl. zweimal getroffen)

		if ( nEdges & SCE_TOP )
			pDocSh->PostPaint( nCol1, nRow1, nTab1, nCol2, nRow1, nTab2, PAINT_MARKS );
		if ( nEdges & SCE_LEFT )
			pDocSh->PostPaint( nCol1, nRow1, nTab1, nCol1, nRow2, nTab2, PAINT_MARKS );
		if ( nEdges & SCE_RIGHT )
			pDocSh->PostPaint( nCol2, nRow1, nTab1, nCol2, nRow2, nTab2, PAINT_MARKS );
		if ( nEdges & SCE_BOTTOM )
			pDocSh->PostPaint( nCol1, nRow2, nTab1, nCol2, nRow2, nTab2, PAINT_MARKS );
	}
	else	// everything in one call
		pDocSh->PostPaint( nCol1, nRow1, nTab1, nCol2, nRow2, nTab2, PAINT_MARKS );
}

void lcl_PaintRefChanged( ScDocShell* pDocSh, const ScRange& rOldUn, const ScRange& rNewUn )
{
	//	Repaint fuer die Teile des Rahmens in Old, die bei New nicht mehr da sind

	ScRange aOld = rOldUn;
	ScRange aNew = rNewUn;
	aOld.Justify();
	aNew.Justify();

	if ( aOld.aStart == aOld.aEnd )					//! Tab ignorieren?
		pDocSh->GetDocument()->ExtendMerge(aOld);
	if ( aNew.aStart == aNew.aEnd )					//! Tab ignorieren?
		pDocSh->GetDocument()->ExtendMerge(aNew);

	SCCOL nOldCol1 = aOld.aStart.Col();
	SCROW nOldRow1 = aOld.aStart.Row();
	SCCOL nOldCol2 = aOld.aEnd.Col();
	SCROW nOldRow2 = aOld.aEnd.Row();
	SCCOL nNewCol1 = aNew.aStart.Col();
	SCROW nNewRow1 = aNew.aStart.Row();
	SCCOL nNewCol2 = aNew.aEnd.Col();
	SCROW nNewRow2 = aNew.aEnd.Row();
	SCTAB nTab1 = aOld.aStart.Tab();		// Tab aendert sich nicht
	SCTAB nTab2 = aOld.aEnd.Tab();

	if ( nNewRow2 < nOldRow1 || nNewRow1 > nOldRow2 ||
		 nNewCol2 < nOldCol1 || nNewCol1 > nOldCol2 ||
		 ( nNewCol1 != nOldCol1 && nNewRow1 != nOldRow1 &&
		   nNewCol2 != nOldCol2 && nNewRow2 != nOldRow2 ) )
	{
		//	komplett weggeschoben oder alle Seiten veraendert
		//	(Abfrage <= statt < geht schief bei einzelnen Zeilen/Spalten)

		lcl_PaintOneRange( pDocSh, aOld, SCE_ALL );
	}
	else		//	alle vier Kanten einzeln testen
	{
		//	oberer Teil
		if ( nNewRow1 < nOldRow1 )					//	nur obere Linie loeschen
			lcl_PaintOneRange( pDocSh, ScRange(
					nOldCol1, nOldRow1, nTab1, nOldCol2, nOldRow1, nTab2 ), SCE_ALL );
		else if ( nNewRow1 > nOldRow1 )				//	den Teil, der oben wegkommt
			lcl_PaintOneRange( pDocSh, ScRange(
					nOldCol1, nOldRow1, nTab1, nOldCol2, nNewRow1-1, nTab2 ),
					SCE_ALL &~ SCE_BOTTOM );

		//	unterer Teil
		if ( nNewRow2 > nOldRow2 )					//	nur untere Linie loeschen
			lcl_PaintOneRange( pDocSh, ScRange(
					nOldCol1, nOldRow2, nTab1, nOldCol2, nOldRow2, nTab2 ), SCE_ALL );
		else if ( nNewRow2 < nOldRow2 )				//	den Teil, der unten wegkommt
			lcl_PaintOneRange( pDocSh, ScRange(
					nOldCol1, nNewRow2+1, nTab1, nOldCol2, nOldRow2, nTab2 ),
					SCE_ALL &~ SCE_TOP );

		//	linker Teil
		if ( nNewCol1 < nOldCol1 )					//	nur linke Linie loeschen
			lcl_PaintOneRange( pDocSh, ScRange(
					nOldCol1, nOldRow1, nTab1, nOldCol1, nOldRow2, nTab2 ), SCE_ALL );
		else if ( nNewCol1 > nOldCol1 )				//	den Teil, der links wegkommt
			lcl_PaintOneRange( pDocSh, ScRange(
					nOldCol1, nOldRow1, nTab1, nNewCol1-1, nOldRow2, nTab2 ),
					SCE_ALL &~ SCE_RIGHT );

		//	rechter Teil
		if ( nNewCol2 > nOldCol2 )					//	nur rechte Linie loeschen
			lcl_PaintOneRange( pDocSh, ScRange(
					nOldCol2, nOldRow1, nTab1, nOldCol2, nOldRow2, nTab2 ), SCE_ALL );
		else if ( nNewCol2 < nOldCol2 )				//	den Teil, der rechts wegkommt
			lcl_PaintOneRange( pDocSh, ScRange(
					nNewCol2+1, nOldRow1, nTab1, nOldCol2, nOldRow2, nTab2 ),
					SCE_ALL &~ SCE_LEFT );
	}
}

void ScGridWindow::RFMouseMove( const MouseEvent& rMEvt, sal_Bool bUp )
{
	ScInputHandler* pHdl = SC_MOD()->GetInputHdl( pViewData->GetViewShell() );
	if (!pHdl)
		return;
	ScRangeFindList* pRangeFinder = pHdl->GetRangeFindList();
	if (!pRangeFinder || nRFIndex >= pRangeFinder->Count())
		return;
	ScRangeFindData* pData = pRangeFinder->GetObject( nRFIndex );
	if (!pData)
		return;

	//	Mauszeiger

	if (bRFSize)
		SetPointer( Pointer( POINTER_CROSS ) );
	else
		SetPointer( Pointer( POINTER_HAND ) );

	//	Scrolling

	sal_Bool bTimer = sal_False;
	Point aPos = rMEvt.GetPosPixel();
	SCsCOL nDx = 0;
	SCsROW nDy = 0;
	if ( aPos.X() < 0 ) nDx = -1;
	if ( aPos.Y() < 0 ) nDy = -1;
	Size aSize = GetOutputSizePixel();
	if ( aPos.X() >= aSize.Width() )
		nDx = 1;
	if ( aPos.Y() >= aSize.Height() )
		nDy = 1;
	if ( nDx != 0 || nDy != 0 )
	{
		if ( nDx != 0) pViewData->GetView()->ScrollX( nDx, WhichH(eWhich) );
		if ( nDy != 0 ) pViewData->GetView()->ScrollY( nDy, WhichV(eWhich) );
		bTimer = sal_True;
	}

	//	Umschalten bei Fixierung (damit Scrolling funktioniert)

	if ( eWhich == pViewData->GetActivePart() )		//??
	{
		if ( pViewData->GetHSplitMode() == SC_SPLIT_FIX )
			if ( nDx > 0 )
			{
				if ( eWhich == SC_SPLIT_TOPLEFT )
					pViewData->GetView()->ActivatePart( SC_SPLIT_TOPRIGHT );
				else if ( eWhich == SC_SPLIT_BOTTOMLEFT )
					pViewData->GetView()->ActivatePart( SC_SPLIT_BOTTOMRIGHT );
			}

		if ( pViewData->GetVSplitMode() == SC_SPLIT_FIX )
			if ( nDy > 0 )
			{
				if ( eWhich == SC_SPLIT_TOPLEFT )
					pViewData->GetView()->ActivatePart( SC_SPLIT_BOTTOMLEFT );
				else if ( eWhich == SC_SPLIT_TOPRIGHT )
					pViewData->GetView()->ActivatePart( SC_SPLIT_BOTTOMRIGHT );
			}
	}

	//	Verschieben

	SCsCOL	nPosX;
	SCsROW	nPosY;
	pViewData->GetPosFromPixel( aPos.X(), aPos.Y(), eWhich, nPosX, nPosY );

	ScRange aOld = pData->aRef;
	ScRange aNew = aOld;
	if ( bRFSize )
	{
		aNew.aEnd.SetCol((SCCOL)nPosX);
		aNew.aEnd.SetRow((SCROW)nPosY);
	}
	else
	{
		long nStartX = nPosX - nRFAddX;
		if ( nStartX < 0 ) nStartX = 0;
		long nStartY = nPosY - nRFAddY;
		if ( nStartY < 0 ) nStartY = 0;
		long nEndX = nStartX + aOld.aEnd.Col() - aOld.aStart.Col();
		if ( nEndX > MAXCOL )
		{
			nStartX -= ( nEndX - MAXROW );
			nEndX = MAXCOL;
		}
		long nEndY = nStartY + aOld.aEnd.Row() - aOld.aStart.Row();
		if ( nEndY > MAXROW )
		{
			nStartY -= ( nEndY - MAXROW );
			nEndY = MAXROW;
		}

		aNew.aStart.SetCol((SCCOL)nStartX);
		aNew.aStart.SetRow((SCROW)nStartY);
		aNew.aEnd.SetCol((SCCOL)nEndX);
		aNew.aEnd.SetRow((SCROW)nEndY);
	}

	if ( bUp )
		aNew.Justify();				// beim ButtonUp wieder richtigherum

	if ( aNew != aOld )
	{
		pHdl->UpdateRange( nRFIndex, aNew );

		ScDocShell* pDocSh = pViewData->GetDocShell();

		//	nur das neuzeichnen, was sich veraendert hat...
		lcl_PaintRefChanged( pDocSh, aOld, aNew );

		//	neuen Rahmen nur drueberzeichnen (synchron)
		pDocSh->Broadcast( ScIndexHint( SC_HINT_SHOWRANGEFINDER, nRFIndex ) );

		Update();	// was man bewegt, will man auch sofort sehen
	}

	//	Timer fuer Scrolling

	if (bTimer)
		pViewData->GetView()->SetTimer( this, rMEvt );			// Event wiederholen
	else
		pViewData->GetView()->ResetTimer();
}

//------------------------------------------------------------------------

sal_Bool ScGridWindow::GetEditUrl( const Point& rPos,
								String* pName, String* pUrl, String* pTarget )
{
	return GetEditUrlOrError( sal_False, rPos, pName, pUrl, pTarget );
}

sal_Bool ScGridWindow::GetEditUrlOrError( sal_Bool bSpellErr, const Point& rPos,
								String* pName, String* pUrl, String* pTarget )
{
	//!	nPosX/Y mit uebergeben?
	SCsCOL nPosX;
	SCsROW nPosY;
	pViewData->GetPosFromPixel( rPos.X(), rPos.Y(), eWhich, nPosX, nPosY );

	SCTAB nTab = pViewData->GetTabNo();
	ScDocShell* pDocSh = pViewData->GetDocShell();
	ScDocument* pDoc = pDocSh->GetDocument();
	ScBaseCell* pCell = NULL;

	sal_Bool bFound = lcl_GetHyperlinkCell( pDoc, nPosX, nPosY, nTab, pCell );
	if( !bFound )
		return sal_False;

	ScHideTextCursor aHideCursor( pViewData, eWhich );	// before GetEditArea (MapMode is changed)

	const ScPatternAttr* pPattern = pDoc->GetPattern( nPosX, nPosY, nTab );
	// bForceToTop = sal_False, use the cell's real position
	Rectangle aEditRect = pViewData->GetEditArea( eWhich, nPosX, nPosY, this, pPattern, sal_False );
	if (rPos.Y() < aEditRect.Top())
		return sal_False;

		//	vertikal kann (noch) nicht angeklickt werden:

    if (pPattern->GetCellOrientation() != SVX_ORIENTATION_STANDARD)
		return sal_False;

	sal_Bool bBreak = ((SfxBoolItem&)pPattern->GetItem(ATTR_LINEBREAK)).GetValue() ||
					((SvxCellHorJustify)((const SvxHorJustifyItem&)pPattern->
						GetItem( ATTR_HOR_JUSTIFY )).GetValue() == SVX_HOR_JUSTIFY_BLOCK);
	SvxCellHorJustify eHorJust = (SvxCellHorJustify)((SvxHorJustifyItem&)pPattern->
						GetItem(ATTR_HOR_JUSTIFY)).GetValue();

		//	EditEngine

	ScFieldEditEngine aEngine( pDoc->GetEditPool() );
	ScSizeDeviceProvider aProv(pDocSh);
	aEngine.SetRefDevice( aProv.GetDevice() );
	aEngine.SetRefMapMode( MAP_100TH_MM );
	SfxItemSet aDefault( aEngine.GetEmptyItemSet() );
	pPattern->FillEditItemSet( &aDefault );
	SvxAdjust eSvxAdjust = SVX_ADJUST_LEFT;
	switch (eHorJust)
	{
		case SVX_HOR_JUSTIFY_LEFT:
		case SVX_HOR_JUSTIFY_REPEAT:			// nicht implementiert
		case SVX_HOR_JUSTIFY_STANDARD:			// always Text if an EditCell type
                eSvxAdjust = SVX_ADJUST_LEFT;
				break;
		case SVX_HOR_JUSTIFY_RIGHT:
				eSvxAdjust = SVX_ADJUST_RIGHT;
				break;
		case SVX_HOR_JUSTIFY_CENTER:
				eSvxAdjust = SVX_ADJUST_CENTER;
				break;
		case SVX_HOR_JUSTIFY_BLOCK:
				eSvxAdjust = SVX_ADJUST_BLOCK;
				break;
	}
    aDefault.Put( SvxAdjustItem( eSvxAdjust, EE_PARA_JUST ) );
	aEngine.SetDefaults( aDefault );
	if (bSpellErr)
		aEngine.SetControlWord( aEngine.GetControlWord() | EE_CNTRL_ONLINESPELLING );

	MapMode aEditMode = pViewData->GetLogicMode(eWhich);			// ohne Drawing-Skalierung
	Rectangle aLogicEdit = PixelToLogic( aEditRect, aEditMode );
	long nThisColLogic = aLogicEdit.Right() - aLogicEdit.Left() + 1;
    Size aPaperSize = Size( 1000000, 1000000 );
    if(pCell->GetCellType() == CELLTYPE_FORMULA)
    {
        long nSizeX  = 0;
        long nSizeY  = 0;
        pViewData->GetMergeSizePixel( nPosX, nPosY, nSizeX, nSizeY );
        aPaperSize = Size(nSizeX, nSizeY );
        aPaperSize = PixelToLogic(aPaperSize);
    }

	if (bBreak)
		aPaperSize.Width() = nThisColLogic;
	aEngine.SetPaperSize( aPaperSize );

    ::std::auto_ptr< EditTextObject > pTextObj;
    const EditTextObject* pData;
    if(pCell->GetCellType() == CELLTYPE_EDIT)
    {
        ((ScEditCell*)pCell)->GetData(pData);
        if (pData)
            aEngine.SetText(*pData);
    }
    else  // HyperLink Formula cell
    {
        pTextObj.reset((static_cast<ScFormulaCell*>(pCell))->CreateURLObject());
        if (pTextObj.get())
            aEngine.SetText(*pTextObj);
    }

	long nStartX = aLogicEdit.Left();

        long nTextWidth = aEngine.CalcTextWidth();
	long nTextHeight = aEngine.GetTextHeight();
	if ( nTextWidth < nThisColLogic )
	{
		if (eHorJust == SVX_HOR_JUSTIFY_RIGHT)
			nStartX += nThisColLogic - nTextWidth;
		else if (eHorJust == SVX_HOR_JUSTIFY_CENTER)
			nStartX += (nThisColLogic - nTextWidth) / 2;
	}

	aLogicEdit.Left() = nStartX;
	if (!bBreak)
		aLogicEdit.Right() = nStartX + nTextWidth;

    // There is one glitch when dealing with a hyperlink cell and
    // the cell content is NUMERIC. This defaults to right aligned and
    // we need to adjust accordingly.
    if(pCell->GetCellType() == CELLTYPE_FORMULA &&
        static_cast<ScFormulaCell*>(pCell)->IsValue() &&
        eHorJust == SVX_HOR_JUSTIFY_STANDARD)
    {
        aLogicEdit.Right() = aLogicEdit.Left() + nThisColLogic - 1;
        aLogicEdit.Left() =  aLogicEdit.Right() - nTextWidth;
    }
    aLogicEdit.Bottom() = aLogicEdit.Top() + nTextHeight;


	Point aLogicClick = PixelToLogic(rPos,aEditMode);
	if ( aLogicEdit.IsInside(aLogicClick) )
	{
//		aEngine.SetUpdateMode(sal_False);
		EditView aTempView( &aEngine, this );
		aTempView.SetOutputArea( aLogicEdit );

		sal_Bool bRet = sal_False;
		MapMode aOld = GetMapMode();
		SetMapMode(aEditMode);					// kein return mehr

		if (bSpellErr)							// Spelling-Fehler suchen
		{
			bRet = aTempView.IsWrongSpelledWordAtPos( rPos );
			if ( bRet )
				pViewData->GetView()->SetCursor( nPosX, nPosY );		// Cursor setzen
		}
		else									// URL suchen
		{
			const SvxFieldItem*	pFieldItem = aTempView.GetFieldUnderMousePointer();

			if (pFieldItem)
			{
				const SvxFieldData* pField = pFieldItem->GetField();
				if ( pField && pField->ISA(SvxURLField) )
				{
					if ( pName || pUrl || pTarget )
					{
						const SvxURLField* pURLField = (const SvxURLField*)pField;
						if (pName)
							*pName = pURLField->GetRepresentation();
						if (pUrl)
							*pUrl = pURLField->GetURL();
						if (pTarget)
							*pTarget = pURLField->GetTargetFrame();
					}
					bRet = sal_True;
				}
			}
		}

		SetMapMode(aOld);

		//	text cursor is restored in ScHideTextCursor dtor

		return bRet;
	}
	return sal_False;
}

sal_Bool ScGridWindow::HasScenarioButton( const Point& rPosPixel, ScRange& rScenRange )
{
	ScDocument* pDoc = pViewData->GetDocument();
	SCTAB nTab = pViewData->GetTabNo();
	SCTAB nTabCount = pDoc->GetTableCount();
	if ( nTab+1<nTabCount && pDoc->IsScenario(nTab+1) && !pDoc->IsScenario(nTab) )
	{
		sal_Bool bLayoutRTL = pDoc->IsLayoutRTL( nTab );

		Size aButSize = pViewData->GetScenButSize();
		long nBWidth  = aButSize.Width();
		if (!nBWidth)
			return sal_False;					// noch kein Button gezeichnet -> da ist auch keiner
		long nBHeight = aButSize.Height();
		long nHSpace  = (long)( SC_SCENARIO_HSPACE * pViewData->GetPPTX() );

		//!	Ranges an der Table cachen!!!!

		ScMarkData aMarks;
		for (SCTAB i=nTab+1; i<nTabCount && pDoc->IsScenario(i); i++)
			pDoc->MarkScenario( i, nTab, aMarks, sal_False, SC_SCENARIO_SHOWFRAME );
		ScRangeList aRanges;
		aMarks.FillRangeListWithMarks( &aRanges, sal_False );


		sal_uLong nRangeCount = aRanges.Count();
		for (sal_uLong j=0; j<nRangeCount; j++)
		{
			ScRange aRange = *aRanges.GetObject(j);
			//	Szenario-Rahmen immer dann auf zusammengefasste Zellen erweitern, wenn
			//	dadurch keine neuen nicht-ueberdeckten Zellen mit umrandet werden
			pDoc->ExtendTotalMerge( aRange );

			sal_Bool bTextBelow = ( aRange.aStart.Row() == 0 );

			Point aButtonPos;
			if ( bTextBelow )
			{
				aButtonPos = pViewData->GetScrPos( aRange.aEnd.Col()+1, aRange.aEnd.Row()+1,
													eWhich, sal_True );
			}
			else
			{
				aButtonPos = pViewData->GetScrPos( aRange.aEnd.Col()+1, aRange.aStart.Row(),
													eWhich, sal_True );
				aButtonPos.Y() -= nBHeight;
			}
			if ( bLayoutRTL )
				aButtonPos.X() -= nHSpace - 1;
			else
				aButtonPos.X() -= nBWidth - nHSpace;	// same for top or bottom

			Rectangle aButRect( aButtonPos, Size(nBWidth,nBHeight) );
			if ( aButRect.IsInside( rPosPixel ) )
			{
				rScenRange = aRange;
				return sal_True;
			}
		}
	}

	return sal_False;
}

void ScGridWindow::UpdateVisibleRange()
{
    // #163911# Update the visible range outside of paint (called when switching sheets).
    // Use the same logic here as in ScGridWindow::Draw.

    SCCOL nPosX = pViewData->GetPosX( eHWhich );
    SCROW nPosY = pViewData->GetPosY( eVWhich );

    SCCOL nXRight = nPosX + pViewData->VisibleCellsX(eHWhich);
    if (nXRight > MAXCOL) nXRight = MAXCOL;
    SCROW nYBottom = nPosY + pViewData->VisibleCellsY(eVWhich);
    if (nYBottom > MAXROW) nYBottom = MAXROW;

    // Store the current visible range.
    maVisibleRange.mnCol1 = nPosX;
    maVisibleRange.mnCol2 = nXRight;
    maVisibleRange.mnRow1 = nPosY;
    maVisibleRange.mnRow2 = nYBottom;
}

// #114409#
void ScGridWindow::DrawLayerCreated()
{
    SetMapMode( GetDrawMapMode() );

	// initially create overlay objects
	ImpCreateOverlayObjects();
}

// #114409#
void ScGridWindow::CursorChanged()
{
	// here the created OverlayObjects may be transformed in later versions. For
	// now, just re-create them

	UpdateCursorOverlay();
}

// #114409#
void ScGridWindow::ImpCreateOverlayObjects()
{
    UpdateCursorOverlay();
    UpdateSelectionOverlay();
    UpdateAutoFillOverlay();
    UpdateDragRectOverlay();
    UpdateHeaderOverlay();
    UpdateShrinkOverlay();
}

// #114409#
void ScGridWindow::ImpDestroyOverlayObjects()
{
    DeleteCursorOverlay();
    DeleteSelectionOverlay();
    DeleteAutoFillOverlay();
    DeleteDragRectOverlay();
    DeleteHeaderOverlay();
    DeleteShrinkOverlay();
}

void ScGridWindow::UpdateAllOverlays()
{
    // delete and re-allocate all overlay objects

    ImpDestroyOverlayObjects();
    ImpCreateOverlayObjects();
}

void ScGridWindow::DeleteCursorOverlay()
{
    DELETEZ( mpOOCursors );
}

void ScGridWindow::UpdateCursorOverlay()
{
    MapMode aDrawMode = GetDrawMapMode();
    MapMode aOldMode = GetMapMode();
    if ( aOldMode != aDrawMode )
        SetMapMode( aDrawMode );

    // Existing OverlayObjects may be transformed in later versions.
    // For now, just re-create them.

    DeleteCursorOverlay();

    std::vector<Rectangle> aPixelRects;

    //
    //  determine the cursor rectangles in pixels (moved from ScGridWindow::DrawCursor)
    //

    SCTAB nTab = pViewData->GetTabNo();
    SCCOL nX = pViewData->GetCurX();
    SCROW nY = pViewData->GetCurY();

    if (!maVisibleRange.isInside(nX, nY))
        return;

    //  don't show the cursor in overlapped cells

    ScDocument* pDoc = pViewData->GetDocument();
    const ScPatternAttr* pPattern = pDoc->GetPattern(nX,nY,nTab);
    const ScMergeFlagAttr& rMergeFlag = (const ScMergeFlagAttr&) pPattern->GetItem(ATTR_MERGE_FLAG);
    sal_Bool bOverlapped = rMergeFlag.IsOverlapped();

    //  left or above of the screen?

    sal_Bool bVis = ( nX>=pViewData->GetPosX(eHWhich) && nY>=pViewData->GetPosY(eVWhich) );
    if (!bVis)
    {
        SCCOL nEndX = nX;
        SCROW nEndY = nY;
        const ScMergeAttr& rMerge = (const ScMergeAttr&) pPattern->GetItem(ATTR_MERGE);
        if (rMerge.GetColMerge() > 1)
            nEndX += rMerge.GetColMerge()-1;
        if (rMerge.GetRowMerge() > 1)
            nEndY += rMerge.GetRowMerge()-1;
        bVis = ( nEndX>=pViewData->GetPosX(eHWhich) && nEndY>=pViewData->GetPosY(eVWhich) );
    }

    if ( bVis && !bOverlapped && !pViewData->HasEditView(eWhich) && pViewData->IsActive() )
    {
        Point aScrPos = pViewData->GetScrPos( nX, nY, eWhich, sal_True );
        sal_Bool bLayoutRTL = pDoc->IsLayoutRTL( nTab );

        //  completely right of/below the screen?
        //  (test with logical start position in aScrPos)
        sal_Bool bMaybeVisible;
        if ( bLayoutRTL )
            bMaybeVisible = ( aScrPos.X() >= -2 && aScrPos.Y() >= -2 );
        else
        {
            Size aOutSize = GetOutputSizePixel();
            bMaybeVisible = ( aScrPos.X() <= aOutSize.Width() + 2 && aScrPos.Y() <= aOutSize.Height() + 2 );
        }
        if ( bMaybeVisible )
        {
            long nSizeXPix;
            long nSizeYPix;
            pViewData->GetMergeSizePixel( nX, nY, nSizeXPix, nSizeYPix );

            if ( bLayoutRTL )
                aScrPos.X() -= nSizeXPix - 2;       // move instead of mirroring

            sal_Bool bFix = ( pViewData->GetHSplitMode() == SC_SPLIT_FIX ||
                            pViewData->GetVSplitMode() == SC_SPLIT_FIX );
            if ( pViewData->GetActivePart()==eWhich || bFix )
            {
                aScrPos.X() -= 2;
                aScrPos.Y() -= 2;
                Rectangle aRect( aScrPos, Size( nSizeXPix + 3, nSizeYPix + 3 ) );

                aPixelRects.push_back(Rectangle( aRect.Left(), aRect.Top(), aRect.Left()+2, aRect.Bottom() ));
                aPixelRects.push_back(Rectangle( aRect.Right()-2, aRect.Top(), aRect.Right(), aRect.Bottom() ));
                aPixelRects.push_back(Rectangle( aRect.Left()+3, aRect.Top(), aRect.Right()-3, aRect.Top()+2 ));
                aPixelRects.push_back(Rectangle( aRect.Left()+3, aRect.Bottom()-2, aRect.Right()-3, aRect.Bottom() ));
            }
            else
            {
                Rectangle aRect( aScrPos, Size( nSizeXPix - 1, nSizeYPix - 1 ) );
                aPixelRects.push_back( aRect );
            }
        }
    }

    if ( aPixelRects.size() )
    {
		// #i70788# get the OverlayManager safely
		::sdr::overlay::OverlayManager* pOverlayManager = getOverlayManager();

		if(pOverlayManager)
        {
            const Color aCursorColor( SC_MOD()->GetColorConfig().GetColorValue(svtools::FONTCOLOR).nColor );
			std::vector< basegfx::B2DRange > aRanges;
			const basegfx::B2DHomMatrix aTransform(GetInverseViewTransformation());
			
			for(sal_uInt32 a(0); a < aPixelRects.size(); a++)
			{
				const Rectangle aRA(aPixelRects[a]);
				basegfx::B2DRange aRB(aRA.Left(), aRA.Top(), aRA.Right() + 1, aRA.Bottom() + 1);
				aRB.transform(aTransform);
				aRanges.push_back(aRB);
			}
			
			sdr::overlay::OverlayObject* pOverlay = new sdr::overlay::OverlaySelection(
				sdr::overlay::OVERLAY_SOLID, 
				aCursorColor, 
				aRanges,
                false);

			pOverlayManager->add(*pOverlay);
			mpOOCursors = new ::sdr::overlay::OverlayObjectList;
			mpOOCursors->append(*pOverlay);
        }
    }

    if ( aOldMode != aDrawMode )
        SetMapMode( aOldMode );
}

void ScGridWindow::DeleteSelectionOverlay()
{
    DELETEZ( mpOOSelection );
}

void ScGridWindow::UpdateSelectionOverlay()
{
    MapMode aDrawMode = GetDrawMapMode();
    MapMode aOldMode = GetMapMode();
    if ( aOldMode != aDrawMode )
        SetMapMode( aDrawMode );

    DeleteSelectionOverlay();
    std::vector<Rectangle> aPixelRects;
    GetSelectionRects( aPixelRects );

#ifdef USE_JAVA
    // Fix bug 3607 by invalidating the merge toolbar buttons
    SfxBindings& rBindings = pViewData->GetBindings();
    rBindings.Invalidate( FID_MERGE_ON );
    rBindings.Invalidate( FID_MERGE_OFF );
    rBindings.Invalidate( FID_MERGE_TOGGLE );

#ifdef USE_NATIVE_HIGHLIGHT_COLOR
    // Always clear last selection even if native highlighting is turned off
    if ( pViewData->IsActive() )
    {
        for ( std::vector< Rectangle >::const_iterator it = aLastSelectionPixelRects.begin(); it != aLastSelectionPixelRects.end(); ++it )
            Invalidate( PixelToLogic( *it ) );
        aLastSelectionPixelRects.clear();
    }
#endif	// USE_NATIVE_HIGHLIGHT_COLOR
#endif	// USE_JAVA

    if ( aPixelRects.size() && pViewData->IsActive() )
    {
#if defined USE_JAVA && defined USE_NATIVE_HIGHLIGHT_COLOR
        if ( UseNativeHighlightColor() )
        {
            for ( std::vector< Rectangle >::const_iterator it = aPixelRects.begin(); it != aPixelRects.end(); ++it )
                Invalidate( PixelToLogic( *it ) );
            aLastSelectionPixelRects = aPixelRects;
        }
        else
        {
#endif	// USE_JAVA && USE_NATIVE_HIGHLIGHT_COLOR
		// #i70788# get the OverlayManager safely
		::sdr::overlay::OverlayManager* pOverlayManager = getOverlayManager();

		if(pOverlayManager)
		{
			std::vector< basegfx::B2DRange > aRanges;
			const basegfx::B2DHomMatrix aTransform(GetInverseViewTransformation());
			
			for(sal_uInt32 a(0); a < aPixelRects.size(); a++)
			{
				const Rectangle aRA(aPixelRects[a]);
				basegfx::B2DRange aRB(aRA.Left() - 1, aRA.Top() - 1, aRA.Right(), aRA.Bottom());
				aRB.transform(aTransform);
				aRanges.push_back(aRB);
			}

            // get the system's hilight color
            const SvtOptionsDrawinglayer aSvtOptionsDrawinglayer;
            const Color aHighlight(aSvtOptionsDrawinglayer.getHilightColor());

			sdr::overlay::OverlayObject* pOverlay = new sdr::overlay::OverlaySelection(
				sdr::overlay::OVERLAY_TRANSPARENT, 
				aHighlight, 
				aRanges,
                true);

            pOverlayManager->add(*pOverlay);
	        mpOOSelection = new ::sdr::overlay::OverlayObjectList;
		    mpOOSelection->append(*pOverlay);
		}
#if defined USE_JAVA && defined USE_NATIVE_HIGHLIGHT_COLOR
        }
#endif	// USE_JAVA && USE_NATIVE_HIGHLIGHT_COLOR
    }

    if ( aOldMode != aDrawMode )
        SetMapMode( aOldMode );
}

void ScGridWindow::DeleteAutoFillOverlay()
{
    DELETEZ( mpOOAutoFill );
    mpAutoFillRect.reset();
}

void ScGridWindow::UpdateAutoFillOverlay()
{
    MapMode aDrawMode = GetDrawMapMode();
    MapMode aOldMode = GetMapMode();
    if ( aOldMode != aDrawMode )
        SetMapMode( aDrawMode );

    DeleteAutoFillOverlay();

    //
    //  get the AutoFill handle rectangle in pixels (moved from ScGridWindow::DrawAutoFillMark)
    //

    if ( bAutoMarkVisible && aAutoMarkPos.Tab() == pViewData->GetTabNo() &&
         !pViewData->HasEditView(eWhich) && pViewData->IsActive() )
    {
        SCCOL nX = aAutoMarkPos.Col();
        SCROW nY = aAutoMarkPos.Row();

        if (!maVisibleRange.isInside(nX, nY))
            // Autofill mark is not visible.  Bail out.
            return;

        SCTAB nTab = pViewData->GetTabNo();
        ScDocument* pDoc = pViewData->GetDocument();
        sal_Bool bLayoutRTL = pDoc->IsLayoutRTL( nTab );

        Point aFillPos = pViewData->GetScrPos( nX, nY, eWhich, sal_True );
        long nSizeXPix;
        long nSizeYPix;
        pViewData->GetMergeSizePixel( nX, nY, nSizeXPix, nSizeYPix );
        if ( bLayoutRTL )
            aFillPos.X() -= nSizeXPix + 3;
        else
            aFillPos.X() += nSizeXPix - 2;

        aFillPos.Y() += nSizeYPix;
        aFillPos.Y() -= 2;
        mpAutoFillRect.reset(new Rectangle(aFillPos, Size(6, 6)));
	
		// #i70788# get the OverlayManager safely
		::sdr::overlay::OverlayManager* pOverlayManager = getOverlayManager();

		if(pOverlayManager)
		{
            const Color aHandleColor( SC_MOD()->GetColorConfig().GetColorValue(svtools::FONTCOLOR).nColor );
			std::vector< basegfx::B2DRange > aRanges;
			const basegfx::B2DHomMatrix aTransform(GetInverseViewTransformation());
            basegfx::B2DRange aRB(mpAutoFillRect->Left(), mpAutoFillRect->Top(), mpAutoFillRect->Right() + 1, mpAutoFillRect->Bottom() + 1);
			
			aRB.transform(aTransform);
			aRanges.push_back(aRB);

			sdr::overlay::OverlayObject* pOverlay = new sdr::overlay::OverlaySelection( 
				sdr::overlay::OVERLAY_SOLID, 
				aHandleColor, 
				aRanges,
                false);

		    pOverlayManager->add(*pOverlay);
			mpOOAutoFill = new ::sdr::overlay::OverlayObjectList;
			mpOOAutoFill->append(*pOverlay);
		}

        if ( aOldMode != aDrawMode )
            SetMapMode( aOldMode );
    }
}

void ScGridWindow::DeleteDragRectOverlay()
{
    DELETEZ( mpOODragRect );
}

void ScGridWindow::UpdateDragRectOverlay()
{
    MapMode aDrawMode = GetDrawMapMode();
    MapMode aOldMode = GetMapMode();
    if ( aOldMode != aDrawMode )
        SetMapMode( aDrawMode );

    DeleteDragRectOverlay();

    //
    //  get the rectangles in pixels (moved from DrawDragRect)
    //

    if ( bDragRect || bPagebreakDrawn )
    {
        std::vector<Rectangle> aPixelRects;

        SCCOL nX1 = bDragRect ? nDragStartX : aPagebreakDrag.aStart.Col();
        SCROW nY1 = bDragRect ? nDragStartY : aPagebreakDrag.aStart.Row();
        SCCOL nX2 = bDragRect ? nDragEndX : aPagebreakDrag.aEnd.Col();
        SCROW nY2 = bDragRect ? nDragEndY : aPagebreakDrag.aEnd.Row();

        SCTAB nTab = pViewData->GetTabNo();

        SCCOL nPosX = pViewData->GetPosX(WhichH(eWhich));
        SCROW nPosY = pViewData->GetPosY(WhichV(eWhich));
        if (nX1 < nPosX) nX1 = nPosX;
        if (nX2 < nPosX) nX2 = nPosX;
        if (nY1 < nPosY) nY1 = nPosY;
        if (nY2 < nPosY) nY2 = nPosY;

        Point aScrPos( pViewData->GetScrPos( nX1, nY1, eWhich ) );

        long nSizeXPix=0;
        long nSizeYPix=0;
        ScDocument* pDoc = pViewData->GetDocument();
        double nPPTX = pViewData->GetPPTX();
        double nPPTY = pViewData->GetPPTY();
        SCCOLROW i;

        sal_Bool bLayoutRTL = pDoc->IsLayoutRTL( nTab );
        long nLayoutSign = bLayoutRTL ? -1 : 1;

        if (ValidCol(nX2) && nX2>=nX1)
            for (i=nX1; i<=nX2; i++)
                nSizeXPix += ScViewData::ToPixel( pDoc->GetColWidth( static_cast<SCCOL>(i), nTab ), nPPTX );
        else
        {
            aScrPos.X() -= nLayoutSign;
            nSizeXPix   += 2;
        }

        if (ValidRow(nY2) && nY2>=nY1)
            for (i=nY1; i<=nY2; i++)
                nSizeYPix += ScViewData::ToPixel( pDoc->GetRowHeight( i, nTab ), nPPTY );
        else
        {
            aScrPos.Y() -= 1;
            nSizeYPix   += 2;
        }

        aScrPos.X() -= 2 * nLayoutSign;
        aScrPos.Y() -= 2;
//      Rectangle aRect( aScrPos, Size( nSizeXPix + 3, nSizeYPix + 3 ) );
        Rectangle aRect( aScrPos.X(), aScrPos.Y(),
                         aScrPos.X() + ( nSizeXPix + 2 ) * nLayoutSign, aScrPos.Y() + nSizeYPix + 2 );
        if ( bLayoutRTL )
        {
            aRect.Left() = aRect.Right();   // end position is left
            aRect.Right() = aScrPos.X();
        }

        if ( meDragInsertMode == INS_CELLSDOWN )
        {
            aPixelRects.push_back( Rectangle( aRect.Left()+1, aRect.Top()+3, aRect.Left()+1, aRect.Bottom()-2 ) );
            aPixelRects.push_back( Rectangle( aRect.Right()-1, aRect.Top()+3, aRect.Right()-1, aRect.Bottom()-2 ) );
            aPixelRects.push_back( Rectangle( aRect.Left()+1, aRect.Top(), aRect.Right()-1, aRect.Top()+2 ) );
            aPixelRects.push_back( Rectangle( aRect.Left()+1, aRect.Bottom()-1, aRect.Right()-1, aRect.Bottom()-1 ) );
        }
        else if ( meDragInsertMode == INS_CELLSRIGHT )
        {
            aPixelRects.push_back( Rectangle( aRect.Left(), aRect.Top()+1, aRect.Left()+2, aRect.Bottom()-1 ) );
            aPixelRects.push_back( Rectangle( aRect.Right()-1, aRect.Top()+1, aRect.Right()-1, aRect.Bottom()-1 ) );
            aPixelRects.push_back( Rectangle( aRect.Left()+3, aRect.Top()+1, aRect.Right()-2, aRect.Top()+1 ) );
            aPixelRects.push_back( Rectangle( aRect.Left()+3, aRect.Bottom()-1, aRect.Right()-2, aRect.Bottom()-1 ) );
        }
        else
        {
            aPixelRects.push_back( Rectangle( aRect.Left(), aRect.Top(), aRect.Left()+2, aRect.Bottom() ) );
            aPixelRects.push_back( Rectangle( aRect.Right()-2, aRect.Top(), aRect.Right(), aRect.Bottom() ) );
            aPixelRects.push_back( Rectangle( aRect.Left()+3, aRect.Top(), aRect.Right()-3, aRect.Top()+2 ) );
            aPixelRects.push_back( Rectangle( aRect.Left()+3, aRect.Bottom()-2, aRect.Right()-3, aRect.Bottom() ) );
        }

		// #i70788# get the OverlayManager safely
		::sdr::overlay::OverlayManager* pOverlayManager = getOverlayManager();

		if(pOverlayManager)
		{
			// Color aHighlight = GetSettings().GetStyleSettings().GetHighlightColor();
			std::vector< basegfx::B2DRange > aRanges;
			const basegfx::B2DHomMatrix aTransform(GetInverseViewTransformation());
			
			for(sal_uInt32 a(0); a < aPixelRects.size(); a++)
			{
				const Rectangle aRA(aPixelRects[a]);
				basegfx::B2DRange aRB(aRA.Left(), aRA.Top(), aRA.Right() + 1, aRA.Bottom() + 1);
				aRB.transform(aTransform);
				aRanges.push_back(aRB);
			}

			sdr::overlay::OverlayObject* pOverlay = new sdr::overlay::OverlaySelection( 
#if defined USE_JAVA && defined USE_NATIVE_HIGHLIGHT_COLOR
				// Fix bug 3619 by using a solid overlay for native highlighting
				UseNativeHighlightColor() ? sdr::overlay::OVERLAY_SOLID : sdr::overlay::OVERLAY_INVERT, 
#else	// USE_JAVA && USE_NATIVE_HIGHLIGHT_COLOR
				sdr::overlay::OVERLAY_INVERT, 
#endif	// USE_JAVA && USE_NATIVE_HIGHLIGHT_COLOR
				Color(COL_BLACK),
				aRanges,
                false);

		    pOverlayManager->add(*pOverlay);
			mpOODragRect = new ::sdr::overlay::OverlayObjectList;
			mpOODragRect->append(*pOverlay);
		}
    }

    if ( aOldMode != aDrawMode )
        SetMapMode( aOldMode );
}

void ScGridWindow::DeleteHeaderOverlay()
{
    DELETEZ( mpOOHeader );
}

void ScGridWindow::UpdateHeaderOverlay()
{
    MapMode aDrawMode = GetDrawMapMode();
    MapMode aOldMode = GetMapMode();
    if ( aOldMode != aDrawMode )
        SetMapMode( aDrawMode );

    DeleteHeaderOverlay();

    //  Pixel rectangle is in aInvertRect
    if ( !aInvertRect.IsEmpty() )
    {
		// #i70788# get the OverlayManager safely
		::sdr::overlay::OverlayManager* pOverlayManager = getOverlayManager();

		if(pOverlayManager)
		{
            // Color aHighlight = GetSettings().GetStyleSettings().GetHighlightColor();
			std::vector< basegfx::B2DRange > aRanges;
			const basegfx::B2DHomMatrix aTransform(GetInverseViewTransformation());
			basegfx::B2DRange aRB(aInvertRect.Left(), aInvertRect.Top(), aInvertRect.Right() + 1, aInvertRect.Bottom() + 1);
			
			aRB.transform(aTransform);
			aRanges.push_back(aRB);

			sdr::overlay::OverlayObject* pOverlay = new sdr::overlay::OverlaySelection(
#if defined USE_JAVA && defined USE_NATIVE_HIGHLIGHT_COLOR
				// Fix bug 3599 by using a solid overlay for native highlighting
				UseNativeHighlightColor() ? sdr::overlay::OVERLAY_SOLID : sdr::overlay::OVERLAY_INVERT, 
#else	// USE_JAVA && USE_NATIVE_HIGHLIGHT_COLOR
				sdr::overlay::OVERLAY_INVERT, 
#endif	// USE_JAVA && USE_NATIVE_HIGHLIGHT_COLOR
				Color(COL_BLACK), 
				aRanges,
                false);

            pOverlayManager->add(*pOverlay);
	        mpOOHeader = new ::sdr::overlay::OverlayObjectList;
		    mpOOHeader->append(*pOverlay);
		}
    }

    if ( aOldMode != aDrawMode )
        SetMapMode( aOldMode );
}

void ScGridWindow::DeleteShrinkOverlay()
{
    DELETEZ( mpOOShrink );
}

void ScGridWindow::UpdateShrinkOverlay()
{
    MapMode aDrawMode = GetDrawMapMode();
    MapMode aOldMode = GetMapMode();
    if ( aOldMode != aDrawMode )
        SetMapMode( aDrawMode );

    DeleteShrinkOverlay();

    //
    //  get the rectangle in pixels
    //

    Rectangle aPixRect;
    ScRange aRange;
    SCTAB nTab = pViewData->GetTabNo();
    if ( pViewData->IsRefMode() && nTab >= pViewData->GetRefStartZ() && nTab <= pViewData->GetRefEndZ() &&
         pViewData->GetDelMark( aRange ) )
    {
        //! limit to visible area
        if ( aRange.aStart.Col() <= aRange.aEnd.Col() &&
             aRange.aStart.Row() <= aRange.aEnd.Row() )
        {
            Point aStart = pViewData->GetScrPos( aRange.aStart.Col(),
                                                 aRange.aStart.Row(), eWhich );
            Point aEnd = pViewData->GetScrPos( aRange.aEnd.Col()+1,
                                               aRange.aEnd.Row()+1, eWhich );
            aEnd.X() -= 1;
            aEnd.Y() -= 1;

            aPixRect = Rectangle( aStart,aEnd );
        }
    }

    if ( !aPixRect.IsEmpty() )
    {
		// #i70788# get the OverlayManager safely
		::sdr::overlay::OverlayManager* pOverlayManager = getOverlayManager();

		if(pOverlayManager)
		{
            // Color aHighlight = GetSettings().GetStyleSettings().GetHighlightColor();
			std::vector< basegfx::B2DRange > aRanges;
			const basegfx::B2DHomMatrix aTransform(GetInverseViewTransformation());
			basegfx::B2DRange aRB(aPixRect.Left(), aPixRect.Top(), aPixRect.Right() + 1, aPixRect.Bottom() + 1);
			
			aRB.transform(aTransform);
			aRanges.push_back(aRB);

			sdr::overlay::OverlayObject* pOverlay = new sdr::overlay::OverlaySelection(
#if defined USE_JAVA && defined USE_NATIVE_HIGHLIGHT_COLOR
				// Fix bug 3619 by using a solid overlay for native highlighting
				UseNativeHighlightColor() ? sdr::overlay::OVERLAY_SOLID : sdr::overlay::OVERLAY_INVERT, 
#else	// USE_JAVA && USE_NATIVE_HIGHLIGHT_COLOR
				sdr::overlay::OVERLAY_INVERT, 
#endif	// USE_JAVA && USE_NATIVE_HIGHLIGHT_COLOR
				Color(COL_BLACK), 
				aRanges,
                false);

            pOverlayManager->add(*pOverlay);
	        mpOOShrink = new ::sdr::overlay::OverlayObjectList;
		    mpOOShrink->append(*pOverlay);
		}
    }

    if ( aOldMode != aDrawMode )
        SetMapMode( aOldMode );
}

// #i70788# central method to get the OverlayManager safely
::sdr::overlay::OverlayManager* ScGridWindow::getOverlayManager()
{
	SdrPageView* pPV = pViewData->GetView()->GetScDrawView()->GetSdrPageView();

	if(pPV)
	{
		SdrPageWindow* pPageWin = pPV->FindPageWindow( *this );

		if ( pPageWin )
		{
			return (pPageWin->GetOverlayManager());
		}
	}

	return 0L;
}

void ScGridWindow::flushOverlayManager()
{
	// #i70788# get the OverlayManager safely
	::sdr::overlay::OverlayManager* pOverlayManager = getOverlayManager();

	if(pOverlayManager)
	{
		pOverlayManager->flush();
	}
}

#ifdef USE_JAVA

void ScGridWindow::GetNativeHightlightColorRects( ::std::vector< Rectangle >& rPixelRects )
{
	rPixelRects.clear();

#ifdef USE_NATIVE_HIGHLIGHT_COLOR
	// Fix bug 3602 by using the cached selection rectangles
	if ( UseNativeHighlightColor() )
		rPixelRects = aLastSelectionPixelRects;
#endif	// USE_NATIVE_HIGHLIGHT_COLOR
}

#endif	// USE_JAVA

// ---------------------------------------------------------------------------
// eof
