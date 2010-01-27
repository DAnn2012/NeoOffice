/*************************************************************************
 *
 * Copyright 2008 by Sun Microsystems, Inc.
 *
 * $RCSfile$
 * $Revision$
 *
 * This file is part of NeoOffice.
 *
 * NeoOffice is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3
 * only, as published by the Free Software Foundation.
 *
 * NeoOffice is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 3 for more details
 * (a copy is included in the LICENSE file that accompanied this code).
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with NeoOffice.  If not, see
 * <http://www.gnu.org/licenses/gpl-3.0.txt>
 * for a copy of the GPLv3 License.
 *
 * Modified January 2010 by Patrick Luby. NeoOffice is distributed under
 * GPL only under modification term 2 of the LGPL.
 *
 ************************************************************************/

// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_sc.hxx"



#include <vcl/svapp.hxx>
#include <svtools/zforlist.hxx>

#include <com/sun/star/uno/Any.hxx>
#include <com/sun/star/uno/Sequence.hxx>

#include "cfgids.hxx"
#include "docoptio.hxx"
#include "rechead.hxx"
#include "scresid.hxx"
#include "sc.hrc"
#include "miscuno.hxx"
#include "global.hxx"

using namespace utl;
using namespace rtl;
using namespace com::sun::star::uno;

//------------------------------------------------------------------------

#define SC_VERSION ((USHORT)251)

TYPEINIT1(ScTpCalcItem, SfxPoolItem);

//------------------------------------------------------------------------

//!	these functions should be moved to some header file
inline long TwipsToHMM(long nTwips)	{ return (nTwips * 127 + 36) / 72; }
inline long HMMToTwips(long nHMM)	{ return (nHMM * 72 + 63) / 127; }

inline long TwipsToEvenHMM(long nTwips)	{ return ( (nTwips * 127 + 72) / 144 ) * 2; }

//------------------------------------------------------------------------

USHORT lcl_GetDefaultTabDist()
{
	if ( ScOptionsUtil::IsMetricSystem() )
		return 709;					// 1,25 cm
	else
		return 720;					// 1/2"
}

//========================================================================
//      ScDocOptions - Dokument-Optionen
//========================================================================

ScDocOptions::ScDocOptions()
{
	ResetDocOptions();
}

//------------------------------------------------------------------------

ScDocOptions::ScDocOptions( const ScDocOptions& rCpy )
		:   fIterEps( rCpy.fIterEps ),
			nIterCount( rCpy.nIterCount ),
			nPrecStandardFormat( rCpy.nPrecStandardFormat ),
			nDay( rCpy.nDay ),
			nMonth( rCpy.nMonth ),
			nYear( rCpy.nYear ),
			nYear2000( rCpy.nYear2000 ),
			nTabDistance( rCpy.nTabDistance ),
            bIsIgnoreCase( rCpy.bIsIgnoreCase ),
			bIsIter( rCpy.bIsIter ),
			bCalcAsShown( rCpy.bCalcAsShown ),
			bMatchWholeCell( rCpy.bMatchWholeCell ),
			bDoAutoSpell( rCpy.bDoAutoSpell ),
            bLookUpColRowNames( rCpy.bLookUpColRowNames ),
            bFormulaRegexEnabled( rCpy.bFormulaRegexEnabled ),
            eFormulaGrammar( rCpy.eFormulaGrammar )
{
}

//------------------------------------------------------------------------

ScDocOptions::~ScDocOptions()
{
}

//------------------------------------------------------------------------

void ScDocOptions::Save(SvStream& rStream, BOOL bConfig) const
{
	ScWriteHeader aHdr( rStream, 28 );

	rStream << bIsIgnoreCase;
	rStream << bIsIter;
	rStream << nIterCount;
	rStream << fIterEps;
	rStream << nPrecStandardFormat;
	rStream << nDay;
	rStream << nMonth;
	rStream << nYear;
	rStream << nTabDistance;
	rStream << bCalcAsShown;
	rStream << bMatchWholeCell;
	rStream << bDoAutoSpell;
	rStream << bLookUpColRowNames;

	if ( bConfig || rStream.GetVersion() > SOFFICE_FILEFORMAT_40 )		// nicht bei 4.0 Export
	{
		if ( !bConfig && 1901 <= nYear2000 && nYear2000 <= 1999 )
		{	// fuer SO5 auf altes Format zweistellig abbilden
			rStream << (USHORT) (nYear2000 - 1901);
		}
		else
		{	// neues Format vierstellig, beliebiges Jahrhundert
			// erzeugt in SO5 vor src513e ein Warning beim Laden
			rStream << (USHORT) 29;		// Dummy, alter SO5 Default
			rStream << nYear2000;		// echter Wert
		}
	}
}

