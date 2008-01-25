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
 *  Patrick Luby, May 2006
 *
 *  GNU General Public License Version 2.1
 *  =============================================
 *  Copyright 2006 Planamesa Inc.
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

#import "quicktimecommon.h"
#import "quicktimewindow.hxx"

#ifndef _COM_SUN_STAR_AWT_KEYMODIFIER_HDL_
#include <com/sun/star/awt/KeyModifier.hpp>
#endif
#ifndef _COM_SUN_STAR_AWT_MOUSEBUTTON_HDL_
#include <com/sun/star/awt/MouseButton.hpp>
#endif

static const short nAVMediaMinDB = -40;
static const short nAVMediaMaxDB = 0;

using namespace ::avmedia::quicktime;
using namespace ::com::sun::star::awt;

static void HandleAndFireMouseEvent( NSEvent *pEvent, AvmediaMovieView *pView, AvmediaMoviePlayer *pMoviePlayer )
{
	// Only process the event if both the event and the view are visible
	if ( pEvent && pView && pMoviePlayer && [pEvent window] && [pView window] && [[pView window] isVisible] )
	{
		MouseEvent aEvt;

		NSPoint aPoint = [pView convertPoint:[pEvent locationInWindow] fromView:nil];
		aEvt.Modifiers = 0;
		aEvt.Buttons = 0;
		aEvt.X = (sal_Int32)aPoint.x;
		aEvt.Y = (sal_Int32)aPoint.y;
		aEvt.ClickCount = 1;
		aEvt.PopupTrigger = sal_False;

		// Set modifiers. Note that we only care about the Shift and Command
		// modifiers like the Windows code.
		unsigned int nKeyModifiers = [pEvent modifierFlags];
		if ( nKeyModifiers & NSShiftKeyMask )
			aEvt.Modifiers |= KeyModifier::SHIFT;
		if ( nKeyModifiers & NSCommandKeyMask )
			aEvt.Modifiers |= KeyModifier::MOD1;

		// Set buttons
		switch ( [pEvent type] )
		{
			case NSLeftMouseDown:
				aEvt.Buttons = MouseButton::LEFT;
				Window::fireMousePressedEvent( pMoviePlayer, aEvt );
				break;
			case NSRightMouseDown:
				aEvt.Buttons = MouseButton::RIGHT;
				Window::fireMousePressedEvent( pMoviePlayer, aEvt );
				break;
			case NSOtherMouseDown:
				aEvt.Buttons = MouseButton::MIDDLE;
				Window::fireMousePressedEvent( pMoviePlayer, aEvt );
				break;
			case NSLeftMouseDragged:
				aEvt.Buttons = MouseButton::LEFT;
				Window::fireMouseMovedEvent( pMoviePlayer, aEvt );
				break;
			case NSRightMouseDragged:
				aEvt.Buttons = MouseButton::RIGHT;
				Window::fireMouseMovedEvent( pMoviePlayer, aEvt );
				break;
			case NSOtherMouseDragged:
				aEvt.Buttons = MouseButton::MIDDLE;
				Window::fireMouseMovedEvent( pMoviePlayer, aEvt );
				break;
			case NSLeftMouseUp:
				aEvt.Buttons = MouseButton::LEFT;
				Window::fireMouseReleasedEvent( pMoviePlayer, aEvt );
				break;
			case NSRightMouseUp:
				aEvt.Buttons = MouseButton::RIGHT;
				Window::fireMouseReleasedEvent( pMoviePlayer, aEvt );
				break;
			case NSOtherMouseUp:
				aEvt.Buttons = MouseButton::MIDDLE;
				Window::fireMouseReleasedEvent( pMoviePlayer, aEvt );
				break;
			default:
				aEvt.ClickCount = 0;
				Window::fireMouseMovedEvent( pMoviePlayer, aEvt );
				break;
		}
	}
}

@implementation AvmediaArgs

+ (id)argsWithArgs:(NSArray *)pArgs
{
	AvmediaArgs *pRet = [[AvmediaArgs alloc] initWithArgs:pArgs];
	if ( pRet )
		[pRet autorelease];

	return pRet;
}

