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
 *  Patrick Luby, June 2003
 *
 *  GNU General Public License Version 2.1
 *  =============================================
 *  Copyright 2003 Planamesa Inc.
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
 ************************************************************************/

#define _SV_COM_SUN_STAR_VCL_VCLEVENT_CXX

#ifndef _SV_COM_SUN_STAR_VCL_VCLEVENT_HXX
#include <com/sun/star/vcl/VCLEvent.hxx>
#endif
#ifndef _SV_COM_SUN_STAR_VCL_VCLFRAME_HXX
#include <com/sun/star/vcl/VCLFrame.hxx>
#endif
#ifndef _SV_COM_SUN_STAR_VCL_VCLGRAPHICS_HXX
#include <com/sun/star/vcl/VCLGraphics.hxx>
#endif
#ifndef _SV_SALDATA_HXX
#include <saldata.hxx>
#endif
#ifndef _SV_SALFRAME_H
#include <salframe.h>
#endif
#ifndef _SV_EVENT_HXX
#include <vcl/event.hxx>
#endif
#ifndef _SV_SALMENU_H
#include <salmenu.h>
#endif
#ifndef _SV_SVAPP_HXX
#include <vcl/svapp.hxx>
#endif
#ifndef _SV_CTRL_HXX
#include <vcl/ctrl.hxx>
#endif
#ifndef _SV_FLOATWIN_HXX
#include <vcl/floatwin.hxx>
#endif
#ifndef _VCL_UNOHELP_HXX
#include <vcl/unohelp.hxx>
#endif
#ifndef _VOS_MODULE_HXX_
#include <vos/module.hxx>
#endif

#include "VCLEventQueue_cocoa.h"

typedef void NativeAboutMenuHandler_Type();
typedef void NativePreferencesMenuHandler_Type();
typedef void NativeShutdownCancelledHandler_Type();

static ::vos::OModule aAboutHandlerModule;
static ::vos::OModule aPreferencesHandlerModule;
static ::vos::OModule aShutdownCancelledHandlerModule;
static NativeAboutMenuHandler_Type *pAboutHandler = NULL;
static NativePreferencesMenuHandler_Type *pPreferencesHandler = NULL;
static NativeShutdownCancelledHandler_Type *pShutdownCancelledHandler = NULL;

using namespace rtl;
using namespace vcl;
using namespace vos;

// ============================================================================

static JavaSalFrame *FindMouseEventFrame( JavaSalFrame *pFrame, const Point &rScreenPoint )
{
	if ( !pFrame->mbVisible )
		return NULL;

	// Iterate through children
	for ( ::std::list< JavaSalFrame* >::const_iterator it = pFrame->maChildren.begin(); it != pFrame->maChildren.end(); ++it )
	{
		JavaSalFrame *pRet = pFrame;
		pRet = FindMouseEventFrame( *it, rScreenPoint );
		if ( pRet && pRet != pFrame )
			return pRet;
	}

	if ( pFrame->IsFloatingFrame() && ! ( pFrame->mnStyle & SAL_FRAME_STYLE_TOOLTIP ) )
	{
		Rectangle aBounds( Point( pFrame->maGeometry.nX - pFrame->maGeometry.nLeftDecoration, pFrame->maGeometry.nY - pFrame->maGeometry.nTopDecoration ), Size( pFrame->maGeometry.nWidth + pFrame->maGeometry.nLeftDecoration + pFrame->maGeometry.nRightDecoration, pFrame->maGeometry.nHeight + pFrame->maGeometry.nTopDecoration + pFrame->maGeometry.nBottomDecoration ) );
		if ( aBounds.IsInside( rScreenPoint ) )
			return pFrame;
	}

	return NULL;
}

// ----------------------------------------------------------------------------

static void InvalidateControls( Window *pWindow )
{
	if ( pWindow && pWindow->IsReallyVisible() )
	{
		Control *pCtrl = dynamic_cast< Control * >( pWindow );
		if ( pCtrl )
		{
			pCtrl->Invalidate();
			return;
		}

		USHORT nCount = pWindow->GetChildCount();
		for ( USHORT i = 0; i < nCount; i++ )
			InvalidateControls( pWindow->GetChild( i ) );
	}
}

// ============================================================================

jclass com_sun_star_vcl_VCLEvent::theClass = NULL;

// ----------------------------------------------------------------------------

jclass com_sun_star_vcl_VCLEvent::getMyClass()
{
	if ( !theClass )
	{
		VCLThreadAttach t;
		if ( !t.pEnv ) return (jclass)NULL;
		jclass tempClass = t.pEnv->FindClass( "com/sun/star/vcl/VCLEvent" );
		OSL_ENSURE( tempClass, "Java : FindClass not found!" );
		theClass = (jclass)t.pEnv->NewGlobalRef( tempClass );
	}
	return theClass;
}

// ----------------------------------------------------------------------------

com_sun_star_vcl_VCLEvent::com_sun_star_vcl_VCLEvent( USHORT nID, const JavaSalFrame *pFrame, void *pData ) : java_lang_Object( (jobject)NULL )
{
	static jmethodID mID = NULL;
	VCLThreadAttach t;
	if ( !t.pEnv )
		return;
	if ( !mID )
	{
		char *cSignature = "(ILcom/sun/star/vcl/VCLFrame;J)V";
		mID = t.pEnv->GetMethodID( getMyClass(), "<init>", cSignature );
	}
	OSL_ENSURE( mID, "Unknown method id!" );
	jvalue args[3];
	args[0].i = jint( nID );
	args[1].l = pFrame ? pFrame->mpVCLFrame->getJavaObject() : NULL;
	args[2].j = jlong( pData );
	jobject tempObj;
	tempObj = t.pEnv->NewObjectA( getMyClass(), mID, args );
	saveRef( tempObj );
}