void ScDocOptions::Load(SvStream& rStream)
{
	ScReadHeader aHdr( rStream );

	rStream >> bIsIgnoreCase;
	rStream >> bIsIter;
	rStream >> nIterCount;
	rStream >> fIterEps;
	rStream >> nPrecStandardFormat;
	rStream >> nDay;
	rStream >> nMonth;
	rStream >> nYear;
	if ( aHdr.BytesLeft() )
		rStream >> nTabDistance;
	else
		nTabDistance = lcl_GetDefaultTabDist();
	if ( aHdr.BytesLeft() )
		rStream >> bCalcAsShown;
	else
		bCalcAsShown = FALSE;
	if ( aHdr.BytesLeft() )
		rStream >> bMatchWholeCell;
	else
		bMatchWholeCell = FALSE;
	if ( aHdr.BytesLeft() )
		rStream >> bDoAutoSpell;
	else
		bDoAutoSpell = FALSE;
	if ( aHdr.BytesLeft() )
		rStream >> bLookUpColRowNames;
	else
		bLookUpColRowNames = TRUE;
	if ( aHdr.BytesLeft() )
	{
		rStream >> nYear2000;		// SO5 ab 24.06.98
		// SO51 ab src513e
		if ( aHdr.BytesLeft() )
			rStream >> nYear2000;	// der echte Wert
		else
			nYear2000 += 1901;		// altes zweistelliges auf neues vierstelliges
	}
	else
		nYear2000 = 18 + 1901; 		// alter Wert vor SO5
}

void ScDocOptions::ResetDocOptions()
{
	bIsIgnoreCase		= FALSE;
	bIsIter				= FALSE;
	nIterCount			= 100;
	fIterEps			= 1.0E-3;
	nPrecStandardFormat	= 2;
	nDay				= 30;
	nMonth				= 12;
	nYear				= 1899;
	nYear2000			= SvNumberFormatter::GetYear2000Default();
	nTabDistance 		= lcl_GetDefaultTabDist();
	bCalcAsShown		= FALSE;
	bMatchWholeCell		= TRUE;
	bDoAutoSpell		= FALSE;
	bLookUpColRowNames	= TRUE;
    bFormulaRegexEnabled= TRUE;
    eFormulaGrammar     = ScGrammar::GRAM_NATIVE;
}

//========================================================================
//      ScTpCalcItem - Daten fuer die CalcOptions-TabPage
//========================================================================

ScTpCalcItem::ScTpCalcItem( USHORT nWhichP ) : SfxPoolItem( nWhichP )
{
}

//------------------------------------------------------------------------

ScTpCalcItem::ScTpCalcItem( USHORT nWhichP, const ScDocOptions& rOpt )
    :   SfxPoolItem ( nWhichP ),
		theOptions	( rOpt )
{
}

//------------------------------------------------------------------------

ScTpCalcItem::ScTpCalcItem( const ScTpCalcItem& rItem )
	:   SfxPoolItem	( rItem ),
		theOptions	( rItem.theOptions )
{
}

//------------------------------------------------------------------------

__EXPORT ScTpCalcItem::~ScTpCalcItem()
{
}

//------------------------------------------------------------------------

String __EXPORT ScTpCalcItem::GetValueText() const
{
	return String::CreateFromAscii( RTL_CONSTASCII_STRINGPARAM("ScTpCalcItem") );
}

//------------------------------------------------------------------------

int __EXPORT ScTpCalcItem::operator==( const SfxPoolItem& rItem ) const
{
	DBG_ASSERT( SfxPoolItem::operator==( rItem ), "unequal Which or Type" );

	const ScTpCalcItem& rPItem = (const ScTpCalcItem&)rItem;

	return ( theOptions == rPItem.theOptions );
}

//------------------------------------------------------------------------

