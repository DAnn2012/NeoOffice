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

#ifndef _SV_SALINST_H
#define _SV_SALINST_H

#include <jni.h>

#include <salframe.h>
#include <vcl/salinst.hxx>
#include <vcl/sv.h>
#include <vcl/svapp.hxx>
#include <osl/conditn.hxx>
#include <vos/mutex.hxx>
#include <vos/thread.hxx>

// Custom event types
#define SALEVENT_OPENDOCUMENT		((USHORT)100)
#define SALEVENT_PRINTDOCUMENT		((USHORT)101)
#define SALEVENT_DEMINIMIZED		((USHORT)102)
#define SALEVENT_MINIMIZED			((USHORT)103)
#define SALEVENT_ABOUT				((USHORT)130)
#define SALEVENT_PREFS				((USHORT)140)

// -----------------
// - SalYieldMutex -
// -----------------

class SalYieldMutex : public ::vos::OMutex
{
	ULONG					mnCount;
	::osl::Condition		maMainThreadCondition;
	::vos::OThread::TThreadIdentifier	mnThreadId;

public:
							SalYieldMutex();
	virtual void			acquire();
	virtual void			release();
	virtual sal_Bool		tryToAcquire();
	ULONG					GetAcquireCount() { return mnCount; }
	::vos::OThread::TThreadIdentifier	GetThreadId() { return mnThreadId; }
};

// -------------------
// - JavaSalInstance -
// -------------------

class JavaSalInstance : public SalInstance
{
	SalYieldMutex*			mpSalYieldMutex;

public:
							JavaSalInstance();
	virtual					~JavaSalInstance();

	virtual SalFrame*		CreateChildFrame( SystemParentData* pParent, ULONG nStyle );
	virtual SalFrame*		CreateFrame( SalFrame* pParent, ULONG nStyle );
	virtual void			DestroyFrame( SalFrame* pFrame );
	virtual SalObject*		CreateObject( SalFrame* pParent, SystemWindowData* pWindowData, BOOL bShow = TRUE );
	virtual void			DestroyObject( SalObject* pObject );
	virtual SalVirtualDevice*	CreateVirtualDevice( SalGraphics* pGraphics, long nDX, long nDY, USHORT nBitCount, const SystemGraphicsData *pData = NULL );
	virtual void			DestroyVirtualDevice( SalVirtualDevice* pDevice );
	virtual SalInfoPrinter* CreateInfoPrinter( SalPrinterQueueInfo* pQueueInfo, ImplJobSetup* pSetupData );
	virtual void			DestroyInfoPrinter( SalInfoPrinter* pPrinter );
	virtual SalPrinter*		CreatePrinter( SalInfoPrinter* pInfoPrinter );
	virtual void			DestroyPrinter( SalPrinter* pPrinter );

	virtual void			GetPrinterQueueInfo( ImplPrnQueueList* pList );
	virtual void			GetPrinterQueueState( SalPrinterQueueInfo* pInfo );
	virtual void			DeletePrinterQueueInfo( SalPrinterQueueInfo* pInfo );
	virtual String			GetDefaultPrinter();
	virtual SalTimer*		CreateSalTimer();
	virtual SalI18NImeStatus*	CreateI18NImeStatus();
	virtual SalSystem*		CreateSalSystem();
	virtual SalBitmap*		CreateSalBitmap();
	virtual vos::IMutex*	GetYieldMutex();
	virtual ULONG			ReleaseYieldMutex();
	virtual void			AcquireYieldMutex( ULONG nCount );
	virtual void			Yield( bool bWait, bool bHandleAllCurrentEvents );
	virtual bool			AnyInput( USHORT nType );
	virtual SalMenu*		CreateMenu( BOOL bMenuBar, Menu* pVCLMenu );
	virtual void			DestroyMenu( SalMenu* pMenu);
	virtual SalMenuItem*	CreateMenuItem( const SalItemParams* pItemData );
	virtual void			DestroyMenuItem( SalMenuItem* pItem );
	virtual SalSession*		CreateSalSession();
	virtual void*			GetConnectionIdentifier( ConnectionIdentifierType& rReturnedType, int& rReturnedBytes );
};

// ----------------
// - JavaSalEvent -
// ----------------

class SAL_DLLPRIVATE JavaSalEvent
{
	::vcl::com_sun_star_vcl_VCLEvent*	mpVCLEvent;

public:
							JavaSalEvent( USHORT nID, const JavaSalFrame *pFrame, void *pData );
							JavaSalEvent( USHORT nID, const JavaSalFrame *pFrame, void *pData, const ::rtl::OString &rPath );
							JavaSalEvent( ::vcl::com_sun_star_vcl_VCLEvent *pVCLEvent );
	virtual					~JavaSalEvent();

	void					cancelShutdown();
	void					dispatch();
	ULONG					getCommittedCharacterCount();
	ULONG					getCursorPosition();
	void*					getData();
	JavaSalFrame*			getFrame();
	USHORT					getKeyChar();
	USHORT					getKeyCode();
	USHORT					getID();
	USHORT					getModifiers();
	JavaSalEvent*			getNextOriginalKeyEvent();
	::rtl::OUString			getPath();
	USHORT					getRepeatCount();
	::rtl::OUString			getText();
	USHORT*					getTextAttributes();
	const Rectangle			getUpdateRect();
	::vcl::com_sun_star_vcl_VCLEvent*	getVCLEvent() const { return mpVCLEvent; }
	ULONG					getWhen();
	long					getX();
	long					getY();
	short					getMenuID();
	void*					getMenuCookie();
	long					getScrollAmount();
	ULONG					getVisiblePosition();
	long					getWheelRotation();
	sal_Bool				isHorizontal();
	sal_Bool				isShutdownCancelled();
};

class SAL_DLLPRIVATE JavaSalEventQueue
{
	static ::osl::Mutex		maMutex;
	static ::vcl::com_sun_star_vcl_VCLEventQueue*	mpVCLEventQueue;

public:
	static ::vcl::com_sun_star_vcl_VCLEventQueue*	getVCLEventQueue();
	static sal_Bool			postCommandEvent( jobject aObj, short nKeyCode, sal_Bool bShiftDown, sal_Bool bControlDown, sal_Bool bAltDown, sal_Bool bMetaDown, jchar nOriginalKeyChar, sal_Bool bOriginalShiftDown, sal_Bool bOriginalControlDown, sal_Bool bOriginalAltDown, sal_Bool bOriginalMetaDown );
	static void				postMouseWheelEvent( jobject aObj, long nX, long nY, long nRotationX, long nRotationY, sal_Bool bShiftDown, sal_Bool bMetaDown, sal_Bool bAltDown, sal_Bool bControlDown );
#ifdef USE_NATIVE_WINDOW
	static void				postMenuItemSelectedEvent( JavaSalFrame *pFrame, USHORT nID, Menu *pMenu );
#endif	// USE_NATIVE_WINDOW
	static void				postWindowMoveSessionEvent( jobject aObj, long nX, long nY, sal_Bool bStartSession );
	static sal_Bool			anyCachedEvent( USHORT nType );
	static void				dispatchNextEvent();
	static JavaSalEvent*	getNextCachedEvent( ULONG nTimeout, sal_Bool bNativeEvents );
	static sal_Bool			isInitialized();
	static sal_Bool			isShutdownDisabled();
	static void				postCachedEvent( const JavaSalEvent *pEvent );
	static void				removeCachedEvents( const JavaSalFrame *pFrame );
	static void				setShutdownDisabled( sal_Bool bShutdownDisabled );
};

SAL_DLLPRIVATE void InitJavaAWT();

#endif // _SV_SALINST_H
