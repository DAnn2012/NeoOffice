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

#import <set>

#include <comphelper/sequenceashashmap.hxx>
#include <sfx2/app.hxx>
#include <svtools/dynamicmenuoptions.hxx>
#include <svtools/moduleoptions.hxx>
#include <tools/link.hxx>
#include <tools/rcid.h>
#include <vcl/svapp.hxx>

#define USE_APP_SHORTCUTS
#include "app.hrc"
#include "../dialog/dialog.hrc"
#include "shutdowniconjava.hrc"
#include "shutdownicon.hxx"

#include <premac.h>
#import <Cocoa/Cocoa.h>
#include <postmac.h>

#include "../view/topfrm_cocoa.h"

#define WRITER_COMMAND_ID				'SDI1'
#define CALC_COMMAND_ID					'SDI2'
#define IMPRESS_COMMAND_ID				'SDI3'
#define DRAW_COMMAND_ID					'SDI4'
#define MATH_COMMAND_ID					'SDI5'
#define BASE_COMMAND_ID					'SDI8'
#define FROMTEMPLATE_COMMAND_ID			'SDI6'
#define FILEOPEN_COMMAND_ID				'SDI7'

#define WRITER_FALLBACK_DESC			"Text Document"
#define CALC_FALLBACK_DESC				"Spreadsheet"
#define IMPRESS_WIZARD_FALLBACK_DESC	"Presentation"
#define DRAW_FALLBACK_DESC				"Drawing"
#define BASE_FALLBACK_DESC				"Database"
#define MATH_FALLBACK_DESC				"Formula"

typedef void VCLOpenPrintFileHandler_Type( const char *pPath, sal_Bool bPrint );
typedef void VCLRequestShutdownHandler_Type();

static const NSString *kMenuItemPrefNameKey = @"MenuItemPrefName";
static const NSString *kMenuItemPrefBooleanValueKey = @"MenuItemPrefBooleanValue";
static const NSString *kMenuItemPrefStringValueKey = @"MenuItemPrefStringValue";
static const NSString *kMenuItemValueIsDefaultForPrefKey = @"MenuItemValueIsDefaultForPref";
static ResMgr *pJavaResMgr = NULL;

using namespace com::sun::star::beans;
using namespace com::sun::star::uno;
using namespace rtl;

static XubString GetJavaResString( int nId )
{
    if ( !pJavaResMgr )
    {
        pJavaResMgr = SfxApplication::CreateResManager( "shutdowniconjava" );
        if ( !pJavaResMgr )
            return OUString();
    }

    ResId aResId( nId, *pJavaResMgr );
    aResId.SetRT( RSC_STRING );
    if ( !pJavaResMgr->IsAvailable( aResId ) )
        return OUString();
 
    return XubString( ResId( nId, *pJavaResMgr ) );
}

class QuickstartMenuItemDescriptor
{
	SEL							maSelector;
	XubString					maText;
	::std::vector< QuickstartMenuItemDescriptor >	maItems;
	CFStringRef					maPrefName;
	CFPropertyListRef			maCheckedPrefValue;
	BOOL						mbValueIsDefaultForPref;

public:
								QuickstartMenuItemDescriptor( SEL aSelector, XubString aText, CFStringRef aPrefName = NULL, CFPropertyListRef aCheckedPrefValue = NULL, BOOL bValueIsDefaultForPref = FALSE ) : maSelector( aSelector ), maText( aText ), maPrefName( aPrefName ), maCheckedPrefValue( aCheckedPrefValue ), mbValueIsDefaultForPref( bValueIsDefaultForPref ) {}
								QuickstartMenuItemDescriptor( ::std::vector< QuickstartMenuItemDescriptor > &rItems, XubString aText ) : maSelector( NULL ), maText( aText ), maItems( rItems ), maPrefName( NULL ), maCheckedPrefValue( NULL ), mbValueIsDefaultForPref( FALSE ) {}
								~QuickstartMenuItemDescriptor() {};
	NSMenuItem*					CreateMenuItem( const NSObject *pDelegate ) const;
};

