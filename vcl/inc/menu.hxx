/*************************************************************************
 *
 *  $RCSfile$
 *
 *  $Revision$
 *
 *  last change: $Author$ $Date$
 *
 *  The Contents of this file are made available subject to the terms of
 *  either of the following licenses
 *
 *         - GNU General Public License Version 2.1
 *
 *  Sun Microsystems Inc., October, 2000
 *
 *  GNU General Public License Version 2.1
 *  =============================================
 *  Copyright 2000 by Sun Microsystems, Inc.
 *  901 San Antonio Road, Palo Alto, CA 94303, USA
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License version 2.1, as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 *  MA  02111-1307  USA
 *
 *  The Initial Developer of the Original Code is: Sun Microsystems, Inc.
 *
 *  Copyright: 2000 by Sun Microsystems, Inc.
 *
 *  All Rights Reserved.
 *
 *  Contributor(s): Edward Peterlin, 2004.  Native Menu Framework derived from
 *  NeoOffice, copyright Dan Willims 2004.
 *
 *
 ************************************************************************/

#ifndef _SV_MENU_HXX
#define _SV_MENU_HXX

#ifndef _SV_SV_H
#include <sv.h>
#endif

#ifndef _SV_RC_HXX
#include <rc.hxx>
#endif

#ifndef _SV_RESID_HXX
#include <resid.hxx>
#endif

#ifndef _SV_BITMAPEX_HXX
#include <bitmapex.hxx>
#endif

#ifndef _SV_COLOR_HXX
#include <color.hxx>
#endif

#ifndef _VCL_VCLEVENT_HXX
#include <vclevent.hxx>
#endif

#ifndef _COM_SUN_STAR_UNO_REFERENCE_HXX_
#include <com/sun/star/uno/Reference.hxx>
#endif

struct MenuItemData;
class Point;
class Size;
class Rectangle;
class MenuItemList;
class HelpEvent;
class Image;
class PopupMenu;
class KeyCode;
class KeyEvent;
class AppBarWindow;
class MenuFloatingWindow;
class Window;
class AccessObjectRef;
class SalMenu;
class SalMenuItem;
struct SystemMenuData;

namespace com {
namespace sun {
namespace star {
namespace accessibility {
    class XAccessible;
}}}}

namespace vcl { struct MenuLayoutData; }

// --------------
// - Menu-Types -
// --------------

#define MENU_APPEND 			((USHORT)0xFFFF)
#define MENU_ITEM_NOTFOUND		((USHORT)0xFFFF)

#define POPUPMENU_EXECUTE_DOWN	((USHORT)0x0001)
#define POPUPMENU_EXECUTE_UP	((USHORT)0x0002)
#define POPUPMENU_EXECUTE_LEFT	((USHORT)0x0004)
#define POPUPMENU_EXECUTE_RIGHT ((USHORT)0x0008)

// By changes you must also change: tools/vclrsc.hxx
enum MenuItemType { MENUITEM_DONTKNOW, MENUITEM_STRING, MENUITEM_IMAGE,
					MENUITEM_STRINGIMAGE, MENUITEM_SEPARATOR };

// By changes you must also change: tools/vclrsc.hxx
typedef USHORT MenuItemBits;
#define MIB_CHECKABLE			((MenuItemBits)0x0001)
#define MIB_RADIOCHECK			((MenuItemBits)0x0002)
#define MIB_AUTOCHECK			((MenuItemBits)0x0004)
#define MIB_ABOUT				((MenuItemBits)0x0008)
#define MIB_HELP				((MenuItemBits)0x0010)
#define MIB_POPUPSELECT 		((MenuItemBits)0x0020)

#define MENU_FLAG_NOAUTOMNEMONICS		0x0001
#define MENU_FLAG_HIDEDISABLEDENTRIES	0x0002

// --------
// - Menu -
// --------

struct MenuLogo
{
	BitmapEx	aBitmap;
	Color		aStartColor;
	Color		aEndColor;
	ULONG		nDummy;
};

