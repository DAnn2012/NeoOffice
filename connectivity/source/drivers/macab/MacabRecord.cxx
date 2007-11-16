/*************************************************************************
 *
 *  OpenOffice.org - a multi-platform office productivity suite
 *
 *  $RCSfile$
 *
 *  $Revision$
 *
 *  last change: $Author$ $Date$
 *
 *  The Contents of this file are made available subject to
 *  the terms of GNU Lesser General Public License Version 2.1.
 *
 *
 *    GNU Lesser General Public License Version 2.1
 *    =============================================
 *    Copyright 2005 by Sun Microsystems, Inc.
 *    901 San Antonio Road, Palo Alto, CA 94303, USA
 *
 *    This library is free software; you can redistribute it and/or
 *    modify it under the terms of the GNU Lesser General Public
 *    License version 2.1, as published by the Free Software Foundation.
 *
 *    This library is distributed in the hope that it will be useful,
 *    but WITHOUT ANY WARRANTY; without even the implied warranty of
 *    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *    Lesser General Public License for more details.
 *
 *    You should have received a copy of the GNU Lesser General Public
 *    License along with this library; if not, write to the Free Software
 *    Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 *    MA  02111-1307  USA
 *
 ************************************************************************/

// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_connectivity.hxx"

#include "MacabRecord.hxx"

#ifndef _CONNECTIVITY_MACAB_UTILITIES_HXX_
#include "macabutilities.hxx"
#endif

#ifndef _COM_SUN_STAR_UTIL_DATETIME_HPP_
#include <com/sun/star/util/DateTime.hpp>
#endif

#include <premac.h>
#include <Carbon/Carbon.h>
#include <AddressBook/ABAddressBookC.h>
#include <postmac.h>

#ifndef _DBHELPER_DBCONVERSION_HXX_
#include <connectivity/dbconversion.hxx>
#endif

using namespace connectivity::macab;
using namespace com::sun::star::util;
using namespace ::dbtools;

// -------------------------------------------------------------------------
MacabRecord::MacabRecord()
{
}

// -------------------------------------------------------------------------
MacabRecord::MacabRecord(const sal_Int32 _size)
{
	size = _size;
	fields = new macabfield *[size];
	sal_Int32 i;
	for(i = 0; i < size; i++)
		fields[i] = NULL;
}

// -------------------------------------------------------------------------
MacabRecord::~MacabRecord()
{
}

// -------------------------------------------------------------------------
void MacabRecord::insertAtColumn (CFTypeRef _value, ABPropertyType _type, const sal_Int32 _column)
{
	if(_column < size)
	{
		if(fields[_column] == NULL)
			fields[_column] = new macabfield;

		fields[_column]->value = _value;
		if (fields[_column]->value)
			CFRetain(fields[_column]->value);
		fields[_column]->type = _type;
	}
}

// -------------------------------------------------------------------------
sal_Bool MacabRecord::contains (const macabfield *_field) const
{
	if(_field == NULL)
		return sal_False;
	else
		return contains(_field->value);
}

// -------------------------------------------------------------------------
sal_Bool MacabRecord::contains (const CFTypeRef _value) const
{
	sal_Int32 i;
	for(i = 0; i < size; i++)
	{
		if(fields[i] != NULL)
		{
			if(CFEqual(fields[i]->value, _value))
			{
				return sal_True;
			}
		}
	}

	return sal_False;
}

// -------------------------------------------------------------------------
sal_Int32 MacabRecord::getSize() const
{
	return size;
}

// -------------------------------------------------------------------------
macabfield *MacabRecord::copy(const sal_Int32 i) const
{
	/* Note: copy(i) creates a new macabfield identical to that at
	 * location i, whereas get(i) returns a pointer to the macabfield
	 * at location i.
	 */
	if(i < size)
	{
		macabfield *_copy = new macabfield;
		_copy->type = fields[i]->type;
		_copy->value = fields[i]->value;
		if (_copy->value)
			CFRetain(_copy->value);
		return _copy;
	}

	return NULL;
}

// -------------------------------------------------------------------------
macabfield *MacabRecord::get(const sal_Int32 i) const
{
	/* Note: copy(i) creates a new macabfield identical to that at
	 * location i, whereas get(i) returns a pointer to the macabfield
	 * at location i.
	 */
	if(i < size)
	{
		return fields[i];
	}

	return NULL;
}

// -------------------------------------------------------------------------
void MacabRecord::releaseFields()
{
	/* This method is, at the moment, only used in MacabHeader.cxx, but
	 * the idea is simple: if you are not destroying this object but want
	 * to clear it of its macabfields, you should release each field's
	 * value.
	 */
	sal_Int32 i;
	for(i = 0; i < size; i++)
		CFRelease(fields[i]->value);
}

