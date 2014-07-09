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
 * Modified March 2014 by Patrick Luby. NeoOffice is distributed under
 * GPL only under modification term 2 of the LGPL.
 *
 ************************************************************************/

#include <osl/file.h>
#include <rtl/ustring.h>

#if defined USE_JAVA && defined MACOSX
#include "../unx/system.h"
#endif	/* defined USE_JAVA && defined MACOSX */

static sal_uInt32 SAL_CALL osl_defCalcTextWidth( rtl_uString *ustrText )
{
	return ustrText ? ustrText->length : 0;
}


oslFileError SAL_CALL osl_abbreviateSystemPath( rtl_uString *ustrSystemPath, rtl_uString **pustrCompacted, sal_uInt32 uMaxWidth, oslCalcTextWidthFunc pfnCalcWidth )
{
	oslFileError	error = osl_File_E_None;
	rtl_uString		*ustrPath = NULL;
	rtl_uString		*ustrFile = NULL;
	sal_uInt32		uPathWidth, uFileWidth;

	if ( !pfnCalcWidth )
		pfnCalcWidth = osl_defCalcTextWidth;

	{
		sal_Int32	iLastSlash = rtl_ustr_lastIndexOfChar_WithLength( ustrSystemPath->buffer, ustrSystemPath->length, SAL_PATHDELIMITER );

		if ( iLastSlash >= 0 )
		{
#if defined USE_JAVA && defined MACOSX
			if ( macxp_isUbiquitousPath( ustrSystemPath->buffer, ustrSystemPath->length ) )
			{
				rtl_uString_newFromAscii( &ustrPath, "iCloud Drive: " );
				if ( ++iLastSlash < ustrSystemPath->length )
					rtl_uString_newFromStr_WithLength( &ustrFile, &ustrSystemPath->buffer[iLastSlash], ustrSystemPath->length - iLastSlash );
				else
					rtl_uString_new( &ustrFile );
			}
			else
			{
#endif	/* defined USE_JAVA && defined MACOSX */
			rtl_uString_newFromStr_WithLength( &ustrPath, ustrSystemPath->buffer, iLastSlash );
			rtl_uString_newFromStr_WithLength( &ustrFile, &ustrSystemPath->buffer[iLastSlash], ustrSystemPath->length - iLastSlash );
#if defined USE_JAVA && defined MACOSX
			}
#endif	/* defined USE_JAVA && defined MACOSX */
		}
		else
		{
			rtl_uString_new( &ustrPath );
			rtl_uString_newFromString( &ustrFile, ustrSystemPath );
		}
	}

	uPathWidth = pfnCalcWidth( ustrPath );
	uFileWidth = pfnCalcWidth( ustrFile );

	/* First abbreviate the directory component of the path */

	while ( uPathWidth + uFileWidth > uMaxWidth )
	{
		if ( ustrPath->length > 3 )
		{
			ustrPath->length--;
			ustrPath->buffer[ustrPath->length-3] = '.';
			ustrPath->buffer[ustrPath->length-2] = '.';
			ustrPath->buffer[ustrPath->length-1] = '.';
			ustrPath->buffer[ustrPath->length] = 0;

			uPathWidth = pfnCalcWidth( ustrPath );
		}
		else
			break;
	}

	/* Now abbreviate file component */

	while ( uPathWidth + uFileWidth > uMaxWidth )
	{
		if ( ustrFile->length > 4 )
		{
			ustrFile->length--;
			ustrFile->buffer[ustrFile->length-3] = '.';
			ustrFile->buffer[ustrFile->length-2] = '.';
			ustrFile->buffer[ustrFile->length-1] = '.';
			ustrFile->buffer[ustrFile->length] = 0;

			uFileWidth = pfnCalcWidth( ustrFile );
		}
		else
			break;
	}

	rtl_uString_newConcat( pustrCompacted, ustrPath, ustrFile );

	/* Event now if path was compacted to ".../..." it can be to large */

	uPathWidth += uFileWidth;

	while ( uPathWidth > uMaxWidth )
	{
		(*pustrCompacted)->length--;
		(*pustrCompacted)->buffer[(*pustrCompacted)->length] = 0;
		uPathWidth = pfnCalcWidth( *pustrCompacted );
	}

	if ( ustrPath )
		rtl_uString_release( ustrPath );

	if ( ustrFile )
		rtl_uString_release( ustrFile );

	return error;
}