NSMenuItem *QuickstartMenuItemDescriptor::QuickstartMenuItemDescriptor::CreateMenuItem( const NSObject *pDelegate ) const
{
	NSMenuItem *pRet = nil;

	if ( maText.Len() )
	{
		NSString *pTitle = [NSString stringWithCharacters:maText.GetBuffer() length:maText.Len()];
		if ( pTitle )
		{
			pRet = [[NSMenuItem alloc] initWithTitle:pTitle action:maSelector keyEquivalent:@""];
			if ( pRet )
			{
				if ( pDelegate )
					[pRet setTarget:pDelegate];

				if ( pRet && maItems.size() )
				{
					NSMenu *pMenu = [[NSMenu alloc] initWithTitle:pTitle];
					if ( pMenu )
					{
						if ( pDelegate )
							[pMenu setDelegate:pDelegate];

						for ( ::std::vector< QuickstartMenuItemDescriptor >::const_iterator it = maItems.begin(); it != maItems.end(); ++it )
						{
							NSMenuItem *pSubmenuItem = it->CreateMenuItem( pDelegate );
							if ( pSubmenuItem );
								[pMenu addItem:pSubmenuItem];

						}

						[pRet setSubmenu:pMenu];
					}
				}
				else if ( maPrefName && maCheckedPrefValue )
				{
					NSObject *pPrefKey = nil;
					NSObject *pPrefValue = nil;
					if ( CFGetTypeID( maCheckedPrefValue ) == CFBooleanGetTypeID() )
					{
						pPrefKey = kMenuItemPrefBooleanValueKey;
						pPrefValue = [NSNumber numberWithBool:(CFBooleanRef)maCheckedPrefValue == kCFBooleanTrue ? YES : NO];
					}
					else if ( CFGetTypeID( maCheckedPrefValue ) == CFStringGetTypeID() )
					{
						pPrefKey = kMenuItemPrefStringValueKey;
						pPrefValue = (NSString *)maCheckedPrefValue;
					}
					NSObject *pValueIsDefaultForPref = [NSNumber numberWithBool:mbValueIsDefaultForPref];

					if ( pPrefKey && pPrefValue && pValueIsDefaultForPref )
					{
						NSDictionary *pDict = [NSDictionary dictionaryWithObjectsAndKeys:(NSString *)maPrefName, kMenuItemPrefNameKey, pPrefValue, pPrefKey, pValueIsDefaultForPref, kMenuItemValueIsDefaultForPrefKey, nil];
						if ( pDict )
							[pRet setRepresentedObject:pDict];
					}
				}
			}
		}
	}

	return pRet;
}

class ShutdownIconEvent
{
	int					mnCommand;

public:
						ShutdownIconEvent( int nCommand ) : mnCommand( nCommand ) {}
						~ShutdownIconEvent() {}
						DECL_LINK( DispatchEvent, void* );
};

IMPL_LINK( ShutdownIconEvent, DispatchEvent, void*, pData )
{
	switch ( mnCommand )
	{
		case WRITER_COMMAND_ID:
			ShutdownIcon::OpenURL( OUString::createFromAscii( WRITER_URL ), OUString::createFromAscii( "_default" ) );
			break;
		case CALC_COMMAND_ID:
			ShutdownIcon::OpenURL( OUString::createFromAscii( CALC_URL ), OUString::createFromAscii( "_default" ) );
			break;
		case IMPRESS_COMMAND_ID:
			ShutdownIcon::OpenURL( OUString::createFromAscii( IMPRESS_WIZARD_URL ), OUString::createFromAscii( "_default" ) );
			break;
		case DRAW_COMMAND_ID:
			ShutdownIcon::OpenURL( OUString::createFromAscii( DRAW_URL ), OUString::createFromAscii( "_default" ) );
			break;
		case MATH_COMMAND_ID:
			ShutdownIcon::OpenURL( OUString::createFromAscii( MATH_URL ), OUString::createFromAscii( "_default" ) );
			break;
		case BASE_COMMAND_ID:
			ShutdownIcon::OpenURL( OUString::createFromAscii( BASE_URL ), OUString::createFromAscii( "_default" ) );
			break;
		case FROMTEMPLATE_COMMAND_ID:
			ShutdownIcon::FromTemplate();
			break;
		case FILEOPEN_COMMAND_ID:
			ShutdownIcon::FileOpen();
			break;
		default:
			break;
	}

	delete this;

	return 0;
}