SfxPoolItem* __EXPORT ScTpCalcItem::Clone( SfxItemPool * ) const
{
	return new ScTpCalcItem( *this );
}

//==================================================================
//	Config Item containing document options
//==================================================================

#define CFGPATH_CALC		"Office.Calc/Calculate"

#define SCCALCOPT_ITER_ITER			0
#define SCCALCOPT_ITER_STEPS		1
#define SCCALCOPT_ITER_MINCHG		2
#define SCCALCOPT_DATE_DAY			3
#define SCCALCOPT_DATE_MONTH		4
#define SCCALCOPT_DATE_YEAR			5
#define SCCALCOPT_DECIMALS			6
#define SCCALCOPT_CASESENSITIVE		7
#define SCCALCOPT_PRECISION			8
#define SCCALCOPT_SEARCHCRIT		9
#define SCCALCOPT_FINDLABEL			10
#define SCCALCOPT_REGEX             11
#define SCCALCOPT_FORMULA_SYNTAX    12
#define SCCALCOPT_COUNT             13

#define CFGPATH_DOCLAYOUT	"Office.Calc/Layout/Other"

#define SCDOCLAYOUTOPT_TABSTOP		0
#define SCDOCLAYOUTOPT_COUNT		1


Sequence<OUString> ScDocCfg::GetCalcPropertyNames()
{
	static const char* aPropNames[] =
	{
		"IterativeReference/Iteration",		// SCCALCOPT_ITER_ITER
		"IterativeReference/Steps",			// SCCALCOPT_ITER_STEPS
		"IterativeReference/MinimumChange",	// SCCALCOPT_ITER_MINCHG
		"Other/Date/DD",					// SCCALCOPT_DATE_DAY
		"Other/Date/MM",					// SCCALCOPT_DATE_MONTH
		"Other/Date/YY",					// SCCALCOPT_DATE_YEAR
		"Other/DecimalPlaces",				// SCCALCOPT_DECIMALS
		"Other/CaseSensitive",				// SCCALCOPT_CASESENSITIVE
		"Other/Precision",					// SCCALCOPT_PRECISION
		"Other/SearchCriteria",				// SCCALCOPT_SEARCHCRIT
        "Other/FindLabel",                  // SCCALCOPT_FINDLABEL
        "Other/RegularExpressions",         // SCCALCOPT_REGEX
        "Other/FormulaSyntax"               // SCCALCOPT_FORMULA_SYNTAX
	};
	Sequence<OUString> aNames(SCCALCOPT_COUNT);
	OUString* pNames = aNames.getArray();
	for(int i = 0; i < SCCALCOPT_COUNT; i++)
		pNames[i] = OUString::createFromAscii(aPropNames[i]);

	return aNames;
}

Sequence<OUString> ScDocCfg::GetLayoutPropertyNames()
{
	static const char* aPropNames[] =
	{
		"TabStop/NonMetric"			// SCDOCLAYOUTOPT_TABSTOP
	};
	Sequence<OUString> aNames(SCDOCLAYOUTOPT_COUNT);
	OUString* pNames = aNames.getArray();
	for(int i = 0; i < SCDOCLAYOUTOPT_COUNT; i++)
		pNames[i] = OUString::createFromAscii(aPropNames[i]);

	//	adjust for metric system
	if (ScOptionsUtil::IsMetricSystem())
		pNames[SCDOCLAYOUTOPT_TABSTOP] = OUString::createFromAscii( "TabStop/Metric" );

	return aNames;
}

