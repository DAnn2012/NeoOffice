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

#ifndef __VCLAPPLICATIONDELEGATE_COCOA_H__
#define __VCLAPPLICATIONDELEGATE_COCOA_H__

@interface VCLApplicationDelegate : NSObject
{
	BOOL				mbAppMenuInitialized;
	BOOL				mbCancelTracking;
	id					mpDelegate;
	BOOL				mbInTermination;
	BOOL				mbInTracking;
}
+ (VCLApplicationDelegate *)sharedDelegate;
- (void)addMenuBarItem:(NSNotification *)pNotification;
- (BOOL)application:(NSApplication *)pApplication openFile:(NSString *)pFilename;
- (BOOL)application:(NSApplication *)pApplication printFile:(NSString *)pFilename;
- (void)applicationDidBecomeActive:(NSNotification *)pNotification;
- (void)applicationDidChangeScreenParameters:(NSNotification *)pNotification;
- (BOOL)applicationShouldHandleReopen:(NSApplication *)pApplication hasVisibleWindows:(BOOL)bFlag;
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)pApplication;
- (void)applicationWillFinishLaunching:(NSNotification *)pNotification;
- (void)cancelTermination;
- (void)dealloc;
- (id)init;
- (void)menuNeedsUpdate:(NSMenu *)pMenu;
- (void)setDelegate:(id)pDelegate;
- (void)showAbout;
- (void)showPreferences;
- (void)trackMenuBar:(NSNotification *)pNotification;
- (BOOL)validateMenuItem:(NSMenuItem *)pMenuItem;
@end

#endif
