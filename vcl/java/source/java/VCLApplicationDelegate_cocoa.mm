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
 *  Patrick Luby, August 2010
 *
 *  GNU General Public License Version 2.1
 *  =============================================
 *  Copyright 2010 Planamesa Inc.
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
#include <list>

#include <premac.h>
#import <objc/objc-runtime.h>
#include <postmac.h>

#include <saldata.hxx>
#include <salframe.h>
#include <rtl/ustring.hxx>
#include <vcl/svapp.hxx>
#include <vcl/svids.hrc>
#include <vos/mutex.hxx>

#include "VCLApplicationDelegate_cocoa.h"
#include "../app/salinst_cocoa.h"

// Comment out the following line to disable native resume support
#define USE_NATIVE_RESUME

#ifndef NSURLBookmarkCreationWithSecurityScope
#define NSURLBookmarkCreationWithSecurityScope ( 1UL << 11 )
#endif	// !NSURLBookmarkCreationWithSecurityScope

#ifndef NSURLBookmarkResolutionWithSecurityScope
#define NSURLBookmarkResolutionWithSecurityScope ( 1UL << 10 )
#endif	// !NSURLBookmarkResolutionWithSecurityScope

typedef void KeyScript_Type( short nCode );

struct ImplPendingOpenPrintFileRequest
{
	::rtl::OString		maPath;
	sal_Bool			mbPrint;

						ImplPendingOpenPrintFileRequest( const ::rtl::OString &rPath, sal_Bool bPrint ) : maPath( rPath ), mbPrint( bPrint ) {}
						~ImplPendingOpenPrintFileRequest() {};
};

static std::list< ImplPendingOpenPrintFileRequest* > aPendingOpenPrintFileRequests;

using namespace rtl;
using namespace vcl;
using namespace vos;

static void HandleAboutRequest()
{
	// If no application mutex exists yet, ignore event as we are likely to
	// crash
	if ( !Application::IsShutDown() && ImplGetSVData() && ImplGetSVData()->mpDefInst )
	{
		JavaSalEvent *pEvent = new JavaSalEvent( SALEVENT_ABOUT, NULL, NULL);
		JavaSalEventQueue::postCachedEvent( pEvent );
		pEvent->release();
	}
}

static void HandleOpenPrintFileRequest( const OString &rPath, sal_Bool bPrint )
{
	if ( rPath.getLength() && !Application::IsShutDown() )
	{
		// If no application mutex exists yet, queue event as we are likely to
		// crash
		if ( ImplGetSVData() && ImplGetSVData()->mpDefInst )
		{
			JavaSalEvent *pEvent = new JavaSalEvent( bPrint ? SALEVENT_PRINTDOCUMENT : SALEVENT_OPENDOCUMENT, NULL, NULL, rPath );
			JavaSalEventQueue::postCachedEvent( pEvent );
			pEvent->release();
		}
		else
		{
			ImplPendingOpenPrintFileRequest *pRequest = new ImplPendingOpenPrintFileRequest( rPath, bPrint );
			if ( pRequest )
				aPendingOpenPrintFileRequests.push_back( pRequest );
		}
	}
}

static void HandlePreferencesRequest()
{
	// If no application mutex exists yet, ignore event as we are likely to
	// crash
	if ( !Application::IsShutDown() && ImplGetSVData() && ImplGetSVData()->mpDefInst )
	{
		JavaSalEvent *pEvent = new JavaSalEvent( SALEVENT_PREFS, NULL, NULL);
		JavaSalEventQueue::postCachedEvent( pEvent );
		pEvent->release();
	}
}