ScDocCfg::ScDocCfg() :
	aCalcItem( OUString::createFromAscii( CFGPATH_CALC ) ),
	aLayoutItem( OUString::createFromAscii( CFGPATH_DOCLAYOUT ) )
{
	sal_Int32 nIntVal = 0;
	double fDoubleVal = 0;

	Sequence<OUString> aNames;
	Sequence<Any> aValues;
	const Any* pValues = NULL;

	USHORT nDateDay, nDateMonth, nDateYear;
	GetDate( nDateDay, nDateMonth, nDateYear );

	aNames = GetCalcPropertyNames();
	aValues = aCalcItem.GetProperties(aNames);
	aCalcItem.EnableNotification(aNames);
	pValues = aValues.getConstArray();
	DBG_ASSERT(aValues.getLength() == aNames.getLength(), "GetProperties failed")
	if(aValues.getLength() == aNames.getLength())
	{
		for(int nProp = 0; nProp < aNames.getLength(); nProp++)
		{
			DBG_ASSERT(pValues[nProp].hasValue(), "property value missing")
			if(pValues[nProp].hasValue())
			{
				switch(nProp)
				{
					case SCCALCOPT_ITER_ITER:
						SetIter( ScUnoHelpFunctions::GetBoolFromAny( pValues[nProp] ) );
						break;
					case SCCALCOPT_ITER_STEPS:
						if (pValues[nProp] >>= nIntVal) SetIterCount( (USHORT) nIntVal );
						break;
					case SCCALCOPT_ITER_MINCHG:
						if (pValues[nProp] >>= fDoubleVal) SetIterEps( fDoubleVal );
						break;
					case SCCALCOPT_DATE_DAY:
						if (pValues[nProp] >>= nIntVal) nDateDay = (USHORT) nIntVal;
						break;
					case SCCALCOPT_DATE_MONTH:
						if (pValues[nProp] >>= nIntVal) nDateMonth = (USHORT) nIntVal;
						break;
					case SCCALCOPT_DATE_YEAR:
						if (pValues[nProp] >>= nIntVal) nDateYear = (USHORT) nIntVal;
						break;
					case SCCALCOPT_DECIMALS:
						if (pValues[nProp] >>= nIntVal) SetStdPrecision( (USHORT) nIntVal );
						break;
					case SCCALCOPT_CASESENSITIVE:
						// content is reversed
						SetIgnoreCase( !ScUnoHelpFunctions::GetBoolFromAny( pValues[nProp] ) );
						break;
					case SCCALCOPT_PRECISION:
						SetCalcAsShown( ScUnoHelpFunctions::GetBoolFromAny( pValues[nProp] ) );
						break;
					case SCCALCOPT_SEARCHCRIT:
						SetMatchWholeCell( ScUnoHelpFunctions::GetBoolFromAny( pValues[nProp] ) );
						break;
					case SCCALCOPT_FINDLABEL:
						SetLookUpColRowNames( ScUnoHelpFunctions::GetBoolFromAny( pValues[nProp] ) );
						break;
                    case SCCALCOPT_REGEX :
                        SetFormulaRegexEnabled( ScUnoHelpFunctions::GetBoolFromAny( pValues[nProp] ) );
                        break;
                    case SCCALCOPT_FORMULA_SYNTAX:
                    {
                        ScGrammar::Grammar eGram = ScGrammar::GRAM_DEFAULT;

                        do
                        {
                            if (!(pValues[nProp] >>= nIntVal))
                                // extractino failed.
                                break;

                            switch (nIntVal)
                            {
                                case 0: // Calc A1
                                    eGram = ScGrammar::GRAM_NATIVE;
                                break;
#ifdef USE_JAVA
                                // Fix bug 3587 by disabling Excel syntax
                                case 1: // Excel A1
                                case 2: // Excel R1C1
                                    eGram = ScGrammar::GRAM_NATIVE;
                                break;
#else	// USE_JAVA
                                case 1: // Excel A1
                                    eGram = ScGrammar::GRAM_NATIVE_XL_A1;
                                break;
                                case 2: // Excel R1C1
                                    eGram = ScGrammar::GRAM_NATIVE_XL_R1C1;
                                break;
#endif	// USE_JAVA
                            }
                        }
                        while (false);
                        SetFormulaSyntax(eGram);
                    }
                    break;
				}
			}
		}
	}
	aCalcItem.SetCommitLink( LINK( this, ScDocCfg, CalcCommitHdl ) );

	SetDate( nDateDay, nDateMonth, nDateYear );

	aNames = GetLayoutPropertyNames();
	aValues = aLayoutItem.GetProperties(aNames);
	aLayoutItem.EnableNotification(aNames);
	pValues = aValues.getConstArray();
	DBG_ASSERT(aValues.getLength() == aNames.getLength(), "GetProperties failed")
	if(aValues.getLength() == aNames.getLength())
	{
		for(int nProp = 0; nProp < aNames.getLength(); nProp++)
		{
			DBG_ASSERT(pValues[nProp].hasValue(), "property value missing")
			if(pValues[nProp].hasValue())
			{
				switch(nProp)
				{
					case SCDOCLAYOUTOPT_TABSTOP:
						// TabDistance in ScDocOptions is in twips
						if (pValues[nProp] >>= nIntVal)
							SetTabDistance( (USHORT) HMMToTwips( nIntVal ) );
						break;
				}
			}
		}
	}
	aLayoutItem.SetCommitLink( LINK( this, ScDocCfg, LayoutCommitHdl ) );
}

