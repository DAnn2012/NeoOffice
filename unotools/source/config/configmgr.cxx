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
 *   Modified November 2016 by Patrick Luby. NeoOffice is only distributed
 *   under the GNU General Public License, Version 3 as allowed by Section 3.3
 *   of the Mozilla Public License, v. 2.0.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <sal/config.h>

#include <list>

#include <boost/noncopyable.hpp>
#include <com/sun/star/beans/NamedValue.hpp>
#include <com/sun/star/container/XHierarchicalNameAccess.hpp>
#include <com/sun/star/configuration/theDefaultProvider.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/uno/Any.hxx>
#include <com/sun/star/uno/Reference.hxx>
#include <com/sun/star/uno/Sequence.hxx>
#include <osl/diagnose.h>
#include <rtl/instance.hxx>
#include <rtl/ustring.h>
#include <rtl/ustring.hxx>
#include <unotools/configitem.hxx>
#include <unotools/configmgr.hxx>
#include <comphelper/processfactory.hxx>

#if defined USE_JAVA && defined MACOSX

#include <premac.h>
#import <CoreFoundation/CoreFoundation.h>
#include <postmac.h>

#endif	// USE_JAVA && MACOSX

namespace {

class RegisterConfigItemHelper: private boost::noncopyable {
public:
    RegisterConfigItemHelper(
        utl::ConfigManager & manager, utl::ConfigItem & item):
            manager_(manager), item_(&item)
    {
        manager.registerConfigItem(item_);
    }

    ~RegisterConfigItemHelper() {
        if (item_ != 0) {
            manager_.removeConfigItem(*item_);
        }
    }

    void keep() { item_ = 0; }

private:
    utl::ConfigManager & manager_;
    utl::ConfigItem * item_;
};

css::uno::Reference< css::lang::XMultiServiceFactory >
getConfigurationProvider() {
    return css::configuration::theDefaultProvider::get( comphelper::getProcessComponentContext() );
}

OUString getConfigurationString(OUString const & module, OUString const & path)
{
    css::uno::Sequence< css::uno::Any > args(1);
    args[0] <<= css::beans::NamedValue(
        OUString("nodepath"),
        css::uno::makeAny(module));
    return
        css::uno::Reference< css::container::XHierarchicalNameAccess >(
            getConfigurationProvider()->createInstanceWithArguments(
                OUString("com.sun.star.configuration.ConfigurationAccess"),
                args),
            css::uno::UNO_QUERY_THROW)->
        getByHierarchicalName(path).get< OUString >();
}

struct theConfigManager:
    public rtl::Static< utl::ConfigManager, theConfigManager >
{};

}

OUString utl::ConfigManager::getAboutBoxProductVersion() {
    return getConfigurationString(
        OUString("/org.openoffice.Setup"),
        OUString("Product/ooSetupVersionAboutBox"));
}

OUString utl::ConfigManager::getAboutBoxProductVersionSuffix() {
    return getConfigurationString(
        OUString("/org.openoffice.Setup"),
        OUString("Product/ooSetupVersionAboutBoxSuffix"));
}

OUString utl::ConfigManager::getDefaultCurrency() {
    return getConfigurationString(
        OUString("/org.openoffice.Setup"),
        OUString("L10N/ooSetupCurrency"));
}

OUString utl::ConfigManager::getLocale() {
    return getConfigurationString(
        OUString("/org.openoffice.Setup"),
        OUString("L10N/ooLocale"));
}

OUString utl::ConfigManager::getProductExtension() {
    return getConfigurationString(
        OUString("/org.openoffice.Setup"),
        OUString("Product/ooSetupExtension"));
}

OUString utl::ConfigManager::getProductName() {
#if defined USE_JAVA && defined MACOSX
    // Use CFBundleName for product name if it exists
    CFBundleRef aBundle = CFBundleGetMainBundle();
    if ( aBundle )
    {
        CFDictionaryRef aDict = CFBundleGetInfoDictionary( aBundle );
        if ( aDict )
        {
            CFStringRef aValue = (CFStringRef)CFDictionaryGetValue( aDict, CFSTR( "CFBundleName" ) );
            if ( aValue && CFGetTypeID( aValue ) == CFStringGetTypeID() )
            {
                CFIndex nValueLen = CFStringGetLength( aValue );
                CFRange aValueRange = CFRangeMake( 0, nValueLen );
                sal_Unicode pValueBuffer[ nValueLen + 1 ];
                CFStringGetCharacters( aValue, aValueRange, pValueBuffer );
                pValueBuffer[ nValueLen ] = 0;
                OUString aProductName( pValueBuffer );
                if ( aProductName.getLength() )
                    return aProductName;
            }
        }
    }
#endif	// USE_JAVA && MACOSX

    return getConfigurationString(
        OUString("/org.openoffice.Setup"),
        OUString("Product/ooName"));
}

OUString utl::ConfigManager::getProductVersion() {
    return getConfigurationString(
        OUString("/org.openoffice.Setup"),
        OUString("Product/ooSetupVersion"));
}

OUString utl::ConfigManager::getVendor() {
    return getConfigurationString(
        OUString("/org.openoffice.Setup"),
        OUString("Product/ooVendor"));
}

void utl::ConfigManager::storeConfigItems() {
    getConfigManager().doStoreConfigItems();
}

utl::ConfigManager & utl::ConfigManager::getConfigManager() {
    return theConfigManager::get();
}

css::uno::Reference< css::container::XHierarchicalNameAccess >
utl::ConfigManager::acquireTree(utl::ConfigItem & item) {
    css::uno::Sequence< css::uno::Any > args(1);
    args[0] <<= css::beans::NamedValue(
        OUString("nodepath"),
        css::uno::makeAny(
            OUString("/org.openoffice.") + item.GetSubTreeName()));
    if ((item.GetMode() & CONFIG_MODE_ALL_LOCALES) != 0) {
        args.realloc(2);
        args[1] <<= css::beans::NamedValue(OUString("locale"), css::uno::makeAny(OUString("*")));
    }
    return css::uno::Reference< css::container::XHierarchicalNameAccess >(
        getConfigurationProvider()->createInstanceWithArguments(
            OUString("com.sun.star.configuration.ConfigurationUpdateAccess"),
            args),
        css::uno::UNO_QUERY_THROW);
}

utl::ConfigManager::ConfigManager() {}

utl::ConfigManager::~ConfigManager() {
    OSL_ASSERT(items_.empty());
}

css::uno::Reference< css::container::XHierarchicalNameAccess >
utl::ConfigManager::addConfigItem(utl::ConfigItem & item) {
    RegisterConfigItemHelper reg(*this, item);
    css::uno::Reference< css::container::XHierarchicalNameAccess > tree(
        acquireTree(item));
    reg.keep();
    return tree;
}

void utl::ConfigManager::removeConfigItem(utl::ConfigItem & item) {
    for (std::list< ConfigItem * >::iterator i(items_.begin());
         i != items_.end(); ++i)
    {
        if (*i == &item) {
            items_.erase(i);
            break;
        }
    }
}

void utl::ConfigManager::registerConfigItem(utl::ConfigItem * item) {
    OSL_ASSERT(item != 0);
    items_.push_back(item);
}

void utl::ConfigManager::doStoreConfigItems() {
    for (std::list< ConfigItem * >::iterator i(items_.begin());
         i != items_.end(); ++i)
    {
        if ((*i)->IsModified()) {
            (*i)->Commit();
            (*i)->ClearModified();
        }
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
