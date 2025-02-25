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
 *   Modified March 2017 by Patrick Luby. NeoOffice is only distributed
 *   under the GNU General Public License, Version 3 as allowed by Section 3.3
 *   of the Mozilla Public License, v. 2.0.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef INCLUDED_VCL_INC_IMPIMAGETREE_HXX
#define INCLUDED_VCL_INC_IMPIMAGETREE_HXX

#include "sal/config.h"

#include <vector>

#include <boost/unordered_map.hpp>

#include "boost/noncopyable.hpp"
#include "com/sun/star/uno/Reference.hxx"
#include "rtl/ustring.hxx"
#include "salhelper/singletonref.hxx"

namespace com { namespace sun { namespace star { namespace container {
    class XNameAccess;
} } } }
class BitmapEx;

class ImplImageTree: private boost::noncopyable {
public:
    ImplImageTree();

    ~ImplImageTree();

    bool loadImage(
        OUString const & name, OUString const & style,
        BitmapEx & bitmap, bool localized = false, bool loadMissing = false );

    bool loadDefaultImage(
        OUString const & style,
        BitmapEx& bitmap);

/** a crude form of life cycle control (called from DeInitVCL; otherwise,
 *  if the ImplImageTree singleton were destroyed during exit that would
 *  be too late for the destructors of the bitmaps in m_iconCache)*/
    void shutDown();

    css::uno::Reference< css::container::XNameAccess > getNameAccess();

private:
    bool doLoadImage(
        OUString const & name, OUString const & style,
        BitmapEx & bitmap, bool localized);

    typedef std::pair<
        OUString,
        css::uno::Reference< css::container::XNameAccess > > Path;

    typedef boost::unordered_map<
        OUString, bool, OUStringHash > CheckStyleCache;
    typedef boost::unordered_map<
        OUString, std::pair< bool, BitmapEx >, OUStringHash > IconCache;
    typedef boost::unordered_map<
        OUString, OUString, OUStringHash > IconLinkHash;

    OUString m_style;
    Path m_path;
    IconCache m_iconCache;
    bool m_cacheIcons;
    IconLinkHash m_linkHash;

    bool checkPathAccess();

    void setStyle(OUString const & style );

    void resetPaths();

    bool iconCacheLookup( OUString const & name, bool localized, BitmapEx & bitmap );

    bool find(std::vector< OUString > const & paths, BitmapEx & bitmap );

    void loadImageLinks();
#ifdef USE_JAVA
    void parseLinkFile(boost::shared_ptr< SvStream > stream, bool bFirst);
#else	// USE_JAVA
    void parseLinkFile(boost::shared_ptr< SvStream > stream);
#endif	// USE_JAVA
    OUString const & getRealImageName(OUString const & name);
};

typedef salhelper::SingletonRef< ImplImageTree > ImplImageTreeSingletonRef;

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