static NSApplicationTerminateReply HandleTerminationRequest()
{
	NSApplicationTerminateReply nRet = NSTerminateCancel;

	// If no application mutex exists yet, ignore event as we are likely to
	// crash
	if ( !Application::IsShutDown() && ImplGetSVData() && ImplGetSVData()->mpDefInst )
	{
		IMutex& rSolarMutex = Application::GetSolarMutex();
		rSolarMutex.acquire();

		if ( !Application::IsShutDown() && !JavaSalEventQueue::isShutdownDisabled() )
		{
			JavaSalEvent *pEvent = new JavaSalEvent( SALEVENT_SHUTDOWN, NULL, NULL );
			JavaSalEventQueue::postCachedEvent( pEvent );
			while ( !Application::IsShutDown() && !pEvent->isShutdownCancelled() && !JavaSalEventQueue::isShutdownDisabled() )
				Application::Yield();
			pEvent->release();

			if ( Application::IsShutDown() )
			{
				// Close any windows still showing so that all windows
				// get the appropriate window closing delegate calls
				NSApplication *pApp = [NSApplication sharedApplication];
				if ( pApp )
				{
					NSArray *pWindows = [pApp windows];
					if ( pWindows )
					{
						unsigned int i = 0;
						unsigned int nCount = [pWindows count];
						for ( ; i < nCount ; i++ )
						{
							NSWindow *pWindow = [pWindows objectAtIndex:i];
							if ( pWindow )
								[pWindow orderOut:pWindow];
						}
					}
				}
			}
		}

		rSolarMutex.release();
	}

	if ( Application::IsShutDown() )
	{
		nRet = NSTerminateLater;

		// Fire a "will termination" notification since our app delegate
		// never returns YES when queried if we should terminate
		NSApplication *pApp = [NSApplication sharedApplication];
		NSNotificationCenter *pNotificationCenter = [NSNotificationCenter defaultCenter];
		if ( pApp && pNotificationCenter )
			[pNotificationCenter postNotificationName:NSApplicationWillTerminateNotification object:pApp];
	}

	return nRet;
}

static void HandleDidChangeScreenParametersRequest()
{
	// If no application mutex exists yet, ignore event as we are likely to
	// crash
	if ( !Application::IsShutDown() && ImplGetSVData() && ImplGetSVData()->mpDefInst )
	{
		IMutex& rSolarMutex = Application::GetSolarMutex();
		rSolarMutex.acquire();

		if ( !Application::IsShutDown() )
		{
			SalData *pSalData = GetSalData();
			for ( std::list< JavaSalFrame* >::const_iterator it = pSalData->maFrameList.begin(); it != pSalData->maFrameList.end(); ++it )
			{
				if ( (*it)->mbVisible )
					(*it)->SetPosSize( 0, 0, 0, 0, 0 );
			}
		}

		rSolarMutex.release();
	}
}

static VCLApplicationDelegate *pSharedAppDelegate = nil;

@interface VCLDocument : NSDocument
- (MacOSBOOL)readFromURL:(NSURL *)pURL ofType:(NSString *)pTypeName error:(NSError **)ppError;
- (void)restoreStateWithCoder:(NSCoder *)pCoder;
@end

@interface NSDocumentController (VCLDocumentController)
- (void)_docController:(NSDocumentController *)pDocController shouldTerminate:(MacOSBOOL)bShouldTerminate;
@end

@interface VCLDocumentController : NSDocumentController
- (void)_closeAllDocumentsWithDelegate:(id)pDelegate shouldTerminateSelector:(SEL)aShouldTerminateSelector;
- (id)makeDocumentWithContentsOfURL:(NSURL *)pAbsoluteURL ofType:(NSString *)pTypeName error:(NSError **)ppError;
@end

@implementation VCLDocument

- (MacOSBOOL)readFromURL:(NSURL *)pURL ofType:(NSString *)pTypeName error:(NSError **)ppError
{
	if ( ppError )
		*ppError = nil;

	return YES;
}

- (void)restoreStateWithCoder:(NSCoder *)pCoder
{
	// Don't allow NSDocument to do the restoration
}

@end

@implementation VCLDocumentController