// ----------------------------------------------------------------------------

com_sun_star_vcl_VCLEvent::com_sun_star_vcl_VCLEvent( USHORT nID, const JavaSalFrame *pFrame, void *pData, const char *str ) : java_lang_Object( (jobject)NULL )
{
	static jmethodID mID = NULL;
	VCLThreadAttach t;
	if ( !t.pEnv )
		return;
	if ( !mID )
	{
		char *cSignature = "(ILcom/sun/star/vcl/VCLFrame;JLjava/lang/String;)V";
		mID = t.pEnv->GetMethodID( getMyClass(), "<init>", cSignature );
	}
	OSL_ENSURE( mID, "Unknown method id!" );
	jvalue args[4];
	args[0].i = jint( nID );
	args[1].l = pFrame ? pFrame->mpVCLFrame->getJavaObject() : NULL;
	args[2].j = jlong( pData );
	args[3].l = t.pEnv->NewStringUTF( str );

	jobject tempObj;
	tempObj = t.pEnv->NewObjectA( getMyClass(), mID, args );
	saveRef( tempObj );
}

// ----------------------------------------------------------------------------

void com_sun_star_vcl_VCLEvent::cancelShutdown()
{
	static jmethodID mID = NULL;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "()V";
			mID = t.pEnv->GetMethodID( getMyClass(), "cancelShutdown", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
			t.pEnv->CallNonvirtualVoidMethod( object, getMyClass(), mID );
	}
}

// ----------------------------------------------------------------------------