class Menu : public Resource
{
	friend class AccessObject;
	friend class MenuBar;
	friend class MenuBarWindow;
	friend class MenuFloatingWindow;
	friend class PopupMenu;
	friend class SystemWindow;

private:
	MenuItemList*		pItemList;			// Liste mit den MenuItems
	MenuLogo*			pLogo;
	Menu*				pStartedFrom;
	Window* 			pWindow;

	Link				aActivateHdl;		// Active-Handler
	Link				aDeactivateHdl; 	// Deactivate-Handler
	Link				aHighlightHdl;		// Highlight-Handler
	Link				aSelectHdl; 		// Highlight-Handler
	
    VclEventListeners   maEventListeners;
    VclEventListeners   maChildEventListeners;

	XubString			aTitleText; 		// PopupMenu-Text

	ULONG				nEventId;
	ULONG				nDummy;
	USHORT				mnHighlightedItemPos; // for native menues: keeps track of the highlighted item 
	USHORT				nMenuFlags;
	USHORT				nDefaultItem;		// Id vom Default-Item
	USHORT				nSelectedId;

	// Fuer Ausgabe:
	USHORT				nCheckPos;
	USHORT				nImagePos;
	USHORT				nTextPos;

	BOOL				bIsMenuBar	: 1,		// Handelt es sich um den MenuBar
						bCanceled	: 1,		// Waehrend eines Callbacks abgebrochen
						bInCallback : 1,		// In Activate/Deactivate
						bKilled 	: 1;		// Gekillt...

    ::com::sun::star::uno::Reference< ::com::sun::star::accessibility::XAccessible > mxAccessible;
	mutable vcl::MenuLayoutData* mpLayoutData;
	SalMenu*			mpSalMenu;
	long				mnDummy;
	BOOL				mbDummy;

protected:
#ifdef _SV_MENU_CXX
	void				ImplInit();
	void				ImplLoadRes( const ResId& rResId );
	Menu*				ImplGetStartMenu();
	Menu*				ImplFindSelectMenu();
	Menu*				ImplFindMenu( USHORT nId );
	Size				ImplCalcSize( Window* pWin );
	BOOL				ImplIsVisible( USHORT nPos ) const;
	USHORT				ImplGetVisibleItemCount() const;
	USHORT				ImplGetFirstVisible() const;
	USHORT				ImplGetPrevVisible( USHORT nPos ) const;
	USHORT				ImplGetNextVisible( USHORT nPos ) const;
	void				ImplPaint( Window* pWin, USHORT nBorder, long nOffY = 0, MenuItemData* pThisDataOnly = 0, BOOL bHighlighted = FALSE, bool bLayout = false ) const;
	void				ImplSelect();
	void				ImplCallHighlight( USHORT nHighlightItem );
    void                ImplCallEventListeners( ULONG nEvent, USHORT nPos );
						DECL_LINK( ImplCallSelect, Menu* );

    void				ImplFillLayoutData() const;
    SalMenu*            ImplGetSalMenu() { return mpSalMenu; }
    void                ImplSetSalMenu( SalMenu *pMenu );
public:
    void				ImplKillLayoutData() const;
#endif

						Menu();
						Menu( BOOL bMenuBar );
	Window* 			ImplGetWindow() const { return pWindow; }

public:
	virtual 			~Menu();

	virtual void		Activate();
	virtual void		Deactivate();
	virtual void		Highlight();
	virtual void		Select();
	virtual void		RequestHelp( const HelpEvent& rHEvt );

	void				InsertItem( USHORT nItemId, const XubString& rStr,
									MenuItemBits nItemBits = 0,
									USHORT nPos = MENU_APPEND );
	void				InsertItem( USHORT nItemId, const Image& rImage,
									MenuItemBits nItemBits = 0,
									USHORT nPos = MENU_APPEND );
	void				InsertItem( USHORT nItemId,
									const XubString& rString, const Image& rImage,
									MenuItemBits nItemBits = 0,
									USHORT nPos = MENU_APPEND );
	void				InsertItem( const ResId& rResId, USHORT nPos = MENU_APPEND );
	void				InsertSeparator( USHORT nPos = MENU_APPEND );
	void				RemoveItem( USHORT nPos );
	void				CopyItem( const Menu& rMenu, USHORT nPos,
								  USHORT nNewPos = MENU_APPEND );
	void				Clear();

