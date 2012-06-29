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

#include <dlfcn.h>
#include <unistd.h>
#include <sys/syslimits.h>

#include <premac.h>
#import <Cocoa/Cocoa.h>
#include <postmac.h>
#undef check

#include <salbmp.h>
#include <saldata.hxx>
#include <salframe.h>
#include <salgdi.h>
#include <saljava.h>
#include <salmenu.h>
#include <salobj.h>
#include <salsys.h>
#include <saltimer.h>
#include <salvd.h>
#include <salprn.h>
#include <vcl/apptypes.hxx>
#include <vcl/ctrl.hxx>
#include <vcl/floatwin.hxx>
#include <vcl/jobset.h>
#include <vcl/print.h>
#include <vcl/salbtype.hxx>
#include <vcl/salimestatus.hxx>
#include <vcl/salptype.hxx>
#include <vcl/saltimer.hxx>
#include <vcl/unohelp.hxx>
#include <vos/module.hxx>
#include <tools/resmgr.hxx>
#include <tools/simplerm.hxx>

#include <premac.h>
#include <CoreServices/CoreServices.h>
#include <postmac.h>

#include "salinst.hrc"
#include "salinst_cocoa.h"
#include "../java/VCLEventQueue_cocoa.h"

class JavaSalI18NImeStatus : public SalI18NImeStatus
{
public:
							JavaSalI18NImeStatus() {}
	virtual					~JavaSalI18NImeStatus() {}

	virtual bool			canToggle() { return false; }
	virtual void			toggle() {}
};

typedef OSErr Gestalt_Type( OSType selector, long *response );
typedef void NativeAboutMenuHandler_Type();
typedef void NativePreferencesMenuHandler_Type();

static bool isLeopard = false;
static bool isSnowLeopard = false;
static bool isLion = false;
static bool isMountainLion = false;
static ::vos::OModule aAboutHandlerModule;
static ::vos::OModule aPreferencesHandlerModule;
static NativeAboutMenuHandler_Type *pAboutHandler = NULL;
static NativePreferencesMenuHandler_Type *pPreferencesHandler = NULL;
static bool bAllowReleaseYieldMutex = false;
static bool bInNativeDrag = false;
static SalYieldMutex aEventQueueMutex;
static ULONG nCurrentTimeout = 0;

using namespace osl;
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
	// Fix bug reported in the following NeoOffice forum support by
	// invalidating the entire window:
	// http://trinity.neooffice.org/modules.php?name=Forums&file=viewtopic&p=62916#62916
	if ( pWindow && pWindow->IsReallyVisible() )
		pWindow->Invalidate();
}

// -----------------------------------------------------------------------

static ULONG ReleaseEventQueueMutex()
{
	if ( aEventQueueMutex.GetThreadId() == OThread::getCurrentIdentifier() )
	{
		ULONG nCount = aEventQueueMutex.GetAcquireCount();
		ULONG n = nCount;
		while ( n )
		{
			aEventQueueMutex.release();
			n--;
		}
		return nCount;
	}
	else
	{
		return 0;
	}
}

// -----------------------------------------------------------------------

static void AcquireEventQueueMutex( ULONG nCount )
{
	while ( nCount )
	{
		aEventQueueMutex.acquire();
		nCount--;
	}
}

// ----------------------------------------------------------------------------

static void InitializeMacOSXVersion()
{
	static bool nInitialized = false;

	if ( nInitialized )
		return;
	
	void *pLib = dlopen( NULL, RTLD_LAZY | RTLD_LOCAL );
	if ( pLib )
	{
		Gestalt_Type *pGestalt = (Gestalt_Type *)dlsym( pLib, "Gestalt" );
		if ( pGestalt )
		{
			SInt32 res = 0;
			pGestalt( gestaltSystemVersionMajor, &res );
			if ( res == 10 )
			{
				res = 0;
				pGestalt( gestaltSystemVersionMinor, &res );
				switch ( res )
				{
					case 5:
						isLeopard = true;
						break;
					case 6:
						isSnowLeopard = true;
						break;
					case 7:
						isLion = true;
						break;
					case 8:
						isMountainLion = true;
						break;
					default:
						break;
				}
			}
		}

		dlclose( pLib );
	}

	nInitialized = true;
}

// ============================================================================

BOOL VCLInstance_setDragLock( BOOL bLock )
{
	BOOL bRet = FALSE;

	if ( bLock )
	{
		if ( !Application::IsShutDown() && !bInNativeDrag )
		{
			IMutex& rSolarMutex = Application::GetSolarMutex();
			rSolarMutex.acquire();
			if ( !Application::IsShutDown() )
			{	
				bInNativeDrag = TRUE;
				bRet = TRUE;
			}
			else
			{
				rSolarMutex.release();
				return bRet;
			}
		}
	}
	else if ( bInNativeDrag )
	{
		bInNativeDrag = FALSE;
		IMutex& rSolarMutex = Application::GetSolarMutex();
		rSolarMutex.release();
		bRet = TRUE;
	}

	return bRet;
}

// ----------------------------------------------------------------------------

BOOL VCLInstance_updateNativeMenus()
{
	BOOL bRet = FALSE;

	// Check if there is a native modal window as we will deadlock when a
	// native modal window is showing.
	if ( Application::IsShutDown() || bInNativeDrag || NSApplication_getModalWindow() )
		return bRet;

	// Fix bug 2783 by cancelling menu actions if the input method if the
	// there is any marked text in the key window
	if ( NSWindow_hasMarkedText( nil ) )
		return bRet;

	// Make sure that any events fetched from the queue while the application
	// mutex was unlocked are already dispatched before we try to lock the
	// mutex. Fix bug 3467 by speeding up acquiring of the event queue mutex
	// by releasing the application mutex if already acquired by this thread.
	bAllowReleaseYieldMutex = true;
	ULONG nCount = Application::ReleaseSolarMutex();
	bAllowReleaseYieldMutex = false;
	aEventQueueMutex.acquire();
	Application::AcquireSolarMutex( nCount );

	IMutex& rSolarMutex = Application::GetSolarMutex();
	rSolarMutex.acquire();

	ImplSVData *pSVData = ImplGetSVData();
	SalData *pSalData = GetSalData();
	if ( Application::IsShutDown() || pSalData->mbInNativeModalSheet || !pSVData || !pSalData )
	{
		rSolarMutex.release();
		aEventQueueMutex.release();
		return bRet;
	}

	pSalData->maNativeEventCondition.reset();

	// Dispatch pending VCL events until the queue is clear
	while ( !Application::IsShutDown() && !pSalData->maNativeEventCondition.check() )
		pSalData->mpFirstInstance->Yield( false, true );

	// Fix bug 3451 by not updating menus when there is a modal dialog
	bRet = ( !Application::IsShutDown() && !pSalData->mbInNativeModalSheet && !pSVData->maWinData.mpLastExecuteDlg );
	if ( bRet )
	{
		if ( pSalData->mpFocusFrame && pSalData->mpFocusFrame->mbVisible )
		{
			// Fix update problem in bug 1577 when the menubar is selected and
			// the focus frame is a child of another frame. Fix bug 3461 by
			// updating parent frame's menu.
			JavaSalFrame *pFrame = pSalData->mpFocusFrame;
			while ( pFrame && pFrame->mbVisible )
			{
				UpdateMenusForFrame( pFrame, NULL, true );
				pFrame = pFrame->mpParent;
			}
		}
		else
		{
			// Fix inability to access menu when there is no backing window by
			// only cancelling dislay of menus when there are visible windows
			for ( ::std::list< JavaSalFrame* >::const_iterator it = pSalData->maFrameList.begin(); it != pSalData->maFrameList.end(); ++it )
			{
				// Fix bug 3571 by only cancelling when at least one visible
				// non-backing, non-floating, and non-floating frame
				if ( (*it)->mbVisible && !(*it)->mbShowOnlyMenus && !(*it)->IsFloatingFrame() && (*it)->GetState() != SAL_FRAMESTATE_MINIMIZED )
				{
					bRet = FALSE;
					break;
				}
			}
		}

		// We need to let any timers run that were added by any menu
		// changes. Otherwise, some menus will be drawn in the state
		// that they were in before we updated the menus.
		CFRunLoopRunInMode( CFSTR( "AWTRunLoopMode" ), 0, false );
	}

	rSolarMutex.release();
	aEventQueueMutex.release();

	return bRet;
}

// ----------------------------------------------------------------------------

bool IsRunningLeopard( )
{
	InitializeMacOSXVersion();
	return isLeopard;
}

// ----------------------------------------------------------------------------

bool IsRunningSnowLeopard( )
{
	InitializeMacOSXVersion();
	return isSnowLeopard;
}

// ----------------------------------------------------------------------------

bool IsRunningLion( )
{
	InitializeMacOSXVersion();
	return isLion;
}

// ----------------------------------------------------------------------------

bool IsRunningMountainLion( )
{
	InitializeMacOSXVersion();
	return isMountainLion;
}

// ----------------------------------------------------------------------------

bool IsFullKeyboardAccessEnabled( )
{
	bool isFullAccessEnabled = false;

	CFPropertyListRef keyboardNavigationPref = CFPreferencesCopyAppValue( CFSTR( "AppleKeyboardUIMode" ), kCFPreferencesCurrentApplication );
	if ( keyboardNavigationPref )
	{
		int prefVal;
		if ( CFGetTypeID( keyboardNavigationPref ) == CFNumberGetTypeID() && CFNumberGetValue( (CFNumberRef)keyboardNavigationPref, kCFNumberIntType, &prefVal ) )
			isFullAccessEnabled = ( prefVal % 2 ? true : false );
		CFRelease( keyboardNavigationPref );
	}

	return isFullAccessEnabled;
}

// -----------------------------------------------------------------------

// Note: this must not be static as the symbol will be loaded by the vos
// module
extern "C" SAL_DLLPUBLIC_EXPORT sal_Bool Application_acquireSolarMutex()
{
	sal_Bool bRet = sal_False;

	if ( !Application::IsShutDown() && ImplGetSVData() && GetSalData() )
	{
		Application::GetSolarMutex().acquire();
		bRet = sal_True;
	}

	return bRet;
}

// -----------------------------------------------------------------------

// Note: this must not be static as the symbol will be loaded by the vos
// module
extern "C" SAL_DLLPUBLIC_EXPORT void Application_releaseSolarMutex()
{
	if ( !Application::IsShutDown() && ImplGetSVData() && GetSalData() )
		Application::GetSolarMutex().release();
}

