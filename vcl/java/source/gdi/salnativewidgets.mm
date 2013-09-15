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
 *  Edward Peterlin, April 2006
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

#define _SV_SALNATIVEWIDGETS_CXX

#include <dlfcn.h>

#include <salgdi.h>
#include <saldata.hxx>
#include <salbmp.h>
#include <salframe.h>
#include <salinst.h>
#include <svsys.h>
#include <rtl/ustring.h>
#include <osl/module.h>
#include <vcl/decoview.hxx>
#include <vcl/svapp.hxx>
#include <vcl/window.hxx>

#include <premac.h>
#include <ApplicationServices/ApplicationServices.h>
// Need to include for HITheme constants but we don't link to it
#import <Carbon/Carbon.h>
#import <Cocoa/Cocoa.h>
#include <postmac.h>

// Comment out the following line to disable native controls
#define USE_NATIVE_CONTROLS

// Comment out the following line to disable native frame
#define USE_NATIVE_CTRL_FRAME

#define COMBOBOX_BUTTON_WIDTH			( IsRunningSnowLeopard() ? 17 : 19 )
#define COMBOBOX_HEIGHT					28
#define COMBOBOX_HEIGHT_SLOP			( IsRunningSnowLeopard() ? 1 : -0.5f )
#define CONTROL_TAB_PANE_TOP_OFFSET		12
// Fix bug 3378 by reducing the editbox height for low screen resolutions
#define EDITBOX_HEIGHT					( 24 * Application::GetSettings().GetStyleSettings().GetToolFont().GetHeight() / 10 )
#define EDITFRAMEPADDING_WIDTH			1
#define FOCUSRING_WIDTH					3
#define FRAME_TRIMWIDTH					1
#define LISTBOX_BUTTON_WIDTH			( IsRunningSnowLeopard() ? 21 : 19 )
#define SCROLLBAR_SUPPRESS_ARROWS		( IsRunningSnowLeopard() ? false : true )
#define SPINNER_TRIMWIDTH				3
#define SPINNER_TRIMHEIGHT				1
#define SPINNER_WIDTH_SLOP				1
#define PROGRESSBAR_HEIGHT_SLOP			( IsRunningSnowLeopard() ? 1 : 0 )
#define PROGRESSBARPADDING_HEIGHT		1
#define TABITEM_HEIGHT_SLOP				4
// Fix most cases of checkbox and radio button clipping reported in the
// following NeoOffice forum post by setting their width and height to the
// minimum amount that will not result in a clipped focus ring when drawn:
// http://trinity.neooffice.org/modules.php?name=Forums&file=viewtopic&p=64288#64288
#define CHECKBOX_WIDTH					14
#define CHECKBOX_HEIGHT					14
#define RADIOBUTTON_WIDTH				14
#define RADIOBUTTON_HEIGHT				14
#define PUSHBUTTON_HEIGHT_SLOP			1
#define DISCLOSUREBTN_WIDTH_SLOP		-2

using namespace osl;
using namespace rtl;
using namespace vcl;

struct SAL_DLLPRIVATE VCLBitmapBuffer : BitmapBuffer
{
	CGContextRef			maContext;
	MutexGuard*				mpGraphicsMutexGuard;
	HIThemeOrientation		mnHIThemeOrientationFlags;
	bool					mbLastDrawToPrintGraphics;
	bool					mbUseLayer;

							VCLBitmapBuffer();
	virtual					~VCLBitmapBuffer();

	BOOL					Create( long nX, long nY, long nWidth, long nHeight, JavaSalGraphics *pGraphics, bool bUseLayer = true );
	void					Destroy();
	void					DrawContextAndDestroy( JavaSalGraphics *pGraphics, CGRect aSrcRect, CGRect aDestRect );
	void					ReleaseContext();
};

typedef OSErr Gestalt_Type( OSType selector, long *response );
typedef OSStatus GetThemeMetric_Type( ThemeMetric nMetric, SInt32 *pMetric);
typedef OSStatus HIThemeDrawButton_Type( const HIRect *pBounds, const HIThemeButtonDrawInfo *pDrawInfo, CGContextRef aContext, HIThemeOrientation nOrientation, HIRect *pLabelRect);
typedef OSStatus HIThemeDrawFrame_Type( const HIRect *pRect, const HIThemeFrameDrawInfo *pDrawInfo, CGContextRef aContext, HIThemeOrientation nOrientation);
typedef OSStatus HIThemeDrawMenuBackground_Type( const HIRect *pMenuRect, const HIThemeMenuDrawInfo *pMenuDrawInfo, CGContextRef aContext, HIThemeOrientation nOrientation);
typedef OSStatus HIThemeDrawSeparator_Type( const HIRect *pRect, const HIThemeSeparatorDrawInfo *pDrawInfo, CGContextRef aContext, HIThemeOrientation nOrientation);
typedef OSStatus HIThemeDrawTab_Type( const HIRect *pRect, const HIThemeTabDrawInfo *pDrawInfo, CGContextRef aContext, HIThemeOrientation nOrientation, HIRect *pLabelRect);
typedef OSStatus HIThemeDrawTabPane_Type( const HIRect *pRect, const HIThemeTabPaneDrawInfo *pDrawInfo, CGContextRef aContext, HIThemeOrientation nOrientation);
typedef OSStatus HIThemeGetGrowBoxBounds_Type( const HIPoint *pOrigin, const HIThemeGrowBoxDrawInfo *pDrawInfo, HIRect *pBounds);
typedef OSStatus HIThemeGetTabShape_Type( const HIRect *pRect, const HIThemeTabDrawInfo *pDrawInfo, HIShapeRef *pShape);

static bool bIsRunningSnowLeopardInitizalized  = false;
static bool bIsRunningSnowLeopard = false;
static bool bHIThemeInitialized = false;
static GetThemeMetric_Type *pGetThemeMetric = NULL;
static HIThemeDrawButton_Type *pHIThemeDrawButton = NULL;
static HIThemeDrawFrame_Type *pHIThemeDrawFrame = NULL;
static HIThemeDrawMenuBackground_Type *pHIThemeDrawMenuBackground = NULL;
static HIThemeDrawSeparator_Type *pHIThemeDrawSeparator = NULL;
static HIThemeDrawTab_Type *pHIThemeDrawTab = NULL;
static HIThemeDrawTabPane_Type *pHIThemeDrawTabPane = NULL;
static HIThemeGetGrowBoxBounds_Type *pHIThemeGetGrowBoxBounds = NULL;
static HIThemeGetTabShape_Type *pHIThemeGetTabShape = NULL;

static VCLBitmapBuffer aSharedComboBoxBuffer;
static VCLBitmapBuffer aSharedListBoxBuffer;
static VCLBitmapBuffer aSharedHorizontalScrollBarBuffer;
static VCLBitmapBuffer aSharedVerticalScrollBarBuffer;
static VCLBitmapBuffer aSharedScrollBarBuffer;
static VCLBitmapBuffer aSharedSpinboxBuffer;
static VCLBitmapBuffer aSharedSpinbuttonBuffer;
static VCLBitmapBuffer aSharedProgressbarBuffer;
static VCLBitmapBuffer aSharedTabBuffer;
static VCLBitmapBuffer aSharedTabBoundingBoxBuffer;
static VCLBitmapBuffer aSharedPrimaryGroupBoxBuffer;
static VCLBitmapBuffer aSharedMenuBackgroundBuffer;
static VCLBitmapBuffer aSharedEditBoxBuffer;
static VCLBitmapBuffer aSharedListViewFrameBuffer;
static VCLBitmapBuffer aSharedDisclosureBtnBuffer;
static VCLBitmapBuffer aSharedSeparatorLineBuffer;
static VCLBitmapBuffer aSharedListViewHeaderBuffer;
static VCLBitmapBuffer aSharedBevelButtonBuffer;
static VCLBitmapBuffer aSharedCheckboxBuffer;

inline long Float32ToLong( Float32 f ) { return (long)( f + 0.5 ); }

// =======================================================================

static bool IsRunningSnowLeopard()
{
	if ( !bIsRunningSnowLeopardInitizalized )
	{
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
					if ( res == 6 )
						bIsRunningSnowLeopard = true;
				}
			}

			dlclose( pLib );
		}

		bIsRunningSnowLeopardInitizalized = true;
	}

	return bIsRunningSnowLeopard;
}

// =======================================================================

@interface VCLNativeControlWindow : NSWindow
{
	MacOSBOOL				mbInactive;
}
+ (id)createAndAttachToView:(NSView *)pView controlState:(ControlState)nControlState;
- (MacOSBOOL)_hasActiveControls;
- (id)initWithContentRect:(NSRect)aContentRect styleMask:(NSUInteger)nStyle backing:(NSBackingStoreType)nBufferingType defer:(MacOSBOOL)bDeferCreation;
- (id)initWithContentRect:(NSRect)aContentRect styleMask:(NSUInteger)nStyle backing:(NSBackingStoreType)nBufferingType defer:(MacOSBOOL)bDeferCreation screen:(NSScreen *)pScreen;
- (void)orderFrontRegardless;
- (void)orderWindow:(NSWindowOrderingMode)nOrderingMode relativeTo:(NSInteger)nOtherWindowNumber;
- (void)setInactive:(MacOSBOOL)bInactive;
@end

@implementation VCLNativeControlWindow

+ (id)createAndAttachToView:(NSView *)pView controlState:(ControlState)nControlState
{
	VCLNativeControlWindow *pRet = nil;

	if ( pView && ( ![pView isKindOfClass:[NSControl class]] || [(NSControl *)pView isEnabled] ) )
	{
		pRet = [[VCLNativeControlWindow alloc] initWithContentRect:[pView frame] styleMask:NSBorderlessWindowMask backing:NSBackingStoreBuffered defer:YES];
		if ( pRet )
		{
			[pRet autorelease];
			[pRet setReleasedWhenClosed:NO];
			[pRet setContentView:pView];
			[pRet setInactive:( nControlState & CTRL_STATE_INACTIVE ? YES : NO )];
		}
	}

	return pRet;
}

- (MacOSBOOL)_hasActiveControls
{
	MacOSBOOL bActive = !mbInactive;
	if ( bActive )
	{
		// If the application is not active, then force inactive state
		NSApplication *pApp = [NSApplication sharedApplication];
		if ( pApp && ![pApp isActive] )
			bActive = NO;
	}

	return bActive;
}

- (id)initWithContentRect:(NSRect)aContentRect styleMask:(NSUInteger)nStyle backing:(NSBackingStoreType)nBufferingType defer:(MacOSBOOL)bDeferCreation
{
	[super initWithContentRect:aContentRect styleMask:nStyle backing:nBufferingType defer:bDeferCreation];

	mbInactive = NO;

	return self;
}

- (id)initWithContentRect:(NSRect)aContentRect styleMask:(NSUInteger)nStyle backing:(NSBackingStoreType)nBufferingType defer:(MacOSBOOL)bDeferCreation screen:(NSScreen *)pScreen
{
	[super initWithContentRect:aContentRect styleMask:nStyle backing:nBufferingType defer:bDeferCreation screen:pScreen];

	mbInactive = NO;

	return self;
}

- (void)orderFrontRegardless
{
	[self orderWindow:NSWindowOut relativeTo:0];
}

- (void)orderWindow:(NSWindowOrderingMode)nOrderingMode relativeTo:(NSInteger)nOtherWindowNumber
{
	[super orderWindow:NSWindowOut relativeTo:0];
}

- (void)setInactive:(MacOSBOOL)bInactive
{
	mbInactive = bInactive;
}

@end

// =======================================================================

@interface VCLNativeButton : NSObject
{
	NSButtonType			mnButtonType;
	NSBezelStyle			mnBezelStyle;
	NSControlSize			mnControlSize;
	NSInteger				mnButtonState;
	ControlState			mnControlState;
	MacOSBOOL				mbDrawRTL;
	VCLBitmapBuffer*		mpBuffer;
	JavaSalGraphics*		mpGraphics;
	CGRect					maDestRect;
	MacOSBOOL				mbDrawn;
	MacOSBOOL				mbRedraw;
	NSSize					maSize;
}
+ (id)createWithButtonType:(NSButtonType)nButtonType bezelStyle:(NSBezelStyle)nBezelStyle controlSize:(NSControlSize)nControlSize buttonState:(NSInteger)nButtonState controlState:(ControlState)nControlState drawRTL:(MacOSBOOL)bDrawRTL bitmapBuffer:(VCLBitmapBuffer *)pBuffer graphics:(JavaSalGraphics *)pGraphics destRect:(CGRect)aDestRect;
- (NSButton *)button;
- (void)draw:(id)pObject;
- (MacOSBOOL)drawn;
- (void)getSize:(id)pObject;
- (id)initWithButtonType:(NSButtonType)nButtonType bezelStyle:(NSBezelStyle)nBezelStyle controlSize:(NSControlSize)nControlSize buttonState:(NSInteger)nButtonState controlState:(ControlState)nControlState drawRTL:(MacOSBOOL)bDrawRTL bitmapBuffer:(VCLBitmapBuffer *)pBuffer graphics:(JavaSalGraphics *)pGraphics destRect:(CGRect)aDestRect;
- (MacOSBOOL)redraw;
- (NSSize)size;
@end

@implementation VCLNativeButton

+ (id)createWithButtonType:(NSButtonType)nButtonType bezelStyle:(NSBezelStyle)nBezelStyle controlSize:(NSControlSize)nControlSize buttonState:(NSInteger)nButtonState controlState:(ControlState)nControlState drawRTL:(MacOSBOOL)bDrawRTL bitmapBuffer:(VCLBitmapBuffer *)pBuffer graphics:(JavaSalGraphics *)pGraphics destRect:(CGRect)aDestRect
{
	VCLNativeButton *pRet = [[VCLNativeButton alloc] initWithButtonType:nButtonType bezelStyle:nBezelStyle controlSize:nControlSize buttonState:nButtonState controlState:nControlState drawRTL:bDrawRTL bitmapBuffer:pBuffer graphics:pGraphics destRect:aDestRect];
	[pRet autorelease];
	return pRet;
}

- (NSButton *)button
{
	NSButton *pButton = [[NSButton alloc] initWithFrame:NSMakeRect( 0, 0, 1, 1 )];
	if ( !pButton )
		return nil;

	[pButton autorelease];

	NSCell *pCell = [pButton cell];
	if ( !pCell )
		return nil;

	[pButton setButtonType:mnButtonType];
	[pButton setBezelStyle:mnBezelStyle];
	[pButton setState:mnButtonState];
	[pButton setTitle:@""];
	[pCell setControlSize:mnControlSize];

	if ( mnControlState & ( CTRL_STATE_PRESSED | CTRL_STATE_SELECTED ) )
	{
		[pButton setEnabled:YES];
		[pButton highlight:YES];
	}
	else if ( mnControlState & CTRL_STATE_ENABLED )
	{
		[pButton setEnabled:YES];
		[pButton highlight:NO];
	}
	else
	{
		[pButton setEnabled:NO];
		[pButton highlight:NO];
	}

	if ( mnControlState & CTRL_STATE_FOCUSED )
		[pCell setShowsFirstResponder:YES];
	else
		[pCell setShowsFirstResponder:NO];

	// The enabled state is controlled by the [NSWindow _hasActiveControls]
	// selector so we need to attach a custom hidden window to draw enabled
	[VCLNativeControlWindow createAndAttachToView:pButton controlState:mnControlState];

	[pButton sizeToFit];

	return pButton;
}

- (void)draw:(id)pObject
{
	if ( !mbDrawn && mpBuffer && mpGraphics && !CGRectIsEmpty( maDestRect ) )
	{
		mbRedraw = NO;

		NSButton *pButton = [self button];
		if ( pButton )
		{
			NSCell *pCell = [pButton cell];
			if ( pCell )
			{
				float fCellHeight = [pCell cellSize].height;
				float fOffscreenHeight = maDestRect.size.height;
				MacOSBOOL bPlacard = NO;
				if ( mnButtonType == NSMomentaryLightButton )
				{
					fCellHeight -= ( FOCUSRING_WIDTH * 2 );
					if ( fCellHeight <= 0 )
						fCellHeight = maDestRect.size.height;
					fOffscreenHeight = ( maDestRect.size.height > fCellHeight ? maDestRect.size.height : fCellHeight );
					bPlacard = ( fOffscreenHeight * 1.5 >= maDestRect.size.width );
					if ( bPlacard )
					{
						fOffscreenHeight = maDestRect.size.height;
						[pButton setBezelStyle:NSShadowlessSquareBezelStyle];
					}
					else
					{
						// The default adornment hides the pressed state so set
						// the adornment to none if the button is pressed. Also,
						// push buttons should never have a focus ring so treat
						// them as default buttons.
						[pCell setShowsFirstResponder:NO];

						if ( mnControlState & ( CTRL_STATE_DEFAULT | CTRL_STATE_FOCUSED ) && ! ( mnControlState & ( CTRL_STATE_PRESSED | CTRL_STATE_SELECTED ) ) )
						{
							// Do not use VCLNativeButtonCell animation as
							// it sometimes draws darker than expected
							[pButton highlight:NO];
							mbRedraw = YES;
						}
					}
				}

				CGRect aAdjustedDestRect = CGRectMake( 0, 0, maDestRect.size.width, fOffscreenHeight );
				if ( mpBuffer->Create( (long)maDestRect.origin.x, (long)maDestRect.origin.y, (long)maDestRect.size.width, (long)fOffscreenHeight, mpGraphics, fOffscreenHeight == maDestRect.size.height ) )
				{
					CGContextSaveGState( mpBuffer->maContext );
					if ( [pButton isFlipped] )
					{
						CGContextTranslateCTM( mpBuffer->maContext, 0, aAdjustedDestRect.size.height );
						CGContextScaleCTM( mpBuffer->maContext, 1.0f, -1.0f );
					}

					if ( mbDrawRTL )
					{
						CGContextTranslateCTM( mpBuffer->maContext, aAdjustedDestRect.size.width, 0 );
						CGContextScaleCTM( mpBuffer->maContext, -1.0f, 1.0f );
					}

					CGContextBeginTransparencyLayerWithRect( mpBuffer->maContext, aAdjustedDestRect, NULL );

					NSGraphicsContext *pContext = [NSGraphicsContext graphicsContextWithGraphicsPort:mpBuffer->maContext flipped:YES];
					if ( pContext )
					{
						NSRect aDrawRect = NSRectFromCGRect( aAdjustedDestRect );
						if ( mnButtonType == NSMomentaryLightButton && !bPlacard )
						{
							// Fix bug 1633 by vertically centering button
							aDrawRect.origin.y += ( ( fOffscreenHeight - fCellHeight ) / 2 ) + PUSHBUTTON_HEIGHT_SLOP;
							aDrawRect.size.height = fCellHeight;
						}
						else if ( mnBezelStyle == NSDisclosureBezelStyle )
						{
							// Horizontally align disclosure button outward
							aDrawRect.origin.x += DISCLOSUREBTN_WIDTH_SLOP;
							aDrawRect.size.width = [pCell cellSize].width;
						}

						NSGraphicsContext *pOldContext = [NSGraphicsContext currentContext];
						[NSGraphicsContext setCurrentContext:pContext];
						[pCell drawWithFrame:aDrawRect inView:pButton];

						if ( mbRedraw )
						{
							// Emulate pulse by painting pressed button on top
							// of the default button with varying alpha
							float fAlpha = 0.15f;
							double fTime = CFAbsoluteTimeGetCurrent();
							fAlpha += fAlpha * sin( ( fTime - (long)fTime ) * 2 * M_PI );
							if ( IsRunningSnowLeopard() )
								fAlpha += 0.4f;
							else
								fAlpha += 0.5f;
							if ( fAlpha > 0 )
							{
								CGContextSetAlpha( mpBuffer->maContext, fAlpha > 1.0f ? 1.0f : fAlpha );
								CGContextBeginTransparencyLayerWithRect( mpBuffer->maContext, aAdjustedDestRect, NULL );

								[pButton highlight:YES];
								[pCell drawWithFrame:aDrawRect inView:pButton];

								CGContextEndTransparencyLayer( mpBuffer->maContext );
							}
						}

						[NSGraphicsContext setCurrentContext:pOldContext];

						mbDrawn = YES;
					}

					CGContextEndTransparencyLayer( mpBuffer->maContext );
					CGContextRestoreGState( mpBuffer->maContext );

					mpBuffer->ReleaseContext();

					if ( mbDrawn )
						mpBuffer->DrawContextAndDestroy( mpGraphics, aAdjustedDestRect, maDestRect );
				}
			}
		}
	}
}

- (MacOSBOOL)drawn
{
	return mbDrawn;
}

- (void)getSize:(id)pObject
{
	if ( NSEqualSizes( maSize, NSZeroSize ) )
	{
		NSButton *pButton = [self button];
		if ( pButton )
		{
			NSCell *pCell = [pButton cell];
			if ( pCell )
				maSize = [pCell cellSize];
		}
	}
}

- (id)initWithButtonType:(NSButtonType)nButtonType bezelStyle:(NSBezelStyle)nBezelStyle controlSize:(NSControlSize)nControlSize buttonState:(NSInteger)nButtonState controlState:(ControlState)nControlState drawRTL:(MacOSBOOL)bDrawRTL bitmapBuffer:(VCLBitmapBuffer *)pBuffer graphics:(JavaSalGraphics *)pGraphics destRect:(CGRect)aDestRect
{
	[super init];

	mnButtonType = nButtonType;
	mnBezelStyle = nBezelStyle;
	mnControlSize = nControlSize;
	mnButtonState = nButtonState;
	mnControlState = nControlState;
	mbDrawRTL = bDrawRTL;
	mpBuffer = pBuffer;
	mpGraphics = pGraphics;
	maDestRect = aDestRect;
	mbDrawn = NO;
	mbRedraw = NO;
	maSize = NSZeroSize;

	return self;
}

- (MacOSBOOL)redraw
{
	return mbRedraw;
}

- (NSSize)size
{
	return maSize;
}

@end

// =======================================================================

@interface VCLNativeComboBox : NSObject
{
	ControlState			mnControlState;
	MacOSBOOL				mbEditable;
	VCLBitmapBuffer*		mpBuffer;
	JavaSalGraphics*		mpGraphics;
	CGRect					maDestRect;
	MacOSBOOL				mbDrawn;
	NSSize					maSize;
}
+ (id)createWithControlState:(ControlState)nControlState editable:(MacOSBOOL)bEditable bitmapBuffer:(VCLBitmapBuffer *)pBuffer graphics:(JavaSalGraphics *)pGraphics destRect:(CGRect)aDestRect;
- (NSControl *)comboBox;
- (NSControl *)popUpButton;
- (void)draw:(id)pObject;
- (MacOSBOOL)drawn;
- (void)getSize:(id)pObject;
- (id)initWithControlState:(ControlState)nControlState editable:(MacOSBOOL)bEditable bitmapBuffer:(VCLBitmapBuffer *)pBuffer graphics:(JavaSalGraphics *)pGraphics destRect:(CGRect)aDestRect;
- (NSSize)size;
@end

@implementation VCLNativeComboBox