void ProcessShutdownIconCommand( int nCommand )
{
	if ( !Application::IsShutDown() )
	{
		switch ( nCommand )
		{
			case WRITER_COMMAND_ID:
			case CALC_COMMAND_ID:
			case IMPRESS_COMMAND_ID:
			case DRAW_COMMAND_ID:
			case MATH_COMMAND_ID:
			case BASE_COMMAND_ID:
			case FROMTEMPLATE_COMMAND_ID:
			case FILEOPEN_COMMAND_ID:
			{
				ShutdownIconEvent *pEvent = new ShutdownIconEvent( nCommand );
				Application::PostUserEvent( LINK( pEvent, ShutdownIconEvent, DispatchEvent ) );
				break;
			}
			default:
			{
				break;
			}
		}
	}
}

@interface NSObject (ShutdownIconDelegate)
- (BOOL)application:(NSApplication *)pApplication openFile:(NSString *)pFilename;
- (BOOL)application:(NSApplication *)pApplication printFile:(NSString *)pFilename;
- (void)applicationDidBecomeActive:(NSNotification *)pNotification;
- (void)applicationDidChangeScreenParameters:(NSNotification *)pNotification;
- (NSMenu *)applicationDockMenu:(NSApplication *)pApplication;
- (BOOL)applicationShouldHandleReopen:(NSApplication *)pApplication hasVisibleWindows:(BOOL)bFlag;
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)pApplication;
- (void)applicationWillFinishLaunching:(NSNotification *)pNotification;
@end

/*
 * Create a class that is a facade for the application delegate set by the JVM.
 * Note that this class only implement the delegate methods in the JVM's
 * ApplicationDelegate and AWTApplicationDelegate classes.
 */
@interface ShutdownIconDelegate : NSObject
{
	id					mpDelegate;
	NSMenu*				mpDockMenu;
}
- (BOOL)application:(NSApplication *)pApplication openFile:(NSString *)pFilename;
- (BOOL)application:(NSApplication *)pApplication printFile:(NSString *)pFilename;
- (void)applicationDidBecomeActive:(NSNotification *)pNotification;
- (void)applicationDidChangeScreenParameters:(NSNotification *)pNotification;
- (NSMenu *)applicationDockMenu:(NSApplication *)pApplication;
- (BOOL)applicationShouldHandleReopen:(NSApplication *)pApplication hasVisibleWindows:(BOOL)bFlag;
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)pApplication;
- (void)applicationWillFinishLaunching:(NSNotification *)pNotification;
- (void)dealloc;
- (id)init;
- (void)handleCalcCommand:(id)pObject;
- (void)handleDrawCommand:(id)pObject;
- (void)handleFileOpenCommand:(id)pObject;
- (void)handleFromTemplateCommand:(id)pObject;
- (void)handleImpressCommand:(id)pObject;
- (void)handleMathCommand:(id)pObject;
- (void)handlePreferenceChangeCommand:(id)pObject;
- (void)handleWriterCommand:(id)pObject;
- (void)setDelegate:(id)pDelegate;
- (void)menuNeedsUpdate:(NSMenu *)pMenu;
- (BOOL)validateMenuItem:(NSMenuItem *)pMenuItem;
@end

@implementation ShutdownIconDelegate

- (BOOL)application:(NSApplication *)pApplication openFile:(NSString *)pFilename
{
	if ( mpDelegate && [mpDelegate respondsToSelector:@selector(application:openFile:)] )
		return [mpDelegate application:pApplication openFile:pFilename];
	else
		return NO;
}

- (BOOL)application:(NSApplication *)pApplication printFile:(NSString *)pFilename
{
	if ( mpDelegate && [mpDelegate respondsToSelector:@selector(application:printFile:)] )
		return [mpDelegate application:pApplication printFile:pFilename];
	else
		return NO;
}

- (NSMenu *)applicationDockMenu:(NSApplication *)pApplication
{
    return mpDockMenu;
} 

- (void)applicationDidBecomeActive:(NSNotification *)pNotification
{
	if ( mpDelegate && [mpDelegate respondsToSelector:@selector(applicationDidBecomeActive:)] )
		[mpDelegate applicationDidBecomeActive:pNotification];
}

- (void)applicationDidChangeScreenParameters:(NSNotification *)pNotification
{
	if ( mpDelegate && [mpDelegate respondsToSelector:@selector(applicationDidChangeScreenParameters:)] )
		[mpDelegate applicationDidChangeScreenParameters:pNotification];
}

