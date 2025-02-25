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
 *  Copyright 2008 by Planamesa Inc.
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
 *************************************************************************/

#import "neomobilei18n.hxx"
#import <unotools/localedatawrapper.hxx>
#import <vcl/svapp.hxx>

static NSString *pDecimalSep = nil;

using namespace com::sun::star::lang;
using namespace rtl;

Locale NeoMobileGetApplicationLocale()
{
	return Application::GetSettings().GetUILocale();
}

NSString *NeoMobileGetLocalizedDecimalSeparator()
{
	if ( !pDecimalSep )
	{
		OUString aDecimalSep( Application::GetAppLocaleDataWrapper().getNumDecimalSep() );
		if ( !aDecimalSep.getLength() )
			aDecimalSep = OUString( RTL_CONSTASCII_USTRINGPARAM( "." ) );
		pDecimalSep = [NSString stringWithCharacters:aDecimalSep.getStr() length:aDecimalSep.getLength()];
		if ( pDecimalSep )
			[pDecimalSep retain];
	}

	return pDecimalSep;
}