+ (id)createWithControlState:(ControlState)nControlState editable:(MacOSBOOL)bEditable bitmapBuffer:(VCLBitmapBuffer *)pBuffer graphics:(JavaSalGraphics *)pGraphics destRect:(CGRect)aDestRect
{
	VCLNativeComboBox *pRet = [[VCLNativeComboBox alloc] initWithControlState:nControlState editable:bEditable bitmapBuffer:pBuffer graphics:pGraphics destRect:aDestRect];
	[pRet autorelease];
	return pRet;
}

- (NSControl *)comboBox
{
	NSComboBox *pComboBox = [[NSComboBox alloc] initWithFrame:NSMakeRect( 0, 0, 1, 1 )];
	if ( !pComboBox )
		return nil;

	[pComboBox autorelease];

	NSCell *pCell = [pComboBox cell];
	if ( !pCell )
		return nil;

	NSButtonCell *pButtonCell = nil;
	@try
	{
		pButtonCell = [pCell valueForKey:@"_buttonCell"];
	}
	@catch ( NSException *pExc )
	{
	}

	if ( !pButtonCell || ![pButtonCell isKindOfClass:[NSButtonCell class]] )
		return nil;

	if ( mnControlState & ( CTRL_STATE_PRESSED | CTRL_STATE_SELECTED ) )
	{
		[pComboBox setEnabled:YES];
		[pButtonCell setHighlighted:YES];
	}
	else if ( mnControlState & CTRL_STATE_ENABLED )
	{
		[pComboBox setEnabled:YES];
		[pButtonCell setHighlighted:NO];
	}
	else
	{
		[pComboBox setEnabled:NO];
		[pButtonCell setHighlighted:NO];
	}

	if ( mnControlState & CTRL_STATE_FOCUSED )
		[pCell setShowsFirstResponder:YES];
	else
		[pCell setShowsFirstResponder:NO];

	// The enabled state is controlled by the [NSWindow _hasActiveControls]
	// selector so we need to attach a custom hidden window to draw enabled
	[VCLNativeControlWindow createAndAttachToView:pComboBox controlState:mnControlState];

	[pComboBox sizeToFit];

	return pComboBox;
}

- (NSControl *)popUpButton
{
	NSPopUpButton *pPopUpButton = [[NSPopUpButton alloc] initWithFrame:NSMakeRect( 0, 0, 1, 1 )];
	if ( !pPopUpButton )
		return nil;

	[pPopUpButton autorelease];

	NSCell *pCell = [pPopUpButton cell];
	if ( !pCell )
		return nil;

	[pPopUpButton setPullsDown:YES];

	if ( mnControlState & ( CTRL_STATE_PRESSED | CTRL_STATE_SELECTED ) )
	{
		[pPopUpButton setEnabled:YES];
		[pCell setHighlighted:YES];
	}
	else if ( mnControlState & CTRL_STATE_ENABLED )
	{
		[pPopUpButton setEnabled:YES];
		[pCell setHighlighted:NO];
	}
	else
	{
		[pPopUpButton setEnabled:NO];
		[pCell setHighlighted:NO];
	}

	if ( mnControlState & CTRL_STATE_FOCUSED )
		[pCell setShowsFirstResponder:YES];
	else
		[pCell setShowsFirstResponder:NO];

	// The enabled state is controlled by the [NSWindow _hasActiveControls]
	// selector so we need to attach a custom hidden window to draw enabled
	[VCLNativeControlWindow createAndAttachToView:pPopUpButton controlState:mnControlState];

	[pPopUpButton sizeToFit];

	return pPopUpButton;
}

- (void)draw:(id)pObject
{
	if ( !mbDrawn && mpBuffer && mpGraphics && !CGRectIsEmpty( maDestRect ) )
	{
		NSControl *pControl = ( mbEditable ? [self comboBox] : [self popUpButton] );
		if ( pControl )
		{
			NSCell *pCell = [pControl cell];
			if ( pCell )
			{
				float fCellHeight = [pCell cellSize].height - ( FOCUSRING_WIDTH * 2 );
				if ( fCellHeight <= 0 )
					fCellHeight = maDestRect.size.height;
				float fOffscreenHeight = ( maDestRect.size.height > fCellHeight ? maDestRect.size.height : fCellHeight );
				CGRect aAdjustedDestRect = CGRectMake( 0, 0, maDestRect.size.width, fOffscreenHeight );
				if ( mpBuffer->Create( (long)maDestRect.origin.x, (long)maDestRect.origin.y, (long)maDestRect.size.width, (long)fOffscreenHeight, mpGraphics, fOffscreenHeight == maDestRect.size.height ) )
				{
					CGContextSaveGState( mpBuffer->maContext );
					if ( [pControl isFlipped] )
					{
						CGContextTranslateCTM( mpBuffer->maContext, 0, aAdjustedDestRect.size.height );
						CGContextScaleCTM( mpBuffer->maContext, 1.0f, -1.0f );
					}
					CGContextBeginTransparencyLayerWithRect( mpBuffer->maContext, aAdjustedDestRect, NULL );

					NSGraphicsContext *pContext = [NSGraphicsContext graphicsContextWithGraphicsPort:mpBuffer->maContext flipped:YES];
					if ( pContext )
					{
						NSRect aDrawRect = NSRectFromCGRect( aAdjustedDestRect );
						// Shift combobox edge to right
						if ( mbEditable )
						{
							aDrawRect.origin.x += FOCUSRING_WIDTH;
							aDrawRect.size.width -= FOCUSRING_WIDTH;
						}

						// Vertically center control
						aDrawRect.origin.y += ( ( fOffscreenHeight - fCellHeight ) / 2 ) + ( mbEditable ? COMBOBOX_HEIGHT_SLOP : 0 );
						aDrawRect.size.height = fCellHeight;

						NSGraphicsContext *pOldContext = [NSGraphicsContext currentContext];
						[NSGraphicsContext setCurrentContext:pContext];
						[pCell drawWithFrame:aDrawRect inView:pControl];
						[NSGraphicsContext setCurrentContext:pOldContext];

						mbDrawn = YES;
					}

					CGContextEndTransparencyLayer( mpBuffer->maContext );
					CGContextRestoreGState( mpBuffer->maContext );

					mpBuffer->ReleaseContext();

					if ( mbDrawn )
						mpBuffer->DrawContextAndDestroy( mpGraphics, aAdjustedDestRect, maDestRect );
				}
			}
		}
	}
}

- (MacOSBOOL)drawn
{
	return mbDrawn;
}

- (void)getSize:(id)pObject
{
	if ( NSEqualSizes( maSize, NSZeroSize ) )
	{
		NSControl *pControl = ( mbEditable ? [self comboBox] : [self popUpButton] );
		if ( pControl )
		{
			NSCell *pCell = [pControl cell];
			if ( pCell )
				maSize = [pCell cellSize];
		}
	}
}

- (id)initWithControlState:(ControlState)nControlState editable:(MacOSBOOL)bEditable bitmapBuffer:(VCLBitmapBuffer *)pBuffer graphics:(JavaSalGraphics *)pGraphics destRect:(CGRect)aDestRect
{
	[super init];

	mnControlState = nControlState;
	mbEditable = bEditable;
	mpBuffer = pBuffer;
	mpGraphics = pGraphics;
	maDestRect = aDestRect;
	mbDrawn = NO;
	maSize = NSZeroSize;

	return self;
}

- (NSSize)size
{
	return maSize;
}

@end

// =======================================================================

@interface NSScroller (VCLNativeScrollbar)
- (void)drawArrow:(NSUInteger)nArrow highlightPart:(NSUInteger)nPart;
- (id)scrollerImp;
- (void)scrollerImp:(id)pObject animateUIStateTransitionWithDuration:(double)fDuration;
@end

@interface VCLNativeScrollbar : NSObject
{
	ControlState			mnControlState;
	VCLBitmapBuffer*		mpBuffer;
	JavaSalGraphics*		mpGraphics;
	ScrollbarValue*			mpScrollbarValue;
	MacOSBOOL				mbDoubleScrollbarArrows;
	CGRect					maDestRect;
	MacOSBOOL				mbDrawn;
	MacOSBOOL				mbHorizontal;
	MacOSBOOL				mbDrawOnlyTrack;
	NSRect					maDecrementArrowBounds;
	NSRect					maIncrementArrowBounds;
	NSRect					maDecrementPageBounds;
	NSRect					maIncrementPageBounds;
	NSRect					maThumbBounds;
	NSRect					maTrackBounds;
	NSRect					maTotalBounds;
}
+ (id)createWithControlState:(ControlState)nControlState bitmapBuffer:(VCLBitmapBuffer *)pBuffer graphics:(JavaSalGraphics *)pGraphics scrollbarValue:(ScrollbarValue *)pScrollbarValue doubleScrollbarArrows:(MacOSBOOL)bDoubleScrollbarArrows destRect:(CGRect)aDestRect;
- (NSScroller *)scroller;
- (void)draw:(id)pObject;
- (MacOSBOOL)drawn;
- (void)getBounds:(id)pObject;
- (MacOSBOOL)horizontal;
- (id)initWithControlState:(ControlState)nControlState bitmapBuffer:(VCLBitmapBuffer *)pBuffer graphics:(JavaSalGraphics *)pGraphics scrollbarValue:(ScrollbarValue *)pScrollbarValue doubleScrollbarArrows:(MacOSBOOL)bDoubleScrollbarArrows destRect:(CGRect)aDestRect;
- (NSRect)decrementArrowBounds;
- (NSRect)incrementArrowBounds;
- (NSRect)decrementPageBounds;
- (NSRect)incrementPageBounds;
- (NSRect)thumbBounds;
- (NSRect)trackBounds;
- (NSRect)totalBounds;
@end

@implementation VCLNativeScrollbar

+ (id)createWithControlState:(ControlState)nControlState bitmapBuffer:(VCLBitmapBuffer *)pBuffer graphics:(JavaSalGraphics *)pGraphics scrollbarValue:(ScrollbarValue *)pScrollbarValue doubleScrollbarArrows:(MacOSBOOL)bDoubleScrollbarArrows destRect:(CGRect)aDestRect
{
	VCLNativeScrollbar *pRet = [[VCLNativeScrollbar alloc] initWithControlState:nControlState bitmapBuffer:pBuffer graphics:pGraphics scrollbarValue:pScrollbarValue doubleScrollbarArrows:bDoubleScrollbarArrows destRect:aDestRect];
	[pRet autorelease];
	return pRet;
}

- (NSScroller *)scroller
{
	NSScroller *pScroller = [[NSScroller alloc] initWithFrame:NSMakeRect( 0, 0, maDestRect.size.width, maDestRect.size.height )];
	if ( !pScroller )
		return nil;

	[pScroller autorelease];

	// For scrollers, the inactive state is the same as the disabled state
	if ( mnControlState & CTRL_STATE_ENABLED && ! ( mnControlState & CTRL_STATE_INACTIVE ) )
		[pScroller setEnabled:YES];
	else
		[pScroller setEnabled:NO];

	if ( mpScrollbarValue )
	{
		float fTrackRange = (float)( mpScrollbarValue->mnMax - mpScrollbarValue->mnVisibleSize - mpScrollbarValue->mnMin );
		float fTrackPosition = (float)( mpScrollbarValue->mnCur - mpScrollbarValue->mnMin );
		if ( mpScrollbarValue->mnVisibleSize > 0 && fTrackRange > 0 )
		{
			mbDrawOnlyTrack = NO;
			[pScroller setDoubleValue:fTrackPosition / fTrackRange];
			[pScroller setKnobProportion:(float)mpScrollbarValue->mnVisibleSize / ( fTrackRange + mpScrollbarValue->mnVisibleSize )];
		}
		else
		{
			// Set the value and knob proportion with "reasonable" values.
			// Fix bug 3359 by drawing on the scrollbar track.
			mbDrawOnlyTrack = YES;
			[pScroller setDoubleValue:0];
			[pScroller setKnobProportion:1.0];
		}

		if ( ( mpScrollbarValue->mnButton1State | mpScrollbarValue->mnButton2State | mpScrollbarValue->mnPage1State | mpScrollbarValue->mnPage2State | mpScrollbarValue->mnThumbState ) & ( CTRL_STATE_PRESSED | CTRL_STATE_ROLLOVER | CTRL_STATE_SELECTED ) )
		{
			// Darken thumb when mouse is within scroller on Mac OS X 10.7 and
			// higher
			if ( [pScroller respondsToSelector:@selector(scrollerImp)] && [pScroller respondsToSelector:@selector(scrollerImp:animateUIStateTransitionWithDuration:)] )
			{
				NSObject *pImp = [pScroller scrollerImp];
				if ( pImp )
					[pScroller scrollerImp:pImp animateUIStateTransitionWithDuration:1.0f];
			}
		}
	}

	// The enabled state is controlled by the [NSWindow _hasActiveControls]
	// selector so we need to attach a custom hidden window to draw enabled
	[VCLNativeControlWindow createAndAttachToView:pScroller controlState:mnControlState];

	// On Mac OS X 10.6, disabling the scroller causes it to be drawn without
	// any knob or arrows so we enabled it and draw with no attached window
	if ( IsRunningSnowLeopard() && ![pScroller window] )
		[pScroller setEnabled:YES];

	[pScroller sizeToFit];

	// Force view size to match width of track otherwise the track will float
	// outward and downward
	NSRect aFrame = [pScroller frame];
	NSRect aTrackBounds = [pScroller rectForPart:NSScrollerKnobSlot];
	if ( aFrame.size.width > aFrame.size.height )
	{
		mbHorizontal = YES;
		aFrame.size.height = aTrackBounds.size.height;
	}
	else
	{
		mbHorizontal = NO;
		aFrame.size.width = aTrackBounds.size.width;
	}
	[pScroller setFrame:aFrame];

	return pScroller;
}

- (void)draw:(id)pObject
{
	if ( !mbDrawn && mpBuffer && mpGraphics && !CGRectIsEmpty( maDestRect ) )
	{
		NSScroller *pScroller = [self scroller];
		if ( pScroller )
		{
			float fOffscreenHeight = maDestRect.size.height;
			CGRect aAdjustedDestRect = CGRectMake( 0, 0, maDestRect.size.width, fOffscreenHeight );
			if ( mpBuffer->Create( (long)maDestRect.origin.x, (long)maDestRect.origin.y, (long)maDestRect.size.width, (long)fOffscreenHeight, mpGraphics, fOffscreenHeight == maDestRect.size.height ) )
			{
				CGContextSaveGState( mpBuffer->maContext );
				if ( [pScroller isFlipped] )
				{
					CGContextTranslateCTM( mpBuffer->maContext, 0, aAdjustedDestRect.size.height );
					CGContextScaleCTM( mpBuffer->maContext, 1.0f, -1.0f );
				}

				// Fix bug 2031 by always filling the background with white
				float whiteColor[] = { 1.0, 1.0, 1.0, 1.0 };
				CGContextSetFillColor( mpBuffer->maContext, whiteColor );
				CGContextFillRect( mpBuffer->maContext, aAdjustedDestRect );

				CGContextBeginTransparencyLayerWithRect( mpBuffer->maContext, aAdjustedDestRect, NULL );

				NSGraphicsContext *pContext = [NSGraphicsContext graphicsContextWithGraphicsPort:mpBuffer->maContext flipped:YES];
				if ( pContext )
				{
					NSGraphicsContext *pOldContext = [NSGraphicsContext currentContext];
					[NSGraphicsContext setCurrentContext:pContext];

					// Draw arrows on Mac OS X 10.6
					if ( !SCROLLBAR_SUPPRESS_ARROWS )
					{
						// Disabling on Mac OS X 10.6 draws the scroller with
						// no arrows
						if ( mbDrawOnlyTrack )
						{
							[pScroller setEnabled:NO];
						}
						else
						{
							MacOSBOOL bHighlight = NO;
							NSUInteger nHighlightArrow = 0;
							NSUInteger nHighlightPart = 0;
							if ( mpScrollbarValue )
							{
								// Note that if a pressed button is selected,
								// we have highlight the inner arrow of the
								// the arrow pair
								if ( mpScrollbarValue->mnButton1State & CTRL_STATE_PRESSED )
								{
									bHighlight = YES;
									nHighlightArrow = NSScrollerDecrementArrow;
									if ( mpScrollbarValue->mnButton1State & CTRL_STATE_SELECTED )
										nHighlightPart = NSScrollerIncrementArrow;
									else
										nHighlightPart = NSScrollerDecrementArrow;
								}
								else if ( mpScrollbarValue->mnButton2State & CTRL_STATE_PRESSED )
								{
									bHighlight = YES;
									nHighlightArrow = NSScrollerIncrementArrow;
									if ( mpScrollbarValue->mnButton2State & CTRL_STATE_SELECTED )
										nHighlightPart = NSScrollerDecrementArrow;
									else
										nHighlightPart = NSScrollerIncrementArrow;
								}
							}

							if ( bHighlight && [pScroller respondsToSelector:@selector(drawArrow:highlightPart:)] )
							{
								if ( mbDoubleScrollbarArrows )
								{
									if ( nHighlightArrow == NSScrollerDecrementArrow )
									{
										[pScroller drawArrow:NSScrollerDecrementArrow highlightPart:nHighlightPart];
										[pScroller drawArrow:NSScrollerIncrementArrow highlight:NO];
									}
									else
									{
										[pScroller drawArrow:NSScrollerDecrementArrow highlight:NO];
										[pScroller drawArrow:NSScrollerIncrementArrow highlightPart:nHighlightPart];
									}
								}
								else
								{
									[pScroller drawArrow:NSScrollerDecrementArrow highlightPart:nHighlightPart];
									[pScroller drawArrow:NSScrollerIncrementArrow highlightPart:nHighlightPart];
								}
							}
							else
							{
								[pScroller drawArrow:NSScrollerDecrementArrow highlight:NO];
								[pScroller drawArrow:NSScrollerIncrementArrow highlight:NO];
							}
						}
					}

					[pScroller drawKnobSlotInRect:[pScroller rectForPart:NSScrollerKnobSlot] highlight:NO];
					if ( !mbDrawOnlyTrack )
						[pScroller drawKnob];
					[NSGraphicsContext setCurrentContext:pOldContext];

					mbDrawn = YES;
				}

				CGContextEndTransparencyLayer( mpBuffer->maContext );
				CGContextRestoreGState( mpBuffer->maContext );

				mpBuffer->ReleaseContext();

				if ( mbDrawn )
					mpBuffer->DrawContextAndDestroy( mpGraphics, aAdjustedDestRect, maDestRect );
			}
		}
	}
}

- (MacOSBOOL)drawn
{
	return mbDrawn;
}

- (void)getBounds:(id)pObject
{
	if ( NSEqualRects( maTotalBounds, NSZeroRect ) )
	{
		NSScroller *pScroller = [self scroller];
		if ( pScroller )
		{
			MacOSBOOL bFlipped = [pScroller isFlipped];

			maDecrementArrowBounds = [pScroller rectForPart:NSScrollerDecrementLine];
			if ( !bFlipped )
				maDecrementArrowBounds.origin.y = maDestRect.size.height - maDecrementArrowBounds.origin.y - maDecrementArrowBounds.size.height;

			maIncrementArrowBounds = [pScroller rectForPart:NSScrollerIncrementLine];
			if ( !bFlipped )
				maIncrementArrowBounds.origin.y = maDestRect.size.height - maIncrementArrowBounds.origin.y - maIncrementArrowBounds.size.height;

			maDecrementPageBounds = [pScroller rectForPart:NSScrollerDecrementPage];
			if ( !bFlipped )
				maDecrementPageBounds.origin.y = maDestRect.size.height - maDecrementPageBounds.origin.y - maDecrementPageBounds.size.height;

			maIncrementPageBounds = [pScroller rectForPart:NSScrollerIncrementPage];
			if ( !bFlipped )
				maIncrementPageBounds.origin.y = maDestRect.size.height - maIncrementPageBounds.origin.y - maIncrementPageBounds.size.height;

			maThumbBounds = [pScroller rectForPart:NSScrollerKnob];
			if ( !bFlipped )
				maThumbBounds.origin.y = maDestRect.size.height - maThumbBounds.origin.y - maThumbBounds.size.height;

			maTrackBounds = [pScroller rectForPart:NSScrollerKnobSlot];
			if ( !bFlipped )
				maTrackBounds.origin.y = maDestRect.size.height - maTrackBounds.origin.y - maTrackBounds.size.height;

			maTotalBounds = [pScroller rectForPart:NSScrollerNoPart];
			if ( !bFlipped )
				maTotalBounds.origin.y = maDestRect.size.height - maTotalBounds.origin.y - maTotalBounds.size.height;
		}
	}
}

- (MacOSBOOL)horizontal
{
	return mbHorizontal;
}

- (id)initWithControlState:(ControlState)nControlState bitmapBuffer:(VCLBitmapBuffer *)pBuffer graphics:(JavaSalGraphics *)pGraphics scrollbarValue:(ScrollbarValue *)pScrollbarValue doubleScrollbarArrows:(MacOSBOOL)bDoubleScrollbarArrows destRect:(CGRect)aDestRect
{
	[super init];

	mnControlState = nControlState;
	mpBuffer = pBuffer;
	mpGraphics = pGraphics;
	mpScrollbarValue = pScrollbarValue;
	mbDoubleScrollbarArrows = bDoubleScrollbarArrows;
	maDestRect = aDestRect;
	mbDrawn = NO;
	mbHorizontal = NO;
	maDecrementArrowBounds = NSZeroRect;
	maIncrementArrowBounds = NSZeroRect;
	maDecrementPageBounds = NSZeroRect;
	maIncrementPageBounds = NSZeroRect;
	maThumbBounds = NSZeroRect;
	maTrackBounds = NSZeroRect;
	maTotalBounds = NSZeroRect;

	return self;
}

- (NSRect)decrementArrowBounds
{
	return maDecrementArrowBounds;
}

- (NSRect)incrementArrowBounds
{
	return maIncrementArrowBounds;
}

- (NSRect)decrementPageBounds
{
	return maDecrementPageBounds;
}

- (NSRect)incrementPageBounds
{
	return maIncrementPageBounds;
}

- (NSRect)thumbBounds
{
	return maThumbBounds;
}

- (NSRect)trackBounds
{
	return maTrackBounds;
}

- (NSRect)totalBounds
{
	return maTotalBounds;
}

@end

// =======================================================================

@interface VCLNativeProgressbar : NSObject
{
	ControlState			mnControlState;
	NSControlSize			mnControlSize;
	VCLBitmapBuffer*		mpBuffer;
	JavaSalGraphics*		mpGraphics;
	ProgressbarValue*		mpProgressbarValue;
	CGRect					maDestRect;
	MacOSBOOL				mbDrawn;
	NSSize					maSize;
}
+ (id)createWithControlState:(ControlState)nControlState controlSize:(NSControlSize)nControlSize bitmapBuffer:(VCLBitmapBuffer *)pBuffer graphics:(JavaSalGraphics *)pGraphics progressbarValue:(ProgressbarValue *)pProgressbarValue destRect:(CGRect)aDestRect;
- (NSProgressIndicator *)progressIndicator;
- (void)draw:(id)pObject;
- (MacOSBOOL)drawn;
- (void)getSize:(id)pObject;
- (id)initWithControlState:(ControlState)nControlState controlSize:(NSControlSize)nControlSize bitmapBuffer:(VCLBitmapBuffer *)pBuffer graphics:(JavaSalGraphics *)pGraphics progressbarValue:(ProgressbarValue *)pProgressbarValue destRect:(CGRect)aDestRect;
- (NSSize)size;
@end