void com_sun_star_vcl_VCLEvent::dispatch()
{
	USHORT nID = getID();
	void *pData = getData();
	SalData *pSalData = GetSalData();

	// Handle events that do not need a JavaSalFrame pointer
	switch ( nID )
	{
		case SALEVENT_SHUTDOWN:
		{
			bool bCancelShutdown = true;

			// Ignore SALEVENT_SHUTDOWN events when recursing into this
			// method or when in presentation mode
			if ( !isShutdownCancelled() )
			{
				ImplSVData *pSVData = ImplGetSVData();
				if ( pSVData->maAppData.mnDispatchLevel == 1 && !pSVData->maWinData.mpFirstFloat && !pSVData->maWinData.mpLastExecuteDlg && !pSalData->mbInNativeModalSheet && pSalData->maFrameList.size() )
				{
					JavaSalFrame *pFrame = pSalData->maFrameList.front();
					if ( pFrame && !pFrame->CallCallback( nID, NULL ) )
						bCancelShutdown = false;
				}
			}

			if ( bCancelShutdown )
			{
				cancelShutdown();

				// Load libsfx and invoke the native shutdown cancelled handler
				if ( !pShutdownCancelledHandler )
				{
					OUString aLibName = ::vcl::unohelper::CreateLibraryName( "sfx", TRUE );
					if ( aShutdownCancelledHandlerModule.load( aLibName ) )
						pShutdownCancelledHandler = (NativeShutdownCancelledHandler_Type *)aShutdownCancelledHandlerModule.getSymbol( OUString::createFromAscii( "NativeShutdownCancelledHandler" ) );
				}

				if ( pShutdownCancelledHandler )
					pShutdownCancelledHandler();
			}

			return;
		}
		case SALEVENT_OPENDOCUMENT:
		case SALEVENT_PRINTDOCUMENT:
		{
			// Fix bug 168 && 607 by reposting SALEVENT_*DOCUMENT events when
			// recursing into this method while opening a document
			ImplSVData *pSVData = ImplGetSVData();
			if ( pSVData && pSVData->maAppData.mnDispatchLevel == 1 && !pSVData->maWinData.mpLastExecuteDlg && !pSalData->mbInNativeModalSheet )
			{
				String aEmptyStr;
				ApplicationEvent aAppEvt( aEmptyStr, aEmptyStr, SALEVENT_OPENDOCUMENT ? APPEVENT_OPEN_STRING : APPEVENT_PRINT_STRING, getPath() );
				pSVData->mpApp->AppEvent( aAppEvt );
			}
			else
			{
				com_sun_star_vcl_VCLEvent *pEvent = new com_sun_star_vcl_VCLEvent( getJavaObject() );
				pSalData->maPendingDocumentEventsList.push_back( pEvent );
			}
			return;
		}
		case SALEVENT_ABOUT:
		{
			// Load libsfx and invoke the native preferences handler
			if ( !pAboutHandler )
			{
				OUString aLibName = ::vcl::unohelper::CreateLibraryName( "sfx", TRUE );
				if ( aAboutHandlerModule.load( aLibName ) )
					pAboutHandler = (NativeAboutMenuHandler_Type *)aAboutHandlerModule.getSymbol( OUString::createFromAscii( "NativeAboutMenuHandler" ) );
			}

			if ( pAboutHandler && !pSalData->mbInNativeModalSheet )
				pAboutHandler();

			return;
		}
		case SALEVENT_PREFS:
		{
			// Load libofa and invoke the native preferences handler
			if ( !pPreferencesHandler )
			{
				OUString aLibName = ::vcl::unohelper::CreateLibraryName( "sfx", TRUE );
				if ( aPreferencesHandlerModule.load( aLibName ) )
					pPreferencesHandler = (NativePreferencesMenuHandler_Type *)aPreferencesHandlerModule.getSymbol( OUString::createFromAscii( "NativePreferencesMenuHandler" ) );
			}

			if ( pPreferencesHandler && !pSalData->mbInNativeModalSheet )
				pPreferencesHandler();

			return;
		}
	}
	
	// Handle events that require a JavaSalFrame pointer
	JavaSalFrame *pFrame = getFrame();
	bool bFound = false;
	if ( pFrame )
	{
		for ( ::std::list< JavaSalFrame* >::const_iterator it = pSalData->maFrameList.begin(); it != pSalData->maFrameList.end(); ++it )
		{
			if ( pFrame == *it )
			{
				if ( pFrame->GetWindow() )
					bFound = true;
				break;
			}
		}
	}

	if ( !bFound )
		pFrame = NULL;

	bool bDeleteDataOnly = false;
	if ( pSalData->mbInNativeModalSheet && pFrame != pSalData->mpNativeModalSheetFrame )
	{
		// We need to prevent dispatching of events other than system events
		// like bounds change or paint events. Fix bug 3429 by only forcing
		// a focus change if there is not a native modal window showing.
		bDeleteDataOnly = true;
		if ( NSApplication_isActive() )
			pSalData->mpNativeModalSheetFrame->ToTop( SAL_FRAME_TOTOP_RESTOREWHENMIN | SAL_FRAME_TOTOP_GRABFOCUS );
	}

	switch ( nID )
	{
		case SALEVENT_CLOSE:
		{
			if ( !bDeleteDataOnly && pFrame && pFrame->mbVisible )
				pFrame->CallCallback( nID, NULL );
			break;
		}
		case SALEVENT_DEMINIMIZED:
		{
			if ( !bDeleteDataOnly && pFrame && pFrame->mbVisible )
			{
				for ( ::std::list< JavaSalFrame* >::const_iterator it = pFrame->maChildren.begin(); it != pFrame->maChildren.end(); ++it )
				{
					if ( (*it)->mbVisible )
					{
						(*it)->Show( FALSE );
						(*it)->Show( TRUE, TRUE );
					}
				}
			}
			break;
		}
		case SALEVENT_MINIMIZED:
		{
			if ( !bDeleteDataOnly && pFrame && pFrame->mbVisible )
			{
				for ( ::std::list< JavaSalFrame* >::const_iterator it = pFrame->maChildren.begin(); it != pFrame->maChildren.end(); ++it )
				{
					// Hide Java window but leave visible flag set to true
					if ( (*it)->mbVisible )
						(*it)->mpVCLFrame->setVisible( sal_False, sal_False );
				}
			}
			break;
		}
		case SALEVENT_ENDEXTTEXTINPUT:
		{
			SalExtTextInputEvent *pInputEvent = (SalExtTextInputEvent *)pData;

			if ( !bDeleteDataOnly && pFrame && pFrame->mbVisible )
			{
				// Fix bug 1158 by resetting the focus to whichever window is
				// receiving key events
				if ( pFrame != pSalData->mpFocusFrame )
				{
					com_sun_star_vcl_VCLEvent aEvent( SALEVENT_GETFOCUS, pFrame, NULL );
					aEvent.dispatch();
				}
				pFrame->CallCallback( nID, pInputEvent );
			}
			if ( pInputEvent )
				delete pInputEvent;
			break;
		}
		case SALEVENT_EXTTEXTINPUT:
		{
			SalExtTextInputEvent *pInputEvent = (SalExtTextInputEvent *)pData;
			if ( !bDeleteDataOnly && pFrame && pFrame->mbVisible )
			{
				ULONG nCommitted = getCommittedCharacterCount();
				if ( !pInputEvent )
				{
					ULONG nCursorPos = getCursorPosition();
					pInputEvent = new SalExtTextInputEvent();
					pInputEvent->mnTime = getWhen();
					pInputEvent->maText = XubString( getText() );
					pInputEvent->mpTextAttr = getTextAttributes();
					pInputEvent->mnCursorPos = nCursorPos > nCommitted ? nCursorPos : nCommitted;
					pInputEvent->mnDeltaStart = 0;
					pInputEvent->mbOnlyCursor = FALSE;
					pInputEvent->mnCursorFlags = 0;
				}
				// Fix bug 1158 by resetting the focus to whichever window is
				// receiving key events
				if ( pFrame != pSalData->mpFocusFrame )
				{
					com_sun_star_vcl_VCLEvent aEvent( SALEVENT_GETFOCUS, pFrame, NULL );
					aEvent.dispatch();
				}
				pFrame->CallCallback( nID, pInputEvent );
				// If there is no text, the character is committed
				if ( pInputEvent->maText.Len() == nCommitted )
					pFrame->CallCallback( SALEVENT_ENDEXTTEXTINPUT, NULL );
				if ( pInputEvent->mpTextAttr )
					rtl_freeMemory( (USHORT *)pInputEvent->mpTextAttr );
				// Update the cached cursor location
			}
			if ( pInputEvent )
				delete pInputEvent;
			break;
		}
		case SALEVENT_EXTTEXTINPUTPOS:
		{
			SalExtTextInputPosEvent *pInputPosEvent = (SalExtTextInputPosEvent *)pData;
			if ( pInputPosEvent && !bDeleteDataOnly && pFrame && pFrame->mbVisible )
				pFrame->CallCallback( SALEVENT_EXTTEXTINPUTPOS, (void *)pInputPosEvent );
			break;
		}
		case SALEVENT_GETFOCUS:
		{
			// Ignore focus events for floating windows
			if ( !bDeleteDataOnly )
			{
				if ( pFrame != pSalData->mpFocusFrame )
				{
					if ( pSalData->mpFocusFrame && pSalData->mpFocusFrame->mbVisible )
						pSalData->mpFocusFrame->CallCallback( SALEVENT_LOSEFOCUS, NULL );
					pSalData->mpFocusFrame = NULL;
				}

				if ( pFrame && pFrame->mbVisible && !pFrame->IsFloatingFrame() )
				{
					pSalData->mpFocusFrame = pFrame;
					pFrame->CallCallback( nID, NULL );

					Window *pWindow = Application::GetFirstTopLevelWindow();
					while ( pWindow && pWindow->ImplGetFrame() != pFrame )
						pWindow = Application::GetNextTopLevelWindow( pWindow );
					InvalidateControls( pWindow );
				}
			}

			break;
		}
		case SALEVENT_LOSEFOCUS:
		{
			if ( !bDeleteDataOnly && pFrame )
			{
				if ( pFrame == pSalData->mpFocusFrame )
				{
					pSalData->mpFocusFrame = NULL;
					pFrame->CallCallback( nID, NULL );
				}

				for ( ::std::list< JavaSalFrame* >::const_iterator it = pSalData->maFrameList.begin(); it != pSalData->maFrameList.end(); ++it )
				{
					if ( pFrame == *it )
					{
						if ( pFrame->mbVisible && !pFrame->IsFloatingFrame() )
						{
							Window *pWindow = Application::GetFirstTopLevelWindow();
							while ( pWindow && pWindow->ImplGetFrame() != pFrame )
								pWindow = Application::GetNextTopLevelWindow( pWindow );
							InvalidateControls( pWindow );
						}
					}
				}
			}

			break;
		}
		case SALEVENT_KEYINPUT:
		case SALEVENT_KEYUP:
		{
			SalKeyEvent *pKeyEvent = (SalKeyEvent *)pData;
			if ( !bDeleteDataOnly && pFrame && pFrame->mbVisible )
			{
				if ( !pKeyEvent )
				{
					pKeyEvent = new SalKeyEvent();
					pKeyEvent->mnTime = getWhen();
					pKeyEvent->mnCode = getKeyCode() | getModifiers();
					pKeyEvent->mnCharCode = getKeyChar();
					pKeyEvent->mnRepeat = getRepeatCount();
				}
				// Fix bug 1158 by resetting the focus to whichever window is
				// receiving key events
				if ( pFrame != pSalData->mpFocusFrame )
				{
					com_sun_star_vcl_VCLEvent aEvent( SALEVENT_GETFOCUS, pFrame, NULL );
					aEvent.dispatch();
				}
				// Pass all potential menu shortcuts received by a utility
				// window to its parent window
				if ( pKeyEvent->mnCode & KEY_MOD1 )
				{
					// Fix bug 3432 by not sending Command-Shift-F10 to parent
					while ( pFrame->mpParent && pFrame->mpParent->mbVisible && pFrame->IsUtilityWindow() && pKeyEvent->mnCode != ( KEY_MOD1 | KEY_SHIFT | KEY_F10 ) )
						pFrame = pFrame->mpParent;
				}
				pFrame->CallCallback( nID, pKeyEvent );
			}
			if ( pKeyEvent )
				delete pKeyEvent;
			break;
		}
		case SALEVENT_KEYMODCHANGE:
		{
			SalKeyModEvent *pKeyModEvent = (SalKeyModEvent *)pData;
			if ( !bDeleteDataOnly && pFrame && pFrame->mbVisible )
			{
				if ( !pKeyModEvent )
				{
					pKeyModEvent = new SalKeyModEvent();
					pKeyModEvent->mnTime = getWhen();
					pKeyModEvent->mnCode = getModifiers();
				}
				pFrame->CallCallback( nID, pKeyModEvent );
			}
			if ( pKeyModEvent )
				delete pKeyModEvent;
			break;
		}
		case SALEVENT_MOUSEBUTTONDOWN:
		case SALEVENT_MOUSEBUTTONUP:
		case SALEVENT_MOUSELEAVE:
		case SALEVENT_MOUSEMOVE:
		{
			SalMouseEvent *pMouseEvent = (SalMouseEvent *)pData;
			if ( !bDeleteDataOnly && pFrame && pFrame->mbVisible )
			{
				if ( !pMouseEvent )
				{
					USHORT nModifiers = getModifiers();
					pMouseEvent = new SalMouseEvent();
					pMouseEvent->mnTime = getWhen();
					pMouseEvent->mnX = getX();
					pMouseEvent->mnY = getY();
					pMouseEvent->mnCode = nModifiers;
					if ( nID == SALEVENT_MOUSELEAVE || nID == SALEVENT_MOUSEMOVE )
						pMouseEvent->mnButton = 0;
					else
						pMouseEvent->mnButton = nModifiers & ( MOUSE_LEFT | MOUSE_MIDDLE | MOUSE_RIGHT );
				}

				USHORT nButtons = pMouseEvent->mnCode & ( MOUSE_LEFT | MOUSE_MIDDLE | MOUSE_RIGHT );
				if ( nButtons && nID == SALEVENT_MOUSEMOVE && !pSalData->mpLastDragFrame )
					pSalData->mpLastDragFrame = pFrame;

				// Find the real mouse frame
				JavaSalFrame *pOriginalFrame = pFrame;
				Point aScreenPoint( pMouseEvent->mnX + pFrame->maGeometry.nX, pMouseEvent->mnY + pFrame->maGeometry.nY );
				if ( pSalData->mpCaptureFrame && pSalData->mpCaptureFrame->mbVisible )
				{
					if ( pSalData->mpCaptureFrame != pFrame )
					{
						pMouseEvent->mnX = aScreenPoint.X() - pSalData->mpCaptureFrame->maGeometry.nX;
						pMouseEvent->mnY = aScreenPoint.Y() - pSalData->mpCaptureFrame->maGeometry.nY;
						pFrame = pSalData->mpCaptureFrame;
					}
				}
				else if ( nID != SALEVENT_MOUSELEAVE )
				{
					JavaSalFrame *pMouseFrame = FindMouseEventFrame( pFrame, aScreenPoint );
					if ( pMouseFrame && pMouseFrame != pFrame && pMouseFrame->mbVisible )
					{
						pMouseEvent->mnX = aScreenPoint.X() - pMouseFrame->maGeometry.nX;
						pMouseEvent->mnY = aScreenPoint.Y() - pMouseFrame->maGeometry.nY;
						pFrame = pMouseFrame;
					}
				}

				// If we are releasing after dragging, send the event to the
				// last dragged frame
				if ( pSalData->mpLastDragFrame && ( nID == SALEVENT_MOUSEBUTTONUP || nID == SALEVENT_MOUSEMOVE ) )
				{
 					if ( pSalData->mpLastDragFrame != pFrame && pSalData->mpLastDragFrame->mbVisible )
					{
						// If dragging and there are floating windows visible,
						// don't let the mouse fall through to a non-floating
						// window
						if ( nID == SALEVENT_MOUSEBUTTONUP || ( pFrame == pOriginalFrame && !pFrame->IsFloatingFrame() ) )
						{
							pMouseEvent->mnX = aScreenPoint.X() - pSalData->mpLastDragFrame->maGeometry.nX;
							pMouseEvent->mnY = aScreenPoint.Y() - pSalData->mpLastDragFrame->maGeometry.nY;
							pFrame = pSalData->mpLastDragFrame;
						}
					}

					if ( nButtons )
						pSalData->mpLastDragFrame = pFrame;
					else
						pSalData->mpLastDragFrame = NULL;
				}

				// Check if we are not clicking on a floating window
				FloatingWindow *pPopupWindow = NULL;
    			if ( nID == SALEVENT_MOUSEBUTTONDOWN )
				{
					ImplSVData* pSVData = ImplGetSVData();
					if ( !pFrame->IsFloatingFrame() && pSVData && pSVData->maWinData.mpFirstFloat )
					{
						static const char* pEnv = getenv( "SAL_FLOATWIN_NOAPPFOCUSCLOSE" );
						if ( !(pSVData->maWinData.mpFirstFloat->GetPopupModeFlags() & FLOATWIN_POPUPMODE_NOAPPFOCUSCLOSE) && !(pEnv && *pEnv) )
							pPopupWindow = pSVData->maWinData.mpFirstFloat;
					}
				}

				// Adjust position for RTL layout
				if ( Application::GetSettings().GetLayoutRTL() )
					pMouseEvent->mnX = pFrame->maGeometry.nWidth - pFrame->maGeometry.nLeftDecoration - pFrame->maGeometry.nRightDecoration - pMouseEvent->mnX - 1;

				// Fix bugs 1583, 2166, and 2320 by setting the last pointer
				// state before dispatching the event
				pSalData->maLastPointerState.mnState = pMouseEvent->mnCode;
				pSalData->maLastPointerState.maPos = Point( aScreenPoint.X(), aScreenPoint.Y() );

				pFrame->CallCallback( nID, pMouseEvent );

    			if ( pPopupWindow )
				{
					ImplSVData* pSVData = ImplGetSVData();
					if ( pSVData && pSVData->maWinData.mpFirstFloat == pPopupWindow )
						pPopupWindow->EndPopupMode( FLOATWIN_POPUPMODEEND_CANCEL | FLOATWIN_POPUPMODEEND_CLOSEALL );
				}
			}
			if ( pMouseEvent )
				delete pMouseEvent;
			break;
		}
		case SALEVENT_MOVE:
		case SALEVENT_MOVERESIZE:
		case SALEVENT_RESIZE:
		{
			Rectangle *pPosSize = (Rectangle *)pData;
			if ( pFrame )
			{
				// Update size
				if ( !pPosSize )
					pPosSize = new Rectangle( pFrame->mpVCLFrame->getBounds() );

				// Fix bug 3252 by always comparing the bounds against the
				// work area
				bool bForceResize = false;
				if ( pFrame->mbInShow )
				{
					Rectangle aRect( *pPosSize );
					pFrame->GetWorkArea( aRect );
					if ( aRect == *pPosSize )
						bForceResize = true;
				}

				bool bPosChanged = false;
				int nX = pPosSize->nLeft + pFrame->maGeometry.nLeftDecoration;
				if ( pFrame->maGeometry.nX != nX )
				{
					bPosChanged = true;
					pFrame->maGeometry.nX = nX;
				}
				int nY = pPosSize->nTop + pFrame->maGeometry.nTopDecoration;
				if ( pFrame->maGeometry.nY != nY )
				{
					bPosChanged = true;
					pFrame->maGeometry.nY = nY;
				}

				bool bSizeChanged = false;
				unsigned int nWidth = pPosSize->GetWidth() - pFrame->maGeometry.nLeftDecoration - pFrame->maGeometry.nRightDecoration;
				if ( pFrame->maGeometry.nWidth != nWidth )
				{
					bSizeChanged = true;
					pFrame->maGeometry.nWidth = nWidth;
				}
				unsigned int nHeight = pPosSize->GetHeight() - pFrame->maGeometry.nTopDecoration - pFrame->maGeometry.nBottomDecoration;
				if ( pFrame->maGeometry.nHeight != nHeight )
				{
					bSizeChanged = true;
					pFrame->maGeometry.nHeight = nHeight;
				}

				// Fix bug 3045 by setting the event ID to the actual changes
				// that have occurred. This also fixes the autodocking of
				// native utility windows problem described in bug 3035
				if ( bForceResize || bPosChanged || bSizeChanged )
				{
					if ( bPosChanged && bSizeChanged )
						nID = SALEVENT_MOVERESIZE;
					else if ( bPosChanged )
						nID = SALEVENT_MOVE;
					else
						nID = SALEVENT_RESIZE;

					// Reset graphics
					com_sun_star_vcl_VCLGraphics *pVCLGraphics = pFrame->mpVCLFrame->getGraphics();
					if ( pVCLGraphics )
					{
						pVCLGraphics->resetGraphics();
						delete pVCLGraphics;
					}

					pFrame->CallCallback( nID, NULL );
				}
			}
			if ( pPosSize )
				delete pPosSize;
			break;
		}
		case SALEVENT_PAINT:
		{
			SalPaintEvent *pPaintEvent = (SalPaintEvent *)pData;
			if ( pFrame && pFrame->mbVisible )
			{
				if ( !pPaintEvent )
				{
					// Get paint region
					const Rectangle &aUpdateRect = getUpdateRect();
					pPaintEvent = new SalPaintEvent( aUpdateRect.nLeft, aUpdateRect.nTop, aUpdateRect.GetWidth(), aUpdateRect.GetHeight() );
				}
				// Adjust position for RTL layout
				if ( Application::GetSettings().GetLayoutRTL() )
					pPaintEvent->mnBoundX = pFrame->maGeometry.nWidth - pFrame->maGeometry.nLeftDecoration - pFrame->maGeometry.nRightDecoration - pPaintEvent->mnBoundWidth - pPaintEvent->mnBoundX;

				// Reset graphics
				com_sun_star_vcl_VCLGraphics *pVCLGraphics = pFrame->mpVCLFrame->getGraphics();
				if ( pVCLGraphics )
				{
					pVCLGraphics->resetGraphics();
					delete pVCLGraphics;
				}

				pFrame->CallCallback( nID, pPaintEvent );
			}
			if ( pPaintEvent )
				delete pPaintEvent;
			break;
		}
		case SALEVENT_USEREVENT:
		{
			if ( pFrame )
				pFrame->CallCallback( nID, pData );
			break;
		}
		case SALEVENT_WHEELMOUSE:
		{
			SalWheelMouseEvent *pWheelMouseEvent = (SalWheelMouseEvent *)pData;
			if ( !bDeleteDataOnly && pFrame && pFrame->mbVisible )
			{
				if ( !pWheelMouseEvent )
				{
					// The OOo code expects the opposite in signedness of Java
					// for vertical scrolling
					long nWheelRotation = getWheelRotation();
					BOOL bHorz = isHorizontal();
					if ( !bHorz )
						nWheelRotation *= -1;
					pWheelMouseEvent = new SalWheelMouseEvent();
					pWheelMouseEvent->mnTime = getWhen();
					pWheelMouseEvent->mnX = getX();
					pWheelMouseEvent->mnY = getY();
					pWheelMouseEvent->mnDelta = nWheelRotation * 120;
					pWheelMouseEvent->mnNotchDelta = nWheelRotation;
					pWheelMouseEvent->mnScrollLines = getScrollAmount();
					pWheelMouseEvent->mnCode = getModifiers();
					pWheelMouseEvent->mbHorz = bHorz;
				}
				// Adjust position for RTL layout
				if ( Application::GetSettings().GetLayoutRTL() )
					pWheelMouseEvent->mnX = pFrame->maGeometry.nWidth - pFrame->maGeometry.nLeftDecoration - pFrame->maGeometry.nRightDecoration - pWheelMouseEvent->mnX - 1;
				pFrame->CallCallback( nID, pWheelMouseEvent );
			}
			if ( pWheelMouseEvent )
				delete pWheelMouseEvent;
			break;
		}
		case SALEVENT_MENUACTIVATE:
		case SALEVENT_MENUCOMMAND:
		case SALEVENT_MENUDEACTIVATE:
		{
			SalMenuEvent *pMenuEvent = (SalMenuEvent *)pData;
			if ( !bDeleteDataOnly && pFrame && pFrame->mbVisible )
			{
				if ( !pMenuEvent )
					pMenuEvent = new SalMenuEvent( getMenuID(), getMenuCookie() );
				// Pass all menu selections received by a utility window to
				// its parent window
				if ( nID == SALEVENT_MENUCOMMAND )
				{
					while ( pFrame->mpParent && pFrame->mpParent->mbVisible && pFrame->IsUtilityWindow() )
						pFrame = pFrame->mpParent;
				}
				pFrame->CallCallback( nID, pMenuEvent );
			}
			if ( pMenuEvent )
				delete pMenuEvent;
			break;
		}
		default:
		{
			if ( pFrame && pFrame->mbVisible )
				pFrame->CallCallback( nID, pData );
			break;
		}
	}
}