    void                CreateAutoMnemonics();

	void				SetMenuFlags( USHORT nFlags ) { nMenuFlags = nFlags; }
	USHORT				GetMenuFlags() const { return nMenuFlags; }

	USHORT				GetItemCount() const;
	USHORT				GetItemId( USHORT nPos ) const;
	USHORT				GetItemPos( USHORT nItemId ) const;
	MenuItemType		GetItemType( USHORT nPos ) const;
	USHORT				GetCurItemId() const;
	SalMenuItem *		GetItemSalItem( USHORT nPos ) const;

	void				SetDefaultItem( USHORT nItemId )	{ nDefaultItem = nItemId; }
	USHORT				GetDefaultItem() const				{ return nDefaultItem; }

	void				SetItemBits( USHORT nItemId, MenuItemBits nBits );
	MenuItemBits		GetItemBits( USHORT nItemId ) const;

	void				SetUserValue( USHORT nItemId, ULONG nValue );
	ULONG		        GetUserValue( USHORT nItemId ) const;

	void				SetPopupMenu( USHORT nItemId, PopupMenu* pMenu );
	PopupMenu*			GetPopupMenu( USHORT nItemId ) const;

	void				SetAccelKey( USHORT nItemId, const KeyCode& rKeyCode );
	KeyCode 			GetAccelKey( USHORT nItemId ) const;

	void				CheckItem( USHORT nItemId, BOOL bCheck = TRUE );
	BOOL				IsItemChecked( USHORT nItemId ) const;

    void				SelectItem( USHORT nItemId );
    void				DeSelect() { SelectItem( 0xFFFF ); } // MENUITEMPOS_INVALID

	void				EnableItem( USHORT nItemId, BOOL bEnable = TRUE );
	BOOL				IsItemEnabled( USHORT nItemId ) const;

    BOOL				IsItemVisible( USHORT nItemId ) const;
    BOOL				IsItemPosVisible( USHORT nItemPos ) const;
    BOOL				IsMenuVisible() const;
    BOOL				IsMenuBar() const { return bIsMenuBar; }

	void				RemoveDisabledEntries( BOOL bCheckPopups = TRUE, BOOL bRemoveEmptyPopups = FALSE );
	BOOL				HasValidEntries( BOOL bCheckPopups = TRUE );

	void				SetItemText( USHORT nItemId, const XubString& rStr );
	XubString			GetItemText( USHORT nItemId ) const;

	void				SetItemImage( USHORT nItemId, const Image& rImage );
	Image				GetItemImage( USHORT nItemId ) const;
    void				SetItemImageAngle( USHORT nItemId, long nAngle10 );
    long				GetItemImageAngle( USHORT nItemId ) const;
    void				SetItemImageMirrorMode( USHORT nItemId, BOOL bMirror );
    BOOL				GetItemImageMirrorMode( USHORT ) const;

	void				SetItemCommand( USHORT nItemId, const XubString& rCommand );
	const XubString&	GetItemCommand( USHORT nItemId ) const;

	void				SetHelpText( USHORT nItemId, const XubString& rString );
	const XubString&	GetHelpText( USHORT nItemId ) const;

	void				SetTipHelpText( USHORT nItemId, const XubString& rString );
	const XubString&	GetTipHelpText( USHORT nItemId ) const;

	void				SetHelpId( USHORT nItemId, ULONG nHelpId );
	ULONG				GetHelpId( USHORT nItemId ) const;

	void				SetActivateHdl( const Link& rLink ) 	{ aActivateHdl = rLink; }
	const Link& 		GetActivateHdl() const					{ return aActivateHdl; }

	void				SetDeactivateHdl( const Link& rLink )	{ aDeactivateHdl = rLink; }
	const Link& 		GetDeactivateHdl() const				{ return aDeactivateHdl; }