- (void)_closeAllDocumentsWithDelegate:(id)pDelegate shouldTerminateSelector:(SEL)aShouldTerminateSelector
{
	if ( pDelegate && [pDelegate respondsToSelector:aShouldTerminateSelector] && sel_isEqual( aShouldTerminateSelector, @selector(_docController:shouldTerminate:) ) )
		[pDelegate _docController:self shouldTerminate:YES];
}

- (id)makeDocumentWithContentsOfURL:(NSURL *)pAbsoluteURL ofType:(NSString *)pTypeName error:(NSError **)ppError
{
	if ( ppError )
		*ppError = nil;

#ifdef USE_NATIVE_RESUME
	if ( pSharedAppDelegate && pAbsoluteURL && [pAbsoluteURL isFileURL] )
	{
		NSApplication *pApp = [NSApplication sharedApplication];
		NSString *pPath = [pAbsoluteURL path];
		if ( pApp && pPath )
		{
			MacOSBOOL bResume = YES;
			CFPropertyListRef aPref = CFPreferencesCopyAppValue( CFSTR( "DisableResume" ), kCFPreferencesCurrentApplication );
			if ( aPref )
			{
				if ( CFGetTypeID( aPref ) == CFBooleanGetTypeID() && (CFBooleanRef)aPref == kCFBooleanTrue )
					bResume = NO;
				CFRelease( aPref );
			}

			if ( bResume )
				[pSharedAppDelegate application:pApp openFile:pPath];
		}
	}
#endif	// USE_NATIVE_RESUME

	VCLDocument *pDoc = [[VCLDocument alloc] init];
	[pDoc autorelease];
	return pDoc;
}

@end

@implementation VCLApplicationDelegate

+ (VCLApplicationDelegate *)sharedDelegate
{
	// Do not retain as invoking alloc disables autorelease
	if ( !pSharedAppDelegate )
		pSharedAppDelegate = [[VCLApplicationDelegate alloc] init];

	return pSharedAppDelegate;
}

- (void)addMenuBarItem:(NSNotification *)pNotification
{
	if ( pNotification )
	{
		NSApplication *pApp = [NSApplication sharedApplication];
		NSMenu *pObject = [pNotification object];
		if ( pApp && pObject && pObject == [pApp mainMenu] )
		{
			NSUInteger i = 0;
			NSUInteger nCount = [pObject numberOfItems];
			for ( ; i < nCount; i++ )
			{
				NSMenuItem *pItem = [pObject itemAtIndex:i];
				if ( pItem )
				{
					NSMenu *pSubmenu = [pItem submenu];
					if ( pSubmenu )
					{
						[pSubmenu setDelegate:self];

						// Set help menu
						if ( i == nCount - 1 && [pApp respondsToSelector:@selector(setHelpMenu:)] )
							[pApp setHelpMenu:pSubmenu];
					}
				}
			}
		}
	}
}

- (MacOSBOOL)application:(NSApplication *)pApplication openFile:(NSString *)pFilename
{
	if ( mbInTermination || !pFilename )
		return NO;

	NSFileManager *pFileManager = [NSFileManager defaultManager];
	if ( pFileManager )
	{
		MacOSBOOL bDir = NO;
		if ( [pFileManager fileExistsAtPath:pFilename isDirectory:&bDir] && !bDir )
			HandleOpenPrintFileRequest( [pFilename UTF8String], sal_False );
	}

	Application_cacheSecurityScopedURL( [NSURL fileURLWithPath:pFilename] );

	return YES;
}

- (MacOSBOOL)application:(NSApplication *)pApplication printFile:(NSString *)pFilename
{
	if ( mbInTermination || !pFilename )
		return NO;

	NSFileManager *pFileManager = [NSFileManager defaultManager];
	if ( pFileManager )
	{
		MacOSBOOL bDir = NO;
		if ( [pFileManager fileExistsAtPath:pFilename isDirectory:&bDir] && !bDir )
			HandleOpenPrintFileRequest( [pFilename UTF8String], sal_True );
	}

	Application_cacheSecurityScopedURL( [NSURL fileURLWithPath:pFilename] );

	return YES;
}