- (BOOL)applicationShouldHandleReopen:(NSApplication *)pApplication hasVisibleWindows:(BOOL)bFlag
{
	if ( mpDelegate && [mpDelegate respondsToSelector:@selector(applicationShouldHandleReopen:hasVisibleWindows:)] )
		return [mpDelegate applicationShouldHandleReopen:pApplication hasVisibleWindows:bFlag];
	else
		return NO;
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)pApplication
{
	if ( mpDelegate && [mpDelegate respondsToSelector:@selector(applicationShouldTerminate:)] )
		return [mpDelegate applicationShouldTerminate:pApplication];
	else
		return NSTerminateCancel;
}

- (void)applicationWillFinishLaunching:(NSNotification *)pNotification
{
	if ( mpDelegate && [mpDelegate respondsToSelector:@selector(applicationWillFinishLaunching:)] )
		[mpDelegate applicationWillFinishLaunching:pNotification];
}

- (void)dealloc
{
	if ( mpDelegate )
		[mpDelegate release];

	if ( mpDockMenu )
		[mpDockMenu release];

	[super dealloc];
}

- (id)init
{
	[super init];

	mpDelegate = nil;
	mpDockMenu = [[NSMenu alloc] initWithTitle:@""];

	return self;
}

- (void)handleCalcCommand:(id)pObject
{
	ProcessShutdownIconCommand( CALC_COMMAND_ID );
}

- (void)handleDrawCommand:(id)pObject
{
	ProcessShutdownIconCommand( DRAW_COMMAND_ID );
}

- (void)handleFileOpenCommand:(id)pObject
{
	ProcessShutdownIconCommand( FILEOPEN_COMMAND_ID );
}

- (void)handleFromTemplateCommand:(id)pObject
{
	ProcessShutdownIconCommand( FROMTEMPLATE_COMMAND_ID );
}

- (void)handleImpressCommand:(id)pObject
{
	ProcessShutdownIconCommand( IMPRESS_COMMAND_ID );
}

- (void)handleMathCommand:(id)pObject
{
	ProcessShutdownIconCommand( MATH_COMMAND_ID );
}

- (void)handlePreferenceChangeCommand:(id)pObject
{
	if ( pObject && [pObject isKindOfClass:[NSMenuItem class]] )
	{
		NSMenuItem *pMenuItem = (NSMenuItem *)pObject;
		NSUserDefaults *pDefaults = [NSUserDefaults standardUserDefaults];
		NSObject *pPrefs = [pMenuItem representedObject];
		if ( pDefaults && pPrefs && [pPrefs isKindOfClass:[NSDictionary class]] )
		{
			NSDictionary *pDict = (NSDictionary *)pPrefs;
			NSString *pPrefName = (NSString *)[pDict objectForKey:kMenuItemPrefNameKey];
			if ( pPrefName )
			{
				NSNumber *pPrefBooleanValue = (NSNumber *)[pDict objectForKey:kMenuItemPrefBooleanValueKey];
				NSString *pPrefStringValue = (NSString *)[pDict objectForKey:kMenuItemPrefStringValueKey];
				if ( pPrefBooleanValue )
				{
					BOOL bValue = [pPrefBooleanValue boolValue];
					[pDefaults setBool:( [pMenuItem state] == NSOffState ? bValue : !bValue ) forKey:pPrefName];
					[pDefaults synchronize];
				}
				else if ( pPrefStringValue )
				{
					[pDefaults setObject:pPrefStringValue forKey:pPrefName];
					[pDefaults synchronize];
				}
			}
		}
	}
}

- (void)handleWriterCommand:(id)pObject
{
	ProcessShutdownIconCommand( WRITER_COMMAND_ID );
}