// -------------------------------------------------------------------------
sal_Int32 MacabRecord::compareFields(const macabfield *_field1, const macabfield *_field2) 
{
	
	/* When comparing records, if either field is NULL (and the other is
	 * not), that field is considered "greater than" the other, so that it
	 * shows up later in the list when fields are ordered.
	 */
	if(_field1 == _field2)
		return 0;
	if(_field1 == NULL)
		return 1;
	if(_field2 == NULL)
		return -1;

	/* If they aren't the same type, for now, return the one with
	 * the smaller type ID... I don't know of a better way to compare
	 * two different data types.
	 */
	if(_field1->type != _field2->type)
		return(_field1->type - _field2->type);

	CFComparisonResult result;

	/* Carbon has a unique compare function for each data type: */
	switch(_field1->type)
	{
		case kABStringProperty:
			result = CFStringCompare(
				(CFStringRef) _field1->value,
				(CFStringRef) _field2->value,
				0); // 0 = no options (like ignore case)
			break;

		case kABDateProperty:
			result = CFDateCompare(
				(CFDateRef) _field1->value,
				(CFDateRef) _field2->value,
				NULL); // NULL = unused variable
			break;

		case kABIntegerProperty:
		case kABRealProperty:
			result = CFNumberCompare(
				(CFNumberRef) _field1->value,
				(CFNumberRef) _field2->value,
				NULL); // NULL = unused variable
		break;

		default:
			result = kCFCompareEqualTo; // can't compare
	}

	return (sal_Int32) result;
}

// -------------------------------------------------------------------------
/* Create a macabfield out of an OUString and type. Together with the
 * method fieldToString() (below), it is possible to switch conveniently
 * between an OUString and a macabfield (for use when creating and handling
 * SQL statement).
 */
macabfield *MacabRecord::createMacabField(const ::rtl::OUString _newFieldString, const ABPropertyType _abType)
{
	macabfield *newField = NULL;
	switch(_abType)
	{
		case kABStringProperty:
			newField = new macabfield;
			newField->value = OUStringToCFString(_newFieldString);
			newField->type = _abType;
			break;
		case kABDateProperty:
			{
				DateTime aDateTime = DBTypeConversion::toDateTime(_newFieldString);
				
				// bad format...
				if(aDateTime.Year == 0 && aDateTime.Month == 0 && aDateTime.Day == 0)
				{
				}
				else
				{
					double nTime = DBTypeConversion::toDouble(aDateTime, DBTypeConversion::getStandardDate());
					nTime -= kCFAbsoluteTimeIntervalSince1970;
					newField = new macabfield;
					newField->value = CFDateCreate(NULL, (CFAbsoluteTime) nTime);
					newField->type = _abType;
				}
			}
			break;
		case kABIntegerProperty:
			try
			{
				sal_Int64 nVal = _newFieldString.toInt64();

				newField = new macabfield;
				newField->value = CFNumberCreate(NULL,kCFNumberLongType, &nVal);
				newField->type = _abType;
			}
			// bad format...
			catch(...)
			{
			}
			break;
		case kABRealProperty:
			try
			{
				double nVal = _newFieldString.toDouble();

				newField = new macabfield;
				newField->value = CFNumberCreate(NULL,kCFNumberDoubleType, &nVal);
				newField->type = _abType;
			}
			// bad format...
			catch(...)
			{
			}
			break;
		default:
			;
	}
	return newField;
}

// -------------------------------------------------------------------------
/* Create an OUString out of a macabfield. Together with the method
 * createMacabField() (above), it is possible to switch conveniently
 * between an OUString and a macabfield (for use when creating and handling
 * SQL statement).
 */
::rtl::OUString MacabRecord::fieldToString(const macabfield *_aField)
{
	if(_aField == NULL)
		return ::rtl::OUString();

	::rtl::OUString fieldString;

	switch(_aField->type)
	{
		case kABStringProperty:
			fieldString = CFStringToOUString((CFStringRef) _aField->value);
			break;
		case kABDateProperty:
			{
				DateTime aTime = CFDateToDateTime((CFDateRef) _aField->value);
				fieldString = DBTypeConversion::toDateTimeString(aTime);
			}
			break;
		case kABIntegerProperty:
			{
				CFNumberType numberType = CFNumberGetType( (CFNumberRef) _aField->value );
				sal_Int64 nVal;
				// Should we check for the wrong type here, e.g., a float?
				sal_Bool m_bSuccess = !CFNumberGetValue((CFNumberRef) _aField->value, numberType, &nVal);
				if(m_bSuccess != sal_False)
					fieldString = ::rtl::OUString::valueOf(nVal);
			}
			break;
		case kABRealProperty:
			{
				CFNumberType numberType = CFNumberGetType( (CFNumberRef) _aField->value );
				double nVal;
				// Should we check for the wrong type here, e.g., an int?
				sal_Bool m_bSuccess = !CFNumberGetValue((CFNumberRef) _aField->value, numberType, &nVal);
				if(m_bSuccess != sal_False)
					fieldString = ::rtl::OUString::valueOf(nVal);
			}
			break;
		default:
			;
	}
	return fieldString;

}