// =======================================================================

void SalAbort( const XubString& rErrorText )
{
	if ( !rErrorText.Len() )
		fprintf( stderr, "Application Error" );
	else
		fprintf( stderr, ByteString( rErrorText, gsl_getSystemTextEncoding() ).GetBuffer() );
	fprintf( stderr, "\n" );
	abort();
}

// -----------------------------------------------------------------------

const ::rtl::OUString& SalGetDesktopEnvironment()
{
	static ::rtl::OUString aDesktopEnvironment( RTL_CONSTASCII_USTRINGPARAM( "Mac OS X" ) );
	return aDesktopEnvironment;
}

// -----------------------------------------------------------------------

void InitSalData()
{
	SalData *pSalData = new SalData();
	SetSalData( pSalData );
}

// -----------------------------------------------------------------------

void DeInitSalData()
{
	SalData *pSalData = GetSalData();
	SetSalData( NULL );
	delete pSalData;
}

// -----------------------------------------------------------------------

void InitSalMain()
{
}

// -----------------------------------------------------------------------

void DeInitSalMain()
{
}

// -----------------------------------------------------------------------

SalInstance* CreateSalInstance()
{
	SalData *pSalData = GetSalData();

	JavaSalInstance *pInst = new JavaSalInstance();
	pSalData->mpFirstInstance = pInst;

	// Set required Mac OS X NWF settings
	ImplGetSVData()->maNWFData.mbNoFocusRects = true;
	ImplGetSVData()->maNWFData.mbCheckBoxNeedsErase = true;

	return pInst;
}

// -----------------------------------------------------------------------

void DestroySalInstance( SalInstance* pInst )
{
	SalData *pSalData = GetSalData();

	if ( pSalData->mpFirstInstance == pInst )
		pSalData->mpFirstInstance = NULL;

	if ( pInst )
		delete pInst;
}

// =======================================================================

JavaSalInstance::JavaSalInstance()
{
	mpSalYieldMutex = new SalYieldMutex();
	mpSalYieldMutex->acquire();
}

// -----------------------------------------------------------------------

JavaSalInstance::~JavaSalInstance()
{
	if ( mpSalYieldMutex )
	{
		mpSalYieldMutex->release();
		delete mpSalYieldMutex;
	}
}

// -----------------------------------------------------------------------

IMutex* JavaSalInstance::GetYieldMutex()
{
	return mpSalYieldMutex;
}

// -----------------------------------------------------------------------

ULONG JavaSalInstance::ReleaseYieldMutex()
{
	// Never release the mutex in the main thread as it can cause crashing
	// when dragging when the OOo code's VCL event dispatching thread runs
	// while we are in the middle of a native drag event
	if ( ( !bAllowReleaseYieldMutex || bInNativeDrag ) && CFRunLoopGetCurrent() == CFRunLoopGetMain() )
		return 0;

	SalYieldMutex* pYieldMutex = mpSalYieldMutex;
	if ( pYieldMutex->GetThreadId() == OThread::getCurrentIdentifier() )
	{
		ULONG nCount = pYieldMutex->GetAcquireCount();
		ULONG n = nCount;
		while ( n )
		{
			pYieldMutex->release();
			n--;
		}
		return nCount;
	}
	else
	{
		return 0;
	}
}

// -----------------------------------------------------------------------

void JavaSalInstance::AcquireYieldMutex( ULONG nCount )
{
	SalYieldMutex* pYieldMutex = mpSalYieldMutex;
	while ( nCount )
	{
		pYieldMutex->acquire();
		nCount--;
	}
}

// -----------------------------------------------------------------------

void JavaSalInstance::Yield( bool bWait, bool bHandleAllCurrentEvents )
{
	ULONG nCount = 0;
	SalData *pSalData = GetSalData();
	bool bMainEventLoop = ( CFRunLoopGetCurrent() == CFRunLoopGetMain() );

	// Fix bug 2575 by manually dispatching native events.
	if ( bMainEventLoop )
	{
		// Fix bug 2731 by not doing this when we are in the begin menubar
		// tracking handler.
		if ( pSalData->maNativeEventCondition.check() )
			NSApplication_dispatchPendingEvents();
	}
	else
	{
		// Fix bug 3455 by always acquiring the event queue mutex during the
		// entire event dispatching process
		nCount = ReleaseYieldMutex();
		aEventQueueMutex.acquire();
		AcquireYieldMutex( nCount );
	}

	JavaSalEvent *pEvent = NULL;

	// Dispatch next pending non-AWT event
	if ( !Application::IsShutDown() && ( pEvent = JavaSalEventQueue::getNextCachedEvent( 0, FALSE ) ) != NULL )
	{
		pEvent->dispatch();
		pEvent->release();
		if ( !bMainEventLoop )
			aEventQueueMutex.release();
		return;
	}

	// Dispatch the next pending document event
	ImplSVData *pSVData = ImplGetSVData();
	if ( !Application::IsShutDown() && pSalData->maPendingDocumentEventsList.size() && pSVData && pSVData->maAppData.mnDispatchLevel == 1 && !pSVData->maWinData.mpLastExecuteDlg && !pSalData->mbInNativeModalSheet )
	{
		pEvent = pSalData->maPendingDocumentEventsList.front();
		pSalData->maPendingDocumentEventsList.pop_front();
		pEvent->dispatch();
		pEvent->release();
		if ( !bMainEventLoop )
			aEventQueueMutex.release();
		return;
	}

	if ( !bMainEventLoop && pSalData->maNativeEventCondition.check() )
	{
		ULONG nEventQueueMutexCount = ReleaseEventQueueMutex();
		nCount = ReleaseYieldMutex();
		OThread::yield();
		AcquireEventQueueMutex( nEventQueueMutexCount );
		AcquireYieldMutex( nCount );
	}

	// Check timer
	if ( pSVData && pSVData->mpSalTimer && pSalData->mnTimerInterval )
	{
		timeval aCurrentTime;
		gettimeofday( &aCurrentTime, NULL );
		if ( aCurrentTime >= pSalData->maTimeout )
		{
			gettimeofday( &pSalData->maTimeout, NULL );
			pSalData->maTimeout += pSalData->mnTimerInterval;
			pSVData->mpSalTimer->CallCallback();

			JavaSalFrame::FlushAllFrames();

			// Reduce noticeable pause when opening a new document by delaying
			// update of submenus until next available timer timeout.
			// Fix bug 3669 by not invoking menu updates while dragging.
			if ( pSalData->maNativeEventCondition.check() && pSalData->mpFocusFrame && pSalData->mpFocusFrame->mbVisible && !pSalData->mpLastDragFrame )
			{
				for ( int i = 0; pSalData->mpFocusFrame->maUpdateMenuList.size() && i < 8; i++ )
					UpdateMenusForFrame( pSalData->mpFocusFrame, pSalData->mpFocusFrame->maUpdateMenuList.front(), false );
			}
		}
	}

	// Determine timeout
	ULONG nTimeout = 0;
	if ( !bMainEventLoop && bWait && pSalData->maNativeEventCondition.check() && !Application::IsShutDown() )
	{
		if ( pSalData->mnTimerInterval )
		{
			timeval aTimeout;
			gettimeofday( &aTimeout, NULL );
			if ( pSalData->maTimeout > aTimeout )
			{
				aTimeout = pSalData->maTimeout - aTimeout;
				nTimeout = aTimeout.tv_sec * 1000 + aTimeout.tv_usec / 1000;
			}
		}

		// Wait a little bit to prevent excessive CPU usage. Fix bug 2588
		// by only doing so when the timeout is already set to a non-zero
		// value.
		if ( nTimeout < 10 )
			nTimeout = 10;
	}

	if ( nTimeout )
	{
		nCount = ReleaseYieldMutex();
		if ( !nCount )
			nTimeout = 0;
	}
	else
	{
		nCount = 0;
	}

	// Cache timeout for other threads
	nCurrentTimeout = nTimeout;

	// Dispatch any pending AWT events. Fix bug 2126 by always acting as if
	// the bHandleAllCurrentEvents parameter is true
	bool bContinue = true;
	while ( bContinue && !Application::IsShutDown() && ( pEvent = JavaSalEventQueue::getNextCachedEvent( nTimeout, sal_True ) ) != NULL )
	{
		nTimeout = 0;
		nCurrentTimeout = 0;

		AcquireYieldMutex( nCount );
		nCount = 0;

		USHORT nID = pEvent->getID();
		size_t nFrames = pSalData->maFrameList.size();
		pEvent->dispatch();
		switch ( nID )
		{
			case SALEVENT_CLOSE:
				// Fix bug 1971 by breaking after closing a window
				bContinue = false;
				break;
			case SALEVENT_KEYINPUT:
			case SALEVENT_MOUSEBUTTONDOWN:
				// Fix bugs 437 and 2264 by ensuring that if the next
				// event is a matching event, it will be dispatched before
				// the painting timer runs.
				OThread::yield();
				break;
			case SALEVENT_KEYUP:
				// Fix bug 3390 by letting any timers run when releasing
				// a Command-key event
				if ( pEvent->getModifiers() & KEY_MOD1 )
					bContinue = false;
				break;
			case SALEVENT_MOUSEMOVE:
                // Make highlighting by dragging more responsive
                if ( pEvent->getModifiers() & ( MOUSE_LEFT | MOUSE_MIDDLE | MOUSE_RIGHT ) )
					bContinue = false;
				break;
		}
		pEvent->release();

		JavaSalFrame::FlushAllFrames();

		// Fix bug 2941 without triggering bugs 2962 and 2963 by
		// breaking if any frames have been created or destroyed
		if ( bContinue && pSalData->maFrameList.size() != nFrames )
			bContinue = false;
	}

	nCurrentTimeout = 0;

	AcquireYieldMutex( nCount );

	if ( !bMainEventLoop )
		aEventQueueMutex.release();

	// Update all objects
	for ( ::std::list< JavaSalFrame* >::const_iterator it = pSalData->maFrameList.begin(); it != pSalData->maFrameList.end(); ++it )
		(*it)->FlushAllObjects();

	// Allow main event loop to proceed
	if ( !pEvent && !pSalData->maNativeEventCondition.check() )
		pSalData->maNativeEventCondition.set();
}

// -----------------------------------------------------------------------