- (void)handleBaseCommand:(id)pObject
{
	ProcessShutdownIconCommand( BASE_COMMAND_ID );
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

- (void)menuNeedsUpdate:(NSMenu *)pMenu
{
	if ( pMenu )
	{
		NSUserDefaults *pDefaults = [NSUserDefaults standardUserDefaults];
		NSMutableDictionary *pCheckedMenuItems = [NSMutableDictionary dictionaryWithCapacity:1];

		unsigned int nCount = [pMenu numberOfItems];
		unsigned i = 0;
		for ( ; i < nCount; i++ )
		{
			NSMenuItem *pMenuItem = [pMenu itemAtIndex:i];
			if ( pMenuItem )
			{
				NSObject *pPrefs = [pMenuItem representedObject];
				if ( pPrefs && [pPrefs isKindOfClass:[NSDictionary class]] )
				{
					[pMenuItem setState:NSOffState];

					NSDictionary *pDict = (NSDictionary *)pPrefs;
					NSString *pPrefName = (NSString *)[pDict objectForKey:kMenuItemPrefNameKey];
					if ( pPrefName )
					{
						NSNumber *pPrefBooleanValue = (NSNumber *)[pDict objectForKey:kMenuItemPrefBooleanValueKey];
						NSString *pPrefStringValue = (NSString *)[pDict objectForKey:kMenuItemPrefStringValueKey];
						NSNumber *pValueIsDefaultForPref = (NSNumber *)[pDict objectForKey:kMenuItemValueIsDefaultForPrefKey];
						if ( pPrefBooleanValue )
						{
							if ( pDefaults )
							{
								NSNumber *pValue = (NSNumber *)[pDefaults objectForKey:pPrefName];
								if ( pValue && [pValue boolValue] == [pPrefBooleanValue boolValue] )
									[pCheckedMenuItems setObject:pMenuItem forKey:pPrefName];
							}
						}
						else if ( pPrefStringValue )
						{
							if ( pDefaults )
							{
								NSString *pValue = [pDefaults stringForKey:pPrefName];
								if ( pValue && [pValue isEqualToString:pPrefStringValue] )
									[pCheckedMenuItems setObject:pMenuItem forKey:pPrefName];
							}
						}

						// If no checked item is set for this key and the menu
						// item is marked as the default checked item, make it
						// the checked item until another menu item becomes the
						// checked item
						if ( pValueIsDefaultForPref && [pValueIsDefaultForPref boolValue] && pCheckedMenuItems && ![pCheckedMenuItems objectForKey:pPrefName] )
							[pCheckedMenuItems setObject:pMenuItem forKey:pPrefName];
					}
				}
			}
		}

		if ( pCheckedMenuItems )
		{
			NSArray *pArray = [pCheckedMenuItems allValues];
			if ( pArray )
			{
				nCount = [pArray count];
				i = 0;
				for ( ; i < nCount; i++ )
				{
					NSMenuItem *pMenuItem = (NSMenuItem *)[pArray objectAtIndex:i];
					if ( pMenuItem )
						[pMenuItem setState:NSOnState];
				}
			}
		}
	}
}

- (BOOL)validateMenuItem:(NSMenuItem *)pMenuItem
{
	if ( mpDelegate && [mpDelegate respondsToSelector:@selector(validateMenuItem:)] )
		return [mpDelegate validateMenuItem:pMenuItem];
	else
		return NO;
}

@end

@interface QuickstartMenuItems : NSObject
{
	const ::std::vector< QuickstartMenuItemDescriptor >*	mpItems;
}
+ (id)createWithItems:(const ::std::vector< QuickstartMenuItemDescriptor >*)pItems;
- (void)addMenuItems:(id)pObject;
- (id)initWithItems:(const ::std::vector< QuickstartMenuItemDescriptor >*)pItems;
@end

@implementation QuickstartMenuItems

+ (id)createWithItems:(const ::std::vector< QuickstartMenuItemDescriptor >*)pItems
{
	QuickstartMenuItems *pRet = [[QuickstartMenuItems alloc] initWithItems:pItems];
	[pRet autorelease];
	return pRet;
}

- (void)addMenuItems:(id)pObject
{
	NSApplication *pApp = [NSApplication sharedApplication];
	if ( pApp && mpItems && mpItems->size() )
	{
		NSMenu *pAppMenu = nil;
		NSMenu *pDockMenu = nil;

		NSMenu *pMainMenu = [pApp mainMenu];
		if ( pMainMenu && [pMainMenu numberOfItems] > 0 )
		{
			NSMenuItem *pItem = [pMainMenu itemAtIndex:0];
			if ( pItem )
				pAppMenu = [pItem submenu];
		}

		NSObject *pDelegate = [pApp delegate];
		if ( pDelegate && [pDelegate respondsToSelector:@selector(applicationDockMenu:)] )
			pDockMenu = [pDelegate applicationDockMenu:pApp];

		if ( !pDockMenu )
		{
			// Do not retain as invoking alloc disables autorelease
			ShutdownIconDelegate *pNewDelegate = [[ShutdownIconDelegate alloc] init];
			if ( pDelegate )
				[pNewDelegate setDelegate:pDelegate];
			// NSApplication does not retain delegates so don't release it
			[pApp setDelegate:pNewDelegate];
			pDelegate = pNewDelegate;
			pDockMenu = [pNewDelegate applicationDockMenu:pApp];
		}


		if ( pAppMenu && pDockMenu )
		{
			// Insert a separator menu item (only in the application menu)
			[pAppMenu insertItem:[NSMenuItem separatorItem] atIndex:2];

			// Work the list of menu items is reverse order
			for ( ::std::vector< QuickstartMenuItemDescriptor >::const_reverse_iterator it = mpItems->rbegin(); it != mpItems->rend(); ++it )
			{
				NSMenuItem *pAppMenuItem = it->CreateMenuItem( pDelegate );
				NSMenuItem *pDockMenuItem = it->CreateMenuItem( pDelegate );
				if ( pAppMenuItem )
					[pAppMenu insertItem:pAppMenuItem atIndex:2];
				if ( pDockMenuItem )
					[pDockMenu insertItem:pDockMenuItem atIndex:0];
			}

			mpItems = nil;
		}
	}
}

- (id)initWithItems:(const ::std::vector< QuickstartMenuItemDescriptor >*)pItems
{
	[super init];

	mpItems = pItems;

	return self;
}

@end

extern "C" void java_init_systray()
{
	ShutdownIcon *pShutdownIcon = ShutdownIcon::getInstance();
	if ( !pShutdownIcon )
		return;

	// Collect the URLs of the entries in the File/New menu
	::std::set< OUString > aFileNewAppsAvailable;
	SvtDynamicMenuOptions aOpt;
	Sequence < Sequence < PropertyValue > > aNewMenu = aOpt.GetMenu( E_NEWMENU );
	const OUString sURLKey( RTL_CONSTASCII_USTRINGPARAM( "URL" ) );

	const Sequence< PropertyValue >* pNewMenu = aNewMenu.getConstArray();
	const Sequence< PropertyValue >* pNewMenuEnd = aNewMenu.getConstArray() + aNewMenu.getLength();
	for ( ; pNewMenu != pNewMenuEnd; ++pNewMenu )
	{
		::comphelper::SequenceAsHashMap aEntryItems( *pNewMenu );
		OUString sURL( aEntryItems.getUnpackedValueOrDefault( sURLKey, OUString() ) );
		if ( sURL.getLength() )
			aFileNewAppsAvailable.insert( sURL );
	}

	// Describe the menu entries for launching the applications
	struct MenuEntryDescriptor
	{
		SvtModuleOptions::EModule	eModuleIdentifier;
		const char*					pAsciiURLDescription;
		const char*					pFallbackDescription;
		SEL							aNewSelector;
		const CFStringRef			aCheckedPrefValue;
		BOOL						bValueIsDefaultForPref;
	} aMenuItems[] =
	{
		{ SvtModuleOptions::E_SWRITER, WRITER_URL, WRITER_FALLBACK_DESC, @selector(handleWriterCommand:), CFSTR( "-writer" ), TRUE },
		{ SvtModuleOptions::E_SCALC, CALC_URL, CALC_FALLBACK_DESC, @selector(handleCalcCommand:), CFSTR( "-calc" ), FALSE },
		{ SvtModuleOptions::E_SIMPRESS, IMPRESS_WIZARD_URL, IMPRESS_WIZARD_FALLBACK_DESC, @selector(handleImpressCommand:), CFSTR( "-impress" ), FALSE },
		{ SvtModuleOptions::E_SDRAW, DRAW_URL, DRAW_FALLBACK_DESC, @selector(handleDrawCommand:), CFSTR( "-draw" ), FALSE },
		{ SvtModuleOptions::E_SDATABASE, BASE_URL, BASE_FALLBACK_DESC, @selector(handleBaseCommand:), CFSTR( "-base" ), FALSE },
		{ SvtModuleOptions::E_SMATH, MATH_URL, MATH_FALLBACK_DESC, @selector(handleMathCommand:), CFSTR( "-math" ), FALSE  }
	};

	// Disable shutdown
	pShutdownIcon->SetVeto( true );
	pShutdownIcon->addTerminateListener();

	NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

	::std::vector< QuickstartMenuItemDescriptor > aAppMenuItems;
	XubString aDesc;

	// Insert the new document and default launch submenu entries
	::std::vector< QuickstartMenuItemDescriptor > aNewSubmenuItems;
	::std::vector< QuickstartMenuItemDescriptor > aOpenAtLaunchSubmenuItems;

	// None menu item is only used in default launch submenu
	aDesc = XubString( pShutdownIcon->GetResString( STR_NONE ) );
	aDesc.EraseAllChars( '~' );
	aOpenAtLaunchSubmenuItems.push_back( QuickstartMenuItemDescriptor( @selector(handlePreferenceChangeCommand:), aDesc, CFSTR( "DefaultLaunchOptions" ), CFSTR( "-nodefault" ), FALSE ) );

	SvtModuleOptions aModuleOptions;
	for ( size_t i = 0; i < sizeof( aMenuItems ) / sizeof( MenuEntryDescriptor ); ++i )
	{
		// the complete application is not even installed
		if ( !aModuleOptions.IsModuleInstalled( aMenuItems[i].eModuleIdentifier ) )
			continue;

		OUString sURL( OUString::createFromAscii( aMenuItems[i].pAsciiURLDescription ) );

		// the application is installed, but the entry has been
		// configured to *not* appear in the File/New menu =>
		//  also let not appear it in the quickstarter
		if ( aFileNewAppsAvailable.find( sURL ) == aFileNewAppsAvailable.end() )
			continue;

		aDesc = XubString( pShutdownIcon->GetUrlDescription( sURL ) );
		aDesc.EraseAllChars( '~' );
		// Fix bug 2206 by putting in some default text if the
		// description is an empty string
		if ( !aDesc.Len() )
		{
			aDesc = XubString::CreateFromAscii( aMenuItems[i].pFallbackDescription );
			aDesc.EraseAllChars( '~' );
		}
		aNewSubmenuItems.push_back( QuickstartMenuItemDescriptor( aMenuItems[i].aNewSelector, aDesc ) );

		// Add module name to open at launch submenu
		aDesc = XubString( aModuleOptions.GetModuleName( aMenuItems[i].eModuleIdentifier ) );
		aDesc.EraseAllChars( '~' );
		if ( aDesc.Len() )
			aOpenAtLaunchSubmenuItems.push_back( QuickstartMenuItemDescriptor( @selector(handlePreferenceChangeCommand:), aDesc, CFSTR( "DefaultLaunchOptions" ), aMenuItems[i].aCheckedPrefValue, aMenuItems[i].bValueIsDefaultForPref ) );
	}

	// Open template menu item is only used in new document submenu
	aDesc = XubString( pShutdownIcon->GetResString( STR_QUICKSTART_FROMTEMPLATE ) );
	aDesc.EraseAllChars( '~' );
	aNewSubmenuItems.push_back( QuickstartMenuItemDescriptor( @selector(handleFromTemplateCommand:), aDesc ) );

	// Insert the new document submenu
	aDesc = XubString( pShutdownIcon->GetResString( STR_NEW ) );
	aDesc.EraseAllChars( '~' );
	aAppMenuItems.push_back( QuickstartMenuItemDescriptor( aNewSubmenuItems, aDesc ) );

	// Insert the open document menu item into the application menu
	aDesc = XubString( pShutdownIcon->GetResString( STR_QUICKSTART_FILEOPEN ) );
	aDesc.EraseAllChars( '~' );
	aAppMenuItems.push_back( QuickstartMenuItemDescriptor( @selector(handleFileOpenCommand:), aDesc ) );

	// Insert the open at launch submenu
	aDesc = GetJavaResString( STR_OPENATLAUNCH );
	aDesc.EraseAllChars( '~' );
	aAppMenuItems.push_back( QuickstartMenuItemDescriptor( aOpenAtLaunchSubmenuItems, aDesc ) );

	// Insert the Mac OS X submenu entries
	::std::vector< QuickstartMenuItemDescriptor > aMacOSXSubmenuItems;

	aDesc = GetJavaResString( STR_IGNORETRACKPADGESTURES );
	aDesc.EraseAllChars( '~' );
	aMacOSXSubmenuItems.push_back( QuickstartMenuItemDescriptor( @selector(handlePreferenceChangeCommand:), aDesc, CFSTR( "IgnoreTrackpadGestures" ), kCFBooleanTrue, FALSE ) );

	aDesc = GetJavaResString( STR_DISABLEMACOSXSERVICESMENU );
	aDesc.EraseAllChars( '~' );
	aMacOSXSubmenuItems.push_back( QuickstartMenuItemDescriptor( @selector(handlePreferenceChangeCommand:), aDesc, CFSTR( "DisableServicesMenu" ), kCFBooleanTrue, FALSE ) );

	aDesc = GetJavaResString( STR_DISABLEMACOSXTEXTHIGHLIGHTING );
	aDesc.EraseAllChars( '~' );
	aMacOSXSubmenuItems.push_back( QuickstartMenuItemDescriptor( @selector(handlePreferenceChangeCommand:), aDesc, CFSTR( "UseNativeHighlightColor" ), kCFBooleanFalse, FALSE ) );

	// Insert the Quick Look submenu entries
	::std::vector< QuickstartMenuItemDescriptor > aQuickLookSubmenuItems;

	aDesc = GetJavaResString( STR_QUICKLOOKDISABLED );
	aDesc.EraseAllChars( '~' );
	aQuickLookSubmenuItems.push_back( QuickstartMenuItemDescriptor( @selector(handlePreferenceChangeCommand:), aDesc, CFSTR( "DisablePDFThumbnailSupport" ), kCFBooleanTrue, FALSE ) );

	aDesc = GetJavaResString( STR_QUICKLOOKFIRSTPAGEONLY );
	aDesc.EraseAllChars( '~' );
	aQuickLookSubmenuItems.push_back( QuickstartMenuItemDescriptor( @selector(handlePreferenceChangeCommand:), aDesc, CFSTR( "DisablePDFThumbnailSupport" ), kCFBooleanFalse, TRUE ) );

	aDesc = GetJavaResString( STR_QUICKLOOKALLPAGES );
	aDesc.EraseAllChars( '~' );
	aQuickLookSubmenuItems.push_back( QuickstartMenuItemDescriptor( @selector(handlePreferenceChangeCommand:), aDesc, CFSTR( "DisablePDFThumbnailSupport" ), CFSTR( "All" ), TRUE ) );

	aDesc = GetJavaResString( STR_QUICKLOOKSUPPORT );
	aDesc.EraseAllChars( '~' );
	aMacOSXSubmenuItems.push_back( QuickstartMenuItemDescriptor( aQuickLookSubmenuItems, aDesc ) );

	if ( NSDocument_versionsSupported() )
	{
		aDesc = GetJavaResString( STR_DISABLEVERSIONSSUPPORT );
		aDesc.EraseAllChars( '~' );
		aMacOSXSubmenuItems.push_back( QuickstartMenuItemDescriptor( @selector(handlePreferenceChangeCommand:), aDesc, CFSTR( "DisableVersions" ), kCFBooleanTrue, FALSE ) );

		aDesc = GetJavaResString( STR_DISABLERESUMESUPPORT );
		aDesc.EraseAllChars( '~' );
		aMacOSXSubmenuItems.push_back( QuickstartMenuItemDescriptor( @selector(handlePreferenceChangeCommand:), aDesc, CFSTR( "DisableResume" ), kCFBooleanTrue, FALSE ) );
	}

	// Insert the Mac OS X submenu
	aDesc = GetJavaResString( STR_MACOSXOPTIONS );
	aDesc.EraseAllChars( '~' );
	aAppMenuItems.push_back( QuickstartMenuItemDescriptor( aMacOSXSubmenuItems, aDesc ) );

	ULONG nCount = Application::ReleaseSolarMutex();

	QuickstartMenuItems *pItems = [QuickstartMenuItems createWithItems:&aAppMenuItems];
	NSArray *pModes = [NSArray arrayWithObjects:NSDefaultRunLoopMode, NSEventTrackingRunLoopMode, NSModalPanelRunLoopMode, @"AWTRunLoopMode", nil];
	[pItems performSelectorOnMainThread:@selector(addMenuItems:) withObject:pItems waitUntilDone:YES modes:pModes];

	Application::AcquireSolarMutex( nCount );

	[pPool release];
}

extern "C" void java_shutdown_systray()
{
}

bool ShutdownIcon::IsQuickstarterInstalled()
{
	return true;
}
