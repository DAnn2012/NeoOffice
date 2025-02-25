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
 *   Modified May 2018 by Patrick Luby. NeoOffice is only distributed
 *   under the GNU General Public License, Version 3 as allowed by Section 3.3
 *   of the Mozilla Public License, v. 2.0.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef INCLUDED_CHART2_SOURCE_INC_MACROS_HXX
#define INCLUDED_CHART2_SOURCE_INC_MACROS_HXX

#include "sal/config.h"

#include <typeinfo>

#include "sal/log.hxx"

#define ASSERT_EXCEPTION(ex)                   \
  SAL_WARN("chart2", "Exception caught. Type: " <<\
    typeid( ex ).name() << ", Message: " << \
    ex.Message )

#if defined USE_JAVA && defined MACOSX
// Fix Mac App Store crash when loading the GL3D bar chart by disabling it
#define ENABLE_GL3D_BARCHART 0
#else	// USE_JAVA && MACOSX
#define ENABLE_GL3D_BARCHART 1
#endif	// USE_JAVA && MACOSX

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