- (NSArray *)args
{
	return mpArgs;
}

- (void)dealloc
{
	if ( mpArgs )
		[mpArgs release];

	if ( mpResult )
		[mpResult release];

	[super dealloc];
}

- (id)initWithArgs:(NSArray *)pArgs
{
	[super init];

	mpResult = nil;
	mpArgs = pArgs;
	if ( mpArgs )
		[mpArgs retain];

	return self;
}

- (NSObject *)result
{
	return mpResult;
}

- (void)setResult:(NSObject *)pResult
{
	if ( mpResult )
		[mpResult release];

	mpResult = pResult;

	if ( mpResult )
		[mpResult retain];
}

@end

@implementation AvmediaMoviePlayer

- (void)bounds:(AvmediaArgs *)pArgs
{
	if ( pArgs )
		[pArgs setResult:[NSValue valueWithRect:[mpMovieView frame]]];
}

- (double)currentTime:(AvmediaArgs *)pArgs
{
	double fRet = 0;

	NSTimeInterval aInterval;
	if ( QTGetTimeInterval( [mpMovie currentTime], &aInterval ) )
		fRet = aInterval;

	if ( pArgs )
		[pArgs setResult:[NSNumber numberWithDouble:fRet]];

	return fRet;
}

- (void)dealloc
{
	if ( mpMovie )
		[mpMovie stop];

	if ( mpMovieView )
	{
		[mpMovieView removeFromSuperview];
		[mpMovieView setMoviePlayer:nil];
		[mpMovieView setMovie:nil];
		[mpMovieView release];
	}

	if ( mpMovie )
		[mpMovie release];

	if ( mpSuperview )
		[mpSuperview release];

	[super dealloc];
}

- (double)duration:(AvmediaArgs *)pArgs
{
	double fRet = 0;

	NSTimeInterval aInterval;
	if ( QTGetTimeInterval( [mpMovie duration], &aInterval ) )
		fRet = aInterval;

	if ( pArgs )
		[pArgs setResult:[NSNumber numberWithDouble:fRet]];

	return fRet;
}

- (id)init
{
	[super init];

	mpMovie = nil;
	mpMovieView = nil;
	mbPlaying = NO;
	maPreferredSize = NSMakeSize( 0, 0 );

	return self;
}

- (void)initialize:(NSURL *)pURL
{
	NSError *pError = nil;
	mpMovie = [QTMovie movieWithURL:pURL error:&pError];
	if ( mpMovie && !pError )
	{
		[mpMovie retain];
		[mpMovie setAttribute:[NSNumber numberWithBool:NO] forKey:QTMovieLoopsAttribute];
		[mpMovie setSelection:QTMakeTimeRange( QTMakeTimeWithTimeInterval( 0 ), [mpMovie duration] )];

		NSImage *pImage = [mpMovie frameImageAtTime:QTMakeTimeWithTimeInterval( 0 )];
		if ( pImage )
			maPreferredSize = [pImage size];
		else
			maPreferredSize = NSMakeSize( 1, 1 );

		NSRect aFrame = NSMakeRect( 0, 0, maPreferredSize.width, maPreferredSize.height );
		mpMovieView = [[AvmediaMovieView alloc] initWithFrame:aFrame];
		[mpMovieView setMoviePlayer:self];
		[mpMovieView setFillColor:[NSColor clearColor]];
		[mpMovieView setControllerVisible:NO];
		[mpMovieView setPreservesAspectRatio:YES];
		[mpMovieView setShowsResizeIndicator:NO];
		[mpMovieView setMovie:mpMovie];
		[mpMovieView setEditable:YES];
	}
}

- (QTMovie *)movie
{
	return mpMovie;
}

- (QTMovieView *)movieView
{
	return mpMovieView;
}

