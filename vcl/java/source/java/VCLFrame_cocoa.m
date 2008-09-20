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
 *  Patrick Luby, July 2005
 *
 *  GNU General Public License Version 2.1
 *  =============================================
 *  Copyright 2005 Planamesa Inc.
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

#import <Cocoa/Cocoa.h>
#import "VCLEventQueue_cocoa.h"
#import "VCLFrame_cocoa.h"

@interface NSObject (CWindow)
- (NSWindow *)getNSWindow;
@end

@interface GetNSWindow : NSObject
{
	id					mpCWindow;
	NSWindow*			mpWindow;
}
+ (id)createWithCWindow:(id)pCWindow;
- (void)getNSWindow:(id)pObject;
- (id)initWithCWindow:(id)pCWindow;
- (NSWindow *)window;
@end

@implementation GetNSWindow

+ (id)createWithCWindow:(id)pCWindow
{
	GetNSWindow *pRet = [[GetNSWindow alloc] initWithCWindow:pCWindow];
	[pRet autorelease];
	return pRet;
}

- (void)getNSWindow:(id)pObject
{
	if ( [mpCWindow respondsToSelector:@selector(getNSWindow)] )
		mpWindow = (NSWindow *)[mpCWindow getNSWindow];
}

- (id)initWithCWindow:(id)pCWindow
{
	[super init];

	mpCWindow = pCWindow;
	mpWindow = nil;

	return self;
}

- (NSWindow *)window
{
	return mpWindow;
}

@end

@interface GetWindowRef : NSObject
{
	id					mpCWindow;
	WindowRef			maWindow;
}
+ (id)createWithCWindow:(id)pCWindow;
- (void)getWindowRef:(id)pObject;
- (id)initWithCWindow:(id)pCWindow;
- (WindowRef)windowRef;
@end

@implementation GetWindowRef

+ (id)createWithCWindow:(id)pCWindow
{
	GetWindowRef *pRet = [[GetWindowRef alloc] initWithCWindow:pCWindow];
	[pRet autorelease];
	return pRet;
}

- (void)getWindowRef:(id)pObject
{
	if ( [mpCWindow respondsToSelector:@selector(getNSWindow)] )
	{
		NSWindow *pWindow = (NSWindow *)[mpCWindow getNSWindow];
		if ( pWindow )
			maWindow = [pWindow windowRef];
	}
}

- (id)initWithCWindow:(id)pCWindow
{
	[super init];

	mpCWindow = pCWindow;
	maWindow = nil;

	return self;
}

- (WindowRef)windowRef
{
	return maWindow;
}

@end

@interface MakeFloatingWindow : NSObject
{
	id					mpCWindow;
	int					mnTopInset;
}
+ (id)createWithCWindow:(id)pCWindow;
- (int)getTopInset;
- (id)initWithCWindow:(id)pCWindow;
- (void)makeFloatingWindow:(id)pObject;
@end

@implementation MakeFloatingWindow

+ (id)createWithCWindow:(id)pCWindow
{
	MakeFloatingWindow *pRet = [[MakeFloatingWindow alloc] initWithCWindow:pCWindow];
	[pRet autorelease];
	return pRet;
}

- (int)getTopInset
{
	return mnTopInset;
}

- (id)initWithCWindow:(id)pCWindow
{
	[super init];

	mpCWindow = pCWindow;
	mnTopInset = 0;

	return self;
}

- (void)makeFloatingWindow:(id)pObject
{
	if ( [mpCWindow respondsToSelector:@selector(getNSWindow)] )
	{
		NSWindow *pWindow = (NSWindow *)[mpCWindow getNSWindow];
		if ( pWindow && ![pWindow isVisible] )
		{
			if ( [pWindow styleMask] & NSTitledWindowMask )
			{
				NSView *pContentView = [pWindow contentView];
				if ( pContentView )
				{
					NSView *pSuperview = [pContentView superview];
					if ( pSuperview && [pSuperview respondsToSelector:@selector(_setUtilityWindow:)] )
					{
						[pWindow setLevel:NSFloatingWindowLevel];
						[pWindow setHidesOnDeactivate:YES];

						// Get the top inset for a utility window
						NSRect aFrameRect = NSMakeRect( 0, 0, 100, 100 );
						NSRect aContentRect = [NSWindow contentRectForFrameRect:aFrameRect styleMask:[pWindow styleMask] | NSUtilityWindowMask];
						mnTopInset = aFrameRect.origin.y + aFrameRect.size.height - aContentRect.origin.y - aContentRect.size.height;
					}
				}
			}
			else
			{
				[pWindow setLevel:NSPopUpMenuWindowLevel];
			}
		}
	}
}

@end

@interface MakeModalWindow : NSObject
{
	id					mpCWindow;
}
+ (id)createWithCWindow:(id)pCWindow;
- (id)initWithCWindow:(id)pCWindow;
- (void)makeModalWindow:(id)pObject;
@end

@implementation MakeModalWindow

+ (id)createWithCWindow:(id)pCWindow
{
	MakeModalWindow *pRet = [[MakeModalWindow alloc] initWithCWindow:pCWindow];
	[pRet autorelease];
	return pRet;
}