IMPL_LINK( ScDocCfg, CalcCommitHdl, void *, EMPTYARG )
{
	Sequence<OUString> aNames = GetCalcPropertyNames();
	Sequence<Any> aValues(aNames.getLength());
	Any* pValues = aValues.getArray();

	USHORT nDateDay, nDateMonth, nDateYear;
	GetDate( nDateDay, nDateMonth, nDateYear );

	for(int nProp = 0; nProp < aNames.getLength(); nProp++)
	{
		switch(nProp)
		{
			case SCCALCOPT_ITER_ITER:
				ScUnoHelpFunctions::SetBoolInAny( pValues[nProp], IsIter() );
				break;
			case SCCALCOPT_ITER_STEPS:
				pValues[nProp] <<= (sal_Int32) GetIterCount();
				break;
			case SCCALCOPT_ITER_MINCHG:
				pValues[nProp] <<= (double) GetIterEps();
				break;
			case SCCALCOPT_DATE_DAY:
				pValues[nProp] <<= (sal_Int32) nDateDay;
				break;
			case SCCALCOPT_DATE_MONTH:
				pValues[nProp] <<= (sal_Int32) nDateMonth;
				break;
			case SCCALCOPT_DATE_YEAR:
				pValues[nProp] <<= (sal_Int32) nDateYear;
				break;
			case SCCALCOPT_DECIMALS:
				pValues[nProp] <<= (sal_Int32) GetStdPrecision();
				break;
			case SCCALCOPT_CASESENSITIVE:
				// content is reversed
				ScUnoHelpFunctions::SetBoolInAny( pValues[nProp], !IsIgnoreCase() );
				break;
			case SCCALCOPT_PRECISION:
				ScUnoHelpFunctions::SetBoolInAny( pValues[nProp], IsCalcAsShown() );
				break;
			case SCCALCOPT_SEARCHCRIT:
				ScUnoHelpFunctions::SetBoolInAny( pValues[nProp], IsMatchWholeCell() );
				break;
			case SCCALCOPT_FINDLABEL:
				ScUnoHelpFunctions::SetBoolInAny( pValues[nProp], IsLookUpColRowNames() );
				break;
            case SCCALCOPT_REGEX :
                ScUnoHelpFunctions::SetBoolInAny( pValues[nProp], IsFormulaRegexEnabled() );
            case SCCALCOPT_FORMULA_SYNTAX :
            {
                sal_Int32 nVal = 0;
#ifndef USE_JAVA
                switch (GetFormulaSyntax())
                {
                    case ScGrammar::GRAM_NATIVE_XL_A1:    nVal = 1; break;
                    case ScGrammar::GRAM_NATIVE_XL_R1C1:  nVal = 2; break;
                }
#endif	// !USE_JAVA
                pValues[nProp] <<= nVal;
            }
		}
	}
	aCalcItem.PutProperties(aNames, aValues);

	return 0;
}

IMPL_LINK( ScDocCfg, LayoutCommitHdl, void *, EMPTYARG )
{
	Sequence<OUString> aNames = GetLayoutPropertyNames();
	Sequence<Any> aValues(aNames.getLength());
	Any* pValues = aValues.getArray();

	for(int nProp = 0; nProp < aNames.getLength(); nProp++)
	{
		switch(nProp)
		{
			case SCDOCLAYOUTOPT_TABSTOP:
				//	TabDistance in ScDocOptions is in twips
				//	use only even numbers, so defaults don't get changed
				//	by modifying other settings in the same config item
				pValues[nProp] <<= (sal_Int32) TwipsToEvenHMM( GetTabDistance() );
				break;
		}
	}
	aLayoutItem.PutProperties(aNames, aValues);

	return 0;
}


void ScDocCfg::SetOptions( const ScDocOptions& rNew )
{
	*(ScDocOptions*)this = rNew;

	aCalcItem.SetModified();
	aLayoutItem.SetModified();
}