@implementation VCLNativeProgressbar

+ (id)createWithControlState:(ControlState)nControlState controlSize:(NSControlSize)nControlSize bitmapBuffer:(VCLBitmapBuffer *)pBuffer graphics:(JavaSalGraphics *)pGraphics progressbarValue:(ProgressbarValue *)pProgressbarValue destRect:(CGRect)aDestRect
{
	VCLNativeProgressbar *pRet = [[VCLNativeProgressbar alloc] initWithControlState:nControlState controlSize:(NSControlSize)nControlSize bitmapBuffer:pBuffer graphics:pGraphics progressbarValue:pProgressbarValue destRect:aDestRect];
	[pRet autorelease];
	return pRet;
}

- (NSProgressIndicator *)progressIndicator
{
	NSProgressIndicator *pProgressIndicator = [[NSProgressIndicator alloc] initWithFrame:NSMakeRect( 0, 0, maDestRect.size.width, maDestRect.size.height / 2 )];
	if ( !pProgressIndicator )
		return nil;

	[pProgressIndicator autorelease];

	[pProgressIndicator setControlSize:mnControlSize];
	[pProgressIndicator setStyle:NSProgressIndicatorBarStyle];
	[pProgressIndicator setIndeterminate:NO];

	double fRange = [pProgressIndicator maxValue] - [pProgressIndicator minValue];
	if ( fRange < 1.0f )
		fRange = 1.0f;

	[pProgressIndicator setMinValue:0];
	[pProgressIndicator setMaxValue:fRange];
	[pProgressIndicator setDoubleValue:0];

	if ( mpProgressbarValue )
	{
		if ( mpProgressbarValue->mbIndeterminate )
			[pProgressIndicator setIndeterminate:YES];
		else
			[pProgressIndicator setDoubleValue:(double)mpProgressbarValue->mdPercentComplete * fRange / 100.0f];
	}

	// The enabled state is controlled by the [NSWindow _hasActiveControls]
	// selector so we need to attach a custom hidden window to draw enabled
	[VCLNativeControlWindow createAndAttachToView:pProgressIndicator controlState:mnControlState];

	[pProgressIndicator sizeToFit];

	return pProgressIndicator;
}

- (void)draw:(id)pObject
{
	if ( !mbDrawn && mpBuffer && mpGraphics && !CGRectIsEmpty( maDestRect ) )
	{
		NSProgressIndicator *pProgressIndicator = [self progressIndicator];
		if ( pProgressIndicator )
		{
			float fOffscreenHeight = maDestRect.size.height;
			CGRect aAdjustedDestRect = CGRectMake( 0, 0, maDestRect.size.width, fOffscreenHeight );
			if ( mpBuffer->Create( (long)maDestRect.origin.x, (long)maDestRect.origin.y, (long)maDestRect.size.width, (long)fOffscreenHeight, mpGraphics, fOffscreenHeight == maDestRect.size.height ) )
			{
				CGContextSaveGState( mpBuffer->maContext );
				if ( [pProgressIndicator isFlipped] )
				{
					CGContextTranslateCTM( mpBuffer->maContext, 0, aAdjustedDestRect.size.height );
					CGContextScaleCTM( mpBuffer->maContext, 1.0f, -1.0f );
				}

				// Clear the background of the control with the fill color
				CGColorRef aFillColor = CreateCGColorFromSalColor( mpGraphics->mnFillColor );
				if ( aFillColor )
				{
					CGContextSetFillColorWithColor( mpBuffer->maContext, aFillColor );
					CGContextFillRect( mpBuffer->maContext, aAdjustedDestRect );
					CGColorRelease( aFillColor );
				}

				// Vertically center control. Note that we translate the
				// context because we must use the [NSView drawRect:] selector
				// to draw the control.
				CGContextTranslateCTM( mpBuffer->maContext, 0, ( ( fOffscreenHeight - [pProgressIndicator frame].size.height ) / 2 ) + PROGRESSBAR_HEIGHT_SLOP );

				CGContextBeginTransparencyLayerWithRect( mpBuffer->maContext, aAdjustedDestRect, NULL );

				NSGraphicsContext *pContext = [NSGraphicsContext graphicsContextWithGraphicsPort:mpBuffer->maContext flipped:YES];
				if ( pContext )
				{
					NSGraphicsContext *pOldContext = [NSGraphicsContext currentContext];
					[NSGraphicsContext setCurrentContext:pContext];
					[pProgressIndicator drawRect:[pProgressIndicator frame]];
					[NSGraphicsContext setCurrentContext:pOldContext];

					mbDrawn = YES;
				}

				CGContextEndTransparencyLayer( mpBuffer->maContext );
				CGContextRestoreGState( mpBuffer->maContext );

				mpBuffer->ReleaseContext();

				if ( mbDrawn )
					mpBuffer->DrawContextAndDestroy( mpGraphics, aAdjustedDestRect, maDestRect );
			}
		}
	}
}

- (MacOSBOOL)drawn
{
	return mbDrawn;
}

- (void)getSize:(id)pObject
{
	if ( NSEqualSizes( maSize, NSZeroSize ) )
	{
		NSProgressIndicator *pProgressIndicator = [self progressIndicator];
		if ( pProgressIndicator )
			maSize = [pProgressIndicator frame].size;
	}
}

- (id)initWithControlState:(ControlState)nControlState controlSize:(NSControlSize)nControlSize bitmapBuffer:(VCLBitmapBuffer *)pBuffer graphics:(JavaSalGraphics *)pGraphics progressbarValue:(ProgressbarValue *)pProgressbarValue destRect:(CGRect)aDestRect
{
	[super init];

	mnControlState = nControlState;
	mnControlSize = nControlSize;
	mpBuffer = pBuffer;
	mpGraphics = pGraphics;
	mpProgressbarValue = pProgressbarValue;
	maDestRect = aDestRect;
	mbDrawn = NO;
	maSize = NSZeroSize;

	return self;
}

- (NSSize)size
{
	return maSize;
}

@end

// =======================================================================

@interface VCLNativeBox : NSObject
{
	ControlState			mnControlState;
	VCLBitmapBuffer*		mpBuffer;
	JavaSalGraphics*		mpGraphics;
	CGRect					maDestRect;
	MacOSBOOL				mbDrawn;
}
+ (id)createWithControlState:(ControlState)nControlState bitmapBuffer:(VCLBitmapBuffer *)pBuffer graphics:(JavaSalGraphics *)pGraphics destRect:(CGRect)aDestRect;
- (NSBox *)box;
- (void)draw:(id)pObject;
- (MacOSBOOL)drawn;
- (id)initWithControlState:(ControlState)nControlState bitmapBuffer:(VCLBitmapBuffer *)pBuffer graphics:(JavaSalGraphics *)pGraphics destRect:(CGRect)aDestRect;
@end

@implementation VCLNativeBox

+ (id)createWithControlState:(ControlState)nControlState bitmapBuffer:(VCLBitmapBuffer *)pBuffer graphics:(JavaSalGraphics *)pGraphics destRect:(CGRect)aDestRect
{
	VCLNativeBox *pRet = [[VCLNativeBox alloc] initWithControlState:nControlState bitmapBuffer:pBuffer graphics:pGraphics destRect:aDestRect];
	[pRet autorelease];
	return pRet;
}

- (NSBox *)box
{
	NSBox *pBox = [[NSBox alloc] initWithFrame:NSMakeRect( 0, 0, maDestRect.size.width, maDestRect.size.height )];
	if ( !pBox )
		return nil;

	[pBox autorelease];

	[pBox setTitle:@""];
	[pBox setTitlePosition:NSNoTitle];

	return pBox;
}

- (void)draw:(id)pObject
{
	if ( !mbDrawn && mpBuffer && mpGraphics && !CGRectIsEmpty( maDestRect ) )
	{
		NSBox *pBox = [self box];
		if ( pBox )
		{
			float fOffscreenHeight = maDestRect.size.height;
			CGRect aAdjustedDestRect = CGRectMake( 0, 0, maDestRect.size.width, fOffscreenHeight );
			if ( mpBuffer->Create( (long)maDestRect.origin.x, (long)maDestRect.origin.y, (long)maDestRect.size.width, (long)fOffscreenHeight, mpGraphics, fOffscreenHeight == maDestRect.size.height ) )
			{
				CGContextSaveGState( mpBuffer->maContext );
				if ( [pBox isFlipped] )
				{
					CGContextTranslateCTM( mpBuffer->maContext, 0, aAdjustedDestRect.size.height );
					CGContextScaleCTM( mpBuffer->maContext, 1.0f, -1.0f );
				}
				CGContextBeginTransparencyLayerWithRect( mpBuffer->maContext, aAdjustedDestRect, NULL );

				NSGraphicsContext *pContext = [NSGraphicsContext graphicsContextWithGraphicsPort:mpBuffer->maContext flipped:YES];
				if ( pContext )
				{
					NSGraphicsContext *pOldContext = [NSGraphicsContext currentContext];
					[NSGraphicsContext setCurrentContext:pContext];
					[pBox drawRect:[pBox frame]];
					[NSGraphicsContext setCurrentContext:pOldContext];

					mbDrawn = YES;
				}

				CGContextEndTransparencyLayer( mpBuffer->maContext );
				CGContextRestoreGState( mpBuffer->maContext );

				mpBuffer->ReleaseContext();

				if ( mbDrawn )
					mpBuffer->DrawContextAndDestroy( mpGraphics, aAdjustedDestRect, maDestRect );
			}
		}
	}
}

- (MacOSBOOL)drawn
{
	return mbDrawn;
}

- (id)initWithControlState:(ControlState)nControlState bitmapBuffer:(VCLBitmapBuffer *)pBuffer graphics:(JavaSalGraphics *)pGraphics destRect:(CGRect)aDestRect
{
	[super init];

	mnControlState = nControlState;
	mpBuffer = pBuffer;
	mpGraphics = pGraphics;
	maDestRect = aDestRect;
	mbDrawn = NO;

	return self;
}

@end

// =======================================================================

@interface VCLNativeScrollView : NSObject
{
	ControlState			mnControlState;
	VCLBitmapBuffer*		mpBuffer;
	JavaSalGraphics*		mpGraphics;
	CGRect					maDestRect;
	MacOSBOOL				mbDrawn;
}
+ (id)createWithControlState:(ControlState)nControlState bitmapBuffer:(VCLBitmapBuffer *)pBuffer graphics:(JavaSalGraphics *)pGraphics destRect:(CGRect)aDestRect;
- (NSScrollView *)scrollView;
- (void)draw:(id)pObject;
- (MacOSBOOL)drawn;
- (id)initWithControlState:(ControlState)nControlState bitmapBuffer:(VCLBitmapBuffer *)pBuffer graphics:(JavaSalGraphics *)pGraphics destRect:(CGRect)aDestRect;
@end

@implementation VCLNativeScrollView

+ (id)createWithControlState:(ControlState)nControlState bitmapBuffer:(VCLBitmapBuffer *)pBuffer graphics:(JavaSalGraphics *)pGraphics destRect:(CGRect)aDestRect
{
	VCLNativeScrollView *pRet = [[VCLNativeScrollView alloc] initWithControlState:nControlState bitmapBuffer:pBuffer graphics:pGraphics destRect:aDestRect];
	[pRet autorelease];
	return pRet;
}

- (NSScrollView *)scrollView
{
	NSScrollView *pScrollView = [[NSScrollView alloc] initWithFrame:NSMakeRect( 0, 0, maDestRect.size.width, maDestRect.size.height )];
	if ( !pScrollView )
		return nil;

	[pScrollView autorelease];

	[pScrollView setBorderType:NSBezelBorder];
	[pScrollView setDrawsBackground:NO];

	return pScrollView;
}

- (void)draw:(id)pObject
{
	if ( !mbDrawn && mpBuffer && mpGraphics && !CGRectIsEmpty( maDestRect ) )
	{
		NSScrollView *pScrollView = [self scrollView];
		if ( pScrollView )
		{
			float fOffscreenHeight = maDestRect.size.height;
			CGRect aAdjustedDestRect = CGRectMake( 0, 0, maDestRect.size.width, fOffscreenHeight );
			if ( mpBuffer->Create( (long)maDestRect.origin.x, (long)maDestRect.origin.y, (long)maDestRect.size.width, (long)fOffscreenHeight, mpGraphics, fOffscreenHeight == maDestRect.size.height ) )
			{
				CGContextSaveGState( mpBuffer->maContext );
				if ( [pScrollView isFlipped] )
				{
					CGContextTranslateCTM( mpBuffer->maContext, 0, aAdjustedDestRect.size.height );
					CGContextScaleCTM( mpBuffer->maContext, 1.0f, -1.0f );
				}
				CGContextBeginTransparencyLayerWithRect( mpBuffer->maContext, aAdjustedDestRect, NULL );

				NSGraphicsContext *pContext = [NSGraphicsContext graphicsContextWithGraphicsPort:mpBuffer->maContext flipped:YES];
				if ( pContext )
				{
					NSGraphicsContext *pOldContext = [NSGraphicsContext currentContext];
					[NSGraphicsContext setCurrentContext:pContext];
					[pScrollView drawRect:[pScrollView frame]];
					[NSGraphicsContext setCurrentContext:pOldContext];

					mbDrawn = YES;
				}

				CGContextEndTransparencyLayer( mpBuffer->maContext );
				CGContextRestoreGState( mpBuffer->maContext );

				mpBuffer->ReleaseContext();

				if ( mbDrawn )
					mpBuffer->DrawContextAndDestroy( mpGraphics, aAdjustedDestRect, maDestRect );
			}
		}
	}
}

- (MacOSBOOL)drawn
{
	return mbDrawn;
}

- (id)initWithControlState:(ControlState)nControlState bitmapBuffer:(VCLBitmapBuffer *)pBuffer graphics:(JavaSalGraphics *)pGraphics destRect:(CGRect)aDestRect
{
	[super init];

	mnControlState = nControlState;
	mpBuffer = pBuffer;
	mpGraphics = pGraphics;
	maDestRect = aDestRect;
	mbDrawn = NO;

	return self;
}

@end

// =======================================================================

@interface VCLNativeTableHeaderColumn : NSObject
{
	ControlState			mnControlState;
	VCLBitmapBuffer*		mpBuffer;
	JavaSalGraphics*		mpGraphics;
	ListViewHeaderValue*	mpListViewHeaderValue;
	CGRect					maDestRect;
	MacOSBOOL				mbDrawn;
}
+ (id)createWithControlState:(ControlState)nControlState bitmapBuffer:(VCLBitmapBuffer *)pBuffer graphics:(JavaSalGraphics *)pGraphics listViewHeaderValue:(ListViewHeaderValue *)pListViewHeaderValue destRect:(CGRect)aDestRect;
- (NSTableColumn *)tableColumn;
- (void)draw:(id)pObject;
- (MacOSBOOL)drawn;
- (id)initWithControlState:(ControlState)nControlState bitmapBuffer:(VCLBitmapBuffer *)pBuffer graphics:(JavaSalGraphics *)pGraphics listViewHeaderValue:(ListViewHeaderValue *)pListViewHeaderValue destRect:(CGRect)aDestRect;
@end

@implementation VCLNativeTableHeaderColumn

+ (id)createWithControlState:(ControlState)nControlState bitmapBuffer:(VCLBitmapBuffer *)pBuffer graphics:(JavaSalGraphics *)pGraphics listViewHeaderValue:(ListViewHeaderValue *)pListViewHeaderValue destRect:(CGRect)aDestRect
{
	VCLNativeTableHeaderColumn *pRet = [[VCLNativeTableHeaderColumn alloc] initWithControlState:nControlState bitmapBuffer:pBuffer graphics:pGraphics listViewHeaderValue:pListViewHeaderValue destRect:aDestRect];
	[pRet autorelease];
	return pRet;
}

- (NSTableColumn *)tableColumn
{
	NSScrollView *pScrollView = [[NSScrollView alloc] initWithFrame:NSMakeRect( 0, 0, maDestRect.size.width, maDestRect.size.height )];
	if ( !pScrollView )
		return nil;

	[pScrollView autorelease];

	NSTableView *pTableView = [[NSTableView alloc] initWithFrame:[pScrollView frame]];
	if ( !pTableView )
		return nil;

	[pTableView autorelease];
	[pScrollView setDocumentView:pTableView];

	NSTableColumn *pTableColumn = [[NSTableColumn alloc] initWithIdentifier:@""];
	if ( !pTableColumn )
		return nil;

	[pTableColumn autorelease];
	[pTableColumn setWidth:[pTableView frame].size.width];
	[pTableView addTableColumn:pTableColumn];

	NSTableHeaderCell *pTableHeaderCell = [pTableColumn headerCell];
	if ( !pTableHeaderCell || ![pTableHeaderCell isKindOfClass:[NSTableHeaderCell class]] )
		return nil;

	[pTableHeaderCell setStringValue:@""];

	if ( mnControlState & CTRL_STATE_PRESSED )
		[pTableHeaderCell setState:NSOnState];
	else
		[pTableHeaderCell setState:NSOffState];

	// The enabled state is controlled by the [NSWindow _hasActiveControls]
	// selector so we need to attach a custom hidden window to draw enabled
	[VCLNativeControlWindow createAndAttachToView:pScrollView controlState:mnControlState];

	return pTableColumn;
}

- (void)draw:(id)pObject
{
	if ( !mbDrawn && mpBuffer && mpGraphics && !CGRectIsEmpty( maDestRect ) )
	{
		NSTableColumn *pTableColumn = [self tableColumn];
		if ( pTableColumn )
		{
			NSTableView *pTableView = [pTableColumn tableView];
			if ( pTableView )
			{
				NSTableHeaderView *pTableHeaderView = [pTableView headerView];
				NSTableHeaderCell *pTableHeaderCell = [pTableColumn headerCell];
				if ( pTableHeaderView && pTableHeaderCell && [pTableHeaderCell isKindOfClass:[NSTableHeaderCell class]] )
				{
					float fCellHeight = [pTableHeaderCell cellSize].height;
					float fOffscreenHeight = ( maDestRect.size.height > fCellHeight ? maDestRect.size.height : fCellHeight );
					CGRect aAdjustedDestRect = CGRectMake( 0, 0, maDestRect.size.width, fOffscreenHeight );
					if ( mpBuffer->Create( (long)maDestRect.origin.x, (long)maDestRect.origin.y, (long)maDestRect.size.width, (long)fOffscreenHeight, mpGraphics, fOffscreenHeight == maDestRect.size.height ) )
					{
						CGContextSaveGState( mpBuffer->maContext );
						if ( [pTableHeaderView isFlipped] )
						{
							CGContextTranslateCTM( mpBuffer->maContext, 0, aAdjustedDestRect.size.height );
							CGContextScaleCTM( mpBuffer->maContext, 1.0f, -1.0f );
						}
						CGContextBeginTransparencyLayerWithRect( mpBuffer->maContext, aAdjustedDestRect, NULL );

						NSGraphicsContext *pContext = [NSGraphicsContext graphicsContextWithGraphicsPort:mpBuffer->maContext flipped:YES];
						if ( pContext )
						{
							NSRect aDrawRect = NSRectFromCGRect( aAdjustedDestRect );
							NSGraphicsContext *pOldContext = [NSGraphicsContext currentContext];
							[NSGraphicsContext setCurrentContext:pContext];
							if ( ( mnControlState & CTRL_STATE_SELECTED ) | ( mpListViewHeaderValue && mpListViewHeaderValue->mbPrimarySortColumn ) )
								[pTableHeaderCell highlight:NO withFrame:aDrawRect inView:pTableHeaderView];
							else
								[pTableHeaderCell drawWithFrame:aDrawRect inView:pTableHeaderView];

							// Draw sort indicator
							if ( mpListViewHeaderValue )
							{
								BOOL bDrawSortIndicator = NO;
								BOOL bSortAscending = YES;
								if ( mpListViewHeaderValue->mnSortDirection == LISTVIEWHEADER_SORT_ASCENDING )
								{
									bDrawSortIndicator = YES;
									bSortAscending = YES;
								}
								else if ( mpListViewHeaderValue->mnSortDirection == LISTVIEWHEADER_SORT_DESCENDING )
								{
									bDrawSortIndicator = YES;
									bSortAscending = NO;
								}

								if ( bDrawSortIndicator )
									[pTableHeaderCell drawSortIndicatorWithFrame:aDrawRect inView:pTableHeaderView ascending:bSortAscending priority:0];
							}

							[NSGraphicsContext setCurrentContext:pOldContext];

							mbDrawn = YES;
						}

						CGContextEndTransparencyLayer( mpBuffer->maContext );
						CGContextRestoreGState( mpBuffer->maContext );

						mpBuffer->ReleaseContext();

						if ( mbDrawn )
							mpBuffer->DrawContextAndDestroy( mpGraphics, aAdjustedDestRect, maDestRect );
					}
				}
			}
		}
	}
}

- (MacOSBOOL)drawn
{
	return mbDrawn;
}

- (id)initWithControlState:(ControlState)nControlState bitmapBuffer:(VCLBitmapBuffer *)pBuffer graphics:(JavaSalGraphics *)pGraphics listViewHeaderValue:(ListViewHeaderValue *)pListViewHeaderValue destRect:(CGRect)aDestRect
{
	[super init];

	mnControlState = nControlState;
	mpBuffer = pBuffer;
	mpGraphics = pGraphics;
	mpListViewHeaderValue = pListViewHeaderValue;
	maDestRect = aDestRect;
	mbDrawn = NO;

	return self;
}

@end

// =======================================================================

@interface VCLNativeStepper : NSStepper
{
	MacOSBOOL				mbShowsFirstResponder;
}
- (MacOSBOOL)_shouldShowFirstResponderForCell:(NSCell *)pCell;
- (id)initWithFrame:(NSRect)aRect;
- (void)setShowsFirstResponder:(MacOSBOOL)bShowsFirstResponder;
@end

@implementation VCLNativeStepper

- (MacOSBOOL)_shouldShowFirstResponderForCell:(NSCell *)pCell
{
	return mbShowsFirstResponder;
}

- (id)initWithFrame:(NSRect)aRect
{
	[super initWithFrame:aRect];

	mbShowsFirstResponder = NO;

	return self;
}

- (void)setShowsFirstResponder:(MacOSBOOL)bShowsFirstResponder
{
	mbShowsFirstResponder = bShowsFirstResponder;
}

@end

// =======================================================================

@interface VCLNativeSpinbuttons : NSObject
{
	ControlState			mnControlState;
	VCLBitmapBuffer*		mpBuffer;
	JavaSalGraphics*		mpGraphics;
	SpinbuttonValue*		mpSpinbuttonValue;
	CGRect					maDestRect;
	MacOSBOOL				mbDrawn;
	NSSize					maSize;
}
+ (id)createWithControlState:(ControlState)nControlState bitmapBuffer:(VCLBitmapBuffer *)pBuffer graphics:(JavaSalGraphics *)pGraphics spinbuttonValue:(SpinbuttonValue *)pSpinbuttonValue destRect:(CGRect)aDestRect;
- (NSStepper *)stepper;
- (void)draw:(id)pObject;
- (MacOSBOOL)drawn;
- (void)getSize:(id)pObject;
- (id)initWithControlState:(ControlState)nControlState bitmapBuffer:(VCLBitmapBuffer *)pBuffer graphics:(JavaSalGraphics *)pGraphics spinbuttonValue:(SpinbuttonValue *)pSpinbuttonValue destRect:(CGRect)aDestRect;
- (NSSize)size;
@end