- (void)applicationDidBecomeActive:(NSNotification *)pNotification
{
	// Fix bug 221 by explicitly reenabling all keyboards
	void *pLib = dlopen( NULL, RTLD_LAZY | RTLD_LOCAL );
	if ( pLib )
	{
		KeyScript_Type *pKeyScript = (KeyScript_Type *)dlsym( pLib, "KeyScript" );
		if ( pKeyScript )
			pKeyScript( smKeyEnableKybds );

		dlclose( pLib );
	}
}

- (void)applicationDidChangeScreenParameters:(NSNotification *)pNotification
{
	if ( mpDelegate && [mpDelegate respondsToSelector:@selector(applicationDidChangeScreenParameters:)] )
		[mpDelegate applicationDidChangeScreenParameters:pNotification];

	// Fix bug 3559 by making sure that the frame fits in the work area
	// if the screen size has changed
	HandleDidChangeScreenParametersRequest();
}

- (NSMenu *)applicationDockMenu:(NSApplication *)pApplication
{
	return mpDockMenu;
}

- (MacOSBOOL)applicationShouldHandleReopen:(NSApplication *)pApplication hasVisibleWindows:(MacOSBOOL)bFlag
{
	// Fix bug reported in the following NeoOffice forum topic by
	// returning true if there is visible windows:
	// http://trinity.neooffice.org/modules.php?name=Forums&file=viewtopic&t=8478
	return bFlag;
}

- (MacOSBOOL)applicationShouldOpenUntitledFile:(NSApplication *)pSender
{
	return NO;
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)pApplication
{
	if ( mbInTermination || ( pApplication && [pApplication modalWindow] ) )
		return NSTerminateCancel;

	mbInTermination = YES;
	return HandleTerminationRequest();
}

- (void)applicationWillFinishLaunching:(NSNotification *)pNotification
{
	// Make our NSDocumentController subclass the shared controller by creating
	// an instance of our subclass before AppKit does
	[[VCLDocumentController alloc] init];

	if ( mpDelegate && [mpDelegate respondsToSelector:@selector(applicationWillFinishLaunching:)] )
		[mpDelegate applicationWillFinishLaunching:pNotification];
}

- (void)cancelTermination
{
	mbInTermination = NO;

	// Dequeue any pending events
	std::list< ImplPendingOpenPrintFileRequest* > aRequests( aPendingOpenPrintFileRequests );
	aPendingOpenPrintFileRequests.clear();
	while ( aRequests.size() )
	{
		ImplPendingOpenPrintFileRequest *pRequest = aRequests.front();
		if ( pRequest )
		{
			HandleOpenPrintFileRequest( pRequest->maPath, pRequest->mbPrint );
			delete pRequest;
		}
		aRequests.pop_front();
	}
}

- (void)dealloc
{
	NSNotificationCenter *pNotificationCenter = [NSNotificationCenter defaultCenter];
	if ( pNotificationCenter )
	{
		[pNotificationCenter removeObserver:self name:NSMenuDidAddItemNotification object:nil];
		[pNotificationCenter removeObserver:self name:NSMenuDidBeginTrackingNotification object:nil];
		[pNotificationCenter removeObserver:self name:NSMenuDidEndTrackingNotification object:nil];
	}

	if ( mpDelegate )
		[mpDelegate release];

	if ( mpDockMenu )
		[mpDockMenu release];

	[super dealloc];
}

