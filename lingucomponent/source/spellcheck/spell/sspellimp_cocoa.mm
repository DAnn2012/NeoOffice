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
 *		 - GNU General Public License Version 2.1
 *
 *  GNU General Public License Version 2.1
 *  =============================================
 *  Copyright 2007 Planamesa Inc.
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

#include <premac.h>
#import <AppKit/AppKit.h>
#include <postmac.h>

#include <com/sun/star/text/TextMarkupType.hpp>

#include "sspellimp_cocoa.h"

using namespace ::com::sun::star::linguistic2;
using namespace ::com::sun::star::text;

static OUString NSStringToOUString( NSString *pString )
{
	OUString aRet;

	if ( pString )
	{
		NSUInteger nLen = [pString length];
		if ( nLen )
		{
			sal_Unicode aBuf[ nLen + 1 ];
			[pString getCharacters:aBuf];
			aBuf[ nLen ] = 0;
			aRet = OUString( aBuf );
		}
	}

	return aRet;
}

@interface RunSpellCheckerArgs : NSObject
{
	NSArray*				mpArgs;
	NSObject*				mpResult;
}
+ (id)argsWithArgs:(NSArray *)pArgs;
- (NSArray *)args;
- (void)dealloc;
- (id)initWithArgs:(NSArray *)pArgs;
- (NSObject *)result;
- (void)setResult:(NSObject *)pResult;
@end

@implementation RunSpellCheckerArgs