@implementation VCLNativeSpinbuttons

+ (id)createWithControlState:(ControlState)nControlState bitmapBuffer:(VCLBitmapBuffer *)pBuffer graphics:(JavaSalGraphics *)pGraphics spinbuttonValue:(SpinbuttonValue *)pSpinbuttonValue destRect:(CGRect)aDestRect
{
	VCLNativeSpinbuttons *pRet = [[VCLNativeSpinbuttons alloc] initWithControlState:nControlState bitmapBuffer:pBuffer graphics:pGraphics spinbuttonValue:pSpinbuttonValue destRect:aDestRect];
	[pRet autorelease];
	return pRet;
}

- (NSStepper *)stepper
{
	VCLNativeStepper *pStepper = [[VCLNativeStepper alloc] initWithFrame:NSMakeRect( 0, 0, maDestRect.size.width, maDestRect.size.height )];
	if ( !pStepper )
		return nil;

	[pStepper autorelease];

	NSCell *pCell = [pStepper cell];
	if ( !pCell )
		return nil;

	[pStepper setAutorepeat:NO];

	if ( mpSpinbuttonValue )
	{
		if ( mpSpinbuttonValue->mnUpperState & CTRL_STATE_PRESSED )
		{
			[pStepper moveUp:self];
			[pCell setHighlighted:YES];
		}
		else if ( mpSpinbuttonValue->mnLowerState & CTRL_STATE_PRESSED )
		{
			[pStepper moveDown:self];
			[pCell setHighlighted:YES];
		}
	}

	if ( mnControlState & ( CTRL_STATE_PRESSED | CTRL_STATE_SELECTED | CTRL_STATE_ENABLED ) )
		[pStepper setEnabled:YES];
	else
		[pStepper setEnabled:NO];

	if ( mnControlState & CTRL_STATE_FOCUSED )
	{
		[pCell setShowsFirstResponder:YES];
		if ( [pStepper isEnabled] )
			[pStepper setShowsFirstResponder:YES];
	}
	else
	{
		[pCell setShowsFirstResponder:NO];
	}

	[pStepper sizeToFit];

	return pStepper;
}

- (void)draw:(id)pObject
{
	if ( !mbDrawn && mpBuffer && mpGraphics && !CGRectIsEmpty( maDestRect ) )
	{
		NSStepper *pStepper = [self stepper];
		if ( pStepper )
		{
			NSCell *pCell = [pStepper cell];
			if ( pCell )
			{
				float fCellWidth = [pCell cellSize].width;
				if ( fCellWidth <= 0 )
					fCellWidth = maDestRect.size.width;
				float fCellHeight = [pCell cellSize].height;
				if ( fCellHeight <= 0 )
					fCellHeight = maDestRect.size.height;
				float fOffscreenHeight = ( maDestRect.size.height > fCellHeight ? maDestRect.size.height : fCellHeight );
				CGRect aAdjustedDestRect = CGRectMake( 0, 0, maDestRect.size.width, fOffscreenHeight );
				if ( mpBuffer->Create( (long)maDestRect.origin.x, (long)maDestRect.origin.y, (long)maDestRect.size.width, (long)fOffscreenHeight, mpGraphics, fOffscreenHeight == maDestRect.size.height ) )
				{
					CGContextSaveGState( mpBuffer->maContext );
					if ( [pStepper isFlipped] )
					{
						CGContextTranslateCTM( mpBuffer->maContext, 0, aAdjustedDestRect.size.height );
						CGContextScaleCTM( mpBuffer->maContext, 1.0f, -1.0f );
					}

					CGContextClipToRect( mpBuffer->maContext, aAdjustedDestRect );

					// Horizontally right align and vertically center control
					float fXAdjust = aAdjustedDestRect.size.width - fCellWidth + SPINNER_WIDTH_SLOP;
					if ( fXAdjust < 0 )
						fXAdjust = 0;
					float fYAdjust = ( fOffscreenHeight - fCellHeight ) / 2;
					CGContextTranslateCTM( mpBuffer->maContext, fXAdjust, fYAdjust );

					CGContextBeginTransparencyLayerWithRect( mpBuffer->maContext, aAdjustedDestRect, NULL );

					NSGraphicsContext *pContext = [NSGraphicsContext graphicsContextWithGraphicsPort:mpBuffer->maContext flipped:YES];
					if ( pContext )
					{
						// Draw view instead of cell otherwise the focus ring
						// will not be drawn
						MacOSBOOL bAddedToKeyWindow = NO;
						if ( [pStepper isEnabled] && [pCell showsFirstResponder] )
						{
							NSApplication *pApp = [NSApplication sharedApplication];
							if ( pApp )
							{
								NSWindow *pKeyWindow = [pApp keyWindow];
								if ( pKeyWindow )
								{
									NSView *pContentView = [pKeyWindow contentView];
									if ( pContentView )
									{
										[pStepper removeFromSuperviewWithoutNeedingDisplay];
										[pContentView addSubview:pStepper positioned:NSWindowBelow relativeTo:nil];
										bAddedToKeyWindow = YES;
									}
								}
							}
						}

						// The enabled state is controlled by the
						// [NSWindow _hasActiveControls] selector so we need to
						// attach a custom hidden window to draw enabled
						if ( !bAddedToKeyWindow )
							[VCLNativeControlWindow createAndAttachToView:pStepper controlState:mnControlState];

						NSGraphicsContext *pOldContext = [NSGraphicsContext currentContext];
						[NSGraphicsContext setCurrentContext:pContext];
						[pStepper drawRect:[pStepper frame]];
						[NSGraphicsContext setCurrentContext:pOldContext];

						[pStepper removeFromSuperviewWithoutNeedingDisplay];

						mbDrawn = YES;
					}

					CGContextEndTransparencyLayer( mpBuffer->maContext );
					CGContextRestoreGState( mpBuffer->maContext );

					mpBuffer->ReleaseContext();

					if ( mbDrawn )
						mpBuffer->DrawContextAndDestroy( mpGraphics, aAdjustedDestRect, maDestRect );
				}
			}
		}
	}
}

- (MacOSBOOL)drawn
{
	return mbDrawn;
}

- (void)getSize:(id)pObject
{
	if ( NSEqualSizes( maSize, NSZeroSize ) )
	{
		NSStepper *pStepper = [self stepper];
		if ( pStepper )
			maSize = [pStepper frame].size;
	}
}

- (id)initWithControlState:(ControlState)nControlState bitmapBuffer:(VCLBitmapBuffer *)pBuffer graphics:(JavaSalGraphics *)pGraphics spinbuttonValue:(SpinbuttonValue *)pSpinbuttonValue destRect:(CGRect)aDestRect
{
	[super init];

	mnControlState = nControlState;
	mpBuffer = pBuffer;
	mpGraphics = pGraphics;
	mpSpinbuttonValue = pSpinbuttonValue;
	maDestRect = aDestRect;
	mbDrawn = NO;
	maSize = NSZeroSize;

	return self;
}

- (NSSize)size
{
	return maSize;
}

@end

// =======================================================================

VCLBitmapBuffer::VCLBitmapBuffer() :
	BitmapBuffer(),
	maContext( NULL ),
	mpGraphicsMutexGuard( NULL ),
	mnHIThemeOrientationFlags( kHIThemeOrientationInverted ),
	mbLastDrawToPrintGraphics( false ),
	mbUseLayer( false )
{
	mnFormat = 0;
	mnWidth = 0;
	mnHeight = 0;
	mnScanlineSize = 0;
	mnBitCount = 0;
	mpBits = NULL;
}

// -----------------------------------------------------------------------

VCLBitmapBuffer::~VCLBitmapBuffer()
{
	Destroy();
}

// -----------------------------------------------------------------------

BOOL VCLBitmapBuffer::Create( long nX, long nY, long nWidth, long nHeight, JavaSalGraphics *pGraphics, bool bUseLayer )
{
	if ( nWidth <= 0 || nHeight <= 0 || !pGraphics )
		return FALSE;

	bool bDrawToPrintGraphics = ( pGraphics->mpPrinter ? true : false );
	mbUseLayer = false;

	// Note that we cannot draw to a frame's layer as it the native window's
	// flipped graphics context will cause the HITheme images to be flipped
	// regardless of the HITTheme orientation or context scaling used
	if ( bUseLayer && bDrawToPrintGraphics )
		bUseLayer = false;

	Destroy();

	mnFormat = JavaSalBitmap::Get32BitNativeFormat() | pGraphics->getBitmapDirectionFormat();
	mnWidth = nWidth;
	mnHeight = nHeight;
	mnScanlineSize = mnWidth * sizeof( sal_uInt32 );
	mnBitCount = 32;

	// If a layer is requested and there is a layer, draw to it directly
	if ( bUseLayer )
	{
		mpGraphicsMutexGuard = new MutexGuard( &pGraphics->getUndrawnNativeOpsMutex() );
		if ( mpGraphicsMutexGuard )
		{
			CGLayerRef aLayer = pGraphics->getLayer();
			if ( aLayer )
			{
				maContext = CGLayerGetContext( aLayer );
				if ( maContext )
				{
					CGContextRetain( maContext );
					CGContextSaveGState( maContext );
					JavaSalGraphics::setContextDefaultSettings( maContext, pGraphics->maFrameClipPath, pGraphics->maNativeClipPath, pGraphics->getNativeLineWidth() );
					CGRect aUnflippedRect = UnflipFlippedRect( CGRectMake( nX, nY, nWidth, nHeight ), pGraphics->maNativeBounds );
					CGContextTranslateCTM( maContext, aUnflippedRect.origin.x, aUnflippedRect.origin.y );
					mbUseLayer = true;
				}
			}

			if ( !mbUseLayer )
			{
				delete mpGraphicsMutexGuard;
				mpGraphicsMutexGuard = NULL;
			}
		}
	}

	if ( !mpBits && !mbUseLayer )
	{
		try
		{
			mpBits = new BYTE[ mnScanlineSize * mnHeight ];
		}
		catch( const std::bad_alloc& ) {}
	}

	if ( !mpBits && !mbUseLayer )
	{
		Destroy();
		return FALSE;
	}

	if ( !maContext && !mbUseLayer )
	{
		CGColorSpaceRef aColorSpace = CGColorSpaceCreateDeviceRGB();
		if ( !aColorSpace )
		{
			Destroy();
			return FALSE;
		}

		// Use requested width and height, not actual width and height of the
		// bitmap buffer
		maContext = CGBitmapContextCreate( mpBits, nWidth, nHeight, 8, mnScanlineSize, aColorSpace, kCGImageAlphaPremultipliedFirst | kCGBitmapByteOrder32Little );
		if ( maContext )
			CGContextSaveGState( maContext );

		CGColorSpaceRelease( aColorSpace );
	}

	if ( !maContext )
	{
		Destroy();
		return FALSE;
	}

	if ( mpBits )
		memset( mpBits, 0, mnScanlineSize * mnHeight );

	mbLastDrawToPrintGraphics = bDrawToPrintGraphics;

	return TRUE;
}

// -----------------------------------------------------------------------

void VCLBitmapBuffer::Destroy()
{
	mnHIThemeOrientationFlags = kHIThemeOrientationInverted;
	mnFormat = 0;
	mnWidth = 0;
	mnHeight = 0;
	mnScanlineSize = 0;
	mnBitCount = 0;
	mbLastDrawToPrintGraphics = false;
	mbUseLayer = false;

	if ( maContext )
	{
		CGContextRestoreGState( maContext );
		CGContextRelease( maContext );
		maContext = NULL;
	}

	if ( mpBits )
	{
		delete[] mpBits;
		mpBits = NULL;
	}

	if ( mpGraphicsMutexGuard )
	{
		delete mpGraphicsMutexGuard;
		mpGraphicsMutexGuard = NULL;
	}
}

// -----------------------------------------------------------------------

void VCLBitmapBuffer::DrawContextAndDestroy( JavaSalGraphics *pGraphics, CGRect aSrcRect, CGRect aDestRect )
{
	if ( pGraphics )
	{
		CGRect aUnflippedRect = UnflipFlippedRect( aDestRect, pGraphics->maNativeBounds );
		if ( mpBits && !mbUseLayer )
		{
			// Assign ownership of bits to a CGDataProvider instance
			CGDataProviderRef aProvider = CGDataProviderCreateWithData( NULL, mpBits, mnScanlineSize * mnHeight, ReleaseBitmapBufferBytePointerCallback );
			if ( aProvider )
			{
				mpBits = NULL;
				pGraphics->addUndrawnNativeOp( new JavaSalGraphicsDrawImageOp( pGraphics->maFrameClipPath, pGraphics->maNativeClipPath, false, false, aProvider, mnBitCount, mnScanlineSize, mnWidth, mnHeight, aSrcRect, aUnflippedRect ) );
				CGDataProviderRelease( aProvider );
			}
		}
		else if ( pGraphics->mpFrame )
		{
			pGraphics->addNeedsDisplayRect( aUnflippedRect, pGraphics->getNativeLineWidth() );
		}
	}

	Destroy();
}
		
// -----------------------------------------------------------------------

void VCLBitmapBuffer::ReleaseContext()
{
	mbUseLayer = false;

	if ( maContext )
	{
		CGContextRestoreGState( maContext );
		CGContextRelease( maContext );
		maContext = NULL;
	}

	if ( mpGraphicsMutexGuard )
	{
		delete mpGraphicsMutexGuard;
		mpGraphicsMutexGuard = NULL;
	}
}

// =======================================================================

static bool HIThemeInitialize()
{
	if ( !bHIThemeInitialized )
	{
		void *pLib = dlopen( NULL, RTLD_LAZY | RTLD_LOCAL );
		if ( pLib )
		{
			pGetThemeMetric = (GetThemeMetric_Type *)dlsym( pLib, "GetThemeMetric" );
			pHIThemeDrawButton = (HIThemeDrawButton_Type *)dlsym( pLib, "HIThemeDrawButton" );
			pHIThemeDrawFrame = (HIThemeDrawFrame_Type *)dlsym( pLib, "HIThemeDrawFrame" );
			pHIThemeDrawMenuBackground = (HIThemeDrawMenuBackground_Type *)dlsym( pLib, "HIThemeDrawMenuBackground" );
			pHIThemeDrawSeparator = (HIThemeDrawSeparator_Type *)dlsym( pLib, "HIThemeDrawSeparator" );
			pHIThemeDrawTab = (HIThemeDrawTab_Type *)dlsym( pLib, "HIThemeDrawTab" );
			pHIThemeDrawTabPane = (HIThemeDrawTabPane_Type *)dlsym( pLib, "HIThemeDrawTabPane" );
			pHIThemeGetGrowBoxBounds = (HIThemeGetGrowBoxBounds_Type *)dlsym( pLib, "HIThemeGetGrowBoxBounds" );
			pHIThemeGetTabShape = (HIThemeGetTabShape_Type *)dlsym( pLib, "HIThemeGetTabShape" );

			dlclose( pLib );
		}

#ifdef DEBUG
		fprintf( stderr, "pGetThemeMetric: %p\n", pGetThemeMetric );
		fprintf( stderr, "pHIThemeDrawButton: %p\n", pHIThemeDrawButton );
		fprintf( stderr, "pHIThemeDrawFrame: %p\n", pHIThemeDrawFrame );
		fprintf( stderr, "pHIThemeDrawMenuBackground: %p\n", pHIThemeDrawMenuBackground );
		fprintf( stderr, "pHIThemeDrawSeparator: %p\n", pHIThemeDrawSeparator );
		fprintf( stderr, "pHIThemeDrawTab: %p\n", pHIThemeDrawTab );
		fprintf( stderr, "pHIThemeDrawTabPane: %p\n", pHIThemeDrawTabPane );
		fprintf( stderr, "pHIThemeGetGrowBoxBounds: %p\n", pHIThemeGetGrowBoxBounds );
		fprintf( stderr, "pHIThemeGetTabShape: %p\n", pHIThemeGetTabShape );
#endif	// DEBUG

		bHIThemeInitialized = true;
	}

	return ( pGetThemeMetric && pHIThemeDrawButton && pHIThemeDrawFrame && pHIThemeDrawMenuBackground && pHIThemeDrawSeparator && pHIThemeDrawTab && pHIThemeDrawTabPane && pHIThemeGetGrowBoxBounds && pHIThemeGetTabShape );
}

// =======================================================================

/**
 * (static) Initialize an HITheme button structure to draw spinner arrows for a
 * spinbox, an editable control with arrow controls for incrementing and
 * decrementing a value.
 *
 * @param pButtonDrawInfo		HITheme button structure for drawing spinner
 * @param nState				overall conrol state of the control; overall
 *								disabled control state will override individual
 *								button state
 * @param pSpinbuttonValue		VCL structure holding enable state of the
 * @return TRUE if successful, FALSE on failure
 */
static BOOL InitSpinbuttonDrawInfo( HIThemeButtonDrawInfo *pButtonDrawInfo, ControlState nState, SpinbuttonValue *pSpinbuttonValue )
{
	memset( pButtonDrawInfo, 0, sizeof( HIThemeButtonDrawInfo ) );
	pButtonDrawInfo->version = 0;
	pButtonDrawInfo->kind = kThemeIncDecButton;
	if( pSpinbuttonValue )
	{
		if( ! ( nState & ( CTRL_STATE_ENABLED | CTRL_STATE_PRESSED ) ) )
		{
			// entire control is disabled, trumps sub-part state
			pButtonDrawInfo->state = kThemeStateInactive;
		}
		else
		{
			// use individual arrow state
			if( pSpinbuttonValue->mnUpperState & CTRL_STATE_PRESSED )
				pButtonDrawInfo->state = kThemeStatePressedUp;
			else if( pSpinbuttonValue->mnLowerState & CTRL_STATE_PRESSED )
				pButtonDrawInfo->state = kThemeStatePressedDown;
			else if( ( ! ( pSpinbuttonValue->mnUpperState & CTRL_STATE_ENABLED ) ) && ( ! ( pSpinbuttonValue->mnLowerState & CTRL_STATE_ENABLED ) ) )
				pButtonDrawInfo->state = kThemeStateInactive;
			else
				pButtonDrawInfo->state = kThemeStateActive;
		}
	}
	else
	{
		if( ! ( nState & CTRL_STATE_ENABLED ) )
			pButtonDrawInfo->state = kThemeStateInactive;
		else
			pButtonDrawInfo->state = kThemeStateActive;
	}
	return TRUE;
}

// =======================================================================

/**
 * (static) Convert VCL tab information structure into an HITheme tab
 * description structure.  This structure can be used to draw an
 * individual tab/segmented control into a window.
 *
 * @param pTabDrawInfo		pointer to HITheme tab drawing structure
 * @param nState			overall control state of the tab item.
 * @param pTabValue			VCL structure containing information about
 * 							the tab's position
 * @return TRUE on success, FALSE on failure
 */
static BOOL InitTabDrawInfo( HIThemeTabDrawInfo *pTabDrawInfo, ControlState nState, TabitemValue *pTabValue )
{
	memset( pTabDrawInfo, 0, sizeof( HIThemeTabDrawInfo ) );
	pTabDrawInfo->version = 1;
	if( nState & CTRL_STATE_SELECTED )
		pTabDrawInfo->style = kThemeTabFront;
	else if( nState & CTRL_STATE_PRESSED )
		pTabDrawInfo->style = kThemeTabNonFrontPressed;
	else if( nState & CTRL_STATE_ENABLED )
		pTabDrawInfo->style = kThemeTabNonFront;
	else
		pTabDrawInfo->style = kThemeTabNonFrontInactive;
	pTabDrawInfo->direction = kThemeTabNorth;	// always assume tabs are at top of tab controls in standard position
	pTabDrawInfo->size = kHIThemeTabSizeNormal;
	if( nState & CTRL_STATE_FOCUSED )
		pTabDrawInfo->adornment = kHIThemeTabAdornmentFocus;
	else
		pTabDrawInfo->adornment = kHIThemeTabAdornmentNone;
	pTabDrawInfo->kind = kHIThemeTabKindNormal;
	pTabDrawInfo->position = kHIThemeTabPositionMiddle;
	if( pTabValue )
	{
		if ( pTabValue->isFirst() && pTabValue->isLast() )
			pTabDrawInfo->position = kHIThemeTabPositionOnly;
		else if( pTabValue->isFirst() )
			pTabDrawInfo->position = kHIThemeTabPositionFirst;
		else if( pTabValue->isLast() )
			pTabDrawInfo->position = kHIThemeTabPositionLast;
	}
	switch( pTabDrawInfo->position )
	{
		case kHIThemeTabPositionMiddle:
		case kHIThemeTabPositionFirst:
			pTabDrawInfo->adornment |= kHIThemeTabAdornmentTrailingSeparator;
			break;
	}
	return TRUE;
}

// =======================================================================

/**
 * (static) Initialize the HITheme strucutre used to draw the bounding box
 * for a tab control background bounding box
 *
 * @param pTabPaneDrawInfo	pointer to HITheme tab drawing structure
 * @param nState			overall control state of the tab item.
 * @return TRUE on success, FALSE on failure
 */
static BOOL InitTabPaneDrawInfo( HIThemeTabPaneDrawInfo *pTabPaneDrawInfo, ControlState nState )
{
	memset( pTabPaneDrawInfo, 0, sizeof( HIThemeTabPaneDrawInfo ) );
	pTabPaneDrawInfo->version = 1;
	pTabPaneDrawInfo->direction = kThemeTabNorth;
	pTabPaneDrawInfo->size = kHIThemeTabSizeNormal;
	pTabPaneDrawInfo->adornment = kHIThemeTabAdornmentNone;
	if( ! ( nState & CTRL_STATE_ENABLED ) )
		pTabPaneDrawInfo->state = kThemeStateInactive;
	else
		pTabPaneDrawInfo->state = kThemeStateActive;
	return TRUE;
}

// =======================================================================

/**
 * (static) Initialize HITheme structures used to draw the frame of an
 * edit field.
 *
 * @param pFrameInfo		pointer to the HITheme frame info structure
 *							to be initialized
 * @param nState			control state of the edit field
 * @return TRUE on success, FALSE on failure
 */
static BOOL InitEditFieldDrawInfo( HIThemeFrameDrawInfo *pFrameInfo, ControlState nState )
{
	memset( pFrameInfo, 0, sizeof( HIThemeFrameDrawInfo ) );
	pFrameInfo->version = 0;
	pFrameInfo->kind = kHIThemeFrameTextFieldSquare;
	if( ! ( nState & CTRL_STATE_ENABLED ) )
		pFrameInfo->state = kThemeStateInactive;
	else
		pFrameInfo->state = kThemeStateActive;
	if( nState & CTRL_STATE_FOCUSED )
	{
		pFrameInfo->isFocused = true;
		pFrameInfo->state |= kThemeStateActive;	// logically we can't have a focused edit field that's inactive
	}
	return TRUE;
}

// =======================================================================

/**
 * (static) Initialize HITheme structures used to draw a separator line
 *
 * @param pSepInfo	pointer to HITheme separator info structure to be
 *						initialized
 * @param nState		control state of the separator
 * @return TRUE on success, FALSE on failure
 */