- (id)init
{
	[super init];

	mbAppMenuInitialized = NO;
	mbCancelTracking = NO;
	mpDelegate = nil;
	mpDockMenu = [[NSMenu alloc] initWithTitle:@""];
	mbInTermination = NO;
	mbInTracking = NO;

	// Set the application delegate as the delegate for the application menu so
	// that the Java menu item target and selector can be replaced with our own
	NSApplication *pApp = [NSApplication sharedApplication];
	if ( pApp )
	{
		NSMenu *pMainMenu = [pApp mainMenu];
		if ( pMainMenu )
		{
			if ( [pMainMenu numberOfItems] > 0 )
			{
				NSMenuItem *pItem = [pMainMenu itemAtIndex:0];
				if ( pItem )
				{
					NSMenu *pAppMenu = [pItem submenu];
					if ( pAppMenu )
						[pAppMenu setDelegate:self];
				}
			}
		
			NSNotificationCenter *pNotificationCenter = [NSNotificationCenter defaultCenter];
			if ( pNotificationCenter )
			{
				[pNotificationCenter addObserver:self selector:@selector(addMenuBarItem:) name:NSMenuDidAddItemNotification object:nil];
				[pNotificationCenter addObserver:self selector:@selector(trackMenuBar:) name:NSMenuDidBeginTrackingNotification object:nil];
				[pNotificationCenter addObserver:self selector:@selector(trackMenuBar:) name:NSMenuDidEndTrackingNotification object:nil];
			}
		}
	}

	// Set the application delegate as the delegate for the dock menu
	if ( mpDockMenu )
		[mpDockMenu setDelegate:self];
	
	return self;
}

- (MacOSBOOL)isInTracking
{
	return mbInTracking;
}

- (void)menuNeedsUpdate:(NSMenu *)pMenu
{
	if ( !mbAppMenuInitialized )
	{
		NSApplication *pApp = [NSApplication sharedApplication];
		if ( pApp )
		{
			NSMenu *pMainMenu = [pApp mainMenu];
			if ( pMainMenu )
			{
				NSMenuItem *pItem = [pMainMenu itemAtIndex:0];
				if ( pItem )
				{
					NSMenu *pAppMenu = [pItem submenu];
					if ( pAppMenu )
					{
						if ( !Application::IsShutDown() && ImplGetSVData() && ImplGetSVData()->mpDefInst )
						{
							IMutex& rSolarMutex = Application::GetSolarMutex();
							rSolarMutex.acquire();
							mbAppMenuInitialized = YES;

							if ( !Application::IsShutDown() )
							{
								NSString *pAbout = nil;
								NSString *pPreferences = nil;
								NSString *pServices = nil;
								NSString *pHide = nil;
								NSString *pHideOthers = nil;
								NSString *pShowAll = nil;
								NSString *pQuit = nil;

								ResMgr *pResMgr = ImplGetResMgr();
								if ( pResMgr )
								{
									String aAbout( ResId( SV_STDTEXT_ABOUT, *pResMgr ) );
									if ( aAbout.Len() )
										pAbout = [NSString stringWithCharacters:aAbout.GetBuffer() length:aAbout.Len()];

									String aPreferences( ResId( SV_STDTEXT_PREFERENCES, *pResMgr ) );
									if ( aPreferences.Len() )
										pPreferences = [NSString stringWithCharacters:aPreferences.GetBuffer() length:aPreferences.Len()];

									String aServices( ResId( SV_MENU_MAC_SERVICES, *pResMgr ) );
									if ( aServices.Len() )
										pServices = [NSString stringWithCharacters:aServices.GetBuffer() length:aServices.Len()];

									String aHide( ResId( SV_MENU_MAC_HIDEAPP, *pResMgr ) );
									if ( aHide.Len() )
										pHide = [NSString stringWithCharacters:aHide.GetBuffer() length:aHide.Len()];

									String aHideOthers( ResId( SV_MENU_MAC_HIDEALL, *pResMgr ) );
									if ( aHideOthers.Len() )
										pHideOthers = [NSString stringWithCharacters:aHideOthers.GetBuffer() length:aHideOthers.Len()];

									String aShowAll( ResId( SV_MENU_MAC_SHOWALL, *pResMgr ) );
									if ( aShowAll.Len() )
										pShowAll = [NSString stringWithCharacters:aShowAll.GetBuffer() length:aShowAll.Len()];

									String aQuit( ResId( SV_MENU_MAC_QUITAPP, *pResMgr ) );
									if ( aQuit.Len() )
										pQuit = [NSString stringWithCharacters:aQuit.GetBuffer() length:aQuit.Len()];
								}

								NSUInteger nItems = [pAppMenu numberOfItems];
								NSUInteger i = 0;
								for ( ; i < nItems; i++ )
								{
									NSMenuItem *pItem = [pAppMenu itemAtIndex:i];
									if ( pItem )
									{
										NSString *pTitle = [pItem title];
										if ( pTitle )
										{
											if ( pAbout && [pTitle isEqualToString:@"About"] )
											{
												[pItem setTarget:self];
												[pItem setAction:@selector(showAbout)];
												[pItem setTitle:pAbout];
											}
											else if ( pPreferences && [pTitle isEqualToString:@"Preferences…"] )
											{
												[pItem setTarget:self];
												[pItem setAction:@selector(showPreferences)];
												[pItem setTitle:pPreferences];
											}
											else if ( pServices && [pTitle isEqualToString:@"Services"] )
											{
												[pItem setTitle:pServices];
											}
											else if ( pHide && [pTitle isEqualToString:@"Hide"] )
											{
												[pItem setTitle:pHide];
											}
											else if ( pHideOthers && [pTitle isEqualToString:@"Hide Others"] )
											{
												[pItem setTitle:pHideOthers];
											}
											else if ( pShowAll && [pTitle isEqualToString:@"Show All"] )
											{
												[pItem setTitle:pShowAll];
											}
											else if ( pQuit && [pTitle isEqualToString:@"Quit"] )
											{
												[pItem setTitle:pQuit];
											}
										}
									}
								}
							}

							rSolarMutex.release();
						}
					}
				}
			}
		}
	}

	if ( pMenu && ( !mbInTracking || mbCancelTracking ) )
		[pMenu cancelTracking];
}