bool JavaSalInstance::AnyInput( USHORT nType )
{
	bool bRet = false;

	if ( nType & INPUT_TIMER )
	{
		// Check timer
		SalData *pSalData = GetSalData();
		ImplSVData* pSVData = ImplGetSVData();
		if ( pSVData && pSVData->mpSalTimer && pSalData->mnTimerInterval )
		{
			timeval aCurrentTime;
			gettimeofday( &aCurrentTime, NULL );
			if ( aCurrentTime >= pSalData->maTimeout )
				bRet = true;
		}
	}

	if ( !bRet )
		bRet = JavaSalEventQueue::anyCachedEvent( nType );

	return bRet;
}

// -----------------------------------------------------------------------

SalFrame* JavaSalInstance::CreateChildFrame( SystemParentData* pSystemParentData, ULONG nSalFrameStyle )
{
#ifdef DEBUG
	fprintf( stderr, "JavaSalInstance::CreateChildFrame not implemented\n" );
#endif
	return NULL;
}

// -----------------------------------------------------------------------

SalFrame* JavaSalInstance::CreateFrame( SalFrame* pParent, ULONG nSalFrameStyle )
{
	JavaSalFrame *pFrame = new JavaSalFrame( nSalFrameStyle, (JavaSalFrame *)pParent );
	if ( !pFrame->mpWindow )
	{
		delete pFrame;
		return NULL;
	}

	// Get work area of the parent window or, if no parent, the main screen
	Rectangle aWorkArea( Point( 0, 0 ), Size( 0, 0 ) );
	pFrame->GetWorkArea( aWorkArea );

	// Set default window size based on style
	long nX = aWorkArea.Left();
	long nY = aWorkArea.Top();
	long nWidth = aWorkArea.GetWidth();
	long nHeight = aWorkArea.GetHeight();
	if ( nSalFrameStyle & SAL_FRAME_STYLE_FLOAT )
	{
		nWidth = 10;
		nHeight = 10;
	}
	else
	{
		if ( nSalFrameStyle & SAL_FRAME_STYLE_SIZEABLE && nSalFrameStyle & SAL_FRAME_STYLE_MOVEABLE )
		{
			nWidth = (long)( nWidth * 0.8 );
			nHeight = (long)( nHeight * 0.8 );

			// Don't let width be too much wider than height as it can look too
			// wide on many LCD screens
			long nMaxWidth = (long)( nHeight * 4 / 3 );
			if ( nWidth > nMaxWidth )
				nWidth = nMaxWidth;
		}
		if ( !pFrame->mpParent )
		{
			SalData *pSalData = GetSalData();

			// Find the next document window if any exist
			JavaSalFrame* pNextFrame = NULL;
			for ( ::std::list< JavaSalFrame* >::const_iterator it = pSalData->maFrameList.begin(); it != pSalData->maFrameList.end(); ++it )
			{
				if ( (*it) && (*it) != pFrame &&
					! (*it)->mpParent &&
					(*it)->mnStyle != SAL_FRAME_STYLE_DEFAULT &&
					(*it)->mnStyle & SAL_FRAME_STYLE_SIZEABLE &&
					(*it)->GetGeometry().nWidth &&
					(*it)->GetGeometry().nHeight )
						pNextFrame = *it;
			}

			if ( pNextFrame )
			{
				// Set screen to same screen as next frame
				pFrame->mbCenter = FALSE;
				const SalFrameGeometry& rGeom( pNextFrame->GetGeometry() );
				nX = rGeom.nX - rGeom.nLeftDecoration;
				nY = rGeom.nY - rGeom.nTopDecoration;
				nWidth = rGeom.nWidth;
				nHeight = rGeom.nHeight;
			}
		}
	}

	long nFlags = SAL_FRAME_POSSIZE_WIDTH | SAL_FRAME_POSSIZE_HEIGHT;
	if ( !pFrame->mbCenter )
		nFlags |= SAL_FRAME_POSSIZE_X | SAL_FRAME_POSSIZE_Y;
	pFrame->SetPosSize( nX, nY, nWidth, nHeight, nFlags );

	// Reset mbCenter flag to default value
	pFrame->mbCenter = TRUE;

	return pFrame;
}

// -----------------------------------------------------------------------

void JavaSalInstance::DestroyFrame( SalFrame* pFrame )
{
	if ( pFrame )
		delete pFrame;
}

// -----------------------------------------------------------------------

SalObject* JavaSalInstance::CreateObject( SalFrame* pParent, SystemWindowData* pWindowData, BOOL bShow )
{
	JavaSalObject *pObject = new JavaSalObject( pParent );
	pObject->Show( bShow );
	return pObject;
}

// -----------------------------------------------------------------------

void JavaSalInstance::DestroyObject( SalObject* pObject )
{
	if ( pObject )
		delete pObject;
}

// -----------------------------------------------------------------------

void* JavaSalInstance::GetConnectionIdentifier( ConnectionIdentifierType& rReturnedType, int& rReturnedBytes )
{
	rReturnedBytes = 1;
	rReturnedType = AsciiCString;
	return (void *)"";
}

// -----------------------------------------------------------------------

void JavaSalInstance::GetPrinterQueueInfo( ImplPrnQueueList* pList )
{
	// Create a dummy queue for our dummy default printer
	SalPrinterQueueInfo *pInfo = new SalPrinterQueueInfo();
	pInfo->maPrinterName = GetDefaultPrinter();
	pInfo->mpSysData = NULL;
	pList->Add( pInfo );
}

// -----------------------------------------------------------------------

void JavaSalInstance::GetPrinterQueueState( SalPrinterQueueInfo* pInfo )
{
}

// -----------------------------------------------------------------------

void JavaSalInstance::DeletePrinterQueueInfo( SalPrinterQueueInfo* pInfo )
{
	if ( pInfo )
		delete pInfo;
}

// -----------------------------------------------------------------------

SalInfoPrinter* JavaSalInstance::CreateInfoPrinter( SalPrinterQueueInfo* pQueueInfo,
                                                ImplJobSetup* pSetupData )
{
	// Populate data
	pSetupData->mnSystem = JOBSETUP_SYSTEM_JAVA;
	pSetupData->maPrinterName = pQueueInfo->maPrinterName;
	pSetupData->maDriver = pQueueInfo->maDriver;

	// Clear driver data
	if ( pSetupData->mpDriverData )
	{
		rtl_freeMemory( pSetupData->mpDriverData );
		pSetupData->mpDriverData = NULL;
		pSetupData->mnDriverDataLen = 0;
	}

	// Create a dummy printer configuration for our dummy printer
	return new JavaSalInfoPrinter( pSetupData );
}

// -----------------------------------------------------------------------

void JavaSalInstance::DestroyInfoPrinter( SalInfoPrinter* pPrinter )
{
	if ( pPrinter )
		delete pPrinter;
}

// -----------------------------------------------------------------------

XubString JavaSalInstance::GetDefaultPrinter()
{
	// Create a dummy default printer
	SalData *pSalData = GetSalData();
	if ( !pSalData->maDefaultPrinter.Len() )
	{
		SimpleResMgr *pResMgr = SimpleResMgr::Create( CREATEVERSIONRESMGR_NAME( salapp ) );
		if ( pResMgr )
		{
			pSalData->maDefaultPrinter = XubString( pResMgr->ReadString( DEFAULT_PRINTER ) );
			delete pResMgr;
		}
	}

	if ( !pSalData->maDefaultPrinter.Len() )
		pSalData->maDefaultPrinter = XubString( OUString::createFromAscii( "Printer" ) );

	return pSalData->maDefaultPrinter;
}

// -----------------------------------------------------------------------

SalPrinter* JavaSalInstance::CreatePrinter( SalInfoPrinter* pInfoPrinter )
{
	return new JavaSalPrinter( (JavaSalInfoPrinter *)pInfoPrinter );
}

// -----------------------------------------------------------------------

void JavaSalInstance::DestroyPrinter( SalPrinter* pPrinter )
{
	if ( pPrinter )
		delete pPrinter;
}

// -----------------------------------------------------------------------

SalVirtualDevice* JavaSalInstance::CreateVirtualDevice( SalGraphics* pGraphics,
                                                    long nDX, long nDY,
                                                    USHORT /* nBitCount - ignore as Mac OS X bit count is always 32 */,
                                                    const SystemGraphicsData *pData )
{
	JavaSalVirtualDevice *pDevice = NULL;

	if ( nDX > 0 && nDY > 0 )
	{
		pDevice = new JavaSalVirtualDevice();

		if ( !pDevice->SetSize( nDX, nDY ) )
		{
			delete pDevice;
			pDevice = NULL;
		}
	}

   	return pDevice;
}

// -----------------------------------------------------------------------

void JavaSalInstance::DestroyVirtualDevice( SalVirtualDevice* pDevice )
{
	if ( pDevice )
		delete pDevice;
}

// -----------------------------------------------------------------------

SalTimer* JavaSalInstance::CreateSalTimer()
{
    return new JavaSalTimer();
}

// -----------------------------------------------------------------------

SalI18NImeStatus* JavaSalInstance::CreateI18NImeStatus()
{
    return new JavaSalI18NImeStatus();
}

// -----------------------------------------------------------------------

SalBitmap* JavaSalInstance::CreateSalBitmap()
{
    return new JavaSalBitmap();
}

// -----------------------------------------------------------------------

SalSession* JavaSalInstance::CreateSalSession()
{
	return NULL;
}

// -----------------------------------------------------------------------

SalSystem* JavaSalInstance::CreateSalSystem()
{
	return new JavaSalSystem();
}

// =========================================================================

SalYieldMutex::SalYieldMutex()
{
	mnCount	 = 0;
	mnThreadId  = 0;

	maMainThreadCondition.set();
}

// -------------------------------------------------------------------------

void SalYieldMutex::acquire()
{
	if ( mnThreadId != OThread::getCurrentIdentifier() )
	{
		// If we are in a signal handler and we don't have the mutex, don't
		// block waiting for the mutex as most likely the thread with the mutex
		// is blocked in an [NSObject performSelectorOnMainThread] message and
		// we will deadlock. Also, fix bug 1496 by not allowing native timers
		// to run before trying to acquire the mutex when in the main thread.
		SalData *pSalData = GetSalData();
		if ( !pSalData || pSalData->mbInSignalHandler && !tryToAcquire() )
		{
			return;
		}
		else if ( CFRunLoopGetCurrent() == CFRunLoopGetMain() )
		{
			// Wait for other thread to release mutex
			// We need to let any pending timers run so that we don't deadlock
			while ( !Application::IsShutDown() )
			{
				// Fix hanging in bug 3503 by posting a dummy event to wakeup
				// the VCL event thread if the VCL event dispatch thread is in
				// a potentially long wait
				if ( nCurrentTimeout > 100 )
				{
					TimeValue aDelay;
					aDelay.Seconds = 0;
					aDelay.Nanosec = 50;
					maMainThreadCondition.reset();
					JavaSalEvent *pUserEvent = new JavaSalEvent( SALEVENT_USEREVENT, NULL, NULL );
					JavaSalEventQueue::postCachedEvent( pUserEvent );
					pUserEvent->release();
					if ( !maMainThreadCondition.check() )
						maMainThreadCondition.wait( &aDelay );
					maMainThreadCondition.set();
				}

				CFRunLoopRunInMode( CFSTR( "AWTRunLoopMode" ), 0, false );
				if ( tryToAcquire() )
					break;
			}

			return;
		}
	}

	OMutex::acquire();
	mnThreadId = OThread::getCurrentIdentifier();
	mnCount++;
}