static BOOL InitSeparatorDrawInfo( HIThemeSeparatorDrawInfo *pSepInfo, ControlState nState )
{
	memset( pSepInfo, 0, sizeof( HIThemeSeparatorDrawInfo ) );
	pSepInfo->version = 0;
	if ( nState & CTRL_STATE_ENABLED )
		pSepInfo->state = kThemeStateActive;
	else
		pSepInfo->state = kThemeStateInactive;
	return TRUE;
}

// =======================================================================

/**
 * (static) Draw a ComboBox into the graphics port at the specified location.
 * ComboBoxes are editable pulldowns, the left portion of which is an edit
 * field and the right portion a downward arrow button used to display the
 * full list contents.
 *
 * Due to VM implementation, JComboBox Swing elements cannot be drawn into
 * a Graphics unless the JComboBox is properly embedded into a visible JFrame.
 * The VM implementation draws these objects asynchronously.  Since we can't
 * easily handle it in Java, we'll use HITheme APIs to draw it into a
 * SalBitmap that we then blit into the graphics.
 *
 * @param pGraphics		pointer to the destination graphics where we'll
 *				be drawing
 * @param rDestBounds		eventual destination rectangle that encompasses
 *				the entire control, editing area as well as
 *				popup arrow
 * @param nState		state of the button to b drawn (enabled/pressed/etc.)
 * @param rCaption		text used for the control.  Presently ignored
 *				as we draw only the frame and let VCL draw
 *				the text
 * @return TRUE if successful, FALSE on error
 */
static BOOL DrawNativeComboBox( JavaSalGraphics *pGraphics, const Rectangle& rDestBounds, ControlState nState, const OUString& rCaption )
{
	BOOL bRet = FALSE;

	NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

	if ( pGraphics->mpFrame && !pGraphics->mpFrame->IsFloatingFrame() && pGraphics->mpFrame != GetSalData()->mpFocusFrame )
		nState |= CTRL_STATE_INACTIVE;

	VCLNativeComboBox *pVCLNativeComboBox = [VCLNativeComboBox createWithControlState:nState editable:YES bitmapBuffer:&aSharedComboBoxBuffer graphics:pGraphics destRect:CGRectMake( rDestBounds.Left(), rDestBounds.Top(), rDestBounds.GetWidth(), rDestBounds.GetHeight() )];
	NSArray *pModes = [NSArray arrayWithObjects:NSDefaultRunLoopMode, NSEventTrackingRunLoopMode, NSModalPanelRunLoopMode, @"AWTRunLoopMode", nil];
	[pVCLNativeComboBox performSelectorOnMainThread:@selector(draw:) withObject:pVCLNativeComboBox waitUntilDone:YES modes:pModes];
	bRet = [pVCLNativeComboBox drawn];

	[pPool release];

	return bRet;
}

// =======================================================================

/**
 * (static) Draw a popupmenu into the graphics port at the specified location.
 * Popup menus, a.k.a listboxes or non-editable combo boxes, consist of a
 * regular popup button and text.
 *
 * Due to VM implementation, JComboBox Swing elements cannot be drawn into
 * a Graphics unless the JComboBox is properly embedded into a visible JFrame.
 * The VM implementation draws these objects asynchronously.  Since we can't
 * easily handle it in Java, we'll use HITheme APIs to draw it into a
 * SalBitmap that we then blit into the graphics.
 *
 * @param pGraphics		pointer to the destination graphics where we'll
 *				be drawing
 * @param rDestBounds		eventual destination rectangle that encompasses
 *				the entire control, editing area as well as
 *				popup arrow
 * @param nState		state of the button to b drawn (enabled/pressed/etc.)
 * @param rCaption		text used for the control.  Presently ignored
 *				as we draw only the frame and let VCL draw
 *				the text
 * @return TRUE if successful, FALSE on error
 */
static BOOL DrawNativeListBox( JavaSalGraphics *pGraphics, const Rectangle& rDestBounds, ControlState nState, const OUString& rCaption )
{
	BOOL bRet = FALSE;

	NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

	if ( pGraphics->mpFrame && !pGraphics->mpFrame->IsFloatingFrame() && pGraphics->mpFrame != GetSalData()->mpFocusFrame )
		nState |= CTRL_STATE_INACTIVE;

	VCLNativeComboBox *pVCLNativeComboBox = [VCLNativeComboBox createWithControlState:nState editable:NO bitmapBuffer:&aSharedListBoxBuffer graphics:pGraphics destRect:CGRectMake( rDestBounds.Left(), rDestBounds.Top(), rDestBounds.GetWidth(), rDestBounds.GetHeight() )];
	NSArray *pModes = [NSArray arrayWithObjects:NSDefaultRunLoopMode, NSEventTrackingRunLoopMode, NSModalPanelRunLoopMode, @"AWTRunLoopMode", nil];
	[pVCLNativeComboBox performSelectorOnMainThread:@selector(draw:) withObject:pVCLNativeComboBox waitUntilDone:YES modes:pModes];
	bRet = [pVCLNativeComboBox drawn];

	[pPool release];

	return bRet;
}

// =======================================================================

/**
 * (static) Draw a scrollbar into the graphics port at the specified location.
 * Swing scrollbars can lead to some odd tracking issues and don't provide
 * easy access to all of the individual subparts of the control proper,
 * so we'll draw them using HIThemes and SalBitmaps.
 *
 * @param pGraphics			pointer to the destination graphics where we'll be drawing
 * @param rDestBounds		eventual destination rectangle for the scrollbar
 * @param nState			overall scrollbar state (active vs. disabled)
 * @param pScrollbarValue	VCL scrollbar info value
 * @return TRUE if successful, FALSE on error
 */
static BOOL DrawNativeScrollBar( JavaSalGraphics *pGraphics, const Rectangle& rDestBounds, ControlState nState, ScrollbarValue *pScrollbarValue )
{
	BOOL bRet = FALSE;

	NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

	VCLBitmapBuffer *pBuffer;
	if ( rDestBounds.GetWidth() > rDestBounds.GetHeight() )
		pBuffer = &aSharedHorizontalScrollBarBuffer;
	else
		pBuffer = &aSharedVerticalScrollBarBuffer;

	if ( pGraphics->mpFrame && !pGraphics->mpFrame->IsFloatingFrame() && pGraphics->mpFrame != GetSalData()->mpFocusFrame )
		nState |= CTRL_STATE_INACTIVE;

	VCLNativeScrollbar *pVCLNativeScrollbar = [VCLNativeScrollbar createWithControlState:nState bitmapBuffer:pBuffer graphics:pGraphics scrollbarValue:pScrollbarValue doubleScrollbarArrows:GetSalData()->mbDoubleScrollbarArrows destRect:CGRectMake( rDestBounds.Left(), rDestBounds.Top(), rDestBounds.GetWidth(), rDestBounds.GetHeight() )];
	NSArray *pModes = [NSArray arrayWithObjects:NSDefaultRunLoopMode, NSEventTrackingRunLoopMode, NSModalPanelRunLoopMode, @"AWTRunLoopMode", nil];
	[pVCLNativeScrollbar performSelectorOnMainThread:@selector(draw:) withObject:pVCLNativeScrollbar waitUntilDone:YES modes:pModes];
	bRet = [pVCLNativeScrollbar drawn];

	[pPool release];

	return bRet;
}

// =======================================================================

/**
 * (static) Draw a spinbox using native controls.  This consists of a set of
 * stepper arrows on the right portion of the control and an edit field on
 * the left.
 *
 * @param pGraphics			pointer into graphics object where spinbox should
 *							be painted
 * @param rDestBounds		destination rectangle where object will be painted
 * @param nState			overall control state
 * @param pValue			optional value giving enabled & pressed state for
 *							subcontrols.
 * @return TRUE if drawing successful, FALSE if not
 */
static BOOL DrawNativeSpinbox( JavaSalGraphics *pGraphics, const Rectangle& rDestBounds, ControlState nState, SpinbuttonValue *pValue )
{
	SInt32 spinnerThemeWidth;
	SInt32 spinnerThemeHeight;
	BOOL bRet = ( pGetThemeMetric( kThemeMetricLittleArrowsWidth, &spinnerThemeWidth ) == noErr && pGetThemeMetric( kThemeMetricLittleArrowsHeight, &spinnerThemeHeight) == noErr );
	if ( bRet )
	{
		spinnerThemeHeight += SPINNER_TRIMHEIGHT * 2;
		long offscreenHeight = ( ( rDestBounds.GetHeight() > spinnerThemeHeight ) ? rDestBounds.GetHeight() : spinnerThemeHeight );

		VCLBitmapBuffer *pBuffer = &aSharedSpinboxBuffer;
		bRet = pBuffer->Create( rDestBounds.Left(), rDestBounds.Top(), rDestBounds.GetWidth(), offscreenHeight, pGraphics, offscreenHeight == rDestBounds.GetHeight() );
		if ( bRet )
		{
			if ( pGraphics->mpFrame && !pGraphics->mpFrame->IsFloatingFrame() && pGraphics->mpFrame != GetSalData()->mpFocusFrame )
				nState &= ~CTRL_STATE_ENABLED;

			HIThemeButtonDrawInfo aButtonDrawInfo;
			InitSpinbuttonDrawInfo( &aButtonDrawInfo, nState, pValue );

			HIRect arrowRect;
			arrowRect.origin.x = rDestBounds.GetWidth() - spinnerThemeWidth - SPINNER_TRIMWIDTH;
			if( arrowRect.origin.x < 0 )
				arrowRect.origin.x = 0;
			arrowRect.origin.y = ( ( offscreenHeight - spinnerThemeHeight ) / 2 ) - SPINNER_TRIMHEIGHT;
			arrowRect.size.width = spinnerThemeWidth + ( SPINNER_TRIMWIDTH * 2 );
			arrowRect.size.height = spinnerThemeHeight;

			bRet = ( pHIThemeDrawButton( &arrowRect, &aButtonDrawInfo, pBuffer->maContext, pBuffer->mnHIThemeOrientationFlags, NULL ) == noErr );

			if( bRet )
			{
				HIRect editRect;
				editRect.origin.x = FOCUSRING_WIDTH;
				editRect.origin.y = FOCUSRING_WIDTH;
				editRect.size.width = rDestBounds.GetWidth() - arrowRect.size.width - ( FOCUSRING_WIDTH * 2 );
				editRect.size.height = offscreenHeight - ( FOCUSRING_WIDTH * 2 );

				// erase out our background first

				float whiteColor[] = { 1.0, 1.0, 1.0, 1.0 };
				CGContextSetFillColor( pBuffer->maContext, whiteColor );
				CGContextFillRect( pBuffer->maContext, editRect );

				// draw our edit frame

				HIThemeFrameDrawInfo aFrameInfo;
				memset( &aFrameInfo, 0, sizeof( HIThemeFrameDrawInfo ) );

				aFrameInfo.kind = kHIThemeFrameTextFieldSquare;
				if( ! nState )
					aFrameInfo.state = kThemeStateInactive;
				else
					aFrameInfo.state = kThemeStateActive;
				if( nState & CTRL_STATE_FOCUSED )
					aFrameInfo.isFocused = TRUE;
				else
					aFrameInfo.isFocused = FALSE;

				bRet = ( pHIThemeDrawFrame( &editRect, &aFrameInfo, pBuffer->maContext, pBuffer->mnHIThemeOrientationFlags ) == noErr );
			}
		}

		pBuffer->ReleaseContext();

		if ( bRet )
			pBuffer->DrawContextAndDestroy( pGraphics, CGRectMake( 0, 0, rDestBounds.GetWidth(), offscreenHeight ), CGRectMake( rDestBounds.Left(), rDestBounds.Top(), rDestBounds.GetWidth(), rDestBounds.GetHeight() ) );
	}

	return bRet;
}

// =======================================================================

/**
 * (static) Draw a spinbutton using Aqua control scaling.  This is a set of
 * stepper arrows 
 *
 * @param pGraphics			pointer into graphics object where spinbutton should
 *							be painted
 * @param rDestBounds		destination rectangle where object will be painted
 * @param nState			overall control state
 * @param pValue			optional value giving enabled & pressed state for
 *							subcontrols.
 * @return TRUE if drawing successful, FALSE if not
 */
static BOOL DrawNativeSpinbutton( JavaSalGraphics *pGraphics, const Rectangle& rDestBounds, ControlState nState, SpinbuttonValue *pValue )
{
	BOOL bRet = FALSE;

	NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

	if ( pGraphics->mpFrame && !pGraphics->mpFrame->IsFloatingFrame() && pGraphics->mpFrame != GetSalData()->mpFocusFrame )
		nState |= CTRL_STATE_INACTIVE;

	VCLNativeSpinbuttons *pVCLNativeSpinbuttons = [VCLNativeSpinbuttons createWithControlState:nState bitmapBuffer:&aSharedSpinbuttonBuffer graphics:pGraphics spinbuttonValue:pValue destRect:CGRectMake( rDestBounds.Left(), rDestBounds.Top(), rDestBounds.GetWidth(), rDestBounds.GetHeight() )];
	NSArray *pModes = [NSArray arrayWithObjects:NSDefaultRunLoopMode, NSEventTrackingRunLoopMode, NSModalPanelRunLoopMode, @"AWTRunLoopMode", nil];
	[pVCLNativeSpinbuttons performSelectorOnMainThread:@selector(draw:) withObject:pVCLNativeSpinbuttons waitUntilDone:YES modes:pModes];
	bRet = [pVCLNativeSpinbuttons drawn];

	[pPool release];

	return bRet;
}

// =======================================================================

/**
 * (static) Draw a progress bar using the native widget appearance.  The
 * progressbar indicates percentage of task completion.
 *
 * @param pGraphics			pointer into graphics object where spinbox should
 *							be painted
 * @param rDestBounds		destination rectangle where object will be painted
 * @param nState			overall control state
 * @param pValue			value providing the percentage completion and other
 *							progressbar state
 * @param bSmall			TRUE to use small progress bar otherwise use large
 * @return TRUE if drawing successful, FALSE if not
 */
static BOOL DrawNativeProgressbar( JavaSalGraphics *pGraphics, const Rectangle& rDestBounds, ControlState nState, ProgressbarValue *pValue, BOOL bSmall )
{
	BOOL bRet = FALSE;

	NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

	if ( pGraphics->mpFrame && !pGraphics->mpFrame->IsFloatingFrame() && pGraphics->mpFrame != GetSalData()->mpFocusFrame )
		nState |= CTRL_STATE_INACTIVE;

	VCLNativeProgressbar *pVCLNativeProgressbar = [VCLNativeProgressbar createWithControlState:nState controlSize:( bSmall ? NSSmallControlSize : NSRegularControlSize ) bitmapBuffer:&aSharedProgressbarBuffer graphics:pGraphics progressbarValue:pValue destRect:CGRectMake( rDestBounds.Left(), rDestBounds.Top(), rDestBounds.GetWidth(), rDestBounds.GetHeight() )];
	NSArray *pModes = [NSArray arrayWithObjects:NSDefaultRunLoopMode, NSEventTrackingRunLoopMode, NSModalPanelRunLoopMode, @"AWTRunLoopMode", nil];
	[pVCLNativeProgressbar performSelectorOnMainThread:@selector(draw:) withObject:pVCLNativeProgressbar waitUntilDone:YES modes:pModes];
	bRet = [pVCLNativeProgressbar drawn];

	[pPool release];

	return bRet;
}

// =======================================================================

/**
 * (static) Draw an individual tab control using the native widget appearance.
 * The tab consists of only a single element in a list of tabs that appear
 * at the top of a full tab group.  Each tab gets drawn independently.
 *
 * @param pGraphics			pointer into graphics object where spinbox should
 *							be painted
 * @param rDestBounds		destination rectangle where object will be painted
 * @param nState			overall control state
 * @param pValue			value providing the tab style, its position in
 *							the tab order relative to other tabs, and other
 *							tab-specific values
 * @return TRUE if drawing successful, FALSE if not
 */
static BOOL DrawNativeTab( JavaSalGraphics *pGraphics, const Rectangle& rDestBounds, ControlState nState, TabitemValue *pValue )
{
	VCLBitmapBuffer *pBuffer = &aSharedTabBuffer;
	BOOL bRet = pBuffer->Create( rDestBounds.Left(), rDestBounds.Top(), rDestBounds.GetWidth(), rDestBounds.GetHeight(), pGraphics );
	if ( bRet )
	{
		if ( pGraphics->mpFrame && !pGraphics->mpFrame->IsFloatingFrame() && pGraphics->mpFrame != GetSalData()->mpFocusFrame )
			nState &= ~CTRL_STATE_ENABLED;

		HIThemeTabDrawInfo pTabDrawInfo;
		InitTabDrawInfo( &pTabDrawInfo, nState, pValue );

		HIRect destRect;
		destRect.origin.x = 0;
		destRect.origin.y = 0;
		destRect.size.width = rDestBounds.GetWidth();
		destRect.size.height = rDestBounds.GetHeight();

		HIRect labelRect; // ignored

		bRet = ( pHIThemeDrawTab( &destRect, (HIThemeTabDrawInfo *)&pTabDrawInfo, pBuffer->maContext, pBuffer->mnHIThemeOrientationFlags, &labelRect ) == noErr );
	}

	pBuffer->ReleaseContext();

	if ( bRet )
		pBuffer->DrawContextAndDestroy( pGraphics, CGRectMake( 0, 0, rDestBounds.GetWidth(), rDestBounds.GetHeight() ), CGRectMake( rDestBounds.Left(), rDestBounds.Top(), rDestBounds.GetWidth(), rDestBounds.GetHeight() ) );

	return bRet;
}

// =======================================================================

/**
 * (static) Draw a native tab box.  This includes the rounded boundary and
 * flat grey tab background.
 *
 * @param pGraphics			pointer into graphics object where spinbox should
 *							be painted
 * @param rDestBounds		destination rectangle where object will be painted
 * @param nState			overall control state
 */
static BOOL DrawNativeTabBoundingBox( JavaSalGraphics *pGraphics, const Rectangle& rDestBounds, ControlState nState )
{
	VCLBitmapBuffer *pBuffer = &aSharedTabBoundingBoxBuffer;
	BOOL bRet = pBuffer->Create( rDestBounds.Left(), rDestBounds.Top(), rDestBounds.GetWidth(), rDestBounds.GetHeight(), pGraphics );
	if ( bRet )
	{
		if ( pGraphics->mpFrame && !pGraphics->mpFrame->IsFloatingFrame() && pGraphics->mpFrame != GetSalData()->mpFocusFrame )
			nState &= ~CTRL_STATE_ENABLED;

		HIThemeTabPaneDrawInfo pTabPaneDrawInfo;
		InitTabPaneDrawInfo( &pTabPaneDrawInfo, nState );

		HIRect destRect;
		destRect.origin.x = 0;
		destRect.origin.y = 0;
		destRect.size.width = rDestBounds.GetWidth();
		destRect.size.height = rDestBounds.GetHeight();

		bRet = ( pHIThemeDrawTabPane( &destRect, (HIThemeTabPaneDrawInfo *)&pTabPaneDrawInfo, pBuffer->maContext, pBuffer->mnHIThemeOrientationFlags ) == noErr );
	}

	pBuffer->ReleaseContext();

	if ( bRet )
		pBuffer->DrawContextAndDestroy( pGraphics, CGRectMake( 0, 0, rDestBounds.GetWidth(), rDestBounds.GetHeight() ), CGRectMake( rDestBounds.Left(), rDestBounds.Top(), rDestBounds.GetWidth(), rDestBounds.GetHeight() ) );

	return bRet;
}

// =======================================================================

/**
 * (static) Draw a primary group box.  This includes the rounded boundary used
 * to group controls together.
 *
 * @param pGraphics			pointer into graphics object where box should
 *							be painted
 * @param rDestBounds		destination rectangle where object will be painted
 * @param nState			overall control state
 */
static BOOL DrawNativePrimaryGroupBox( JavaSalGraphics *pGraphics, const Rectangle& rDestBounds, ControlState nState )
{
	BOOL bRet = FALSE;

	NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

	if ( pGraphics->mpFrame && !pGraphics->mpFrame->IsFloatingFrame() && pGraphics->mpFrame != GetSalData()->mpFocusFrame )
		nState |= CTRL_STATE_INACTIVE;

	VCLNativeBox *pVCLNativeBox = [VCLNativeBox createWithControlState:nState bitmapBuffer:&aSharedPrimaryGroupBoxBuffer graphics:pGraphics destRect:CGRectMake( rDestBounds.Left(), rDestBounds.Top(), rDestBounds.GetWidth(), rDestBounds.GetHeight() )];
	NSArray *pModes = [NSArray arrayWithObjects:NSDefaultRunLoopMode, NSEventTrackingRunLoopMode, NSModalPanelRunLoopMode, @"AWTRunLoopMode", nil];
	[pVCLNativeBox performSelectorOnMainThread:@selector(draw:) withObject:pVCLNativeBox waitUntilDone:YES modes:pModes];
	bRet = [pVCLNativeBox drawn];

	[pPool release];

	return bRet;
}

// =======================================================================

/**
 * (static) Draw the background for a native menu.
 *
 * @param pGraphics			pointer into graphics object where box should
 *							be painted
 * @param rDestBounds		destination rectangle where object will be painted
 */
static BOOL DrawNativeMenuBackground( JavaSalGraphics *pGraphics, const Rectangle& rDestBounds )
{
	VCLBitmapBuffer *pBuffer = &aSharedMenuBackgroundBuffer;
	BOOL bRet = pBuffer->Create( rDestBounds.Left(), rDestBounds.Top(), rDestBounds.GetWidth(), rDestBounds.GetHeight(), pGraphics );
	if ( bRet )
	{
		HIThemeMenuDrawInfo pMenuDrawInfo;
		memset( &pMenuDrawInfo, 0, sizeof( pMenuDrawInfo ) );
		pMenuDrawInfo.version = 0;
		pMenuDrawInfo.menuType = kThemeMenuTypePopUp;

		HIRect destRect;
		destRect.origin.x = 0;
		destRect.origin.y = 0;
		destRect.size.width = rDestBounds.GetWidth();
		destRect.size.height = rDestBounds.GetHeight();

		bRet = ( pHIThemeDrawMenuBackground( &destRect, &pMenuDrawInfo, pBuffer->maContext, pBuffer->mnHIThemeOrientationFlags ) == noErr );
	}

	pBuffer->ReleaseContext();

	if ( bRet )
		pBuffer->DrawContextAndDestroy( pGraphics, CGRectMake( 0, 0, rDestBounds.GetWidth(), rDestBounds.GetHeight() ), CGRectMake( rDestBounds.Left(), rDestBounds.Top(), rDestBounds.GetWidth(), rDestBounds.GetHeight() ) );

	return bRet;
}

// =======================================================================

/**
 * (static) Draw the background for a native edit field.
 *
 * @param pGraphics			pointer into graphics object where box should
 *							be painted
 * @param rDestBounds		destination rectangle where object will be painted
 */