- (MacOSBOOL)isPlaying:(AvmediaArgs *)pArgs
{
	// Check if we are at the end of the movie
	if ( mbPlaying )
	{
		NSTimeInterval aCurrentInterval;
		NSTimeInterval aDurationInterval;
		if ( QTGetTimeInterval( [mpMovie currentTime], &aCurrentInterval ) && QTGetTimeInterval( [mpMovie duration], &aDurationInterval ) && aCurrentInterval >= aDurationInterval )
		{
			NSNumber *pLooping = [mpMovie attributeForKey:QTMovieLoopsAttribute];
			if ( pLooping && ![pLooping boolValue] )
				mbPlaying = NO;
		}
	}

	if ( mbPlaying )
		[mpMovie play];
	else
		[mpMovie stop];

	if ( pArgs )
		[pArgs setResult:[NSNumber numberWithBool:mbPlaying]];

	return mbPlaying;
}

- (double)rate:(AvmediaArgs *)pArgs
{
	double fRet = (double)[mpMovie rate];

	if ( pArgs )
		[pArgs setResult:[NSNumber numberWithDouble:fRet]];

	return fRet;
}

- (void)release:(id)pObject
{
	[mpMovie stop];
	mbPlaying = NO;

	[self release];
}

- (MacOSBOOL)mute:(AvmediaArgs *)pArgs
{
	MacOSBOOL bRet = [mpMovie muted];

	if ( pArgs )
		[pArgs setResult:[NSNumber numberWithBool:bRet]];

	return bRet;
}

- (void)play:(id)pObject
{
	[mpMovie play];
	mbPlaying = YES;
}

- (void)preferredSize:(AvmediaArgs *)pArgs
{
	if ( pArgs )
		[pArgs setResult:[NSValue valueWithSize:maPreferredSize]];
}

- (double)selectionEnd:(AvmediaArgs *)pArgs
{
	double fRet = 0;

	NSTimeInterval aInterval;
	if ( QTGetTimeInterval( [mpMovie selectionEnd], &aInterval ) )
	{
		fRet = aInterval;
	}
	else if ( QTGetTimeInterval( [mpMovie duration], &aInterval ) )
	{
		fRet = aInterval;
	}

	if ( pArgs )
		[pArgs setResult:[NSNumber numberWithDouble:fRet]];

	return fRet;
}

- (void)setBounds:(AvmediaArgs *)pArgs
{
	NSArray *pArgArray = [pArgs args];
	if ( !pArgArray || [pArgArray count] < 2 )
		return;

	NSView *pSuperview = (NSView *)[pArgArray objectAtIndex:0];
	if ( !pSuperview )
		return;

	NSValue *pRect = (NSValue *)[pArgArray objectAtIndex:1];
	if ( !pRect )
		return;

	NSRect aNewFrame = [pRect rectValue];
	if ( aNewFrame.size.width < 1 )
		aNewFrame.size.width = 1;
	if ( aNewFrame.size.height < 1 )
		aNewFrame.size.height = 1;

	if ( NSEqualRects( aNewFrame, [mpMovieView frame] ) )
		return;

	if ( mbPlaying )
		[mpMovie stop];

	// No need to flip coordinates as the JavaSalObject view is already flipped
	// like our view
	[mpMovieView setFrame:aNewFrame];

	if ( mbPlaying )
		[mpMovie play];
}

- (void)setCurrentTime:(AvmediaArgs *)pArgs
{
	NSArray *pArgArray = [pArgs args];
	if ( !pArgArray || [pArgArray count] < 1 )
		return;

	NSNumber *pTime = (NSNumber *)[pArgArray objectAtIndex:0];
	if ( !pTime )
		return;

	if ( mbPlaying )
		[mpMovie stop];

	[mpMovie setCurrentTime:QTMakeTimeWithTimeInterval( [pTime doubleValue] )];

	if ( mbPlaying )
		[mpMovie play];
}

- (void)setFocus:(id)pObject
{
	NSWindow *pWindow = [mpMovieView window];
	if ( pWindow )
		[pWindow makeFirstResponder:mpMovieView];
}

- (void)setLooping:(AvmediaArgs *)pArgs
{
	NSArray *pArgArray = [pArgs args];
	if ( !pArgArray || [pArgArray count] < 1 )
		return;

	NSNumber *pLooping = (NSNumber *)[pArgArray objectAtIndex:0];
	if ( !pLooping )
		return;

	[mpMovie setAttribute:pLooping forKey:QTMovieLoopsAttribute];
}