	void				SetHighlightHdl( const Link& rLink )	{ aHighlightHdl = rLink; }
	const Link& 		GetHighlightHdl() const 				{ return aHighlightHdl; }

	void				SetSelectHdl( const Link& rLink )		{ aSelectHdl = rLink; }
	const Link& 		GetSelectHdl() const					{ return aSelectHdl; }

	void				SetLogo( const MenuLogo& rLogo );
	void				SetLogo();
	BOOL				HasLogo() const { return pLogo ? TRUE : FALSE; }
	MenuLogo			GetLogo() const;
	
    void                AddEventListener( const Link& rEventListener );
    void                RemoveEventListener( const Link& rEventListener );
    //void                AddChildEventListener( const Link& rEventListener );
    //void                RemoveChildEventListener( const Link& rEventListener );
	
	Menu&				operator =( const Menu& rMenu );

	void				GetAccessObject( AccessObjectRef& rAcc ) const;

	// Fuer Menu-'Funktionen'
	MenuItemList*		GetItemList() const 					{ return pItemList; }

    // returns the system's menu handle if native menues are supported
    // pData must point to a SystemMenuData structure
    BOOL                GetSystemMenuData( SystemMenuData* pData ) const;

    // accessibility helpers

    // gets the displayed text
    String GetDisplayText() const;
    // returns the bounding box for the character at index nIndex
	// where nIndex is relative to the starting index of the item
    // with id nItemId (in coordinates of the displaying window)
    Rectangle GetCharacterBounds( USHORT nItemId, long nIndex ) const;
    // -1 is returned if no character is at that point
    // if an index is found the corresponding item id is filled in (else 0)
    long GetIndexForPoint( const Point& rPoint, USHORT& rItemID ) const;
    // returns the number of lines in the result of GetDisplayText()
    long GetLineCount() const;
    // returns the interval [start,end] of line nLine
    // returns [-1,-1] for an invalid line
    Pair GetLineStartEnd( long nLine ) const;
    // like GetLineStartEnd but first finds the line number for the item
    Pair GetItemStartEnd( USHORT nItemId ) const;
    // returns the item id for line nLine or 0 if nLine is invalid
    USHORT GetDisplayItemId( long nLine ) const;
    // returns the bounding rectangle for an item at pos nItemPos
    Rectangle GetBoundingRectangle( USHORT nItemPos ) const;
    BOOL ConvertPoint( Point& rPoint, Window* pReferenceWindow ) const;

    ::com::sun::star::uno::Reference< ::com::sun::star::accessibility::XAccessible > GetAccessible();
	void SetAccessible( const ::com::sun::star::uno::Reference< ::com::sun::star::accessibility::XAccessible >& rxAccessible );

    // gets the activation key of the specified item
    KeyEvent GetActivationKey( USHORT nItemId ) const;

	Window* 			GetWindow() const { return pWindow; }

	void				SetAccessibleName( USHORT nItemId, const XubString& rStr );
	XubString			GetAccessibleName( USHORT nItemId ) const;

	void				SetAccessibleDescription( USHORT nItemId, const XubString& rStr );
	XubString			GetAccessibleDescription( USHORT nItemId ) const;
};

// -----------
// - MenuBar -
// -----------

class MenuBar : public Menu
{
	void*				pDummy;
	Link				maCloserHdl;
	Link				maFloatHdl;
	Link				maHideHdl;
	BOOL				mbCloserVisible;
	BOOL				mbFloatBtnVisible;
	BOOL				mbHideBtnVisible;
	BOOL				mbDisplayable;

#if _SOLAR__PRIVATE
	friend class Application;
	friend class Menu;
	friend class MenuBarWindow;
	friend class MenuFloatingWindow;
	friend class SystemWindow;

	static Window*		ImplCreate( Window* pParent, Window* pWindow, MenuBar* pMenu );
	static void 		ImplDestroy( MenuBar* pMenu, BOOL bDelete );
	BOOL				ImplHandleKeyEvent( const KeyEvent& rKEvent, BOOL bFromMenu = TRUE );
#endif

public:
						MenuBar();
						MenuBar( const ResId& rResId );
						MenuBar( const MenuBar& rMenu );
						~MenuBar();

