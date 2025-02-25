/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 * 
 *   Modified February 2021 by Patrick Luby. NeoOffice is only distributed
 *   under the GNU General Public License, Version 3 as allowed by Section 3.3
 *   of the Mozilla Public License, v. 2.0.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDED_SVTOOLS_HTMLOUT_HXX
#define INCLUDED_SVTOOLS_HTMLOUT_HXX

#include <svtools/svtdllapi.h>
#include <tools/solar.h>
#include <rtl/textenc.h>
#include <rtl/string.hxx>
#include <svl/macitem.hxx>

#ifdef USE_JAVA
#include <svtools/HtmlWriter.hxx>
#else	// USE_JAVA
#include "HtmlWriter.hxx"
#endif	// USE_JAVA

class Color;
class ImageMap;
class SvStream;
class SvxMacroTableDtor;
class SvNumberFormatter;

struct HTMLOutEvent
{
    const sal_Char *pBasicName;
    const sal_Char *pJavaName;
    sal_uInt16 nEvent;
};

struct SVT_DLLPUBLIC HTMLOutContext
{
    rtl_TextEncoding m_eDestEnc;
    rtl_TextToUnicodeConverter m_hConv;
    rtl_TextToUnicodeContext   m_hContext;

    HTMLOutContext( rtl_TextEncoding eDestEnc );
    ~HTMLOutContext();
};

struct HTMLOutFuncs
{
    SVT_DLLPUBLIC static OString ConvertStringToHTML( const OUString& sSrc,
                        rtl_TextEncoding eDestEnc = RTL_TEXTENCODING_MS_1252,
                        OUString *pNonConvertableChars = 0 );

    SVT_DLLPUBLIC static SvStream& Out_AsciiTag( SvStream&, const sal_Char* pStr,
                                   bool bOn = true,
                        rtl_TextEncoding eDestEnc = RTL_TEXTENCODING_MS_1252);
    SVT_DLLPUBLIC static SvStream& Out_Char( SvStream&, sal_Unicode cChar,
                        HTMLOutContext& rContext,
                        OUString *pNonConvertableChars = 0 );
    SVT_DLLPUBLIC static SvStream& Out_String( SvStream&, const OUString&,
                        rtl_TextEncoding eDestEnc = RTL_TEXTENCODING_MS_1252,
                        OUString *pNonConvertableChars = 0 );
    SVT_DLLPUBLIC static SvStream& Out_Hex( SvStream&, sal_uLong nHex, sal_uInt8 nLen,
                        rtl_TextEncoding eDestEnc = RTL_TEXTENCODING_MS_1252 );
    SVT_DLLPUBLIC static SvStream& Out_Color( SvStream&, const Color&,
                        rtl_TextEncoding eDestEnc = RTL_TEXTENCODING_MS_1252 );
    SVT_DLLPUBLIC static SvStream& Out_ImageMap( SvStream&, const OUString&, const ImageMap&, const OUString&,
                                   const HTMLOutEvent *pEventTable,
                                   bool bOutStarBasic,
                                   const sal_Char *pDelim = 0,
                                   const sal_Char *pIndentArea = 0,
                                   const sal_Char *pIndentMap = 0,
                        rtl_TextEncoding eDestEnc = RTL_TEXTENCODING_MS_1252,
                        OUString *pNonConvertableChars = 0 );
    SVT_DLLPUBLIC static SvStream& FlushToAscii( SvStream&, HTMLOutContext& rContext );

    SVT_DLLPUBLIC static SvStream& OutScript( SvStream& rStrm,
                                const OUString& rBaseURL,
                                const OUString& rSource,
                                const OUString& rLanguage,
                                ScriptType eScriptType,
                                const OUString& rSrc,
                                const OUString *pSBLibrary = 0,
                                const OUString *pSBModule = 0,
                        rtl_TextEncoding eDestEnc = RTL_TEXTENCODING_MS_1252,
                        OUString *pNonConvertableChars = 0 );

    // der 3. Parameter ist ein Array von HTMLOutEvents, das mit einem
    // nur aus 0 bestehen Eintrag terminiert ist.
    SVT_DLLPUBLIC static SvStream& Out_Events( SvStream&, const SvxMacroTableDtor&,
                                 const HTMLOutEvent*, bool bOutStarBasic,
                        rtl_TextEncoding eDestEnc = RTL_TEXTENCODING_MS_1252,
                        OUString *pNonConvertableChars = 0 );

    // <TD SDVAL="..." SDNUM="...">
    SVT_DLLPUBLIC static OString CreateTableDataOptionsValNum(
                bool bValue, double fVal, sal_uLong nFormat,
                SvNumberFormatter& rFormatter,
                rtl_TextEncoding eDestEnc = RTL_TEXTENCODING_MS_1252,
                OUString *pNonConvertableChars = 0);
#ifndef NO_LIBO_BUG_63211_FIX
    SVT_DLLPUBLIC static bool PrivateURLToInternalImg( OUString& rURL );
#endif	// !NO_LIBO_BUG_63211_FIX
};

struct HtmlWriterHelper
{
    SVT_DLLPUBLIC static void applyColor( HtmlWriter& rHtmlWriter, const OString &aAttributeName, const Color& rColor);
    SVT_DLLPUBLIC static void applyEvents(HtmlWriter& rHtmlWriter, const SvxMacroTableDtor& rMacroTable, const HTMLOutEvent* pEventTable, bool bOutStarBasic);
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