- (void)setMute:(AvmediaArgs *)pArgs
{
	NSArray *pArgArray = [pArgs args];
	if ( !pArgArray || [pArgArray count] < 1 )
		return;

	NSNumber *pMute = (NSNumber *)[pArgArray objectAtIndex:0];
	if ( !pMute )
		return;

	[mpMovie setMuted:[pMute boolValue]];
}

- (void)setRate:(AvmediaArgs *)pArgs
{
	NSArray *pArgArray = [pArgs args];
	if ( !pArgArray || [pArgArray count] < 1 )
		return;

	NSNumber *pTime = (NSNumber *)[pArgArray objectAtIndex:0];
	if ( !pTime )
		return;

	if ( mbPlaying )
		[mpMovie stop];

	[mpMovie setRate:[pTime floatValue]];

	if ( mbPlaying )
		[mpMovie play];
}

- (void)setSelection:(AvmediaArgs *)pArgs
{
	NSArray *pArgArray = [pArgs args];
	if ( !pArgArray || [pArgArray count] < 1 )
		return;

	NSNumber *pTime = (NSNumber *)[pArgArray objectAtIndex:0];
	if ( !pTime )
		return;

	if ( mbPlaying )
		[mpMovie stop];

	[mpMovie setSelection:QTMakeTimeRange( QTMakeTimeWithTimeInterval( 0 ), QTMakeTimeWithTimeInterval( [pTime doubleValue] ) )];

	if ( mbPlaying )
		[mpMovie play];
}

- (void)setSuperview:(AvmediaArgs *)pArgs
{
	NSArray *pArgArray = [pArgs args];
	if ( !pArgArray || [pArgArray count] < 2 )
	{
		if ( mbPlaying )
			[mpMovie stop];

		[mpMovieView removeFromSuperview];
		if ( mpSuperview )
		{
			[mpSuperview release];
			mpSuperview = nil;
		}

		if ( mbPlaying )
			[mpMovie play];

		return;
	}

	NSView *pSuperview = (NSView *)[pArgArray objectAtIndex:0];
	if ( !pSuperview )
	{
		if ( mbPlaying )
			[mpMovie stop];

		[mpMovieView removeFromSuperview];
		if ( mpSuperview )
		{
			[mpSuperview release];
			mpSuperview = nil;
		}

		if ( mbPlaying )
			[mpMovie play];

		return;
	}

	NSValue *pRect = (NSValue *)[pArgArray objectAtIndex:1];
	if ( !pRect )
		return;

	NSRect aNewFrame = [pRect rectValue];
	if ( aNewFrame.size.width < 1 )
		aNewFrame.size.width = 1;
	if ( aNewFrame.size.height < 1 )
		aNewFrame.size.height = 1;

	if ( mbPlaying )
		[mpMovie stop];

	// No need to flip coordinates as the JavaSalObject view is already flipped
	// like our view
	[mpMovieView setFrame:aNewFrame];

	[mpMovieView removeFromSuperview];
	if ( mpSuperview )
	{
		[mpSuperview release];
		mpSuperview = nil;
	}

	mpSuperview = pSuperview;
	[mpSuperview retain];

	[mpSuperview addSubview:mpMovieView positioned:NSWindowAbove relativeTo:nil];

	if ( mbPlaying )
		[mpMovie play];
}

- (void)setVolumeDB:(AvmediaArgs *)pArgs
{
	NSArray *pArgArray = [pArgs args];
	if ( !pArgArray || [pArgArray count] < 1 )
		return;

	NSNumber *pDB = (NSNumber *)[pArgArray objectAtIndex:0];
	if ( !pDB )
		return;

	[mpMovie setVolume:( (float)( [pDB shortValue] - nAVMediaMinDB ) / (float)( nAVMediaMaxDB - nAVMediaMinDB ) )];
}

- (void)stop:(id)pObject
{
	[mpMovie stop];
	mbPlaying = NO;
}