	MenuBar&			operator =( const MenuBar& rMenu );

	void				ShowCloser( BOOL bShow = TRUE );
	BOOL				HasCloser() const { return mbCloserVisible; }
	void				ShowFloatButton( BOOL bShow = TRUE );
	BOOL				HasFloatButton() const { return mbFloatBtnVisible; }
	void				ShowHideButton( BOOL bShow = TRUE );
	BOOL				HasHideButton() const { return mbHideBtnVisible; }
	void				ShowButtons( BOOL bClose, BOOL bFloat, BOOL bHide );

	void				SelectEntry( USHORT nId );
	BOOL                HandleMenuActivateEvent( Menu *pMenu ) const;
	BOOL                HandleMenuDeActivateEvent( Menu *pMenu ) const;
	BOOL                HandleMenuHighlightEvent( Menu *pMenu, USHORT nEventId ) const;
	BOOL                HandleMenuCommandEvent( Menu *pMenu, USHORT nEventId ) const;

	void				SetCloserHdl( const Link& rLink )			{ maCloserHdl = rLink; }
	const Link& 		GetCloserHdl() const						{ return maCloserHdl; }
	void				SetFloatButtonClickHdl( const Link& rLink ) { maFloatHdl = rLink; }
	const Link& 		GetFloatButtonClickHdl() const				{ return maFloatHdl; }
	void				SetHideButtonClickHdl( const Link& rLink )	{ maHideHdl = rLink; }
	const Link& 		GetHideButtonClickHdl() const				{ return maHideHdl; }

    //  - by default a menubar is displayable
    //  - if a menubar is not displayable, its MenuBarWindow will never be shown
    //    and it will be hidden if it was visible before
    //  - note: if a menubar is diplayable, this does not necessarily mean that it is currently visible
    void                SetDisplayable( BOOL bDisplayable );
    BOOL                IsDisplayable() const                       { return mbDisplayable; }
};

inline MenuBar& MenuBar::operator =( const MenuBar& rMenu )
{
	Menu::operator =( rMenu );
	return *this;
}


// -------------
// - PopupMenu -
// -------------

class PopupMenu : public Menu
{
	friend class Menu;
	friend class MenuFloatingWindow;
	friend class MenuBarWindow;
    friend struct MenuItemData;

private:
	Menu**				pRefAutoSubMenu;    // keeps track if a pointer to this Menu is stored in the MenuItemData

	MenuFloatingWindow* ImplGetFloatingWindow() const { return (MenuFloatingWindow*)Menu::ImplGetWindow(); }

protected:
	USHORT				ImplExecute( Window* pWindow, const Rectangle& rRect, ULONG nPopupFlags, Menu* pStaredFrom, BOOL bPreSelectFirst );
	long				ImplCalcHeight( USHORT nEntries ) const;
	USHORT				ImplCalcVisEntries( long nMaxHeight, USHORT nStartEntry = 0, USHORT* pLastVisible = NULL ) const;

public:
						PopupMenu();
						PopupMenu( const PopupMenu& rMenu );
						PopupMenu( const ResId& rResId );
						~PopupMenu();

	void				SetText( const XubString& rTitle )	{ aTitleText = rTitle; }
	const XubString&	GetText() const 					{ return aTitleText; }

	USHORT				Execute( Window* pWindow, const Point& rPopupPos );
	USHORT				Execute( Window* pWindow, const Rectangle& rRect, USHORT nFlags = 0 );

	// Fuer das TestTool
	void				EndExecute( USHORT nSelect = 0 );
	void				SelectEntry( USHORT nId );

	static BOOL 		IsInExecute();
	static PopupMenu*	GetActivePopupMenu();

	PopupMenu&			operator =( const PopupMenu& rMenu );
};

inline PopupMenu& PopupMenu::operator =( const PopupMenu& rMenu )
{
	Menu::operator =( rMenu );
	return *this;
}

#endif // _SV_MENU_HXX