// ----------------------------------------------------------------------------

ULONG com_sun_star_vcl_VCLEvent::getCommittedCharacterCount()
{
	static jmethodID mID = NULL;
	ULONG out = 0;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "()I";
			mID = t.pEnv->GetMethodID( getMyClass(), "getCommittedCharacterCount", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
			out = (ULONG)t.pEnv->CallNonvirtualIntMethod( object, getMyClass(), mID );
	}
	return out;
}

// ----------------------------------------------------------------------------

ULONG com_sun_star_vcl_VCLEvent::getCursorPosition()
{
	static jmethodID mID = NULL;
	ULONG out = 0;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "()I";
			mID = t.pEnv->GetMethodID( getMyClass(), "getCursorPosition", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
			out = (long)t.pEnv->CallNonvirtualIntMethod( object, getMyClass(), mID );
	}
	return out;
}

// ----------------------------------------------------------------------------

void *com_sun_star_vcl_VCLEvent::getData()
{
	static jmethodID mID = NULL;
	void *out = NULL;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "()J";
			mID = t.pEnv->GetMethodID( getMyClass(), "getData", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
			out = (void *)t.pEnv->CallNonvirtualLongMethod( object, getMyClass(), mID );
	}
	return out;
}

// ----------------------------------------------------------------------------

