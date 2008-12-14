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
 *  Patrick Luby, August 2006
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

#import <Cocoa/Cocoa.h>
#import <premac.h>
#import <vcl/keycod.hxx>
#import <com/sun/star/awt/Key.hdl>
#import <postmac.h>
#import "VCLResponder_cocoa.h"

using namespace ::com::sun::star::awt;

@implementation VCLResponder

- (void)clearLastText
{
	mnLastCommandKey = 0;
	mnLastModifiers = 0;

	if ( mpLastText )
	{
		[mpLastText release];
		mpLastText = nil;
	}
}

- (void)dealloc
{
	[self clearLastText];

	[super dealloc];
}

- (void)cancelOperation:(id)pSender
{
	// Fix bugs 2125 and 2167 by not overriding Java's handling of the cancel
	// action
	mnLastCommandKey = 0;
}

- (void)deleteBackward:(id)pSender
{
	mnLastCommandKey = KEY_BACKSPACE;
}

- (void)deleteBackwardByDecomposingPreviousCharacter:(id)pSender
{
	mnLastCommandKey = KEY_BACKSPACE;
}

- (void)deleteForward:(id)pSender
{
	mnLastCommandKey = KEY_DELETE;
}

- (void)deleteToBeginningOfLine:(id)pSender
{
	mnLastCommandKey = Key::DELETE_TO_BEGIN_OF_LINE;
}

- (void)deleteToBeginningOfParagraph:(id)pSender
{
	mnLastCommandKey = Key::DELETE_TO_BEGIN_OF_PARAGRAPH;
}

- (void)deleteToEndOfLine:(id)pSender
{
	mnLastCommandKey = Key::DELETE_TO_END_OF_LINE;
}

- (void)deleteToEndOfParagraph:(id)pSender
{
	mnLastCommandKey = Key::DELETE_TO_END_OF_PARAGRAPH;
}

- (void)deleteWordBackward:(id)pSender
{
	mnLastCommandKey = Key::DELETE_WORD_BACKWARD;
}

- (void)deleteWordForward:(id)pSender
{
	mnLastCommandKey = Key::DELETE_WORD_FORWARD;
}

- (void)doCommandBySelector:(SEL)aSelector
{
	// Do not invoke the superclass as it can trigger beeping
	if ( [self respondsToSelector:aSelector] )
		[self performSelector:aSelector withObject:nil];
}

- (BOOL)ignoreTrackpadGestures
{
	BOOL bIgnoreTrackpadGestures = NO;

	CFPropertyListRef aPref = CFPreferencesCopyAppValue( CFSTR( "IgnoreTrackpadGestures" ), kCFPreferencesCurrentApplication );
	if( aPref )
	{
		if ( CFGetTypeID( aPref ) == CFBooleanGetTypeID() && (CFBooleanRef)aPref == kCFBooleanTrue )
			bIgnoreTrackpadGestures = YES;
		CFRelease( aPref );
	}

	return bIgnoreTrackpadGestures;
}

- (id)init
{
	[super init];

	mnLastCommandKey = 0;
	mnLastModifiers = 0;
	mpLastText = nil;

	return self;
}

- (void)insertBacktab:(id)pSender
{
	mnLastCommandKey = KEY_TAB;
	mnLastModifiers = KEY_SHIFT;
}

- (void)insertLineBreak:(id)pSender
{
	mnLastCommandKey = Key::INSERT_LINEBREAK;
}

- (void)insertNewline:(id)pSender
{
	mnLastCommandKey = KEY_RETURN;
	mnLastModifiers = KEY_SHIFT;
}

- (void)insertParagraphSeparator:(id)pSender
{
	mnLastCommandKey = Key::INSERT_PARAGRAPH;
}

- (void)insertTab:(id)pSender
{
	mnLastCommandKey = KEY_TAB;
}

- (void)insertText:(NSString *)pString
{
	[self clearLastText];

	mpLastText = pString;
	if ( mpLastText )
		[mpLastText retain];
}

- (void)interpretKeyEvents:(NSArray *)pEvents
{
	[self clearLastText];

	[super interpretKeyEvents:pEvents];
}

- (short)lastCommandKey
{
	return mnLastCommandKey;
}

- (short)lastModifiers
{
	return mnLastModifiers;
}

- (NSString *)lastText
{
	return mpLastText;
}

- (void)moveBackwardAndModifySelection:(id)pSender
{
	mnLastCommandKey = Key::SELECT_BACKWARD;
}

- (void)moveDown:(id)pSender
{
	mnLastCommandKey = KEY_DOWN;
}

- (void)moveForwardAndModifySelection:(id)pSender
{
	mnLastCommandKey = Key::SELECT_FORWARD;
}

- (void)moveLeft:(id)pSender
{
	mnLastCommandKey = KEY_LEFT;
}

- (void)moveLeftAndModifySelection:(id)pSender
{
	mnLastCommandKey = KEY_LEFT;
	mnLastModifiers = KEY_SHIFT;
}

- (void)moveRight:(id)pSender
{
	mnLastCommandKey = KEY_RIGHT;
}

- (void)moveRightAndModifySelection:(id)pSender
{
	mnLastCommandKey = KEY_RIGHT;
	mnLastModifiers = KEY_SHIFT;
}

- (void)moveToBeginningOfLine:(id)pSender
{
	mnLastCommandKey = Key::MOVE_TO_BEGIN_OF_PARAGRAPH;
}

- (void)moveToBeginningOfParagraph:(id)pSender
{
	mnLastCommandKey = Key::MOVE_TO_BEGIN_OF_PARAGRAPH;
}

- (void)moveToEndOfLine:(id)pSender
{
	mnLastCommandKey = Key::MOVE_TO_END_OF_LINE;
}

- (void)moveToEndOfParagraph:(id)pSender
{
	mnLastCommandKey = Key::MOVE_TO_END_OF_PARAGRAPH;
}

- (void)moveUp:(id)pSender
{
	mnLastCommandKey = KEY_UP;
}

- (void)moveWordBackward:(id)pSender
{
	mnLastCommandKey = Key::MOVE_WORD_BACKWARD;
}

- (void)moveWordBackwardAndModifySelection:(id)pSender
{
	mnLastCommandKey = Key::SELECT_WORD_BACKWARD;
}

- (void)moveWordForward:(id)pSender
{
	mnLastCommandKey = Key::MOVE_WORD_FORWARD;
}

- (void)moveWordForwardAndModifySelection:(id)pSender
{
	mnLastCommandKey = Key::SELECT_WORD_FORWARD;
}

- (void)moveWordLeft:(id)pSender
{
	mnLastCommandKey = Key::MOVE_WORD_BACKWARD;
}

- (void)moveWordRight:(id)pSender
{
	mnLastCommandKey = Key::MOVE_WORD_FORWARD;
}

- (void)pageDown:(id)pSender
{
	mnLastCommandKey = KEY_PAGEDOWN;
}

- (void)pageUp:(id)pSender
{
	mnLastCommandKey = KEY_PAGEUP;
}

- (void)selectAll:(id)pSender
{
	mnLastCommandKey = Key::SELECT_ALL;
}

- (void)selectLine:(id)pSender
{
	mnLastCommandKey = Key::SELECT_LINE;
}

- (void)selectParagraph:(id)pSender
{
	mnLastCommandKey = Key::SELECT_PARAGRAPH;
}

- (void)selectWord:(id)pSender
{
	mnLastCommandKey = Key::SELECT_WORD;
}

@end
