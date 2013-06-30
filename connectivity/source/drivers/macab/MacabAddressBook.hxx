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
 * Modified June 2013 by Patrick Luby. NeoOffice is distributed under
 * GPL only under modification term 2 of the LGPL.
 *
 ************************************************************************/

#ifndef _CONNECTIVITY_MACAB_ADDRESSBOOK_HXX_
#define _CONNECTIVITY_MACAB_ADDRESSBOOK_HXX_

#include "MacabRecords.hxx"
#include "MacabGroup.hxx"

#include <vector>

#include <premac.h>
#include <Carbon/Carbon.h>
#include <AddressBook/ABAddressBookC.h>
#include <postmac.h>

namespace connectivity
{
	namespace macab
	{
		class MacabAddressBook
		{
			protected:
				ABAddressBookRef m_aAddressBook;
				MacabRecords *m_xMacabRecords;
				::std::vector<MacabGroup *> m_xMacabGroups;
				sal_Bool m_bRetrievedGroups;
			private:
				void manageDuplicateGroups(::std::vector<MacabGroup *> _xGroups) const;
			public:
				MacabAddressBook();
				~MacabAddressBook();
				static const ::rtl::OUString & getDefaultTableName();

				MacabRecords *getMacabRecords();
				::std::vector<MacabGroup *> getMacabGroups();

				MacabGroup *getMacabGroup(::rtl::OUString _groupName);
				MacabRecords *getMacabRecords(const ::rtl::OUString _tableName);

				MacabGroup *getMacabGroupMatch(::rtl::OUString _groupName);
				MacabRecords *getMacabRecordsMatch(const ::rtl::OUString _tableName);

#ifdef USE_JAVA
				bool isValid() { return ( m_aAddressBook ? true : false ); }
#endif	// USE_JAVA
		};
		
	}
}

#endif // _CONNECTIVITY_MACAB_ADDRESSBOOK_HXX_