// -------------------------------------------------------------------------

void SalYieldMutex::release()
{
	if ( mnThreadId == OThread::getCurrentIdentifier() )
	{
		if ( mnCount == 1 )
			mnThreadId = 0;

		if ( mnCount )
			mnCount--;

		// Notify main thread that it can grab the mutex
		if ( !mnCount && !maMainThreadCondition.check() )
		{
			maMainThreadCondition.set();
			OThread::yield();
		}
	}

	OMutex::release();
}

// -------------------------------------------------------------------------

sal_Bool SalYieldMutex::tryToAcquire()
{
	if ( OMutex::tryToAcquire() )
	{
		mnThreadId = OThread::getCurrentIdentifier();
		mnCount++;
		return sal_True;
	}
	else
		return sal_False;
}

// =========================================================================

JavaSalEvent::JavaSalEvent( USHORT nID, JavaSalFrame *pFrame, void *pData, const ::rtl::OString& rPath, ULONG nCommittedCharacters, ULONG nCursorPosition ) :
	mnID( nID  ),
	mpFrame( pFrame ),
	mbNative( false ),
	mbShutdownCancelled( sal_False ),
	mnCommittedCharacters( nCommittedCharacters ),
	mnCursorPosition( nCursorPosition ),
	mpData( pData ),
	mnRefCount( 1 )
{
	switch ( mnID )
	{
		case SALEVENT_CLOSE:
		case SALEVENT_ENDEXTTEXTINPUT:
		case SALEVENT_EXTTEXTINPUT:
		case SALEVENT_EXTTEXTINPUTPOS:
		case SALEVENT_GETFOCUS:
		case SALEVENT_LOSEFOCUS:
		case SALEVENT_KEYINPUT:
		case SALEVENT_KEYMODCHANGE:
		case SALEVENT_KEYUP:
		case SALEVENT_MOUSEBUTTONDOWN:
		case SALEVENT_MOUSEBUTTONUP:
		case SALEVENT_MOUSELEAVE:
		case SALEVENT_MOUSEMOVE:
		case SALEVENT_MOVERESIZE:
		case SALEVENT_PAINT:
		case SALEVENT_WHEELMOUSE:
			mbNative = true;
	}

	maPath = OUString( rPath.getStr(), rPath.getLength(), RTL_TEXTENCODING_UTF8 );
}

// -------------------------------------------------------------------------

JavaSalEvent::~JavaSalEvent()
{
	if ( mpData )
	{
		switch ( mnID )
		{
			case SALEVENT_ENDEXTTEXTINPUT:
			case SALEVENT_EXTTEXTINPUT:
				SalExtTextInputEvent *pInputEvent = (SalExtTextInputEvent *)mpData;
				if ( pInputEvent->mpTextAttr )
					rtl_freeMemory( (USHORT *)pInputEvent->mpTextAttr );
				delete pInputEvent;
				break;
			case SALEVENT_EXTTEXTINPUTPOS:
				SalExtTextInputPosEvent *pInputPosEvent = (SalExtTextInputPosEvent *)mpData;
				delete pInputPosEvent;
				break;
			case SALEVENT_KEYINPUT:
			case SALEVENT_KEYUP:
				SalKeyEvent *pKeyEvent = (SalKeyEvent *)mpData;
				delete pKeyEvent;
				break;
			case SALEVENT_KEYMODCHANGE:
				SalKeyModEvent *pKeyModEvent = (SalKeyModEvent *)mpData;
				delete pKeyModEvent;
				break;
			case SALEVENT_MOUSEBUTTONDOWN:
			case SALEVENT_MOUSEBUTTONUP:
			case SALEVENT_MOUSELEAVE:
			case SALEVENT_MOUSEMOVE:
				SalMouseEvent *pMouseEvent = (SalMouseEvent *)mpData;
				delete pMouseEvent;
				break;
			case SALEVENT_MOVE:
			case SALEVENT_MOVERESIZE:
			case SALEVENT_RESIZE:
				Rectangle *pPosSize = (Rectangle *)mpData;
				delete pPosSize;
				break;
			case SALEVENT_PAINT:
				SalPaintEvent *pPaintEvent = (SalPaintEvent *)mpData;
				delete pPaintEvent;
				break;
			case SALEVENT_WHEELMOUSE:
				SalWheelMouseEvent *pWheelMouseEvent = (SalWheelMouseEvent *)mpData;
				delete pWheelMouseEvent;
				break;
			default:
				break;
		}
	}

	while ( maOriginalKeyEvents.size() )
	{
		JavaSalEvent *pEvent = maOriginalKeyEvents.front();
		maOriginalKeyEvents.pop_front();
		pEvent->release();
	}
}

// -------------------------------------------------------------------------

void JavaSalEvent::addRepeatCount( USHORT nCount )
{
	if ( mpData && mnID == SALEVENT_KEYINPUT )
	{
		SalKeyEvent *pKeyEvent = (SalKeyEvent *)mpData;
		pKeyEvent->mnRepeat += nCount;
	}
}

// -------------------------------------------------------------------------

void JavaSalEvent::addOriginalKeyEvent( JavaSalEvent *pEvent )
{
	if ( pEvent && ( mnID == SALEVENT_KEYINPUT || mnID == SALEVENT_KEYUP ) )
	{
		pEvent->reference();
		maOriginalKeyEvents.push_back( pEvent );
	}
}

// -------------------------------------------------------------------------

void JavaSalEvent::addUpdateRect( const Rectangle& rRect )
{
	if ( mpData && mnID == SALEVENT_PAINT )
	{
		Rectangle aUpdateRect( getUpdateRect() );
		aUpdateRect.Justify();
		
		Rectangle aRect( rRect );
		aRect.Justify();

		if ( aRect.GetWidth() > 0 && aRect.GetHeight() > 0 )
		{
			if ( aUpdateRect.GetWidth() > 0 && aUpdateRect.GetHeight() > 0 )
				aRect = aRect.GetUnion( aUpdateRect );

			SalPaintEvent *pPaintEvent = (SalPaintEvent *)mpData;
			pPaintEvent->mnBoundX = aRect.Left();
			pPaintEvent->mnBoundY = aRect.Top();
			pPaintEvent->mnBoundWidth = aRect.GetWidth();
			pPaintEvent->mnBoundHeight = aRect.GetHeight();
		}
	}
}

// -------------------------------------------------------------------------

void JavaSalEvent::addWheelRotation( long nRotation )
{
	if ( mpData && mnID == SALEVENT_WHEELMOUSE )
	{
		SalWheelMouseEvent *pWheelMouseEvent = (SalWheelMouseEvent *)mpData;
		pWheelMouseEvent->mnNotchDelta += nRotation;
		pWheelMouseEvent->mnDelta = pWheelMouseEvent->mnNotchDelta * WHEEL_ROTATION_FACTOR;
	}
}

// -------------------------------------------------------------------------

void JavaSalEvent::cancelShutdown()
{
	if ( mnID == SALEVENT_SHUTDOWN )
		mbShutdownCancelled = sal_True;
}

// -------------------------------------------------------------------------