JavaSalFrame *com_sun_star_vcl_VCLEvent::getFrame()
{
	static jmethodID mID = NULL;
	JavaSalFrame *out = NULL;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "()J";
			mID = t.pEnv->GetMethodID( getMyClass(), "getFrame", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
			out = (JavaSalFrame *)t.pEnv->CallNonvirtualLongMethod( object, getMyClass(), mID );
	}
	return out;
}

// ----------------------------------------------------------------------------

USHORT com_sun_star_vcl_VCLEvent::getKeyChar()
{
	static jmethodID mID = NULL;
	USHORT out = 0;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "()C";
			mID = t.pEnv->GetMethodID( getMyClass(), "getKeyChar", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
			out = (USHORT)t.pEnv->CallNonvirtualCharMethod( object, getMyClass(), mID );
	}
	return out;
}

// ----------------------------------------------------------------------------

USHORT com_sun_star_vcl_VCLEvent::getKeyCode()
{
	static jmethodID mID = NULL;
	USHORT out = 0;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "()I";
			mID = t.pEnv->GetMethodID( getMyClass(), "getKeyCode", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
			out = (USHORT)t.pEnv->CallNonvirtualCharMethod( object, getMyClass(), mID );
	}
	return out;
}

// ----------------------------------------------------------------------------