- (id)initWithCWindow:(id)pCWindow
{
	[super init];

	mpCWindow = pCWindow;

	return self;
}

- (void)makeModalWindow:(id)pObject
{
	if ( [mpCWindow respondsToSelector:@selector(getNSWindow)] )
	{
		NSWindow *pWindow = (NSWindow *)[mpCWindow getNSWindow];
		if ( pWindow && [pWindow styleMask] & NSTitledWindowMask && [pWindow respondsToSelector:@selector(_setModalWindowLevel)] )
		{
			[pWindow _setModalWindowLevel];

			// Run VCLWindow selector to ensure that the window level is set
			// correctly if the application is not active
			[VCLWindow clearModalWindowLevel];
		}
	}
}

@end

@interface UpdateLocation : NSObject
{
	id					mpCWindow;
}
+ (id)createWithCWindow:(id)pCWindow;
- (id)initWithCWindow:(id)pCWindow;
- (void)updateLocation:(id)pObject;
@end

@implementation UpdateLocation

+ (id)createWithCWindow:(id)pCWindow
{
	UpdateLocation *pRet = [[UpdateLocation alloc] initWithCWindow:pCWindow];
	[pRet autorelease];
	return pRet;
}

- (id)initWithCWindow:(id)pCWindow
{
	[super init];

	mpCWindow = pCWindow;

	return self;
}

- (void)updateLocation:(id)pObject
{
	if ( [mpCWindow respondsToSelector:@selector(getNSWindow)] )
	{
		NSWindow *pWindow = (NSWindow *)[mpCWindow getNSWindow];
		if ( pWindow )
		{
			NSRect aBounds = [pWindow frame];
			NSPoint aPoint = NSMakePoint( aBounds.origin.x + 1, aBounds.origin.y + 1 );
			[pWindow setFrameOrigin:aPoint];
			[pWindow setFrameOrigin:aBounds.origin];
		}
	}
}

@end

id CWindow_getNSWindow( id pCWindow )
{
	NSWindow *pNSWindow = nil;

	NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

	if ( pCWindow )
	{
		GetNSWindow *pGetNSWindow = [GetNSWindow createWithCWindow:pCWindow];
		NSArray *pModes = [NSArray arrayWithObjects:NSDefaultRunLoopMode, NSEventTrackingRunLoopMode, NSModalPanelRunLoopMode, @"AWTRunLoopMode", nil];
		[pGetNSWindow performSelectorOnMainThread:@selector(getNSWindow:) withObject:pGetNSWindow waitUntilDone:YES modes:pModes];
		pNSWindow = [pGetNSWindow window];
	}

	[pPool release];

	return pNSWindow;
}

WindowRef CWindow_getWindowRef( id pCWindow )
{
	NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

	WindowRef aWindow = nil;

	if ( pCWindow )
	{
		NSArray *pModes = [NSArray arrayWithObjects:NSDefaultRunLoopMode, NSEventTrackingRunLoopMode, NSModalPanelRunLoopMode, @"AWTRunLoopMode", nil];
		GetWindowRef *pGetWindowRef = [GetWindowRef createWithCWindow:pCWindow];
		[pGetWindowRef performSelectorOnMainThread:@selector(getWindowRef:) withObject:pGetWindowRef waitUntilDone:YES modes:pModes];
		aWindow = [pGetWindowRef windowRef];
	}

	[pPool release];

	return aWindow;
}

int CWindow_makeFloatingWindow( id pCWindow )
{
	int nRet = 0;

	NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

	if ( pCWindow )
	{
		NSArray *pModes = [NSArray arrayWithObjects:NSDefaultRunLoopMode, NSEventTrackingRunLoopMode, NSModalPanelRunLoopMode, @"AWTRunLoopMode", nil];
		MakeFloatingWindow *pMakeFloatingWindow = [MakeFloatingWindow createWithCWindow:pCWindow];
		[pMakeFloatingWindow performSelectorOnMainThread:@selector(makeFloatingWindow:) withObject:pMakeFloatingWindow waitUntilDone:YES modes:pModes];
		nRet = [pMakeFloatingWindow getTopInset];
	}

	[pPool release];

	return nRet;
}

void CWindow_makeModalWindow( id pCWindow )
{
	NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

	if ( pCWindow )
	{
		NSArray *pModes = [NSArray arrayWithObjects:NSDefaultRunLoopMode, NSEventTrackingRunLoopMode, NSModalPanelRunLoopMode, @"AWTRunLoopMode", nil];
		MakeModalWindow *pMakeModalWindow = [MakeModalWindow createWithCWindow:pCWindow];
		[pMakeModalWindow performSelectorOnMainThread:@selector(makeModalWindow:) withObject:pMakeModalWindow waitUntilDone:NO modes:pModes];
	}

	[pPool release];
}

void CWindow_updateLocation( id pCWindow )
{
	NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

	if ( pCWindow )
	{
		UpdateLocation *pUpdateLocation = [UpdateLocation createWithCWindow:pCWindow];
		[pUpdateLocation performSelectorOnMainThread:@selector(updateLocation:) withObject:pUpdateLocation waitUntilDone:NO];
	}

	[pPool release];
}