void JavaSalEvent::dispatch()
{
	USHORT nID = mnID;
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
				if ( pSVData && !pSVData->maWinData.mpFirstFloat && !pSVData->maWinData.mpLastExecuteDlg && !pSalData->mbInNativeModalSheet && pSalData->maFrameList.size() )
				{
					JavaSalFrame *pFrame = pSalData->maFrameList.front();
					if ( pFrame && !pFrame->CallCallback( nID, NULL ) )
						bCancelShutdown = false;
				}
				else if ( pSVData && pSVData->maWinData.mpFirstFloat )
				{
					// Close all popups but do not shutdown since that will
					// cause a crash when a second level popup menu is open
					static const char* pEnv = getenv( "SAL_FLOATWIN_NOAPPFOCUSCLOSE" );
					if ( !(pSVData->maWinData.mpFirstFloat->GetPopupModeFlags() & FLOATWIN_POPUPMODE_NOAPPFOCUSCLOSE) && !(pEnv && *pEnv) )
						pSVData->maWinData.mpFirstFloat->EndPopupMode( FLOATWIN_POPUPMODEEND_CANCEL | FLOATWIN_POPUPMODEEND_CLOSEALL );
				}

			}

			if ( bCancelShutdown )
			{
				cancelShutdown();

				// Invoke the native shutdown cancelled handler
				JavaSalEventQueue::setShutdownDisabled( sal_False );
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
				ApplicationEvent aAppEvt( aEmptyStr, ApplicationAddress(), nID == SALEVENT_OPENDOCUMENT ? APPEVENT_OPEN_STRING : APPEVENT_PRINT_STRING, getPath() );
				pSVData->mpApp->AppEvent( aAppEvt );
			}
			else
			{
				reference();
				pSalData->maPendingDocumentEventsList.push_back( this );
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

	if ( pSalData->mbInNativeModalSheet && pSalData->mpNativeModalSheetFrame && pFrame != pSalData->mpNativeModalSheetFrame )
	{
		// We need to prevent dispatching of events other than system events
		// like bounds change or paint events. Fix bug 3429 by only forcing
		// a focus change if there is not a native modal window showing.
		if ( NSApplication_isActive() )
			pSalData->mpNativeModalSheetFrame->ToTop( SAL_FRAME_TOTOP_RESTOREWHENMIN | SAL_FRAME_TOTOP_GRABFOCUS );

		switch ( nID )
		{
			case SALEVENT_CLOSE:
			case SALEVENT_ENDEXTTEXTINPUT:
			case SALEVENT_EXTTEXTINPUT:
			case SALEVENT_GETFOCUS:
			case SALEVENT_LOSEFOCUS:
			case SALEVENT_KEYINPUT:
			case SALEVENT_KEYMODCHANGE:
			case SALEVENT_KEYUP:
			case SALEVENT_MOUSEBUTTONDOWN:
			case SALEVENT_MOUSEBUTTONUP:
			case SALEVENT_MOUSELEAVE:
			case SALEVENT_MOUSEMOVE:
			case SALEVENT_WHEELMOUSE:
			case SALEVENT_MENUACTIVATE:
			case SALEVENT_MENUCOMMAND:
			case SALEVENT_MENUDEACTIVATE:
				return;
			default:
				break;
		}
	}

	switch ( nID )
	{
		case SALEVENT_CLOSE:
		{
			if ( pFrame && pFrame->mbVisible )
				pFrame->CallCallback( nID, NULL );
			break;
		}
		case SALEVENT_DEMINIMIZED:
		case SALEVENT_MINIMIZED:
		{
			// Fix bug 3649 by not hiding any windows when minimizing
			break;
		}
		case SALEVENT_ENDEXTTEXTINPUT:
		{
			if ( pFrame && pFrame->mbVisible )
			{
				SalExtTextInputEvent *pInputEvent = (SalExtTextInputEvent *)mpData;

				// Fix bug 1158 by resetting the focus to whichever window is
				// receiving key events
				if ( pFrame != pSalData->mpFocusFrame )
				{
					JavaSalEvent aEvent( SALEVENT_GETFOCUS, pFrame, NULL );
					aEvent.dispatch();
				}
				pFrame->CallCallback( nID, pInputEvent );
			}
			break;
		}
		case SALEVENT_EXTTEXTINPUT:
		{
			if ( pFrame && pFrame->mbVisible )
			{
				SalExtTextInputEvent *pInputEvent = (SalExtTextInputEvent *)mpData;
				ULONG nCommitted = getCommittedCharacterCount();
				if ( !pInputEvent )
				{
					ULONG nCursorPos = getCursorPosition();
					pInputEvent = new SalExtTextInputEvent();
					pInputEvent->mnTime = getWhen();
					pInputEvent->maText = getText();
					pInputEvent->mpTextAttr = getTextAttributes();
					pInputEvent->mnCursorPos = nCursorPos > nCommitted ? nCursorPos : nCommitted;
					pInputEvent->mnDeltaStart = 0;
					pInputEvent->mbOnlyCursor = FALSE;
					pInputEvent->mnCursorFlags = 0;

					mpData = pInputEvent;
				}
				// Fix bug 1158 by resetting the focus to whichever window is
				// receiving key events
				if ( pFrame != pSalData->mpFocusFrame )
				{
					JavaSalEvent aEvent( SALEVENT_GETFOCUS, pFrame, NULL );
					aEvent.dispatch();
				}
				pFrame->CallCallback( nID, pInputEvent );
				// If there is no text, the character is committed
				if ( pInputEvent->maText.Len() == nCommitted )
					pFrame->CallCallback( SALEVENT_ENDEXTTEXTINPUT, NULL );
			}
			break;
		}
		case SALEVENT_EXTTEXTINPUTPOS:
		{
			SalExtTextInputPosEvent *pInputPosEvent = (SalExtTextInputPosEvent *)mpData;
			if ( pInputPosEvent && pFrame && pFrame->mbVisible )
				pFrame->CallCallback( SALEVENT_EXTTEXTINPUTPOS, (void *)pInputPosEvent );
			break;
		}
		case SALEVENT_GETFOCUS:
		{
			// Ignore focus events for floating windows
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
				JavaSalMenu::SetMenuBarToFocusFrame();

				Window *pWindow = Application::GetFirstTopLevelWindow();
				while ( pWindow && pWindow->ImplGetFrame() != pFrame )
					pWindow = Application::GetNextTopLevelWindow( pWindow );
				InvalidateControls( pWindow );
			}
			else
			{
				JavaSalMenu::SetMenuBarToFocusFrame();
			}
			break;
		}
		case SALEVENT_LOSEFOCUS:
		{
			if ( pFrame )
			{
				if ( pFrame == pSalData->mpFocusFrame )
				{
					pSalData->mpFocusFrame = NULL;
					pFrame->CallCallback( nID, NULL );
					JavaSalMenu::SetMenuBarToFocusFrame();
				}

				// Fix bug 3098 by hiding tooltip windows but leaving the
				// visible flag set to true
				for ( ::std::list< JavaSalFrame* >::const_iterator cit = pFrame->maChildren.begin(); cit != pFrame->maChildren.end(); ++cit )
				{
						if ( (*cit)->mbVisible && (*cit)->mnStyle & SAL_FRAME_STYLE_TOOLTIP )
					(*cit)->SetVisible( sal_False, sal_False );
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
			if ( pFrame && pFrame->mbVisible )
			{
				SalKeyEvent *pKeyEvent = (SalKeyEvent *)mpData;
				if ( !pKeyEvent )
				{
					pKeyEvent = new SalKeyEvent();
					pKeyEvent->mnTime = getWhen();
					pKeyEvent->mnCode = getKeyCode() | getModifiers();
					pKeyEvent->mnCharCode = getKeyChar();
					pKeyEvent->mnRepeat = getRepeatCount();

					mpData = pKeyEvent;
				}
				// Fix bug 1158 by resetting the focus to whichever window is
				// receiving key events
				if ( pFrame != pSalData->mpFocusFrame )
				{
					JavaSalEvent aEvent( SALEVENT_GETFOCUS, pFrame, NULL );
					aEvent.dispatch();
				}
				// Pass all potential menu shortcuts received by a utility
				// window to its parent window. Fix bug 3432 by not sending
				// Command-Shift-F10 to parent.
				if ( pKeyEvent->mnCode & KEY_MOD1 && pKeyEvent->mnCode != ( KEY_MOD1 | KEY_SHIFT | KEY_F10 ) )
				{
					while ( pFrame->mpParent && pFrame->mpParent->mbVisible && pFrame->IsUtilityWindow() )
						pFrame = pFrame->mpParent;
				}
				// Fix bug reported in the following NeoOffice forum post that
				// by only skipping a key binding event when there are original
				// events attached to the key event event:
				// http://trinity.neooffice.org/modules.php?name=Forums&file=viewtopic&p=62803#62803
				if ( ( !pFrame->mbAllowKeyBindings && maOriginalKeyEvents.size() ) || !pFrame->CallCallback( nID, pKeyEvent ) )
				{
					// If the key event fails and this is a command event,
					// dispatch the original events
					JavaSalEvent *pEvent;
					while ( ( pEvent = getNextOriginalKeyEvent() ) != NULL )
					{
						pEvent->dispatch();
						pEvent->release();
					}
				}
			}
			break;
		}
		case SALEVENT_KEYMODCHANGE:
		{
			if ( pFrame && pFrame->mbVisible )
			{
				SalKeyModEvent *pKeyModEvent = (SalKeyModEvent *)mpData;
				if ( !pKeyModEvent )
				{
					pKeyModEvent = new SalKeyModEvent();
					pKeyModEvent->mnTime = getWhen();
					pKeyModEvent->mnCode = getModifiers();
					pKeyModEvent->mnModKeyCode = 0;

					mpData = pKeyModEvent;
				}
				pFrame->CallCallback( nID, pKeyModEvent );
			}
			break;
		}
		case SALEVENT_MOUSEBUTTONDOWN:
		case SALEVENT_MOUSEBUTTONUP:
		case SALEVENT_MOUSELEAVE:
		case SALEVENT_MOUSEMOVE:
		{
			if ( pFrame && pFrame->mbVisible )
			{
				SalMouseEvent *pMouseEvent = (SalMouseEvent *)mpData;
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

					mpData = pMouseEvent;
				}

				USHORT nButtons = pMouseEvent->mnCode & ( MOUSE_LEFT | MOUSE_MIDDLE | MOUSE_RIGHT );
				if ( nID == SALEVENT_MOUSEMOVE )
				{
					if ( nButtons && !pSalData->mpLastDragFrame )
						pSalData->mpLastDragFrame = pFrame;
					if ( !nButtons && !pSalData->mpLastMouseMoveFrame )
						pSalData->mpLastMouseMoveFrame = pFrame;
				}

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

				// Create a synthetic mouse leave event when a mouse move event
				// resolves to a different window than the last mouse move event
				if ( pSalData->mpLastMouseMoveFrame && nID == SALEVENT_MOUSEMOVE && !nButtons )
				{
					if ( pSalData->mpLastMouseMoveFrame != pFrame )
					{
						nID = SALEVENT_MOUSELEAVE;
						pMouseEvent->mnX = aScreenPoint.X() - pSalData->mpLastMouseMoveFrame->maGeometry.nX;
						pMouseEvent->mnY = aScreenPoint.Y() - pSalData->mpLastMouseMoveFrame->maGeometry.nY;

						JavaSalFrame *pLastMouseMoveFrame = pSalData->mpLastMouseMoveFrame;
						pSalData->mpLastMouseMoveFrame = pFrame;
						pFrame = pLastMouseMoveFrame;
					}
					else
					{
						pSalData->mpLastMouseMoveFrame = pFrame;
					}
				}
				else
				{
					pSalData->mpLastMouseMoveFrame = NULL;
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
						pPopupWindow->EndPopupMode( FLOATWIN_POPUPMODEEND_CLOSEALL );
				}
			}
			break;
		}
		case SALEVENT_MOVE:
		case SALEVENT_MOVERESIZE:
		case SALEVENT_RESIZE:
		{
			if ( pFrame )
			{
				Rectangle *pPosSize = (Rectangle *)mpData;

				// Update size
				sal_Bool bInLiveResize = sal_False;
				if ( !pPosSize )
				{
					pPosSize = new Rectangle( pFrame->GetBounds( &bInLiveResize ) );
					mpData = pPosSize;
				}

				// If in live resize, ignore event and just repaint
				bool bSkipEvent = false;
				if ( bInLiveResize )
				{
					timeval aCurrentTime;
					gettimeofday( &aCurrentTime, NULL );
					if ( pSalData->mpLastResizeFrame == pFrame && pSalData->maLastResizeTime >= aCurrentTime )
						bSkipEvent = true;
				}

				// If too little time has passed since the last "in live resize"
				// event, skip it and repost this event
				if ( bSkipEvent )
				{
					JavaSalEvent *pEvent = new JavaSalEvent( nID, pFrame, NULL );
					JavaSalEventQueue::postCachedEvent( pEvent );
					pEvent->release();
				}
				else
				{
					// Update resize timer
					pSalData->mpLastResizeFrame = ( bInLiveResize ? pFrame : NULL );
					if ( pSalData->mpLastResizeFrame )
					{
						gettimeofday( &pSalData->maLastResizeTime, NULL );
						pSalData->maLastResizeTime += 100;
					}

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
					int nX = pPosSize->Left() + pFrame->maGeometry.nLeftDecoration;
					if ( pFrame->maGeometry.nX != nX )
					{
						bPosChanged = true;
						if ( pFrame == pSalData->mpLastDragFrame )
							pSalData->maLastPointerState.maPos.X() += nX - pFrame->maGeometry.nX;
						pFrame->maGeometry.nX = nX;
					}
					int nY = pPosSize->Top() + pFrame->maGeometry.nTopDecoration;
					if ( pFrame->maGeometry.nY != nY )
					{
						bPosChanged = true;
						if ( pFrame == pSalData->mpLastDragFrame )
							pSalData->maLastPointerState.maPos.Y() += nY - pFrame->maGeometry.nY;
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

					// Fix bug 3045 by setting the event ID to the actual
					// changes that have occurred. This also fixes the
					// autodocking of native utility windows problem described
					// in bug 3035
					if ( bForceResize || bPosChanged || bSizeChanged )
					{
						if ( bPosChanged && bSizeChanged )
							nID = SALEVENT_MOVERESIZE;
						else if ( bPosChanged )
							nID = SALEVENT_MOVE;
						else
							nID = SALEVENT_RESIZE;

						if ( bForceResize || bSizeChanged )
							pFrame->UpdateLayer();

						// Fix bug 2769 by creating synthetic mouse dragged
						// events when dragging a window's title bar
						if ( bPosChanged && pFrame == pSalData->mpLastDragFrame )
						{
							NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

							SalMouseEvent *pMouseEvent = new SalMouseEvent();
							pMouseEvent->mnTime = (ULONG)( JavaSalEventQueue::getLastNativeEventTime() * 1000 );
							pMouseEvent->mnX = pSalData->maLastPointerState.maPos.X() - pFrame->maGeometry.nX;
							pMouseEvent->mnY = pSalData->maLastPointerState.maPos.Y() - pFrame->maGeometry.nY;
							pMouseEvent->mnCode = pSalData->maLastPointerState.mnState;
							pMouseEvent->mnButton = 0;

							JavaSalEvent *pMouseDraggedEvent = new JavaSalEvent( SALEVENT_MOUSEMOVE, pFrame, pMouseEvent );
							JavaSalEventQueue::postCachedEvent( pMouseDraggedEvent );
							pMouseDraggedEvent->release();

							[pPool release];
						}

						pFrame->CallCallback( nID, NULL );
					}
				}
			}
			break;
		}
		case SALEVENT_PAINT:
		{
			if ( pFrame && pFrame->mbVisible )
			{
				SalPaintEvent *pPaintEvent = (SalPaintEvent *)mpData;
				if ( !pPaintEvent )
				{
					// Get paint region
					Rectangle aUpdateRect( getUpdateRect() );
					pPaintEvent = new SalPaintEvent( aUpdateRect.Left(), aUpdateRect.Top(), aUpdateRect.GetWidth(), aUpdateRect.GetHeight() );

					mpData = pPaintEvent;
				}
				// Adjust position for RTL layout
				if ( Application::GetSettings().GetLayoutRTL() )
					pPaintEvent->mnBoundX = pFrame->maGeometry.nWidth - pFrame->maGeometry.nLeftDecoration - pFrame->maGeometry.nRightDecoration - pPaintEvent->mnBoundWidth - pPaintEvent->mnBoundX;

				pFrame->CallCallback( nID, pPaintEvent );
			}
			break;
		}
		case SALEVENT_USEREVENT:
		{
			if ( pFrame )
				pFrame->CallCallback( nID, mpData );
			break;
		}
		case SALEVENT_WHEELMOUSE:
		{
			if ( pFrame && pFrame->mbVisible )
			{
				SalWheelMouseEvent *pWheelMouseEvent = (SalWheelMouseEvent *)mpData;
				if ( !pWheelMouseEvent )
				{
					long nWheelRotation = getWheelRotation();
					BOOL bHorz = isHorizontal();
					pWheelMouseEvent = new SalWheelMouseEvent();
					pWheelMouseEvent->mnTime = getWhen();
					pWheelMouseEvent->mnX = getX();
					pWheelMouseEvent->mnY = getY();
					pWheelMouseEvent->mnDelta = nWheelRotation * WHEEL_ROTATION_FACTOR;
					pWheelMouseEvent->mnNotchDelta = nWheelRotation;
					pWheelMouseEvent->mnScrollLines = getScrollAmount();
					pWheelMouseEvent->mnCode = getModifiers();
					pWheelMouseEvent->mbHorz = bHorz;

					mpData = pWheelMouseEvent;
				}
				// Adjust position for RTL layout
				if ( Application::GetSettings().GetLayoutRTL() )
					pWheelMouseEvent->mnX = pFrame->maGeometry.nWidth - pFrame->maGeometry.nLeftDecoration - pFrame->maGeometry.nRightDecoration - pWheelMouseEvent->mnX - 1;
				pFrame->CallCallback( nID, pWheelMouseEvent );
			}
			break;
		}
		case SALEVENT_MENUACTIVATE:
		case SALEVENT_MENUCOMMAND:
		case SALEVENT_MENUDEACTIVATE:
		{
			if ( pFrame && pFrame->mbVisible )
			{
				SalMenuEvent *pMenuEvent = (SalMenuEvent *)mpData;
				if ( !pMenuEvent )
				{
					pMenuEvent = new SalMenuEvent( getMenuID(), getMenuCookie() );

					mpData = pMenuEvent;
				}
				// Pass all menu selections received by a utility window to
				// its parent window
				if ( nID == SALEVENT_MENUCOMMAND )
				{
					bool bIsInFocusFrameHierarchy = ( pFrame == pSalData->mpFocusFrame );
					while ( pFrame->mpParent && pFrame->mpParent->mbVisible && pFrame->IsUtilityWindow() )
					{
						pFrame = pFrame->mpParent;
						if ( !bIsInFocusFrameHierarchy )
							bIsInFocusFrameHierarchy = ( pFrame == pSalData->mpFocusFrame );
					}

					// Ignore menu command events that are not for the focus
					// frame as it indicates that the menu updating has fallen
					// out of sync with our focus tracking
					if ( !bIsInFocusFrameHierarchy )
					{
						pFrame = NULL;
					}
					else
					{
						// Close all popups
						ImplSVData *pSVData = ImplGetSVData();
						if ( pSVData && pSVData->maWinData.mpFirstFloat )
						{
							static const char* pEnv = getenv( "SAL_FLOATWIN_NOAPPFOCUSCLOSE" );
							if ( !(pSVData->maWinData.mpFirstFloat->GetPopupModeFlags() & FLOATWIN_POPUPMODE_NOAPPFOCUSCLOSE) && !(pEnv && *pEnv) )
								pSVData->maWinData.mpFirstFloat->EndPopupMode( FLOATWIN_POPUPMODEEND_CANCEL | FLOATWIN_POPUPMODEEND_CLOSEALL );
						}
					}
				}

				if ( pFrame )
					pFrame->CallCallback( nID, pMenuEvent );
			}
			break;
		}
		default:
		{
			if ( pFrame )
				pFrame->CallCallback( nID, mpData );
			break;
		}
	}
}

// -------------------------------------------------------------------------

ULONG JavaSalEvent::getCommittedCharacterCount()
{
	ULONG nRet = 0;

	if ( mnID == SALEVENT_EXTTEXTINPUT )
		nRet = mnCommittedCharacters;

	return nRet;
}

// -------------------------------------------------------------------------

ULONG JavaSalEvent::getCursorPosition()
{
	ULONG nRet = 0;

	if ( mnID == SALEVENT_EXTTEXTINPUT )
		nRet = mnCursorPosition;

	return nRet;
}

// -------------------------------------------------------------------------

JavaSalFrame *JavaSalEvent::getFrame()
{
	return mpFrame;
}

// -------------------------------------------------------------------------

USHORT JavaSalEvent::getKeyChar()
{
	USHORT nRet = 0;

	if ( mpData && ( mnID == SALEVENT_KEYINPUT || mnID == SALEVENT_KEYUP ) )
	{
		SalKeyEvent *pKeyEvent = (SalKeyEvent *)mpData;
		nRet = pKeyEvent->mnCharCode;
	}

	return nRet;
}

// -------------------------------------------------------------------------

USHORT JavaSalEvent::getKeyCode()
{
	USHORT nRet = 0;

	if ( mpData && ( mnID == SALEVENT_KEYINPUT || mnID == SALEVENT_KEYUP ) )
	{
		SalKeyEvent *pKeyEvent = (SalKeyEvent *)mpData;
		nRet = pKeyEvent->mnCode & ~( KEY_MOD1 | KEY_MOD2 | KEY_MOD3 | KEY_SHIFT | MOUSE_LEFT | MOUSE_MIDDLE | MOUSE_RIGHT );
	}

	return nRet;
}

// -------------------------------------------------------------------------

USHORT JavaSalEvent::getID()
{
	return mnID;
}

// -------------------------------------------------------------------------

USHORT JavaSalEvent::getModifiers()
{
	USHORT nRet = 0;

	if ( mpData )
	{
		switch ( mnID )
		{
			case SALEVENT_KEYINPUT:
			case SALEVENT_KEYUP:
				SalKeyEvent *pKeyEvent = (SalKeyEvent *)mpData;
				nRet = pKeyEvent->mnCode;
				break;
			case SALEVENT_KEYMODCHANGE:
				SalKeyModEvent *pKeyModEvent = (SalKeyModEvent *)mpData;
				nRet = pKeyModEvent->mnCode;
				break;
			case SALEVENT_MOUSEBUTTONDOWN:
			case SALEVENT_MOUSEBUTTONUP:
			case SALEVENT_MOUSELEAVE:
			case SALEVENT_MOUSEMOVE:
				SalMouseEvent *pMouseEvent = (SalMouseEvent *)mpData;
				nRet = pMouseEvent->mnCode;
				break;
			case SALEVENT_WHEELMOUSE:
				SalWheelMouseEvent *pWheelMouseEvent = (SalWheelMouseEvent *)mpData;
				nRet = pWheelMouseEvent->mnCode;
				break;
			default:
				break;
		}
	}

	return nRet;
}

// -------------------------------------------------------------------------

JavaSalEvent *JavaSalEvent::getNextOriginalKeyEvent()
{
	JavaSalEvent *pRet = NULL;

	if ( maOriginalKeyEvents.size() )
	{
		pRet = maOriginalKeyEvents.front();
		maOriginalKeyEvents.pop_front();
	}

	return pRet;
}

// -------------------------------------------------------------------------

OUString JavaSalEvent::getPath()
{
	return maPath;
}

// -------------------------------------------------------------------------

USHORT JavaSalEvent::getRepeatCount()
{
	USHORT nRet = 0;

	if ( mpData && mnID == SALEVENT_KEYINPUT )
	{
		SalKeyEvent *pKeyEvent = (SalKeyEvent *)mpData;
		nRet = pKeyEvent->mnRepeat;
	}

	return nRet;
}

// -------------------------------------------------------------------------

XubString JavaSalEvent::getText()
{
	XubString aRet;

	if ( mpData && mnID == SALEVENT_EXTTEXTINPUT )
	{
		SalExtTextInputEvent *pInputEvent = (SalExtTextInputEvent *)mpData;
		aRet = pInputEvent->maText;
	}

	return aRet;
}

// -------------------------------------------------------------------------

const USHORT *JavaSalEvent::getTextAttributes()
{
	const USHORT *pRet = 0;

	if ( mpData && mnID == SALEVENT_EXTTEXTINPUT )
	{
		SalExtTextInputEvent *pInputEvent = (SalExtTextInputEvent *)mpData;
		pRet = pInputEvent->mpTextAttr;
	}

	return pRet;
}

// -------------------------------------------------------------------------

const Rectangle JavaSalEvent::getUpdateRect()
{
	Rectangle aRet( Point( 0, 0 ), Size( 0, 0 ) );

	if ( mpData && mnID == SALEVENT_PAINT )
	{
		SalPaintEvent *pPaintEvent = (SalPaintEvent *)mpData;
		aRet = Rectangle( Point( pPaintEvent->mnBoundX, pPaintEvent->mnBoundY ), Size( pPaintEvent->mnBoundWidth, pPaintEvent->mnBoundHeight ) );
	}

	return aRet;
}

// -------------------------------------------------------------------------

ULONG JavaSalEvent::getWhen()
{
	ULONG nRet = 0;

	if ( mpData )
	{
		switch ( mnID )
		{
			case SALEVENT_ENDEXTTEXTINPUT:
			case SALEVENT_EXTTEXTINPUT:
				SalExtTextInputEvent *pInputEvent = (SalExtTextInputEvent *)mpData;
				nRet = pInputEvent->mnTime;
				break;
			case SALEVENT_KEYINPUT:
			case SALEVENT_KEYUP:
				SalKeyEvent *pKeyEvent = (SalKeyEvent *)mpData;
				nRet = pKeyEvent->mnTime;
				break;
			case SALEVENT_KEYMODCHANGE:
				SalKeyModEvent *pKeyModEvent = (SalKeyModEvent *)mpData;
				nRet = pKeyModEvent->mnCode;
				break;
			case SALEVENT_MOUSEBUTTONDOWN:
			case SALEVENT_MOUSEBUTTONUP:
			case SALEVENT_MOUSELEAVE:
			case SALEVENT_MOUSEMOVE:
				SalMouseEvent *pMouseEvent = (SalMouseEvent *)mpData;
				nRet = pMouseEvent->mnTime;
				break;
			case SALEVENT_WHEELMOUSE:
				SalWheelMouseEvent *pWheelMouseEvent = (SalWheelMouseEvent *)mpData;
				nRet = pWheelMouseEvent->mnTime;
				break;
			default:
				break;
		}
	}

	return nRet;
}

// -------------------------------------------------------------------------

long JavaSalEvent::getX()
{
	long nRet = 0;

	if ( mpData )
	{
		switch ( mnID )
		{
			case SALEVENT_MOUSEBUTTONDOWN:
			case SALEVENT_MOUSEBUTTONUP:
			case SALEVENT_MOUSELEAVE:
			case SALEVENT_MOUSEMOVE:
				SalMouseEvent *pMouseEvent = (SalMouseEvent *)mpData;
				nRet = pMouseEvent->mnX;
				break;
			case SALEVENT_WHEELMOUSE:
				SalWheelMouseEvent *pWheelMouseEvent = (SalWheelMouseEvent *)mpData;
				nRet = pWheelMouseEvent->mnX;
				break;
			default:
				break;
		}
	}

	return nRet;
}

// -------------------------------------------------------------------------

long JavaSalEvent::getY()
{
	long nRet = 0;

	if ( mpData )
	{
		switch ( mnID )
		{
			case SALEVENT_MOUSEBUTTONDOWN:
			case SALEVENT_MOUSEBUTTONUP:
			case SALEVENT_MOUSELEAVE:
			case SALEVENT_MOUSEMOVE:
				SalMouseEvent *pMouseEvent = (SalMouseEvent *)mpData;
				nRet = pMouseEvent->mnY;
				break;
			case SALEVENT_WHEELMOUSE:
				SalWheelMouseEvent *pWheelMouseEvent = (SalWheelMouseEvent *)mpData;
				nRet = pWheelMouseEvent->mnY;
				break;
			default:
				break;
		}
	}

	return nRet;
}

// -------------------------------------------------------------------------

short JavaSalEvent::getMenuID()
{
	short nRet = 0;

	if ( mpData && ( mnID == SALEVENT_MENUACTIVATE || mnID == SALEVENT_MENUCOMMAND || mnID == SALEVENT_MENUDEACTIVATE ) )
	{
		SalMenuEvent *pMenuEvent = (SalMenuEvent *)mpData;
		nRet = pMenuEvent->mnId;
	}

	return nRet;
}

// -------------------------------------------------------------------------

void *JavaSalEvent::getMenuCookie()
{
	void *pRet = NULL;

	if ( mpData && ( mnID == SALEVENT_MENUACTIVATE || mnID == SALEVENT_MENUCOMMAND || mnID == SALEVENT_MENUDEACTIVATE ) )
	{
		SalMenuEvent *pMenuEvent = (SalMenuEvent *)mpData;
		pRet = pMenuEvent->mpMenu;
	}

	return pRet;
}

// -------------------------------------------------------------------------

long JavaSalEvent::getScrollAmount()
{
	long nRet = 0;

	if ( mpData && mnID == SALEVENT_WHEELMOUSE )
	{
		SalWheelMouseEvent *pWheelMouseEvent = (SalWheelMouseEvent *)mpData;
		nRet = pWheelMouseEvent->mnScrollLines;
	}

	return nRet;
}

// -------------------------------------------------------------------------

long JavaSalEvent::getWheelRotation()
{
	long nRet = 0;

	if ( mpData && mnID == SALEVENT_WHEELMOUSE )
	{
		SalWheelMouseEvent *pWheelMouseEvent = (SalWheelMouseEvent *)mpData;
		nRet = pWheelMouseEvent->mnNotchDelta;
	}

	return nRet;
}

// -------------------------------------------------------------------------

sal_Bool JavaSalEvent::isHorizontal()
{
	sal_Bool bRet = sal_False;

	if ( mpData && mnID == SALEVENT_WHEELMOUSE )
	{
		SalWheelMouseEvent *pWheelMouseEvent = (SalWheelMouseEvent *)mpData;
		bRet = (sal_Bool)pWheelMouseEvent->mbHorz;
	}

	return bRet;
}

// -------------------------------------------------------------------------

sal_Bool JavaSalEvent::isShutdownCancelled()
{
	return mbShutdownCancelled;
}

// ----------------------------------------------------------------------------

void JavaSalEvent::reference() const
{ 
	++mnRefCount;
}

// ----------------------------------------------------------------------------

void JavaSalEvent::release() const
{
	if ( --mnRefCount > 0 )
		return;

	// const_cast because some compilers violate ANSI C++ spec
	delete const_cast< JavaSalEvent* >( this );
}

// =========================================================================

JavaSalEventQueueItem::JavaSalEventQueueItem( JavaSalEvent *pEvent, const ::std::list< JavaSalEventQueueItem* > *pEventQueue ) :
	mpEvent( pEvent ),
	mpEventQueue( pEventQueue ),
	mbRemove( false ),
	mnType( 0 )
{
	if ( mpEvent )
	{
		mpEvent->reference();

		// Set event type
		switch ( mpEvent->getID() )
		{
			case SALEVENT_EXTTEXTINPUT:
			case SALEVENT_KEYINPUT:
			case SALEVENT_KEYMODCHANGE:
			case SALEVENT_KEYUP:
				mnType = INPUT_KEYBOARD;
				break;
			case SALEVENT_MOUSEMOVE:
			case SALEVENT_MOUSELEAVE:
			case SALEVENT_MOUSEBUTTONUP:
			case SALEVENT_MOUSEBUTTONDOWN:
			case SALEVENT_WHEELMOUSE:
				mnType = INPUT_MOUSE;
				break;
			case SALEVENT_PAINT:
				mnType = INPUT_PAINT;
				break;
			default:
				mnType = INPUT_OTHER;
				break;
		}
	}
	else
	{
		mbRemove = true;
	}
}

// -------------------------------------------------------------------------

JavaSalEventQueueItem::~JavaSalEventQueueItem()
{
	if ( mpEvent )
		mpEvent->release();
}

// =========================================================================

Mutex JavaSalEventQueue::maMutex;

// -------------------------------------------------------------------------

Condition JavaSalEventQueue::maCondition;

// -------------------------------------------------------------------------

::std::list< JavaSalEventQueueItem* > JavaSalEventQueue::maNativeEventQueue;

// -------------------------------------------------------------------------

::std::list< JavaSalEventQueueItem* > JavaSalEventQueue::maNonNativeEventQueue;

// -------------------------------------------------------------------------

double JavaSalEventQueue::mfLastNativeEventTime = 0;

// -------------------------------------------------------------------------

JavaSalEventQueueItem* JavaSalEventQueue::mpKeyInputItem = NULL;

// -------------------------------------------------------------------------

JavaSalEventQueueItem* JavaSalEventQueue::mpMoveResizeItem = NULL;

// -------------------------------------------------------------------------

JavaSalEventQueueItem* JavaSalEventQueue::mpPaintItem = NULL;

// -------------------------------------------------------------------------

sal_Bool JavaSalEventQueue::mbShutdownDisabled = sal_True;

// -------------------------------------------------------------------------

void JavaSalEventQueue::purgeRemovedEventsFromFront( ::std::list< JavaSalEventQueueItem* > *pEventQueue )
{
	MutexGuard aGuard( maMutex );

	if ( pEventQueue )
	{
		while ( pEventQueue->size() && pEventQueue->front()->isRemove() )
		{
			JavaSalEventQueueItem *pItem = pEventQueue->front();
			pEventQueue->pop_front();
			if ( mpKeyInputItem == pItem )
				mpKeyInputItem = NULL;
			if ( mpMoveResizeItem == pItem )
				mpMoveResizeItem = NULL;
			if ( mpPaintItem == pItem )
				mpPaintItem = NULL;
			delete pItem;
		}
	}
}

// -------------------------------------------------------------------------

sal_Bool JavaSalEventQueue::anyCachedEvent( USHORT nType )
{
	sal_Bool bRet = sal_False;

	MutexGuard aGuard( maMutex );

	::std::list< JavaSalEventQueueItem* >::const_iterator it = maNativeEventQueue.begin();
	for ( ; it != maNativeEventQueue.end(); ++it )
	{
		if ( !(*it)->isRemove() && (*it)->getType() & nType )
		{
			bRet = sal_True;
			break;
		}
	}

	if ( !bRet )
	{
		it = maNonNativeEventQueue.begin();
		for ( ; it != maNonNativeEventQueue.end(); ++it )
		{
			if ( !(*it)->isRemove() && (*it)->getType() & nType )
			{
				bRet = sal_True;
				break;
			}
		}
	}

	return bRet;
}

// -------------------------------------------------------------------------

void JavaSalEventQueue::dispatchNextEvent()
{
	if ( CFRunLoopGetCurrent() == CFRunLoopGetMain() )
		CFRunLoopRunInMode( CFSTR( "AWTRunLoopMode" ), 0, false );
}

// -------------------------------------------------------------------------

double JavaSalEventQueue::getLastNativeEventTime()
{
	return mfLastNativeEventTime;
}

// -------------------------------------------------------------------------

JavaSalEvent *JavaSalEventQueue::getNextCachedEvent( ULONG nTimeout, sal_Bool bNativeEvents )
{
	JavaSalEvent *pRet = NULL;

	ResettableGuard< Mutex > aGuard( maMutex );

	// Wait if there are no events in either queue and a timeout is requested
	if ( nTimeout && !maNativeEventQueue.size() && !maNonNativeEventQueue.size() )
	{
		maCondition.reset();
		aGuard.clear();

		TimeValue aWait;
		aWait.Seconds = nTimeout / 1000;
		aWait.Nanosec = ( nTimeout % 1000 ) * 1000000;
		maCondition.wait( &aWait );

		aGuard.reset();
	}

	::std::list< JavaSalEventQueueItem* > *pEventQueue;
	if ( bNativeEvents )
		pEventQueue = &maNativeEventQueue;
	else
		pEventQueue = &maNonNativeEventQueue;

	if ( pEventQueue->size() )
	{
		JavaSalEventQueueItem *pItem = pEventQueue->front();
		pRet = pItem->getEvent();
		if ( pRet )
			pRet->reference();
		pItem->remove();
		purgeRemovedEventsFromFront( pEventQueue );
	}

	return pRet;
}

// -------------------------------------------------------------------------

sal_Bool JavaSalEventQueue::isShutdownDisabled()
{
	MutexGuard aGuard( maMutex );

	return mbShutdownDisabled;
}

// -------------------------------------------------------------------------

void JavaSalEventQueue::postCachedEvent( JavaSalEvent *pEvent )
{
	if ( !pEvent )
		return;

	MutexGuard aGuard( maMutex );

	::std::list< JavaSalEventQueueItem* > *pEventQueue;
	if ( pEvent->isNative() )
		pEventQueue = &maNativeEventQueue;
	else
		pEventQueue = &maNonNativeEventQueue;

	JavaSalEventQueueItem *pQueueItem = new JavaSalEventQueueItem( pEvent, pEventQueue );
	if ( pQueueItem )
	{
		// Coalesce events
		JavaSalEvent *pNewEvent = pQueueItem->getEvent();
		if ( pNewEvent )
		{
			JavaSalFrame *pFrame = pNewEvent->getFrame();
			USHORT nID = pNewEvent->getID();
			switch ( nID )
			{
				case SALEVENT_CLOSE:
				{
					for ( ::std::list< JavaSalEventQueueItem* >::const_iterator it = pEventQueue->begin(); it != pEventQueue->end(); ++it )
					{
						JavaSalEvent *pOldEvent = (*it)->getEvent();
						if ( pOldEvent && pOldEvent->getID() == nID && pOldEvent->getFrame() == pFrame && (*it)->getEventQueue() == pEventQueue && !(*it)->isRemove() )
						{
							pQueueItem->remove();
							break;
						}
					}

					break;
				}
				case SALEVENT_EXTTEXTINPUT:
				{
					// Reduce flicker when backspacing through uncommitted text
					if ( pEventQueue->size() )
					{
						JavaSalEvent *pOldEvent = pEventQueue->back()->getEvent();
						if ( pOldEvent && pOldEvent->getID() == nID && pOldEvent->getFrame() == pFrame && !pEventQueue->back()->isRemove() && !pOldEvent->getText().Len() )
							pEventQueue->back()->remove();
					}

					break;
				}
				case SALEVENT_KEYINPUT:
				{
					if ( mpKeyInputItem )
					{
						JavaSalEvent *pOldEvent = mpKeyInputItem->getEvent();
						if ( pOldEvent && pOldEvent->getFrame() == pFrame && mpKeyInputItem->getEventQueue() == pEventQueue && !mpKeyInputItem->isRemove() && pOldEvent->getKeyChar() && pNewEvent->getKeyChar() && pOldEvent->getKeyCode() == pNewEvent->getKeyCode() && pOldEvent->getModifiers() == pNewEvent->getModifiers() )
						{
							mpKeyInputItem->remove();
							pNewEvent->addRepeatCount( 1 );
						}
					}

					mpKeyInputItem = pQueueItem;
					break;
				}
				case SALEVENT_KEYUP:
				{
					mpKeyInputItem = NULL;
					break;
				}
				case SALEVENT_MOUSEMOVE:
				{
					if ( pEventQueue->size() )
					{
						JavaSalEvent *pOldEvent = pEventQueue->back()->getEvent();
						if ( pOldEvent && pOldEvent->getID() == nID && pOldEvent->getFrame() == pFrame && !pEventQueue->back()->isRemove() )
							pEventQueue->back()->remove();
					}

					break;
				}
				case SALEVENT_WHEELMOUSE:
				{
					if ( pEventQueue->size() )
					{
						JavaSalEvent *pOldEvent = pEventQueue->back()->getEvent();
						if ( pOldEvent && pOldEvent->getID() == nID && pOldEvent->getFrame() == pFrame && !pEventQueue->back()->isRemove() && pOldEvent->isHorizontal() == pNewEvent->isHorizontal() )
						{
							pEventQueue->back()->remove();
							pNewEvent->addWheelRotation( pOldEvent->getWheelRotation() );
						}
					}

					break;
				}
				case SALEVENT_MOVERESIZE:
				{
					if ( mpMoveResizeItem )
					{
						JavaSalEvent *pOldEvent = mpMoveResizeItem->getEvent();
						if ( pOldEvent && pOldEvent->getFrame() == pFrame && mpMoveResizeItem->getEventQueue() == pEventQueue && !mpMoveResizeItem->isRemove() )
							mpMoveResizeItem->remove();
					}

					mpMoveResizeItem = pQueueItem;
					break;
				}
				case SALEVENT_PAINT:
				{
					if ( mpPaintItem )
					{
						JavaSalEvent *pOldEvent = mpPaintItem->getEvent();
						if ( pOldEvent && pOldEvent->getFrame() == pFrame && mpPaintItem->getEventQueue() == pEventQueue && !mpPaintItem->isRemove() )
						{
							mpPaintItem->remove();
							pNewEvent->addUpdateRect( pOldEvent->getUpdateRect() );
						}
					}

					mpPaintItem = pQueueItem;
					break;
				}
				default:
					break;
			}
		}

		pEventQueue->push_back( pQueueItem );
		purgeRemovedEventsFromFront( pEventQueue );
	}

	maCondition.set();
}

// -------------------------------------------------------------------------

void JavaSalEventQueue::removeCachedEvents( const JavaSalFrame *pFrame )
{
	MutexGuard aGuard( maMutex );

	::std::list< JavaSalEventQueueItem* >::const_iterator it = maNativeEventQueue.begin();
	for ( ; it != maNativeEventQueue.end(); ++it )
	{
		JavaSalEvent *pEvent = (*it)->getEvent();
		if ( pEvent && pEvent->getFrame() == pFrame )
			(*it)->remove();
	}

	purgeRemovedEventsFromFront( &maNativeEventQueue );

	it = maNonNativeEventQueue.begin();
	for ( ; it != maNonNativeEventQueue.end(); ++it )
	{
		JavaSalEvent *pEvent = (*it)->getEvent();
		if ( pEvent && pEvent->getFrame() == pFrame )
			(*it)->remove();
	}

	purgeRemovedEventsFromFront( &maNonNativeEventQueue );
}

// -------------------------------------------------------------------------

void JavaSalEventQueue::setLastNativeEventTime( double nEventTime )
{
	mfLastNativeEventTime = nEventTime;
}

// -------------------------------------------------------------------------

void JavaSalEventQueue::setShutdownDisabled( sal_Bool bShutdownDisabled )
{
	VCLEventQueue_cancelTermination();

	MutexGuard aGuard( maMutex );

	mbShutdownDisabled = bShutdownDisabled;
	if ( mbShutdownDisabled )
	{
		::std::list< JavaSalEventQueueItem* >::const_iterator it = maNativeEventQueue.begin();
		for ( ; it != maNativeEventQueue.end(); ++it )
		{
			JavaSalEvent *pEvent = (*it)->getEvent();
			if ( pEvent && pEvent->getID() == SALEVENT_SHUTDOWN )
				pEvent->cancelShutdown();
		}

		purgeRemovedEventsFromFront( &maNativeEventQueue );

		it = maNonNativeEventQueue.begin();
		for ( ; it != maNonNativeEventQueue.end(); ++it )
		{
			JavaSalEvent *pEvent = (*it)->getEvent();
			if ( pEvent && pEvent->getID() == SALEVENT_SHUTDOWN )
				pEvent->cancelShutdown();
		}

		purgeRemovedEventsFromFront( &maNonNativeEventQueue );
	}
}