USHORT com_sun_star_vcl_VCLEvent::getID()
{
	static jmethodID mID = NULL;
	USHORT out = 0;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "()I";
			mID = t.pEnv->GetMethodID( getMyClass(), "getID", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
			out = (USHORT)t.pEnv->CallNonvirtualIntMethod( object, getMyClass(), mID );
	}
	return out;
}

// ----------------------------------------------------------------------------

USHORT com_sun_star_vcl_VCLEvent::getModifiers()
{
	static jmethodID mID = NULL;
	USHORT out = 0;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "()I";
			mID = t.pEnv->GetMethodID( getMyClass(), "getModifiers", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
			out = (USHORT)t.pEnv->CallNonvirtualIntMethod( object, getMyClass(), mID );
	}
	return out;
}

// ----------------------------------------------------------------------------

OUString com_sun_star_vcl_VCLEvent::getPath()
{
	static jmethodID mID = NULL;
	OUString out;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "()Ljava/lang/String;";
			mID = t.pEnv->GetMethodID( getMyClass(), "getPath", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
		{
			jstring tempObj = (jstring)t.pEnv->CallNonvirtualObjectMethod( object, getMyClass(), mID );
			if ( tempObj )
				out = JavaString2String( t.pEnv, tempObj );
		}
	}
	return out;
}