- (void)setDelegate:(id)pDelegate
{
	if ( mpDelegate )
	{
    	[mpDelegate release];
    	mpDelegate = nil;
	}

	if ( pDelegate )
	{
    	mpDelegate = pDelegate;
    	[mpDelegate retain];
	}
}

- (void)showAbout
{
	if ( !mbInTermination )
		HandleAboutRequest();
}

- (void)showPreferences
{
	if ( !mbInTermination )
		HandlePreferencesRequest();
}

- (void)trackMenuBar:(NSNotification *)pNotification
{
	if ( pNotification )
	{
		NSApplication *pApp = [NSApplication sharedApplication];
		NSMenu *pObject = [pNotification object];
		if ( pApp )
		{
			NSMenu *pMainMenu = [pApp mainMenu];
			if ( pObject && pObject == pMainMenu )
			{
				NSString *pName = [pNotification name];
				if ( [NSMenuDidBeginTrackingNotification isEqualToString:pName] )
				{
					mbCancelTracking = NO;
					mbInTracking = NO;
					if ( VCLInstance_updateNativeMenus() )
					{
						mbInTracking = YES;

						// Fix bug reported in the following NeoOffice forum
						// topic by forcing any pending menu changes to be done
						// before any menus are displayed:
						// http://trinity.neooffice.org/modules.php?name=Forums&file=viewtopic&t=8532
						[VCLMainMenuDidEndTracking mainMenuDidEndTracking:YES];
					}
					else
					{
						mbCancelTracking = YES;
					}
				}
				else if ( [NSMenuDidEndTrackingNotification isEqualToString:pName] )
				{
					mbCancelTracking = YES;
					mbInTracking = NO;
				}
			}
		}
	}
}

- (MacOSBOOL)validateMenuItem:(NSMenuItem *)pMenuItem
{
	return ( !mbInTermination && pMenuItem );
}

@end
