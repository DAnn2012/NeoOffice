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
 *   Modified October 2016 by Patrick Luby. NeoOffice is only distributed
 *   under the GNU General Public License, Version 3 as allowed by Section 3.3
 *   of the Mozilla Public License, v. 2.0.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDED_CONNECTIVITY_SOURCE_DRIVERS_MACAB_MACABADDRESSBOOK_HXX
#define INCLUDED_CONNECTIVITY_SOURCE_DRIVERS_MACAB_MACABADDRESSBOOK_HXX

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
                bool m_bRetrievedGroups;
            private:
                void manageDuplicateGroups(::std::vector<MacabGroup *> _xGroups) const;
            public:
                MacabAddressBook();
                ~MacabAddressBook();
                static const OUString & getDefaultTableName();

                MacabRecords *getMacabRecords();
                ::std::vector<MacabGroup *> getMacabGroups();

                MacabGroup *getMacabGroup(const OUString& _groupName);
                MacabRecords *getMacabRecords(const OUString& _tableName);

                MacabGroup *getMacabGroupMatch(const OUString& _groupName);
                MacabRecords *getMacabRecordsMatch(const OUString& _tableName);

#ifdef USE_JAVA
                bool isValid() { return ( m_aAddressBook ? true : false ); }
#endif	// USE_JAVA
        };

    }
}

#endif // INCLUDED_CONNECTIVITY_SOURCE_DRIVERS_MACAB_MACABADDRESSBOOK_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