// ----------------------------------------------------------------------------

USHORT com_sun_star_vcl_VCLEvent::getRepeatCount()
{
	static jmethodID mID = NULL;
	USHORT out = 0;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "()S";
			mID = t.pEnv->GetMethodID( getMyClass(), "getRepeatCount", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
			out = (USHORT)t.pEnv->CallNonvirtualShortMethod( object, getMyClass(), mID );
	}
	return out;
}

// ----------------------------------------------------------------------------

OUString com_sun_star_vcl_VCLEvent::getText()
{
	static jmethodID mID = NULL;
	OUString out;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "()Ljava/lang/String;";
			mID = t.pEnv->GetMethodID( getMyClass(), "getText", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
		{
			jstring tempObj = (jstring)t.pEnv->CallNonvirtualObjectMethod( object, getMyClass(), mID );
			if ( tempObj )
				out = JavaString2String( t.pEnv, tempObj );
		}
	}
	return out;
}

// ----------------------------------------------------------------------------

USHORT *com_sun_star_vcl_VCLEvent::getTextAttributes()
{
	static jmethodID mID = NULL;
	USHORT *out = NULL;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "()[I";
			mID = t.pEnv->GetMethodID( getMyClass(), "getTextAttributes", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
		{
			jintArray tempObj = (jintArray)t.pEnv->CallNonvirtualObjectMethod( object, getMyClass(), mID );
			if ( tempObj )
			{
				jsize nAttributes = t.pEnv->GetArrayLength( tempObj );
				if ( nAttributes )
				{
					jboolean bCopy( sal_False );
					jint *pAttributes = (jint *)t.pEnv->GetPrimitiveArrayCritical( tempObj, &bCopy );
					out = (USHORT *)rtl_allocateMemory( sizeof( USHORT* ) * nAttributes );
					if ( out )
					{
						for ( jsize i = 0; i < nAttributes; i++)
							out[i] = pAttributes[i];
					}
					t.pEnv->ReleasePrimitiveArrayCritical( tempObj, (void *)pAttributes, JNI_ABORT );
				}
			}
		}
	}
	return out;
}

// ----------------------------------------------------------------------------