- (short)volumeDB:(AvmediaArgs *)pArgs
{
	short nRet = (short)( [mpMovie volume] * ( nAVMediaMaxDB - nAVMediaMinDB ) ) + nAVMediaMinDB;

	if ( pArgs )
		[pArgs setResult:[NSNumber numberWithShort:nRet]];

	return nRet;
}

@end

@implementation AvmediaMovieView

- (MacOSBOOL)becomeFirstResponder
{
	MacOSBOOL bRet = [super becomeFirstResponder];

	// Only process the event if both the event and the view are visible
	if ( bRet && mpMoviePlayer && [self window] && [[self window] isVisible] )
	{
		FocusEvent aEvt;
		Window::fireFocusGainedEvent( mpMoviePlayer, aEvt );
	}

	return bRet;
}

- (IBAction)copy:(id)pSender
{
	if ( mpMoviePlayer )
		[mpMoviePlayer stop:nil];

	[super copy:pSender];
}

- (IBAction)cut:(id)pSender
{
	if ( mpMoviePlayer )
		[mpMoviePlayer stop:nil];

	[super cut:pSender];
}

- (void)dealloc
{
	[self setMoviePlayer:nil];

	[super dealloc];
}

- (id)initWithFrame:(NSRect)aFrame
{
	[super initWithFrame:aFrame];

	mpMoviePlayer = nil;

	return self;
}

- (MacOSBOOL)isFlipped
{
	return YES;
}

- (void)mouseDown:(NSEvent *)pEvent
{
	HandleAndFireMouseEvent( pEvent, self, mpMoviePlayer );
}

- (void)mouseDragged:(NSEvent *)pEvent
{
	HandleAndFireMouseEvent( pEvent, self, mpMoviePlayer );
}

- (void)mouseEntered:(NSEvent *)pEvent
{
	HandleAndFireMouseEvent( pEvent, self, mpMoviePlayer );
}

- (void)mouseExited:(NSEvent *)pEvent
{
	HandleAndFireMouseEvent( pEvent, self, mpMoviePlayer );
}

- (void)mouseMoved:(NSEvent *)pEvent
{
	HandleAndFireMouseEvent( pEvent, self, mpMoviePlayer );
}

- (void)mouseUp:(NSEvent *)pEvent
{
	HandleAndFireMouseEvent( pEvent, self, mpMoviePlayer );
}

- (IBAction)pause:(id)pSender
{
	if ( mpMoviePlayer )
		[mpMoviePlayer stop:nil];
}

- (IBAction)play:(id)pSender
{
	if ( mpMoviePlayer )
		[mpMoviePlayer play:nil];
}

- (void)rightMouseDown:(NSEvent *)pEvent
{
	HandleAndFireMouseEvent( pEvent, self, mpMoviePlayer );
}

- (void)rightMouseDragged:(NSEvent *)pEvent
{
	HandleAndFireMouseEvent( pEvent, self, mpMoviePlayer );
}

- (void)rightMouseUp:(NSEvent *)pEvent
{
	HandleAndFireMouseEvent( pEvent, self, mpMoviePlayer );
}

- (void)otherMouseDown:(NSEvent *)pEvent
{
	HandleAndFireMouseEvent( pEvent, self, mpMoviePlayer );
}

- (void)otherMouseDragged:(NSEvent *)pEvent
{
	HandleAndFireMouseEvent( pEvent, self, mpMoviePlayer );
}

- (void)otherMouseUp:(NSEvent *)pEvent
{
	HandleAndFireMouseEvent( pEvent, self, mpMoviePlayer );
}

- (IBAction)paste:(id)pSender
{
	if ( mpMoviePlayer )
		[mpMoviePlayer stop:nil];

	[super paste:pSender];
}

- (void)setMoviePlayer:(AvmediaMoviePlayer *)pPlayer
{
	if ( mpMoviePlayer )
		[mpMoviePlayer release];

	mpMoviePlayer = pPlayer;

	if ( mpMoviePlayer )
		[mpMoviePlayer retain];
}

@end