static BOOL DrawNativeEditBox( JavaSalGraphics *pGraphics, const Rectangle& rDestBounds, ControlState nState )
{
	VCLBitmapBuffer *pBuffer = &aSharedEditBoxBuffer;
	BOOL bRet = pBuffer->Create( rDestBounds.Left(), rDestBounds.Top(), rDestBounds.GetWidth(), rDestBounds.GetHeight(), pGraphics );
	if ( bRet )
	{
		if ( pGraphics->mpFrame && !pGraphics->mpFrame->IsFloatingFrame() && pGraphics->mpFrame != GetSalData()->mpFocusFrame )
			nState &= ~CTRL_STATE_ENABLED;

		HIThemeFrameDrawInfo pFrameInfo;
		InitEditFieldDrawInfo( &pFrameInfo, nState );

		HIRect destRect;
		destRect.origin.x = FOCUSRING_WIDTH;
		destRect.origin.y = FOCUSRING_WIDTH;
		destRect.size.width = rDestBounds.GetWidth() - 2*FOCUSRING_WIDTH;
		destRect.size.height = rDestBounds.GetHeight() - 2*FOCUSRING_WIDTH;

		// clear the active editing portion of the control
		float whiteColor[] = { 1.0, 1.0, 1.0, 1.0 };
		CGContextSetFillColor( pBuffer->maContext, whiteColor );
		CGContextFillRect( pBuffer->maContext, destRect );
		// draw frame around the background
		bRet = ( pHIThemeDrawFrame( &destRect, &pFrameInfo, pBuffer->maContext, pBuffer->mnHIThemeOrientationFlags ) == noErr );
	}

	pBuffer->ReleaseContext();

	if ( bRet )
		pBuffer->DrawContextAndDestroy( pGraphics, CGRectMake( 0, 0, rDestBounds.GetWidth(), rDestBounds.GetHeight() ), CGRectMake( rDestBounds.Left(), rDestBounds.Top(), rDestBounds.GetWidth(), rDestBounds.GetHeight() ) );

	return bRet;
}

// =======================================================================

/**
 * (static) Draw the background for a native lis view widget.
 *
 * @param pGraphics			pointer into graphics object where box should
 *							be painted
 * @param rDestBounds		destination rectangle where object will be painted
 */
static BOOL DrawNativeListBoxFrame( JavaSalGraphics *pGraphics, const Rectangle& rDestBounds, ControlState nState )
{
	BOOL bRet = FALSE;

	NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

	if ( pGraphics->mpFrame && !pGraphics->mpFrame->IsFloatingFrame() && pGraphics->mpFrame != GetSalData()->mpFocusFrame )
		nState |= CTRL_STATE_INACTIVE;

	VCLNativeScrollView *pVCLNativeScrollView = [VCLNativeScrollView createWithControlState:nState bitmapBuffer:&aSharedListViewFrameBuffer graphics:pGraphics destRect:CGRectMake( rDestBounds.Left(), rDestBounds.Top(), rDestBounds.GetWidth(), rDestBounds.GetHeight() )];
	NSArray *pModes = [NSArray arrayWithObjects:NSDefaultRunLoopMode, NSEventTrackingRunLoopMode, NSModalPanelRunLoopMode, @"AWTRunLoopMode", nil];
	[pVCLNativeScrollView performSelectorOnMainThread:@selector(draw:) withObject:pVCLNativeScrollView waitUntilDone:YES modes:pModes];
	bRet = [pVCLNativeScrollView drawn];

	[pPool release];

	return bRet;
}

// =======================================================================

/**
 * (static) Draw a native disclosure button used on the side of items in
 * hierarchical lists that can collapse and expand a container.
 *
 * @param pGraphics		pointer to the graphics object where the button should
 *						be painted
 * @param rDestBounds	destination drawing rectangle for the disclosure button
 * @param nState		current control enabled/disabled/focused state
 * @param pValue		NWF structure providing additional disclosure
 *						button state including alignment and collapsed/expanded
 *						state
 * @return OK if successful, FALSE on failure
 */
static BOOL DrawNativeDisclosureBtn( JavaSalGraphics *pGraphics, const Rectangle& rDestBounds, ControlState nState, DisclosureBtnValue *pValue )
{
	BOOL bRet = FALSE;

	NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

	NSInteger nButtonState;
	if ( pValue && pValue->mnOpenCloseState == DISCLOSUREBTN_OPEN )
		nButtonState = NSOnState;
	else
		nButtonState = NSOffState;

	if ( pGraphics->mpFrame && !pGraphics->mpFrame->IsFloatingFrame() && pGraphics->mpFrame != GetSalData()->mpFocusFrame )
		nState |= CTRL_STATE_INACTIVE;

	VCLNativeButton *pVCLNativeButton = [VCLNativeButton createWithButtonType:NSOnOffButton bezelStyle:NSDisclosureBezelStyle controlSize:NSRegularControlSize buttonState:nButtonState controlState:nState drawRTL:( pValue && pValue->mnAlignment == DISCLOSUREBTN_ALIGN_RIGHT ? YES : NO ) bitmapBuffer:&aSharedDisclosureBtnBuffer graphics:pGraphics destRect:CGRectMake( rDestBounds.Left(), rDestBounds.Top(), rDestBounds.GetWidth(), rDestBounds.GetHeight() )];
	NSArray *pModes = [NSArray arrayWithObjects:NSDefaultRunLoopMode, NSEventTrackingRunLoopMode, NSModalPanelRunLoopMode, @"AWTRunLoopMode", nil];
	[pVCLNativeButton performSelectorOnMainThread:@selector(draw:) withObject:pVCLNativeButton waitUntilDone:YES modes:pModes];
	bRet = [pVCLNativeButton drawn];

	[pPool release];

	return bRet;
}

// =======================================================================

/**
 * (static) Draw a native separator line.
 *
 * @param pGraphics		pointer to the graphics object where the line should
 *						be painted
 * @param rDestBounds	destination drawing rectangle for the separator
 * @param nState		current control enabled/disabled/focused state.
 *						really, this is meaningless for separators but is used
 *						by both VCL and Carbon, so we go for it
 */
static BOOL DrawNativeSeparatorLine( JavaSalGraphics *pGraphics, const Rectangle& rDestBounds, ControlState nState )
{
	VCLBitmapBuffer *pBuffer = &aSharedSeparatorLineBuffer;
	BOOL bRet = pBuffer->Create( rDestBounds.Left(), rDestBounds.Top(), rDestBounds.GetWidth(), rDestBounds.GetHeight(), pGraphics );
	if ( bRet )
	{
		if ( pGraphics->mpFrame && !pGraphics->mpFrame->IsFloatingFrame() && pGraphics->mpFrame != GetSalData()->mpFocusFrame )
			nState &= ~CTRL_STATE_ENABLED;

		HIThemeSeparatorDrawInfo pSepInfo;
		InitSeparatorDrawInfo( &pSepInfo, nState );

		HIRect destRect;
		destRect.origin.x = 0;
		destRect.origin.y = 0;
		destRect.size.width = rDestBounds.GetWidth();
		destRect.size.height = rDestBounds.GetHeight();

		bRet = ( pHIThemeDrawSeparator( &destRect, &pSepInfo, pBuffer->maContext, pBuffer->mnHIThemeOrientationFlags ) == noErr );
	}

	pBuffer->ReleaseContext();

	if ( bRet )
		pBuffer->DrawContextAndDestroy( pGraphics, CGRectMake( 0, 0, rDestBounds.GetWidth(), rDestBounds.GetHeight() ), CGRectMake( rDestBounds.Left(), rDestBounds.Top(), rDestBounds.GetWidth(), rDestBounds.GetHeight() ) );

	return bRet;
}

// =======================================================================

/**
 * (static) Draw a native header button for a list view.  The header buttons
 * are drawn using the theme brushes for primary sort columns and non-primary
 * columns.  Sort indicators are not drawn at present.
 *
 * @param pGraphics		pointer to the graphics object where the button should
 *						be painted
 * @param rDestBounds	destination drawing rectangle for the disclosure button
 * @param nState		current control enabled/disabled/focused state
 * @param pValue		NWF structure providing information about primary
 *						sort column and additional sort order state
 */
static BOOL DrawNativeListViewHeader( JavaSalGraphics *pGraphics, const Rectangle& rDestBounds, ControlState nState, ListViewHeaderValue *pValue )
{
	BOOL bRet = FALSE;

	NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

	if ( pGraphics->mpFrame && !pGraphics->mpFrame->IsFloatingFrame() && pGraphics->mpFrame != GetSalData()->mpFocusFrame )
		nState |= CTRL_STATE_INACTIVE;

	// If the mouse is pressed and in the list view header, set the pressed flag
	BOOL bRedraw = FALSE;
	if ( pGraphics->mpFrame && GetSalData()->maLastPointerState.mnState & MOUSE_LEFT )
	{
		Point aScreenPoint( GetSalData()->maLastPointerState.maPos );
		if ( rDestBounds.IsInside( Point( aScreenPoint.X() - pGraphics->mpFrame->maGeometry.nX, aScreenPoint.Y() - pGraphics->mpFrame->maGeometry.nY ) ) )
			nState |= CTRL_STATE_PRESSED;
		bRedraw = TRUE;
	}

	VCLNativeTableHeaderColumn *pVCLNativeTableHeaderColumn = [VCLNativeTableHeaderColumn createWithControlState:nState bitmapBuffer:&aSharedListViewHeaderBuffer graphics:pGraphics listViewHeaderValue:pValue destRect:CGRectMake( rDestBounds.Left(), rDestBounds.Top(), rDestBounds.GetWidth(), rDestBounds.GetHeight() )];
	NSArray *pModes = [NSArray arrayWithObjects:NSDefaultRunLoopMode, NSEventTrackingRunLoopMode, NSModalPanelRunLoopMode, @"AWTRunLoopMode", nil];
	[pVCLNativeTableHeaderColumn performSelectorOnMainThread:@selector(draw:) withObject:pVCLNativeTableHeaderColumn waitUntilDone:YES modes:pModes];
	bRet = [pVCLNativeTableHeaderColumn drawn];

	if ( bRet && bRedraw && pGraphics->mpFrame )
	{
		// Invalidate bounds to force redraw
		Window *pWindow = Application::GetFirstTopLevelWindow();
		while ( pWindow && pWindow->ImplGetFrame() != pGraphics->mpFrame )
			pWindow = Application::GetNextTopLevelWindow( pWindow );
		if ( pWindow && pWindow->IsReallyVisible() )
			pWindow->Invalidate( rDestBounds );
	}

	[pPool release];

	return bRet;
}

// =======================================================================

/**
 * (static) Draw a native bevel button frame.  This is used for buttons in
 * toolbars.
 *
 * @param pGraphics		pointer to the graphics object where the button should
 *						be painted
 * @param rDestBounds	destination drawing rectangle for the disclosure button
 * @param nState		current control enabled/disabled/focused state
 * @param aValue		control value
 */
static BOOL DrawNativeBevelButton( JavaSalGraphics *pGraphics, const Rectangle& rDestBounds, ControlState nState, const ImplControlValue& aValue )
{
	BOOL bRet = FALSE;

	NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

	NSInteger nButtonState;
	if ( aValue.getTristateVal() == BUTTONVALUE_ON )
		nButtonState = NSOnState;
	else
		nButtonState = NSOffState;

	if ( pGraphics->mpFrame && !pGraphics->mpFrame->IsFloatingFrame() && pGraphics->mpFrame != GetSalData()->mpFocusFrame )
		nState |= CTRL_STATE_INACTIVE;

	VCLNativeButton *pVCLNativeButton = [VCLNativeButton createWithButtonType:NSOnOffButton bezelStyle:NSShadowlessSquareBezelStyle controlSize:NSRegularControlSize buttonState:nButtonState controlState:nState drawRTL:NO bitmapBuffer:&aSharedBevelButtonBuffer graphics:pGraphics destRect:CGRectMake( rDestBounds.Left(), rDestBounds.Top(), rDestBounds.GetWidth(), rDestBounds.GetHeight() )];
	NSArray *pModes = [NSArray arrayWithObjects:NSDefaultRunLoopMode, NSEventTrackingRunLoopMode, NSModalPanelRunLoopMode, @"AWTRunLoopMode", nil];
	[pVCLNativeButton performSelectorOnMainThread:@selector(draw:) withObject:pVCLNativeButton waitUntilDone:YES modes:pModes];
	bRet = [pVCLNativeButton drawn];

	[pPool release];

	return bRet;
}

// =======================================================================

/**
 * (static) Draw a native checkbox.
 *
 * @param pGraphics		pointer to the graphics object where the checkbox should
 *						be painted
 * @param rDestBounds	destination drawing rectangle for the checkbox
 * @param nState		current control enabled/disabled/focused state
 * @param aValue		control value
 */
static BOOL DrawNativeCheckbox( JavaSalGraphics *pGraphics, const Rectangle& rDestBounds, ControlState nState, const ImplControlValue& aValue )
{
	BOOL bRet = FALSE;

	NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

	NSControlSize nControlSize = ( rDestBounds.GetWidth() < CHECKBOX_WIDTH - ( FOCUSRING_WIDTH * 2 ) || rDestBounds.GetHeight() < CHECKBOX_HEIGHT - ( FOCUSRING_WIDTH * 2 ) ? NSSmallControlSize : NSRegularControlSize );
	NSInteger nButtonState;
	if ( aValue.getTristateVal() == BUTTONVALUE_ON )
		nButtonState = NSOnState;
	else if ( aValue.getTristateVal() == BUTTONVALUE_MIXED )
		nButtonState = NSMixedState;
	else
		nButtonState = NSOffState;

	if ( pGraphics->mpFrame && !pGraphics->mpFrame->IsFloatingFrame() && pGraphics->mpFrame != GetSalData()->mpFocusFrame )
		nState |= CTRL_STATE_INACTIVE;

	VCLNativeButton *pVCLNativeButton = [VCLNativeButton createWithButtonType:NSSwitchButton bezelStyle:NSRoundedBezelStyle controlSize:nControlSize buttonState:nButtonState controlState:nState drawRTL:NO bitmapBuffer:&aSharedCheckboxBuffer graphics:pGraphics destRect:CGRectMake( rDestBounds.Left(), rDestBounds.Top(), rDestBounds.GetWidth(), rDestBounds.GetHeight() )];
	NSArray *pModes = [NSArray arrayWithObjects:NSDefaultRunLoopMode, NSEventTrackingRunLoopMode, NSModalPanelRunLoopMode, @"AWTRunLoopMode", nil];
	[pVCLNativeButton performSelectorOnMainThread:@selector(draw:) withObject:pVCLNativeButton waitUntilDone:YES modes:pModes];
	bRet = [pVCLNativeButton drawn];

	[pPool release];

	return bRet;
}

// =======================================================================

/**
 * (static) Draw a native radio button.
 *
 * @param pGraphics		pointer to the graphics object where the button should
 *						be painted
 * @param rDestBounds	destination drawing rectangle for the button
 * @param nState		current control enabled/disabled/focused state
 * @param aValue		control value
 */
static BOOL DrawNativeRadioButton( JavaSalGraphics *pGraphics, const Rectangle& rDestBounds, ControlState nState, const ImplControlValue& aValue )
{
	BOOL bRet = FALSE;

	NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

	NSControlSize nControlSize = ( rDestBounds.GetWidth() < RADIOBUTTON_WIDTH - ( FOCUSRING_WIDTH * 2 ) || rDestBounds.GetHeight() < RADIOBUTTON_HEIGHT - ( FOCUSRING_WIDTH * 2 ) ? NSSmallControlSize : NSRegularControlSize );
	NSInteger nButtonState;
	if ( aValue.getTristateVal() == BUTTONVALUE_ON )
		nButtonState = NSOnState;
	else if ( aValue.getTristateVal() == BUTTONVALUE_MIXED )
		nButtonState = NSMixedState;
	else
		nButtonState = NSOffState;

	if ( pGraphics->mpFrame && !pGraphics->mpFrame->IsFloatingFrame() && pGraphics->mpFrame != GetSalData()->mpFocusFrame )
		nState |= CTRL_STATE_INACTIVE;

	VCLNativeButton *pVCLNativeButton = [VCLNativeButton createWithButtonType:NSRadioButton bezelStyle:NSRoundedBezelStyle controlSize:nControlSize buttonState:nButtonState controlState:nState drawRTL:NO bitmapBuffer:&aSharedCheckboxBuffer graphics:pGraphics destRect:CGRectMake( rDestBounds.Left(), rDestBounds.Top(), rDestBounds.GetWidth(), rDestBounds.GetHeight() )];
	NSArray *pModes = [NSArray arrayWithObjects:NSDefaultRunLoopMode, NSEventTrackingRunLoopMode, NSModalPanelRunLoopMode, @"AWTRunLoopMode", nil];
	[pVCLNativeButton performSelectorOnMainThread:@selector(draw:) withObject:pVCLNativeButton waitUntilDone:YES modes:pModes];
	bRet = [pVCLNativeButton drawn];

	[pPool release];


	return bRet;
}

// =======================================================================

/**
 * (static) Draw a native push button.
 *
 * @param pGraphics		pointer to the graphics object where the button should
 *						be painted
 * @param rDestBounds	destination drawing rectangle for the button
 * @param nState		current control enabled/disabled/focused state
 * @param aValue		control value
 */
static BOOL DrawNativePushButton( JavaSalGraphics *pGraphics, const Rectangle& rDestBounds, ControlState nState, const ImplControlValue& aValue )
{
	BOOL bRet = FALSE;

	NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

	NSInteger nButtonState;
	if ( aValue.getTristateVal() == BUTTONVALUE_ON )
		nButtonState = NSOnState;
	else if ( aValue.getTristateVal() == BUTTONVALUE_MIXED )
		nButtonState = NSMixedState;
	else
		nButtonState = NSOffState;

	if ( pGraphics->mpFrame && !pGraphics->mpFrame->IsFloatingFrame() && pGraphics->mpFrame != GetSalData()->mpFocusFrame )
		nState |= CTRL_STATE_INACTIVE;

	VCLNativeButton *pVCLNativeButton = [VCLNativeButton createWithButtonType:NSMomentaryLightButton bezelStyle:NSRoundedBezelStyle controlSize:NSRegularControlSize buttonState:nButtonState controlState:nState drawRTL:NO bitmapBuffer:&aSharedCheckboxBuffer graphics:pGraphics destRect:CGRectMake( rDestBounds.Left(), rDestBounds.Top(), rDestBounds.GetWidth(), rDestBounds.GetHeight() )];
	NSArray *pModes = [NSArray arrayWithObjects:NSDefaultRunLoopMode, NSEventTrackingRunLoopMode, NSModalPanelRunLoopMode, @"AWTRunLoopMode", nil];
	[pVCLNativeButton performSelectorOnMainThread:@selector(draw:) withObject:pVCLNativeButton waitUntilDone:YES modes:pModes];
	bRet = [pVCLNativeButton drawn];

	if ( bRet && pGraphics->mpFrame && [pVCLNativeButton redraw] )
	{
		// Invalidate bounds to force redraw
		Window *pWindow = Application::GetFirstTopLevelWindow();
		while ( pWindow && pWindow->ImplGetFrame() != pGraphics->mpFrame )
			pWindow = Application::GetNextTopLevelWindow( pWindow );
		if ( pWindow && pWindow->IsReallyVisible() )
			pWindow->Invalidate( rDestBounds );
	}

	[pPool release];

	return bRet;
}

// =======================================================================

/**
 * (static) Exclude the window's grow box region from the specified region if
 * the window has a grow box.
 *
 * @param pGraphics		pointer to the graphics object where the region should
 *						be painted
 * @param nType         control flavor that is requsted to be drawn
 * @param rControlRegion the control's region
 */
static const Region GetRegionAdjustedForGrowBox( JavaSalGraphics *pGraphics, ControlType nType, const Region &rControlRegion )
{
	Region aRegion( rControlRegion );

	// Only adjust for grow box on pre-Mac OS X 10.7 releases
	if ( IsRunningSnowLeopard() && pGraphics->mpFrame && pGraphics->mpFrame->mnStyle & SAL_FRAME_STYLE_SIZEABLE )
	{
		HIPoint origin;
		origin.x = 0;
		origin.y = 0;
		HIThemeGrowBoxDrawInfo growBoxInfo;
		memset( &growBoxInfo, 0, sizeof( HIThemeGrowBoxDrawInfo ) );
		growBoxInfo.version = 0;
		growBoxInfo.state = kThemeStateActive;
		growBoxInfo.kind = kHIThemeGrowBoxKindNormal;
		growBoxInfo.direction = kThemeGrowLeft | kThemeGrowRight | kThemeGrowUp | kThemeGrowDown;
		growBoxInfo.size = kHIThemeGrowBoxSizeNormal;
		HIRect bounds;
		if ( pHIThemeGetGrowBoxBounds( &origin, &growBoxInfo, &bounds ) == noErr )
		{
			Rectangle boundingRect = aRegion.GetBoundRect();
			long nExcessWidth = boundingRect.Right() - pGraphics->mpFrame->maGeometry.nWidth + (long)bounds.size.width + 1;
			long nExcessHeight = boundingRect.Bottom() - pGraphics->mpFrame->maGeometry.nHeight + (long)bounds.size.height + 1;
			if ( nExcessWidth > 0 && nExcessHeight > 0 )
			{
				if ( nType == CTRL_SCROLLBAR && boundingRect.GetHeight() > boundingRect.GetWidth() )
				{
					if ( boundingRect.GetHeight() - nExcessHeight > 0 )
						boundingRect.Bottom() -= nExcessHeight;
				}
				else if ( nExcessWidth > 0 && boundingRect.GetWidth() - nExcessWidth > 0 )
				{
					boundingRect.Right() -= nExcessWidth;
				}
				else if ( nExcessHeight > 0 && boundingRect.GetHeight() - nExcessHeight > 0 )
				{
					boundingRect.Bottom() -= nExcessHeight;
				}
				aRegion = Region( boundingRect );
			}
		}
	}

	return aRegion;
}

// =======================================================================

/**
 * Determine if support exists for drawing a particular native widget in the
 * interface.
 *
 * @param nType         control flavor that is requsted to be drawn
 * @param nPart         subpart of the control that is requested to be drawn
 * @return TRUE if supported, FALSE if not
 */