const Rectangle com_sun_star_vcl_VCLEvent::getUpdateRect()
{
	static jmethodID mID = NULL;
	static jfieldID fIDX = NULL;
	static jfieldID fIDY = NULL;
	static jfieldID fIDWidth = NULL;
	static jfieldID fIDHeight = NULL;
	Rectangle out( Point( 0, 0 ), Size( 0, 0 ) );
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "()Ljava/awt/Rectangle;";
			mID = t.pEnv->GetMethodID( getMyClass(), "getUpdateRect", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
		{
			jobject tempObj = t.pEnv->CallNonvirtualObjectMethod( object, getMyClass(), mID );
			if ( tempObj )
			{
				jclass tempObjClass = t.pEnv->GetObjectClass( tempObj );
				if ( !fIDX )
				{
					char *cSignature = "I";
					fIDX = t.pEnv->GetFieldID( tempObjClass, "x", cSignature );
				}
				OSL_ENSURE( fIDX, "Unknown field id!" );
				if ( !fIDY )
				{
					char *cSignature = "I";
					fIDY = t.pEnv->GetFieldID( tempObjClass, "y", cSignature );
				}
				OSL_ENSURE( fIDY, "Unknown field id!" );
				if ( !fIDWidth )
				{
					char *cSignature = "I";
					fIDWidth = t.pEnv->GetFieldID( tempObjClass, "width", cSignature );
				}
				OSL_ENSURE( fIDWidth, "Unknown field id!" );
				if ( !fIDHeight )
				{
					char *cSignature = "I";
					fIDHeight = t.pEnv->GetFieldID( tempObjClass, "height", cSignature );
				}
				OSL_ENSURE( fIDHeight, "Unknown field id!" );
				if ( fIDX && fIDY && fIDWidth && fIDHeight )
				{
					Point aPoint( (long)t.pEnv->GetIntField( tempObj, fIDX ), (long)t.pEnv->GetIntField( tempObj, fIDY ) );
					Size aSize( (long)t.pEnv->GetIntField( tempObj, fIDWidth ), (long)t.pEnv->GetIntField( tempObj, fIDHeight ) );
					out = Rectangle( aPoint, aSize );
				}
			}
		}
	}
	return out;
}

// ----------------------------------------------------------------------------

ULONG com_sun_star_vcl_VCLEvent::getWhen()
{
	static jmethodID mID = NULL;
	ULONG out = 0;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "()J";
			mID = t.pEnv->GetMethodID( getMyClass(), "getWhen", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
			out = (ULONG)t.pEnv->CallNonvirtualLongMethod( object, getMyClass(), mID );
	}
	return out;
}

// ----------------------------------------------------------------------------

long com_sun_star_vcl_VCLEvent::getX()
{
	static jmethodID mID = NULL;
	long out = 0;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "()I";
			mID = t.pEnv->GetMethodID( getMyClass(), "getX", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
			out = (long)t.pEnv->CallNonvirtualIntMethod( object, getMyClass(), mID );
	}
	return out;
}

// ----------------------------------------------------------------------------

long com_sun_star_vcl_VCLEvent::getY()
{
	static jmethodID mID = NULL;
	long out = 0;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "()I";
			mID = t.pEnv->GetMethodID( getMyClass(), "getY", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
			out = (long)t.pEnv->CallNonvirtualIntMethod( object, getMyClass(), mID );
	}
	return out;
}

// ----------------------------------------------------------------------------

short com_sun_star_vcl_VCLEvent::getMenuID()
{
	static jmethodID mID = NULL;
	short out = 0;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "()S";
			mID = t.pEnv->GetMethodID( getMyClass(), "getMenuID", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
			out = (short)t.pEnv->CallNonvirtualIntMethod( object, getMyClass(), mID );
	}
	return out;
}

// ----------------------------------------------------------------------------

void *com_sun_star_vcl_VCLEvent::getMenuCookie()
{
	static jmethodID mID = NULL;
	void *out = 0;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "()J";
			mID = t.pEnv->GetMethodID( getMyClass(), "getMenuCookie", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
			out = (void *)t.pEnv->CallNonvirtualLongMethod( object, getMyClass(), mID );
	}
	return out;
}

// ----------------------------------------------------------------------------

long com_sun_star_vcl_VCLEvent::getScrollAmount()
{
	static jmethodID mID = NULL;
	long out = 0;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "()I";
			mID = t.pEnv->GetMethodID( getMyClass(), "getScrollAmount", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
			out = (long)t.pEnv->CallNonvirtualIntMethod( object, getMyClass(), mID );
	}
	return out;
}

// ----------------------------------------------------------------------------

ULONG com_sun_star_vcl_VCLEvent::getVisiblePosition()
{
	static jmethodID mID = NULL;
	ULONG out = 0;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "()I";
			mID = t.pEnv->GetMethodID( getMyClass(), "getVisiblePosition", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
			out = (long)t.pEnv->CallNonvirtualIntMethod( object, getMyClass(), mID );
	}
	return out;
}

// ----------------------------------------------------------------------------

long com_sun_star_vcl_VCLEvent::getWheelRotation()
{
	static jmethodID mID = NULL;
	long out = 0;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "()I";
			mID = t.pEnv->GetMethodID( getMyClass(), "getWheelRotation", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
			out = (long)t.pEnv->CallNonvirtualIntMethod( object, getMyClass(), mID );
	}
	return out;
}

// ----------------------------------------------------------------------------

sal_Bool com_sun_star_vcl_VCLEvent::isHorizontal()
{
	static jmethodID mID = NULL;
	sal_Bool out = sal_False;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "()Z";
			mID = t.pEnv->GetMethodID( getMyClass(), "isHorizontal", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
			out = (sal_Bool)t.pEnv->CallNonvirtualBooleanMethod( object, getMyClass(), mID );
	}
	return out;
}

// ----------------------------------------------------------------------------

sal_Bool com_sun_star_vcl_VCLEvent::isShutdownCancelled()
{
	static jmethodID mID = NULL;
	sal_Bool out = sal_False;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "()Z";
			mID = t.pEnv->GetMethodID( getMyClass(), "isShutdownCancelled", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
			out = (sal_Bool)t.pEnv->CallNonvirtualBooleanMethod( object, getMyClass(), mID );
	}
	return out;
}