+ (id)argsWithArgs:(NSArray *)pArgs
{
	RunSpellCheckerArgs *pRet = [[RunSpellCheckerArgs alloc] initWithArgs:(NSArray *)pArgs];
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

@interface RunSpellChecker : NSObject
+ (id)create;
- (void)checkGrammarOfString:(RunSpellCheckerArgs *)pArgs;
- (void)checkSpellingOfString:(RunSpellCheckerArgs *)pArgs;
- (void)getGuesses:(RunSpellCheckerArgs *)pArgs;
- (void)getLocales:(RunSpellCheckerArgs *)pArgs;
@end

@implementation RunSpellChecker

+ (id)create
{
	RunSpellChecker *pRet = [[RunSpellChecker alloc] init];
	[pRet autorelease];
	return pRet;
}

- (void)checkGrammarOfString:(RunSpellCheckerArgs *)pArgs
{
    NSArray *pArgArray = [pArgs args];
    if ( !pArgArray || [pArgArray count] < 2 )
        return;

	NSValue *pResultValue = (NSValue *)[pArgArray objectAtIndex:0];
	if ( !pResultValue )
		return;

    ProofreadingResult *pResult = (ProofreadingResult *)[pResultValue pointerValue];
	if ( !pResult )
		return;

	pResult->aErrors.realloc( 0 );

	if ( !pResult->aText.getLength() || pResult->nStartOfSentencePosition < 0 || pResult->nStartOfSentencePosition >= pResult->nBehindEndOfSentencePosition || pResult->nStartOfSentencePosition >= pResult->aText.getLength() )
		return;

    NSString *pLocale = (NSString *)[pArgArray objectAtIndex:1];
	if ( !pLocale )
		return;

	@try
	{
		NSSpellChecker *pChecker = [NSSpellChecker sharedSpellChecker];
		if ( pChecker )
		{
			NSUInteger nLen;
			if ( pResult->aText.getLength() < pResult->nBehindEndOfSentencePosition > pResult->aText.getLength() )
				nLen = pResult->aText.getLength() - pResult->nStartOfSentencePosition;
			else
 				nLen = pResult->nBehindEndOfSentencePosition - pResult->nStartOfSentencePosition;
			NSString *pString = [NSString stringWithCharacters:pResult->aText.getStr() + pResult->nStartOfSentencePosition length:nLen];
			if ( pString )
			{
				// Doing a combined spelling and grammar check appears to yield
				// results more consistent with Apple's TextEdit application
				[pChecker setLanguage:pLocale];
				NSArray *pCheckingResults = [pChecker checkString:pString range:NSMakeRange( 0, [pString length] ) types:NSTextCheckingTypeSpelling | NSTextCheckingTypeGrammar options:nil inSpellDocumentWithTag:0 orthography:nil wordCount:nil];
				if ( pCheckingResults && [pCheckingResults count] )
				{
					NSMutableArray< NSDictionary< NSString*, id > * > *pGrammarDetails = [NSMutableArray arrayWithCapacity:[pCheckingResults count] * 2];
					if ( pGrammarDetails )
					{
						for ( NSTextCheckingResult *pCheckingResult in pCheckingResults )
						{
							if ( pCheckingResult )
							{
								NSArray *pCheckingDetails = [pCheckingResult grammarDetails];
								if ( pCheckingDetails )
								{
									for ( NSDictionary *pCheckingDetail in pCheckingDetails )
									{
										if ( pCheckingDetail )
											[pGrammarDetails addObject:pCheckingDetail];
									}
								}
							}
						}

						sal_Int32 nErrors = pResult->aErrors.getLength();
						pResult->aErrors.realloc( nErrors + [pGrammarDetails count] );
						SingleProofreadingError *pErrors = pResult->aErrors.getArray();
						if ( pErrors )
						{
							for ( NSDictionary *pGrammarDetail in pGrammarDetails )
							{
								if ( !pGrammarDetail )
									continue;

								NSRange aRange = NSMakeRange( NSNotFound, 0 );
								NSValue *pRangeValue = [pGrammarDetail objectForKey:NSGrammarRange];
								if ( pRangeValue )
									aRange = [pRangeValue rangeValue];

								if ( aRange.location != NSNotFound && aRange.length > 0 )
								{
									OUString aDesc = NSStringToOUString( [pGrammarDetail objectForKey:NSGrammarUserDescription] );
									if ( aDesc.getLength() )
									{
										SingleProofreadingError aError;
										aError.nErrorStart = pResult->nStartOfSentencePosition + aRange.location;
										aError.nErrorLength = aRange.length;
										aError.nErrorType = TextMarkupType::PROOFREADING;
										aError.aRuleIdentifier = aDesc;
										aError.aShortComment = aDesc;
										aError.aFullComment = aDesc;
										NSArray *pCorrections = [pGrammarDetail objectForKey:NSGrammarCorrections];
										if ( pCorrections )
										{
											NSUInteger nCorrections = [pCorrections count];
											NSUInteger j = 0;
											sal_Int32 nSuggestions = 0;

											aError.aSuggestions.realloc( nCorrections );
											OUString *pSuggestions = aError.aSuggestions.getArray();
											if ( pSuggestions )
											{
												for ( ; j < nCorrections; j++ )
												{
													OUString aSuggestion = NSStringToOUString( [pCorrections objectAtIndex:j] );
													if ( aSuggestion.getLength() )
														pSuggestions[ nSuggestions++ ] = aSuggestion;
												}
											}
											aError.aSuggestions.realloc( nSuggestions );
										}

										pErrors[ nErrors++ ] = aError;
									}
								}
							}
						}

						pResult->aErrors.realloc( nErrors );
					}
				}
			}
		}
	}
	@catch ( NSException *pExc )
	{
		NSLog( @"%@", [pExc reason] );
	}
}

- (void)checkSpellingOfString:(RunSpellCheckerArgs *)pArgs
{
    NSArray *pArgArray = [pArgs args];
    if ( !pArgArray || [pArgArray count] < 2 )
        return;

    NSString *pString = (NSString *)[pArgArray objectAtIndex:0];
	if ( !pString )
		return;

    NSString *pLocale = (NSString *)[pArgArray objectAtIndex:1];
	if ( !pLocale )
		return;

	@try
	{
		NSSpellChecker *pChecker = [NSSpellChecker sharedSpellChecker];
		if ( pChecker )
		{
			// Fix bug 3613 by explicitly specifying the locale
			NSRange aRange = [pChecker checkSpellingOfString:pString startingAt:0 language:pLocale wrap:NO inSpellDocumentWithTag:0 wordCount:nil];
			if ( aRange.location != NSNotFound && aRange.length > 0 )
				[pArgs setResult:[NSNumber numberWithBool:NO]];
		}
	}
	@catch ( NSException *pExc )
	{
		NSLog( @"%@", [pExc reason] );
	}
}

- (void)getGuesses:(RunSpellCheckerArgs *)pArgs
{
    NSArray *pArgArray = [pArgs args];
    if ( !pArgArray || [pArgArray count] < 2 )
        return;

    NSString *pString = (NSString *)[pArgArray objectAtIndex:0];
	if ( !pString )
		return;

    NSString *pLocale = (NSString *)[pArgArray objectAtIndex:1];
	if ( !pLocale )
		return;

	@try
	{
		NSSpellChecker *pChecker = [NSSpellChecker sharedSpellChecker];
		if ( pChecker )
		{
			NSArray *pArray = [pChecker guessesForWordRange:NSMakeRange( 0, [pString length] ) inString:pString language:pLocale inSpellDocumentWithTag:0];
			if ( pArray && [pArray count] )
				[pArgs setResult:pArray];
		}
	}
	@catch ( NSException *pExc )
	{
		NSLog( @"%@", [pExc reason] );
	}
}

- (void)getLocales:(RunSpellCheckerArgs *)pArgs
{
    NSArray *pArgArray = [pArgs args];
    if ( !pArgArray || [pArgArray count] < 1 )
        return;

    NSMutableArray *pLocales = (NSMutableArray *)[pArgArray objectAtIndex:0];
	if ( !pLocales )
		return;

	@try
	{
		NSSpellChecker *pChecker = [NSSpellChecker sharedSpellChecker];
		if ( pChecker )
		{
			NSArray *pLanguages = [pChecker availableLanguages];
			if ( pLanguages)
				[pLocales addObjectsFromArray:pLanguages];
			NSBundle *pBundle = [NSBundle mainBundle];
			if ( pBundle )
				[pLocales addObjectsFromArray:[pBundle localizations]];

			NSMutableSet *pCanonicalLocales = [NSMutableSet setWithCapacity:[pLocales count]];
			if ( pCanonicalLocales )
			{
				for ( NSString *pLocale in pLocales )
				{
					if ( pLocale )
					{
						NSString *pLocaleID = [NSLocale canonicalLocaleIdentifierFromString:pLocale];
						if ( pLocaleID )
							[pCanonicalLocales addObject:pLocaleID];
					}
				}

				NSMutableArray *pRet = [NSMutableArray arrayWithCapacity:[pCanonicalLocales count]];
				if ( pRet )
				{
					for ( NSString *pLocale in pCanonicalLocales )
					{
						if ( pLocale && [pChecker setLanguage:pLocale] )
							[pRet addObject:pLocale];
					}

					[pArgs setResult:pRet];
				}
			}
		}
	}
	@catch ( NSException *pExc )
	{
		NSLog( @"%@", [pExc reason] );
	}
}

@end

void NSSpellChecker_checkGrammarOfString( ::com::sun::star::linguistic2::ProofreadingResult *pResult, CFStringRef aLocale )
{
	NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

	if ( pResult && aLocale && CFStringGetLength( aLocale ) && pResult->aText.getLength() && pResult->aText.getLength() > pResult->nStartOfSentencePosition )
	{
		RunSpellCheckerArgs *pArgs = [RunSpellCheckerArgs argsWithArgs:[NSArray arrayWithObjects:[NSValue valueWithPointer:pResult], (NSString *)aLocale, nil]];
		NSArray *pModes = [NSArray arrayWithObjects:NSDefaultRunLoopMode, NSEventTrackingRunLoopMode, NSModalPanelRunLoopMode, @"AWTRunLoopMode", nil];
		RunSpellChecker *pRunSpellChecker = [RunSpellChecker create];
		[pRunSpellChecker performSelectorOnMainThread:@selector(checkGrammarOfString:) withObject:pArgs waitUntilDone:YES modes:pModes];
	}

	[pPool release];
}

sal_Bool NSSpellChecker_checkSpellingOfString( CFStringRef aString, CFStringRef aLocale )
{
	sal_Bool bRet = sal_True;

	NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

	if ( aString && aLocale && CFStringGetLength( aString ) && CFStringGetLength( aLocale ) )
	{
		RunSpellCheckerArgs *pArgs = [RunSpellCheckerArgs argsWithArgs:[NSArray arrayWithObjects:(NSString *)aString, (NSString *)aLocale, nil]];
		NSArray *pModes = [NSArray arrayWithObjects:NSDefaultRunLoopMode, NSEventTrackingRunLoopMode, NSModalPanelRunLoopMode, @"AWTRunLoopMode", nil];
		RunSpellChecker *pRunSpellChecker = [RunSpellChecker create];
		[pRunSpellChecker performSelectorOnMainThread:@selector(checkSpellingOfString:) withObject:pArgs waitUntilDone:YES modes:pModes];
		NSNumber *pRet = (NSNumber *)[pArgs result];
		if ( pRet )
			bRet = (sal_Bool)[pRet boolValue];
	}

	[pPool release];

	return bRet;
}

CFArrayRef NSSpellChecker_getGuesses( CFStringRef aString, CFStringRef aLocale )
{
	CFArrayRef aRet = nil;

	NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

	if ( aString && aLocale && CFStringGetLength( aString ) && CFStringGetLength( aLocale ) )
	{
		RunSpellCheckerArgs *pArgs = [RunSpellCheckerArgs argsWithArgs:[NSArray arrayWithObjects:(NSString *)aString, (NSString *)aLocale, nil]];
		NSArray *pModes = [NSArray arrayWithObjects:NSDefaultRunLoopMode, NSEventTrackingRunLoopMode, NSModalPanelRunLoopMode, @"AWTRunLoopMode", nil];
		RunSpellChecker *pRunSpellChecker = [RunSpellChecker create];
		[pRunSpellChecker performSelectorOnMainThread:@selector(getGuesses:) withObject:pArgs waitUntilDone:YES modes:pModes];
		NSArray *pRet = (NSArray *)[pArgs result];
		if ( pRet )
		{
			[pRet retain];
			aRet = (CFArrayRef)pRet;
		}
	}

	[pPool release];

	return aRet;
}

CFArrayRef NSSpellChecker_getLocales( CFArrayRef aAppLocales )
{
	CFArrayRef aRet = nil;

	NSAutoreleasePool *pPool = [[NSAutoreleasePool alloc] init];

	RunSpellCheckerArgs *pArgs = [RunSpellCheckerArgs argsWithArgs:[NSArray arrayWithObject:( aAppLocales ? [NSMutableArray arrayWithArray:(NSArray *)aAppLocales] : [NSMutableArray arrayWithCapacity:64] )]];
	NSArray *pModes = [NSArray arrayWithObjects:NSDefaultRunLoopMode, NSEventTrackingRunLoopMode, NSModalPanelRunLoopMode, @"AWTRunLoopMode", nil];
	RunSpellChecker *pRunSpellChecker = [RunSpellChecker create];
	[pRunSpellChecker performSelectorOnMainThread:@selector(getLocales:) withObject:pArgs waitUntilDone:YES modes:pModes];
	NSArray *pRet = (NSArray *)[pArgs result];
	if ( pRet )
	{
		[pRet retain];
		aRet = (CFArrayRef)pRet;
	}

	[pPool release];

	return aRet;
}