BOOL JavaSalGraphics::IsNativeControlSupported( ControlType nType, ControlPart nPart )
{
	BOOL isSupported = FALSE;

#ifndef USE_NATIVE_CONTROLS
	return isSupported;
#endif	// !USE_NATIVE_CONTROLS

	if ( !HIThemeInitialize() )
		return isSupported;

	switch( nType )
	{
		case CTRL_PUSHBUTTON:
			if( nPart == PART_ENTIRE_CONTROL )
				isSupported = TRUE;
			break;

		case CTRL_RADIOBUTTON:
			if( nPart == PART_ENTIRE_CONTROL )
				isSupported = TRUE;
			break;

		case CTRL_CHECKBOX:
			if( nPart == PART_ENTIRE_CONTROL )
				isSupported = TRUE;
			break;

		case CTRL_COMBOBOX:
			if( ( nPart == PART_ENTIRE_CONTROL ) || ( nPart == HAS_BACKGROUND_TEXTURE ) )
				isSupported = TRUE;
			break;

		case CTRL_LISTBOX:
			if( ( nPart == PART_ENTIRE_CONTROL ) || ( nPart == HAS_BACKGROUND_TEXTURE ) )
				isSupported = TRUE;
			break;

		case CTRL_SCROLLBAR:
			if( ( nPart == PART_ENTIRE_CONTROL ) || nPart == PART_DRAW_BACKGROUND_HORZ || nPart == PART_DRAW_BACKGROUND_VERT )
				isSupported = TRUE;
			break;

		case CTRL_SPINBOX:
			if( nPart == PART_ENTIRE_CONTROL || nPart == PART_ALL_BUTTONS || nPart == HAS_BACKGROUND_TEXTURE )
				isSupported = TRUE;
			break;

		case CTRL_SPINBUTTONS:
			if( nPart == PART_ENTIRE_CONTROL || nPart == PART_ALL_BUTTONS )
				isSupported = TRUE;
			break;

		case CTRL_PROGRESS:
		case CTRL_INTROPROGRESS:
			if( nPart == PART_ENTIRE_CONTROL )
				isSupported = TRUE;
			break;

		case CTRL_TAB_ITEM:
			if( nPart == PART_ENTIRE_CONTROL )
				isSupported = TRUE;
			break;

		case CTRL_TAB_PANE:
			if( nPart == PART_ENTIRE_CONTROL )
				isSupported = TRUE;
			break;

		case CTRL_FIXEDBORDER:
		case CTRL_GROUPBOX:
			if( nPart == PART_ENTIRE_CONTROL )
				isSupported = TRUE;
			break;

		case CTRL_MENU_POPUP:
			if( nPart == PART_ENTIRE_CONTROL )
				isSupported = TRUE;
			break;

		case CTRL_EDITBOX:
			if( ( nPart == PART_ENTIRE_CONTROL ) || ( nPart == HAS_BACKGROUND_TEXTURE ) )
				isSupported = TRUE;
			break;

		case CTRL_DISCLOSUREBTN:
			if( nPart == PART_ENTIRE_CONTROL )
				isSupported = TRUE;
			break;

		case CTRL_LISTVIEWHEADER:
			if( ( nPart == PART_ENTIRE_CONTROL ) || ( nPart == PART_LISTVIEWHEADER_SORT_MARK ) )
				isSupported = TRUE;
			break;

		case CTRL_FIXEDLINE:
			if( nPart == PART_ENTIRE_CONTROL )
				isSupported = TRUE;
			break;

		case CTRL_LISTVIEWBOX:
			if( nPart == PART_ENTIRE_CONTROL )
				isSupported = TRUE;
			break;

		case CTRL_TOOLBAR:
			// Suppress the non-native toolbar background
			if( nPart == CTRL_PUSHBUTTON || nPart == PART_BUTTON )
				isSupported = TRUE;
			break;

#ifdef USE_NATIVE_CTRL_FRAME
		case CTRL_FRAME:
			if ( nPart == PART_BORDER )
				isSupported = TRUE;
			break;
#endif	// USE_NATIVE_CTRL_FRAME

		default:
			isSupported = FALSE;
			break;
	}

	return isSupported;
}

// =======================================================================

/**
 * Called to perform hit testing on native widgets.  If the widget itself
 * conforms to standard OpenOffice.org hit testing and subcontrol positions,
 * overloading the OOo hit testing should not be necessary.
 *
 * @param nType                 primary type of control to be hit tested
 * @param nPart                 subportion of the control to be hit tested
 * @param rControlRegion
 * @param aPos                  coordinate to hit test
 * @param rControlHandle
 * @param rIsInside             return parameter indicating whether aPos was
 *                              within the control or not
 * @return TRUE if the function performed hit testing, FALSE if default OOo
 *      hit testing should be used instead
 */
BOOL JavaSalGraphics::hitTestNativeControl( ControlType nType, ControlPart nPart, const Region& rControlRegion, const Point& aPos, SalControlHandle& rControlHandle, BOOL& rIsInside )
{
	rIsInside = FALSE;

#ifndef USE_NATIVE_CONTROLS
	if ( !IsNativeControlSupported( nType, nPart ) )
		return rIsInside;
#endif	// !USE_NATIVE_CONTROLS

	if ( !HIThemeInitialize() )
		return rIsInside;

	// [ed] Scrollbars are a special case:  in order to get proper regions,
	// a full description of the scrollbar is required including its values
	// and visible width.  We'll rely on the VCL scrollbar, which queried
	// these regions, to perform our hit testing.

	if ( nType == CTRL_SCROLLBAR )
		return FALSE;

	Region aNativeBoundingRegion;
	Region aNativeContentRegion;
	if ( getNativeControlRegion( nType, nPart, rControlRegion, 0, ImplControlValue(), rControlHandle, OUString(), aNativeBoundingRegion, aNativeContentRegion ) )
	{
		rIsInside = aNativeBoundingRegion.IsInside( aPos );
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

// =======================================================================

/**
 * Draw a widget using the native platform appearance.
 *
 * @param nType                 type of control to draw
 * @param nPart			subpart of the control to draw
 * @param nState                current control state (e.g. pressed, disabled)
 * @param rControlRegion        bounding region of the entire control in VCL
 *				frame coordinates
 * @param aValue		An optional value used for certain control types
 * @param rControlHandle	Platform dependent control data
 * @param rCaption		Caption title or string for the control.
 *				Contains keyboard shortcuts prefixed with ~
 * @return TRUE if drawing was successful, FALSE if drawing was not successful
 */
BOOL JavaSalGraphics::drawNativeControl( ControlType nType, ControlPart nPart, const Region& rControlRegion, ControlState nState, const ImplControlValue& aValue, SalControlHandle& rControlHandle, const OUString& rCaption )
{
	BOOL bOK = FALSE;

#ifndef USE_NATIVE_CONTROLS
	if ( !IsNativeControlSupported( nType, nPart ) )
		return bOK;
#endif	// !USE_NATIVE_CONTROLS

	if ( !HIThemeInitialize() )
		return bOK;

	NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

	const Region &rRealControlRegion = GetRegionAdjustedForGrowBox( this, nType, rControlRegion );

	switch( nType )
	{
		case CTRL_PUSHBUTTON:
			if( nPart == PART_ENTIRE_CONTROL )
			{
				Rectangle buttonRect = rRealControlRegion.GetBoundRect();
				bOK = DrawNativePushButton( this, buttonRect, nState, aValue );
			}
			break;

		case CTRL_RADIOBUTTON:
			if( nPart == PART_ENTIRE_CONTROL )
			{
				Rectangle buttonRect = rRealControlRegion.GetBoundRect();
				bOK = DrawNativeRadioButton( this, buttonRect, nState, aValue );
			}
			break;

		case CTRL_CHECKBOX:
			if( nPart == PART_ENTIRE_CONTROL )
			{
				Rectangle buttonRect = rRealControlRegion.GetBoundRect();
				bOK = DrawNativeCheckbox( this, buttonRect, nState, aValue );
			}
			break;

		case CTRL_COMBOBOX:
			if( nPart == PART_ENTIRE_CONTROL )
			{
				Rectangle buttonRect = rRealControlRegion.GetBoundRect();
				bOK = DrawNativeComboBox( this, buttonRect, nState, rCaption );
			}
			break;

		case CTRL_LISTBOX:
			if( nPart == PART_ENTIRE_CONTROL )
			{
				Rectangle buttonRect = rRealControlRegion.GetBoundRect();
				bOK = DrawNativeListBox( this, buttonRect, nState, rCaption );
			}
			break;

		case CTRL_SCROLLBAR:
			if( ( nPart == PART_ENTIRE_CONTROL) || ( nPart == PART_DRAW_BACKGROUND_HORZ ) || ( nPart == PART_DRAW_BACKGROUND_VERT ) )
			{
				Rectangle buttonRect = rRealControlRegion.GetBoundRect();
				ScrollbarValue *pValue = static_cast<ScrollbarValue *> ( aValue.getOptionalVal() );
				bOK = DrawNativeScrollBar( this, buttonRect, nState, pValue );
			}
			break;

		case CTRL_SPINBOX:
			if( ( nPart == PART_ENTIRE_CONTROL ) || ( nPart == PART_ALL_BUTTONS ) )
			{
				Rectangle buttonRect = rRealControlRegion.GetBoundRect();
				SpinbuttonValue *pValue = static_cast<SpinbuttonValue *> ( aValue.getOptionalVal() );
				bOK = DrawNativeSpinbox( this, buttonRect, nState, pValue );
			}
			break;

		case CTRL_SPINBUTTONS:
			if( ( nPart == PART_ENTIRE_CONTROL ) || ( nPart == PART_ALL_BUTTONS ) )
			{
				Rectangle buttonRect = rRealControlRegion.GetBoundRect();
				SpinbuttonValue *pValue = static_cast<SpinbuttonValue *> ( aValue.getOptionalVal() );
				bOK = DrawNativeSpinbutton( this, buttonRect, nState, pValue );
			}
			break;

		case CTRL_PROGRESS:
		case CTRL_INTROPROGRESS:
			if( nPart == PART_ENTIRE_CONTROL )
			{
				Rectangle ctrlRect = rRealControlRegion.GetBoundRect();
				ProgressbarValue aProgressbarValue;
				ProgressbarValue *pValue = static_cast<ProgressbarValue *> ( aValue.getOptionalVal() );
				if ( pValue )
				{
					aProgressbarValue.mbIndeterminate = pValue->mbIndeterminate;
					aProgressbarValue.mdPercentComplete = pValue->mdPercentComplete;
				}
				else
				{
					aProgressbarValue.mdPercentComplete = (double)( aValue.getNumericVal() * 100 / ctrlRect.GetWidth() );
				}

				bOK = DrawNativeProgressbar( this, ctrlRect, nState, &aProgressbarValue, nType == CTRL_INTROPROGRESS ? TRUE : FALSE );
			}
			break;

		case CTRL_TAB_ITEM:
			if( nPart == PART_ENTIRE_CONTROL )
			{
				Rectangle ctrlRect = rRealControlRegion.GetBoundRect();
				TabitemValue *pValue = static_cast<TabitemValue *> ( aValue.getOptionalVal() );
				bOK = DrawNativeTab( this, ctrlRect, nState, pValue );
			}
			break;

		case CTRL_TAB_PANE:
			if( nPart == PART_ENTIRE_CONTROL )
			{
				Rectangle ctrlRect = rRealControlRegion.GetBoundRect();
				// hack - on 10.3+ tab panes visually need to intersect the
				// middle of the associated segmented control.  Subtract
				// 15 off the height to shoehorn the drawing in.
				ctrlRect.setY( ctrlRect.getY() - CONTROL_TAB_PANE_TOP_OFFSET );
				ctrlRect.setHeight( ctrlRect.getHeight() + CONTROL_TAB_PANE_TOP_OFFSET );
				bOK = DrawNativeTabBoundingBox( this, ctrlRect, nState );
			}
			break;

		case CTRL_FIXEDBORDER:
		case CTRL_GROUPBOX:
			if( nPart == PART_ENTIRE_CONTROL )
			{
				Rectangle ctrlRect = rRealControlRegion.GetBoundRect();
				bOK = DrawNativePrimaryGroupBox( this, ctrlRect, nState );
			}
			break;

		case CTRL_MENU_POPUP:
			if ( nPart == PART_ENTIRE_CONTROL )
			{
				Rectangle ctrlRect = rRealControlRegion.GetBoundRect();
				bOK = DrawNativeMenuBackground( this, ctrlRect );
			}
			break;

		case CTRL_EDITBOX:
			if( nPart == PART_ENTIRE_CONTROL )
			{
				Rectangle ctrlRect = rRealControlRegion.GetBoundRect();
				bOK = DrawNativeEditBox( this, ctrlRect, nState );
			}
			break;

		case CTRL_DISCLOSUREBTN:
			if( nPart == PART_ENTIRE_CONTROL )
			{
				Rectangle ctrlRect = rRealControlRegion.GetBoundRect();
				DisclosureBtnValue *pValue = static_cast<DisclosureBtnValue *> ( aValue.getOptionalVal() );
				bOK = DrawNativeDisclosureBtn( this, ctrlRect, nState, pValue );
			}
			break;

		case CTRL_LISTVIEWHEADER:
			if( nPart == PART_ENTIRE_CONTROL )
			{
				Rectangle ctrlRect = rRealControlRegion.GetBoundRect();
				ListViewHeaderValue *pValue = static_cast<ListViewHeaderValue *> ( aValue.getOptionalVal() );
				bOK = DrawNativeListViewHeader( this, ctrlRect, nState, pValue );
			}
			break;

		case CTRL_FIXEDLINE:
			if( nPart == PART_ENTIRE_CONTROL )
			{
				Rectangle ctrlRect = rRealControlRegion.GetBoundRect();
				bOK = DrawNativeSeparatorLine( this, ctrlRect, nState );
			}
			break;

		case CTRL_LISTVIEWBOX:
			if( nPart == PART_ENTIRE_CONTROL )
			{
				Rectangle ctrlRect = rRealControlRegion.GetBoundRect();
				bOK = DrawNativeListBoxFrame( this, ctrlRect, nState );
			}
			break;

		case CTRL_TOOLBAR:
			if( nPart == PART_BUTTON )
			{
				Rectangle ctrlRect = rRealControlRegion.GetBoundRect();
				bOK = DrawNativeBevelButton( this, ctrlRect, nState, aValue );
			}
			break;

		case CTRL_FRAME:
			if ( nPart == PART_BORDER )
			{
				USHORT nValue = (USHORT)aValue.getNumericVal();
				if ( ! ( nValue & ( FRAME_DRAW_MENU | FRAME_DRAW_WINDOWBORDER ) ) )
				{
					Rectangle ctrlRect = rRealControlRegion.GetBoundRect();
					ctrlRect.Left() -= FRAME_TRIMWIDTH;
					ctrlRect.Top() -= FRAME_TRIMWIDTH;
					if ( nValue & FRAME_DRAW_DOUBLEIN )
					{
						ctrlRect.Right() += FRAME_TRIMWIDTH;
						ctrlRect.Bottom() += FRAME_TRIMWIDTH;
					}
					bOK = DrawNativeListBoxFrame( this, ctrlRect, CTRL_STATE_ENABLED );
				}
			}
			break;
	}

	[pPool release];

	return bOK;
}

// =======================================================================

/**
 * Draws the requested text for a native control.  If the caption of the
 * control is drawn by drawNativeControl, this function does not need to be
 * implemented.
 *
 * @param nType			type of control to draw
 * @param nPart			subpart of the control to draw
 * @param rControlRegion	bounding region of the complete control in VCL frame coordiantes.
 * @param nState		current control state (e.g. pressed, disabled)
 * @param aValue		An optional value used for certain control types
 * @param rControlHandle	Platform dependent control data
 * @param rCaption		Caption title or string for the control.
 *				Contains keyboard shortcuts prefixed with ~
 * @return TRUE if the text was drawn, FALSE if the control had its text drawn
 *	with drawNativeControl()
 */
BOOL JavaSalGraphics::drawNativeControlText( ControlType nType, ControlPart nPart, const Region& rControlRegion, ControlState nState, const ImplControlValue& aValue, SalControlHandle& rControlHandle, const OUString& rCaption )
{
	return FALSE;
}

// =======================================================================

/**
 * Obtains information about the actual bounding area and available drawing
 * area of a native control.
 *
 * @param nType			type of control to query
 * @param nPart			subpart of the control to query
 * @param rControlRegion	bounding region of the control in VCL frame coordinates
 * @param nState		current control state (e.g. pressed, disabled)
 * @param aValue		An optional value used for certain control types
 * @param rControlHandle	Platform dependent control data
 * @param rCaption		Caption title or string for the control.
 *				Contains keyboard shortcuts prefixed with ~
 * @param rNativeBoundingRegion	return parameter that contains the true bounds
 *				of the control along with any platform specific
 *				control adornment (shadows, etc.)
 * @param rNativeContentRegion	return parameter that indicates the region which
 *				can safely be overdrawn with custom content
 *				without overdrawing the system adornments of
 *				the control
 * @return TRUE if appropriate information was returned about any native widget
 *	drawing areas, FALSE if the entire control region can be considered
 *	an accurate enough representation of the native widget area
 */
BOOL JavaSalGraphics::getNativeControlRegion( ControlType nType, ControlPart nPart, const Region& rControlRegion, ControlState nState, const ImplControlValue& aValue, SalControlHandle& rControlHandle, const OUString& rCaption, Region &rNativeBoundingRegion, Region &rNativeContentRegion )
{
	BOOL bReturn = FALSE;

#ifndef USE_NATIVE_CONTROLS
	if ( !IsNativeControlSupported( nType, nPart ) )
		return bReturn;
#endif	// !USE_NATIVE_CONTROLS

	if ( !HIThemeInitialize() )
		return bReturn;

	const Region &rRealControlRegion = GetRegionAdjustedForGrowBox( this, nType, rControlRegion );

	switch( nType )
	{
		case CTRL_PUSHBUTTON:
			if( nPart == PART_ENTIRE_CONTROL )
			{
				// If the button width is less than the preferred height,
				// assume that it's intended to be a "placard" type button with
				// an icon. In that case, return the requested height as it
				// will then draw it as a placard button instead of a rounded
				// button. This makes buttons used as parts of subcontrols
				// (combo boxes, small toolbar buttons) draw with the
				// appropriate style.
				NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

				VCLNativeButton *pVCLNativeButton = [VCLNativeButton createWithButtonType:NSMomentaryLightButton bezelStyle:NSRoundedBezelStyle controlSize:NSRegularControlSize buttonState:NSOffState controlState:0 drawRTL:NO bitmapBuffer:NULL graphics:NULL destRect:CGRectZero];
				NSArray *pModes = [NSArray arrayWithObjects:NSDefaultRunLoopMode, NSEventTrackingRunLoopMode, NSModalPanelRunLoopMode, @"AWTRunLoopMode", nil];
				[pVCLNativeButton performSelectorOnMainThread:@selector(getSize:) withObject:pVCLNativeButton waitUntilDone:YES modes:pModes];
				NSSize aSize = [pVCLNativeButton size];
				if ( !NSEqualSizes( aSize, NSZeroSize ) )
				{
					Rectangle buttonRect = rRealControlRegion.GetBoundRect();
					long buttonWidth = buttonRect.GetWidth();
					long buttonHeight = (long)aSize.height;
					if ( buttonHeight >= buttonWidth )
						buttonHeight = buttonRect.GetHeight();
					else if ( buttonHeight != buttonRect.GetHeight() && buttonRect.GetHeight() > 0 )
						buttonHeight = buttonRect.GetHeight();

					Point topLeft( (long)(buttonRect.Left() - FOCUSRING_WIDTH), (long)(buttonRect.Top() + ((buttonRect.GetHeight() - buttonHeight) / 2) - FOCUSRING_WIDTH) );
					Size boundsSize( (long)buttonWidth + ( FOCUSRING_WIDTH * 2 ), (long)buttonHeight + ( FOCUSRING_WIDTH * 2 ) );
					rNativeBoundingRegion = Region( Rectangle( topLeft, boundsSize ) );
					rNativeContentRegion = Region( rNativeBoundingRegion );
					bReturn = TRUE;
				}

				[pPool release];
			}
			break;

		case CTRL_RADIOBUTTON:
			if( nPart == PART_ENTIRE_CONTROL )
			{
				Rectangle buttonRect = rRealControlRegion.GetBoundRect();
				Point topLeft( (long)(buttonRect.Left() - FOCUSRING_WIDTH), (long)(buttonRect.Top() - FOCUSRING_WIDTH) );
				Size boundsSize( (long)RADIOBUTTON_WIDTH + ( FOCUSRING_WIDTH * 2 ), (long)RADIOBUTTON_HEIGHT + ( FOCUSRING_WIDTH * 2 ) );
				rNativeBoundingRegion = Region( Rectangle( topLeft, boundsSize ) );
				rNativeContentRegion = Region( rNativeBoundingRegion );
				bReturn = TRUE;
			}
			break;

		case CTRL_CHECKBOX:
			if( nPart == PART_ENTIRE_CONTROL )
			{
				Rectangle buttonRect = rRealControlRegion.GetBoundRect();
				Point topLeft( (long)(buttonRect.Left() - FOCUSRING_WIDTH), (long)(buttonRect.Top() - FOCUSRING_WIDTH) );
				Size boundsSize( (long)CHECKBOX_WIDTH + ( FOCUSRING_WIDTH * 2 ), (long)CHECKBOX_HEIGHT + ( FOCUSRING_WIDTH * 2 ) );
				rNativeBoundingRegion = Region( Rectangle( topLeft, boundsSize ) );
				rNativeContentRegion = Region( rNativeBoundingRegion );
				bReturn = TRUE;
			}
			break;

		case CTRL_COMBOBOX:
		case CTRL_LISTBOX:
			{
				Rectangle comboBoxRect = rRealControlRegion.GetBoundRect();
				long nHeightAdjust = ( COMBOBOX_HEIGHT - comboBoxRect.GetHeight() ) / 2;
				if ( nHeightAdjust > 0 )
				{
					comboBoxRect.Top() -= nHeightAdjust;
					comboBoxRect.Bottom() += nHeightAdjust;
				}

				bool bEditable = ( nType == CTRL_COMBOBOX ? YES : NO );

				NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

				VCLNativeComboBox *pVCLNativeComboBox = [VCLNativeComboBox createWithControlState:nState editable:bEditable bitmapBuffer:NULL graphics:NULL destRect:CGRectMake( comboBoxRect.Left(), comboBoxRect.Top(), comboBoxRect.GetWidth(), comboBoxRect.GetHeight() )];
				NSArray *pModes = [NSArray arrayWithObjects:NSDefaultRunLoopMode, NSEventTrackingRunLoopMode, NSModalPanelRunLoopMode, @"AWTRunLoopMode", nil];
				[pVCLNativeComboBox performSelectorOnMainThread:@selector(getSize:) withObject:pVCLNativeComboBox waitUntilDone:YES modes:pModes];
				NSSize aSize = [pVCLNativeComboBox size];
				if ( !NSEqualSizes( aSize, NSZeroSize ) )
				{
					// Vertically center the preferred bounds
					float fYAdjust = ( (float)comboBoxRect.GetHeight() - aSize.height ) / 2;
					NSRect preferredRect = NSMakeRect( comboBoxRect.Left(), comboBoxRect.Top() + fYAdjust, aSize.width > comboBoxRect.GetWidth() ? aSize.width : comboBoxRect.GetWidth(), aSize.height );

					switch( nPart )
					{
						case PART_ENTIRE_CONTROL:
							{
								// Fix clipping of edges by returning the
								// passed in bounds
								rNativeBoundingRegion = Region( comboBoxRect );
								rNativeContentRegion = Region( rNativeBoundingRegion );
								bReturn = TRUE;
							}
							break;

						case PART_BUTTON_DOWN:
							{
								if ( bEditable )
								{
									Point topLeft( (long)preferredRect.origin.x + (long)preferredRect.size.width - COMBOBOX_BUTTON_WIDTH - FOCUSRING_WIDTH, (long)preferredRect.origin.y );
									Size boundsSize( COMBOBOX_BUTTON_WIDTH + FOCUSRING_WIDTH, (long)preferredRect.size.height );
									rNativeBoundingRegion = Region( Rectangle( topLeft, boundsSize ) );
								}
								else
								{
									Point topLeft( (long)preferredRect.origin.x, (long)preferredRect.origin.y );
									Size boundsSize( (long)preferredRect.size.width, (long)preferredRect.size.height );
									rNativeBoundingRegion = Region( Rectangle( topLeft, boundsSize ) );
								}

								rNativeContentRegion = Region( rNativeBoundingRegion );
								bReturn = TRUE;
							}
							break;

						case PART_SUB_EDIT:
							{
								Point topLeft( (long)preferredRect.origin.x + FOCUSRING_WIDTH + EDITFRAMEPADDING_WIDTH, (long)preferredRect.origin.y + FOCUSRING_WIDTH + EDITFRAMEPADDING_WIDTH );
								Size boundsSize( (long)preferredRect.size.width - ( bEditable ? COMBOBOX_BUTTON_WIDTH : LISTBOX_BUTTON_WIDTH ) - ( ( FOCUSRING_WIDTH + EDITFRAMEPADDING_WIDTH ) * 2 ), (long)preferredRect.size.height - ( ( FOCUSRING_WIDTH + EDITFRAMEPADDING_WIDTH ) * 2 ) );
								rNativeBoundingRegion = Region( Rectangle( topLeft, boundsSize ) );
								rNativeContentRegion = Region( rNativeBoundingRegion );
								bReturn = TRUE;
							}
							break;
					}
				}

				[pPool release];
			}
			break;

		case CTRL_SCROLLBAR:
			{
				// Fix bug 1600 by detecting if double arrows are at both ends
				Rectangle scrollbarRect = rRealControlRegion.GetBoundRect();

				ScrollbarValue *pValue = static_cast<ScrollbarValue *> ( aValue.getOptionalVal() );
				bool bDoubleScrollbarArrows = GetSalData()->mbDoubleScrollbarArrows;

				NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

				VCLNativeScrollbar *pVCLNativeScrollbar = [VCLNativeScrollbar createWithControlState:nState bitmapBuffer:NULL graphics:NULL scrollbarValue:pValue doubleScrollbarArrows:bDoubleScrollbarArrows destRect:CGRectMake( scrollbarRect.Left(), scrollbarRect.Top(), scrollbarRect.GetWidth(), scrollbarRect.GetHeight() )];
				NSArray *pModes = [NSArray arrayWithObjects:NSDefaultRunLoopMode, NSEventTrackingRunLoopMode, NSModalPanelRunLoopMode, @"AWTRunLoopMode", nil];
				[pVCLNativeScrollbar performSelectorOnMainThread:@selector(getBounds:) withObject:pVCLNativeScrollbar waitUntilDone:YES modes:pModes];

				MacOSBOOL bHorizontal = [pVCLNativeScrollbar horizontal];
				NSRect aDecrementArrowBounds = [pVCLNativeScrollbar decrementArrowBounds];
				NSRect aIncrementArrowBounds = [pVCLNativeScrollbar incrementArrowBounds];
				NSRect aDecrementPageBounds = [pVCLNativeScrollbar decrementPageBounds];
				NSRect aIncrementPageBounds = [pVCLNativeScrollbar incrementPageBounds];
				NSRect aThumbBounds = [pVCLNativeScrollbar thumbBounds];
				NSRect aTrackBounds = [pVCLNativeScrollbar trackBounds];
				NSRect aTotalBounds = [pVCLNativeScrollbar totalBounds];
				aTotalBounds = NSUnionRect( aTotalBounds, aDecrementArrowBounds );
				aTotalBounds = NSUnionRect( aTotalBounds, aIncrementArrowBounds );
				aTotalBounds = NSUnionRect( aTotalBounds, aDecrementPageBounds );
				aTotalBounds = NSUnionRect( aTotalBounds, aIncrementPageBounds );
				aTotalBounds = NSUnionRect( aTotalBounds, aThumbBounds );
				aTotalBounds = NSUnionRect( aTotalBounds, aTrackBounds );

				NSRect bounds = NSZeroRect;
				switch ( nPart )
				{
					case PART_ENTIRE_CONTROL:
						bounds = aTotalBounds;
						break;

					case PART_BUTTON_LEFT:
					case PART_BUTTON_UP:
						bounds = aDecrementArrowBounds;
						if ( bDoubleScrollbarArrows )
						{
							if ( bHorizontal )
								bounds.size.width *= 2;
							else
								bounds.size.height *= 2;
						}
						break;

					case PART_BUTTON_RIGHT:
					case PART_BUTTON_DOWN:
						bounds = aIncrementArrowBounds;
						if ( bDoubleScrollbarArrows )
						{
							if ( bHorizontal )
							{
								bounds.origin.x -= bounds.size.width;
								bounds.size.width *= 2;
							}
							else
							{
								bounds.origin.y -= bounds.size.height;
								bounds.size.height *= 2;
							}
						}
						break;

					case PART_TRACK_HORZ_LEFT:
					case PART_TRACK_VERT_UPPER:
						// If bounds are empty, set to the entire track
						bounds = aDecrementPageBounds;
						if ( NSEqualRects( bounds, NSZeroRect ) )
							bounds = aTrackBounds;
						break;

					case PART_TRACK_HORZ_RIGHT:
					case PART_TRACK_VERT_LOWER:
						// If bounds are empty, set to the entire track
						bounds = aIncrementPageBounds;
						if ( NSEqualRects( bounds, NSZeroRect ) )
							bounds = aTrackBounds;
						break;

					case PART_THUMB_HORZ:
					case PART_THUMB_VERT:
						bounds = aThumbBounds;
						break;
					
					case PART_TRACK_HORZ_AREA:
					case PART_TRACK_VERT_AREA:
						bounds = aTrackBounds;
						break;
				}

				// Fix bug 2031 by incrementing the scrollbar width slightly
				if ( !NSEqualRects( bounds, NSZeroRect ) )
				{
					if ( bHorizontal )
						bounds.size.height++;
					else
						bounds.size.width++;
				}

				Point topLeft( (long)( scrollbarRect.Left() + bounds.origin.x ), (long)( scrollbarRect.Top() + bounds.origin.y ) );
				Size boundsSize( (long)bounds.size.width, (long)bounds.size.height );
				rNativeBoundingRegion = Region( Rectangle( topLeft, boundsSize ) );
				rNativeContentRegion = Region( rNativeBoundingRegion );

				bReturn = TRUE;

				[pPool release];
			}
			break;

		case CTRL_SPINBOX:
			{
				Rectangle spinboxRect = rRealControlRegion.GetBoundRect();
				long nHeightAdjust = ( EDITBOX_HEIGHT - spinboxRect.GetHeight() ) / 2;
				if ( nHeightAdjust > 0 )
				{
					spinboxRect.Top() -= nHeightAdjust;
					spinboxRect.Bottom() += nHeightAdjust;
				}

				// leave room for left edge adornments

				SInt32 spinnerThemeWidth;
				SInt32 spinnerThemeHeight;
				bReturn = ( pGetThemeMetric( kThemeMetricLittleArrowsWidth, &spinnerThemeWidth ) == noErr && pGetThemeMetric( kThemeMetricLittleArrowsHeight, &spinnerThemeHeight ) == noErr );
				if ( ! bReturn )
					return bReturn;

				spinnerThemeWidth += SPINNER_TRIMWIDTH * 2;

				switch( nPart )
				{
					case PART_ENTIRE_CONTROL:
						{
							rNativeBoundingRegion = Region( spinboxRect );
							rNativeContentRegion = Region( rNativeBoundingRegion );
							bReturn = TRUE;
						}
						break;

					case PART_BUTTON_UP:
						{
							Point topLeft( (long)( spinboxRect.Right() - spinnerThemeWidth ), (long)( spinboxRect.Top() + ( spinboxRect.GetHeight() / 2 ) - spinnerThemeHeight ) );
							Size boundsSize( (long)spinnerThemeWidth, (long)spinnerThemeHeight );
							rNativeBoundingRegion = Region( Rectangle( topLeft, boundsSize ) );
							rNativeContentRegion = Region( rNativeBoundingRegion );
							bReturn = TRUE;
						}
						break;

					case PART_BUTTON_DOWN:
						{
							Point topLeft( (long)( spinboxRect.Right() - spinnerThemeWidth ), (long)( spinboxRect.Top() + ( spinboxRect.GetHeight() / 2 ) ) );
							Size boundsSize( (long)spinnerThemeWidth, (long)spinnerThemeHeight );
							rNativeBoundingRegion = Region( Rectangle( topLeft, boundsSize ) );
							rNativeContentRegion = Region( rNativeBoundingRegion );
							bReturn = TRUE;
						}
						break;

					case PART_SUB_EDIT:
						{
							SInt32 editFramePadding;
							bReturn = ( pGetThemeMetric( kThemeMetricEditTextFrameOutset, &editFramePadding) == noErr );
							if ( ! bReturn )
								return bReturn;

							rNativeBoundingRegion = Region( Rectangle( Point( spinboxRect.Left() + editFramePadding, spinboxRect.Top() + editFramePadding ), Size( (long)( spinboxRect.GetWidth() - spinnerThemeWidth - 4 - ( editFramePadding * 2 ) ), spinboxRect.GetHeight() - ( editFramePadding * 2 ) ) ) );
							rNativeContentRegion = Region( rNativeBoundingRegion );
							bReturn = TRUE;
						}
						break;
				}
			}
			break;

		case CTRL_SPINBUTTONS:
			{
				Rectangle spinboxRect = rRealControlRegion.GetBoundRect();

				SpinbuttonValue *pValue = static_cast<SpinbuttonValue *> ( aValue.getOptionalVal() );

				NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

				VCLNativeSpinbuttons *pVCLNativeSpinbuttons = [VCLNativeSpinbuttons createWithControlState:nState bitmapBuffer:NULL graphics:NULL spinbuttonValue:pValue destRect:CGRectMake( spinboxRect.Left(), spinboxRect.Top(), spinboxRect.GetWidth(), spinboxRect.GetHeight() )];
				NSArray *pModes = [NSArray arrayWithObjects:NSDefaultRunLoopMode, NSEventTrackingRunLoopMode, NSModalPanelRunLoopMode, @"AWTRunLoopMode", nil];
				[pVCLNativeSpinbuttons performSelectorOnMainThread:@selector(getSize:) withObject:pVCLNativeSpinbuttons waitUntilDone:YES modes:pModes];
				NSSize aSize = [pVCLNativeSpinbuttons size];
				if ( !NSEqualSizes( aSize, NSZeroSize ) )
				{
					switch( nPart )
					{
						case PART_ENTIRE_CONTROL:
							{
								Point topLeft( (long)( spinboxRect.Right() - aSize.width - FOCUSRING_WIDTH ), (long)( spinboxRect.Top() + ( ( spinboxRect.GetHeight() - aSize.height ) / 2 ) - FOCUSRING_WIDTH ) );
								Size boundsSize( (long)( aSize.width + ( FOCUSRING_WIDTH * 2 ) ), (long)( aSize.height + ( FOCUSRING_WIDTH * 2 ) ) );
								rNativeBoundingRegion = Region( Rectangle( topLeft, boundsSize ) );
								rNativeContentRegion = Region( rNativeBoundingRegion );
								bReturn = TRUE;
							}
							break;

						case PART_BUTTON_UP:
							{
								Point topLeft( (long)( spinboxRect.Right() - aSize.width - FOCUSRING_WIDTH ), (long)( spinboxRect.Top() + ( ( spinboxRect.GetHeight() - aSize.height ) / 2 ) - FOCUSRING_WIDTH ) );
								Size boundsSize( (long)( aSize.width + ( FOCUSRING_WIDTH * 2 ) ), (long)( ( aSize.height / 2 ) + FOCUSRING_WIDTH ) );
								rNativeBoundingRegion = Region( Rectangle( topLeft, boundsSize ) );
								rNativeContentRegion = Region( rNativeBoundingRegion );
								bReturn = TRUE;
							}
							break;

						case PART_BUTTON_DOWN:
							{
								Point topLeft( (long)( spinboxRect.Right() - aSize.width - FOCUSRING_WIDTH ), (long)( spinboxRect.Top() + ( spinboxRect.GetHeight() / 2 ) ) );
								Size boundsSize( (long)( aSize.width + ( FOCUSRING_WIDTH * 2 ) ), (long)( ( aSize.height / 2 ) + FOCUSRING_WIDTH ) );
								rNativeBoundingRegion = Region( Rectangle( topLeft, boundsSize ) );
								rNativeContentRegion = Region( rNativeBoundingRegion );
								bReturn = TRUE;
							}
							break;
					}
				}

				[pPool release];
			}
			break;

		case CTRL_PROGRESS:
		case CTRL_INTROPROGRESS:
			if ( nPart == PART_ENTIRE_CONTROL )
			{
				Rectangle controlRect = rRealControlRegion.GetBoundRect();

				ProgressbarValue aProgressbarValue;
				ProgressbarValue *pValue = static_cast<ProgressbarValue *> ( aValue.getOptionalVal() );
				if ( pValue )
				{
					aProgressbarValue.mbIndeterminate = pValue->mbIndeterminate;
					aProgressbarValue.mdPercentComplete = pValue->mdPercentComplete;
				}
				else
				{
					aProgressbarValue.mdPercentComplete = (double)( aValue.getNumericVal() * 100 / controlRect.GetWidth() );
				}

				NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

				VCLNativeProgressbar *pVCLNativeProgressbar = [VCLNativeProgressbar createWithControlState:nState controlSize:( nType == CTRL_INTROPROGRESS ? NSSmallControlSize : NSRegularControlSize ) bitmapBuffer:NULL graphics:NULL progressbarValue:&aProgressbarValue destRect:CGRectMake( controlRect.Left(), controlRect.Top(), controlRect.GetWidth(), controlRect.GetHeight() )];
				NSArray *pModes = [NSArray arrayWithObjects:NSDefaultRunLoopMode, NSEventTrackingRunLoopMode, NSModalPanelRunLoopMode, @"AWTRunLoopMode", nil];
				[pVCLNativeProgressbar performSelectorOnMainThread:@selector(getSize:) withObject:pVCLNativeProgressbar waitUntilDone:YES modes:pModes];
				NSSize aSize = [pVCLNativeProgressbar size];
				if ( !NSEqualSizes( aSize, NSZeroSize ) )
				{
					// Vertically center the preferred bounds
					float fYAdjust = ( (float)controlRect.GetHeight() - aSize.height ) / 2;
					NSRect preferredRect = NSMakeRect( controlRect.Left(), controlRect.Top() + fYAdjust, aSize.width > controlRect.GetWidth() ? aSize.width : controlRect.GetWidth(), aSize.height );

					// Fix clipping of small progress bar by adding padding
					Point topLeft( (long)preferredRect.origin.x, (long)preferredRect.origin.y - PROGRESSBARPADDING_HEIGHT );
					Size boundsSize( (long)preferredRect.size.width, (long)preferredRect.size.height + ( PROGRESSBARPADDING_HEIGHT * 2 ) );
					rNativeBoundingRegion = Region( Rectangle( topLeft, boundsSize ) );
					rNativeContentRegion = Region( rNativeBoundingRegion );

					bReturn = TRUE;
				}

				[pPool release];
			}
			break;

		case CTRL_TAB_ITEM:
			if ( nPart == PART_ENTIRE_CONTROL )
			{
				Rectangle controlRect = rRealControlRegion.GetBoundRect();

				TabitemValue *pValue = static_cast<TabitemValue *> ( aValue.getOptionalVal() );

				HIThemeTabDrawInfo pTabDrawInfo;
				InitTabDrawInfo( &pTabDrawInfo, nState, pValue );

				HIRect proposedBounds;
				proposedBounds.origin.x = 0;
				proposedBounds.origin.y = 0;
				proposedBounds.size.width = controlRect.Right() - controlRect.Left();
				proposedBounds.size.height = controlRect.Bottom() - controlRect.Top();

				HIShapeRef tabShape;
				pHIThemeGetTabShape( &proposedBounds, (HIThemeTabDrawInfo *)&pTabDrawInfo, &tabShape );

				HIRect preferredRect;
				HIShapeGetBounds( tabShape, &preferredRect );
				CFRelease( tabShape );

				preferredRect.size.height += TABITEM_HEIGHT_SLOP;

				Point topLeft( controlRect.Left(), controlRect.Top() );
				Size boundsSize( (long)preferredRect.size.width, (long)preferredRect.size.height );
				rNativeBoundingRegion = Region( Rectangle( topLeft, boundsSize ) );
				rNativeContentRegion = Region( rNativeBoundingRegion );

				bReturn = TRUE;
			}
			break;

		case CTRL_TAB_PANE:
			if ( nPart == PART_ENTIRE_CONTROL )
			{
				// for now, assume tab panes will occupy the full rectangle and
				// not require bound adjustment.
				Rectangle controlRect = rRealControlRegion.GetBoundRect();
				rNativeBoundingRegion = Region( controlRect );
				rNativeContentRegion = Region( rNativeBoundingRegion );

				bReturn = TRUE;
			}
			break;

		case CTRL_FIXEDBORDER:
		case CTRL_GROUPBOX:
			if ( nPart == PART_ENTIRE_CONTROL )
			{
				// for now, assume primary group boxes will occupy the full rectangle and
				// not require bound adjustment.
				Rectangle controlRect = rRealControlRegion.GetBoundRect();
				rNativeBoundingRegion = Region( controlRect );
				rNativeContentRegion = Region( rNativeBoundingRegion );

				bReturn = TRUE;
			}
			break;

		case CTRL_MENU_POPUP:
			if ( nPart == PART_ENTIRE_CONTROL )
			{
				// we can draw menu backgrounds for any size rectangular area
				Rectangle controlRect = rRealControlRegion.GetBoundRect();
				rNativeBoundingRegion = Region( controlRect );
				rNativeContentRegion = Region( rNativeBoundingRegion );

				bReturn = TRUE;
			}
			break;

		case CTRL_EDITBOX:
			if ( nPart == PART_ENTIRE_CONTROL )
			{
				// fill entire control area with edit box
				Rectangle controlRect = rRealControlRegion.GetBoundRect();
				long nHeightAdjust = ( EDITBOX_HEIGHT - controlRect.GetHeight() ) / 2;
				if ( nHeightAdjust > 0 )
				{
					controlRect.Top() -= nHeightAdjust;
					controlRect.Bottom() += nHeightAdjust;
				}

				rNativeBoundingRegion = Region( controlRect );
				rNativeContentRegion = Region( rNativeBoundingRegion );

				bReturn = TRUE;
			}
			break;

		case CTRL_DISCLOSUREBTN:
			if ( nPart == PART_ENTIRE_CONTROL )
			{
				Rectangle controlRect = rRealControlRegion.GetBoundRect();
				rNativeBoundingRegion = Region( controlRect );
				rNativeContentRegion = Region( rNativeBoundingRegion );

				bReturn = TRUE;
			}
			break;

		case CTRL_LISTVIEWHEADER:
			if ( nPart == PART_ENTIRE_CONTROL )
			{
				Rectangle controlRect = rRealControlRegion.GetBoundRect();
				rNativeBoundingRegion = Region( controlRect );
				rNativeContentRegion = Region( rNativeBoundingRegion );

				bReturn = TRUE;
			}
			break;

		case CTRL_LISTVIEWBOX:
			if ( nPart == PART_ENTIRE_CONTROL )
			{
				Rectangle controlRect = rRealControlRegion.GetBoundRect();
				rNativeBoundingRegion = Region( controlRect );
				rNativeContentRegion = Region( rNativeBoundingRegion );

				bReturn = TRUE;
			}
			break;

		case CTRL_TOOLBAR:
			if ( nPart == PART_BUTTON )
			{
				Rectangle controlRect = rRealControlRegion.GetBoundRect();
				rNativeBoundingRegion = Region( controlRect );
				rNativeContentRegion = Region( rNativeBoundingRegion );

				bReturn = TRUE;
			}
			break;

		case CTRL_FRAME:
			if ( nPart == PART_BORDER )
			{
				USHORT nValue = (USHORT)aValue.getNumericVal();
				if ( ! ( nValue & ( FRAME_DRAW_MENU | FRAME_DRAW_WINDOWBORDER ) ) )
				{
					Rectangle controlRect = rRealControlRegion.GetBoundRect();
					controlRect.Left() += FRAME_TRIMWIDTH;
					controlRect.Top() += FRAME_TRIMWIDTH;
					if ( nValue & FRAME_DRAW_DOUBLEIN )
					{
						controlRect.Right() -= FRAME_TRIMWIDTH;
						controlRect.Bottom() -= FRAME_TRIMWIDTH;
					}
					rNativeBoundingRegion = Region( controlRect );
					rNativeContentRegion = Region( rNativeBoundingRegion );
					bReturn = TRUE;
				}
			}
			break;
	}

	return bReturn;
}

/**
 * Get the color that should be used to draw the textual element of a control.
 * This allows VCL controls that use widget renderig to get control backgrounds
 * and parts to use the correct color for the VCL rendered control text.
 *
 * @param nType		type of control whose text is being drawn
 * @param nPart		part of the control whose text is being drawn
 * @param nState	current state of the control
 * @param aValue	extra control-specific data of the ucrrent control state
 * @param textColor	location in which color to draw text should be returned
 * @return TRUE if a native widget text color is provided, FALSE if the standard
 *	VCL text color should be used.
 */
BOOL JavaSalGraphics::getNativeControlTextColor( ControlType nType, ControlPart nPart, ControlState nState, const ImplControlValue& aValue, SalColor& textColor )
{
	BOOL bReturn = FALSE;

#ifndef USE_NATIVE_CONTROLS
	if ( !IsNativeControlSupported( nType, nPart ) )
		return bReturn;
#endif	// !USE_NATIVE_CONTROLS

	switch( nType )
	{

		case CTRL_PUSHBUTTON:
		case CTRL_RADIOBUTTON:
		case CTRL_CHECKBOX:
		case CTRL_LISTBOX:
			{				
				if( nState & CTRL_STATE_PRESSED )
					bReturn = JavaSalFrame::GetSelectedControlTextColor( textColor );
				else if ( ! ( nState & CTRL_STATE_ENABLED ) )
					bReturn = JavaSalFrame::GetDisabledControlTextColor( textColor );
				else
					bReturn = JavaSalFrame::GetControlTextColor( textColor );
			}
			break;

		case CTRL_TAB_ITEM:
			{
				if ( nState & CTRL_STATE_SELECTED )
					bReturn = IsRunningSnowLeopard() ? JavaSalFrame::GetSelectedControlTextColor( textColor ) : JavaSalFrame::GetAlternateSelectedControlTextColor( textColor );
				else if ( nState & CTRL_STATE_PRESSED )
					bReturn = JavaSalFrame::GetSelectedControlTextColor( textColor );
				else if ( ! ( nState & CTRL_STATE_ENABLED ) )
					bReturn = JavaSalFrame::GetDisabledControlTextColor( textColor );
				else
					bReturn = JavaSalFrame::GetControlTextColor( textColor );
			}
			break;

		case CTRL_MENU_POPUP:
			{
				if ( nState & CTRL_STATE_SELECTED )
					bReturn = JavaSalFrame::GetSelectedMenuItemTextColor( textColor );
				else if ( ! ( nState & CTRL_STATE_ENABLED ) )
					bReturn = JavaSalFrame::GetDisabledControlTextColor( textColor );
				else
					bReturn = JavaSalFrame::GetControlTextColor( textColor );
			}
			break;
	}

	return bReturn;
}
