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

#include <string.h>
#include <boost/ptr_container/ptr_vector.hpp>

#include <comphelper/processfactory.hxx>
#include <comphelper/string.hxx>
#include <osl/mutex.hxx>
#include <rtl/process.h>
#include <tools/debug.hxx>
#include <tools/resary.hxx>
#include <tools/gen.hxx>
#include <cppuhelper/implbase1.hxx>
#include <uno/current_context.hxx>

#include "vcl/configsettings.hxx"
#include "vcl/svapp.hxx"
#include "vcl/settings.hxx"
#include "vcl/wrkwin.hxx"
#include "vcl/layout.hxx"
#include "vcl/button.hxx"
#include "vcl/dockwin.hxx"
#include "salinst.hxx"
#include "salframe.hxx"
#include "salgdi.hxx"
#include "svdata.hxx"
#include "window.h"
#include "salimestatus.hxx"
#include "salsys.hxx"
#include "svids.hrc"

#include "com/sun/star/accessibility/MSAAService.hpp"

#include "officecfg/Office/Common.hxx"

#include "vcl/opengl/OpenGLContext.hxx"

#include <stdio.h>

using namespace com::sun::star::uno;
using namespace com::sun::star::lang;
using namespace com::sun::star::awt;

namespace
{
    struct private_aImplSVData :
        public rtl::Static<ImplSVData, private_aImplSVData> {};
}

// static SV-Data
ImplSVData* pImplSVData = NULL;

SalSystem* ImplGetSalSystem()
{
    ImplSVData* pSVData = ImplGetSVData();
    if( ! pSVData->mpSalSystem )
        pSVData->mpSalSystem = pSVData->mpDefInst->CreateSalSystem();
    return pSVData->mpSalSystem;
}

void ImplInitSVData()
{
    pImplSVData = &private_aImplSVData::get();

    // init global instance data
    memset( pImplSVData, 0, sizeof( ImplSVData ) );
    pImplSVData->maHelpData.mbAutoHelpId = true;
    pImplSVData->maNWFData.maMenuBarHighlightTextColor = Color( COL_TRANSPARENT );

    // mark default layout border as unitialized
    pImplSVData->maAppData.mnDefaultLayoutBorder = -1;
}

void ImplDeInitSVData()
{
    ImplSVData* pSVData = ImplGetSVData();

#if defined USE_JAVA && defined MACOSX
    // Fix random crashing in native callbacks that get called after destroying
    // the SalInstance by destroying it here instead of in DeInitVCL()
    DestroySalInstance( pSVData->mpDefInst );
#endif  // USE_JAVA && MACOSX

    // delete global instance data
    if( pSVData->mpSettingsConfigItem )
        delete pSVData->mpSettingsConfigItem;

    if( pSVData->mpDockingManager )
        delete pSVData->mpDockingManager;

    if( pSVData->maCtrlData.mpFieldUnitStrings )
        delete pSVData->maCtrlData.mpFieldUnitStrings, pSVData->maCtrlData.mpFieldUnitStrings = NULL;
    if( pSVData->maCtrlData.mpCleanUnitStrings )
        delete pSVData->maCtrlData.mpCleanUnitStrings, pSVData->maCtrlData.mpCleanUnitStrings = NULL;
    if( pSVData->mpPaperNames )
        delete pSVData->mpPaperNames, pSVData->mpPaperNames = NULL;
}

void ImplDestroySVData()
{
    pImplSVData = NULL;
}

vcl::Window* ImplGetDefaultWindow()
{
    ImplSVData* pSVData = ImplGetSVData();
    if ( pSVData->maWinData.mpAppWin )
        return pSVData->maWinData.mpAppWin;

    // First test if we already have a default window.
    // Don't only place a single if..else inside solar mutex lockframe
    // because then we might have to wait for the solar mutex what is not necessary
    // if we already have a default window.

    if ( !pSVData->mpDefaultWin )
    {
        Application::GetSolarMutex().acquire();

        // Test again because the thread who released the solar mutex could have called
        // the same method

        if ( !pSVData->mpDefaultWin && !pSVData->mbDeInit )
        {
            DBG_WARNING( "ImplGetDefaultWindow(): No AppWindow" );
            pSVData->mpDefaultWin = new WorkWindow( 0, WB_DEFAULTWIN );
            pSVData->mpDefaultWin->SetText( OUString( "VCL ImplGetDefaultWindow"  ) );

            // Add a reference to the default context so it never gets deleted
            OpenGLContext* pContext = pSVData->mpDefaultWin->GetGraphics()->GetOpenGLContext();
            if( pContext )
            {
#ifdef DBG_UTIL
                pContext->AddRef(NULL);
#else
                pContext->AddRef();
#endif
            }
        }
        Application::GetSolarMutex().release();
    }

    return pSVData->mpDefaultWin;
}

ResMgr* ImplGetResMgr()
{
    ImplSVData* pSVData = ImplGetSVData();
    if ( !pSVData->mpResMgr )
    {
        LanguageTag aLocale( Application::GetSettings().GetUILanguageTag());
        pSVData->mpResMgr = ResMgr::SearchCreateResMgr( "vcl", aLocale );

        static bool bMessageOnce = false;
        if( !pSVData->mpResMgr && ! bMessageOnce )
        {
            bMessageOnce = true;
            const char* pMsg =
                "Missing vcl resource. This indicates that files vital to localization are missing. "
                "You might have a corrupt installation.";
            fprintf( stderr, "%s\n", pMsg );
            MessageDialog aBox(NULL, OUString(pMsg, strlen(pMsg), RTL_TEXTENCODING_ASCII_US));
            aBox.Execute();
        }
    }
    return pSVData->mpResMgr;
}

ResId VclResId( sal_Int32 nId )
{
    ResMgr* pMgr = ImplGetResMgr();
    if( ! pMgr )
        throw std::bad_alloc();

    return ResId( nId, *pMgr );
}

FieldUnitStringList* ImplGetFieldUnits()
{
    ImplSVData* pSVData = ImplGetSVData();
    if( ! pSVData->maCtrlData.mpFieldUnitStrings )
    {
        ResMgr* pResMgr = ImplGetResMgr();
        if( pResMgr )
        {
            ResStringArray aUnits( ResId (SV_FUNIT_STRINGS, *pResMgr) );
            sal_uInt32 nUnits = aUnits.Count();
            pSVData->maCtrlData.mpFieldUnitStrings = new FieldUnitStringList();
            pSVData->maCtrlData.mpFieldUnitStrings->reserve( nUnits );
            for( sal_uInt32 i = 0; i < nUnits; i++ )
            {
                std::pair< OUString, FieldUnit > aElement( aUnits.GetString(i), static_cast<FieldUnit>(aUnits.GetValue(i)) );
                pSVData->maCtrlData.mpFieldUnitStrings->push_back( aElement );
            }
        }
    }
    return pSVData->maCtrlData.mpFieldUnitStrings;
}

FieldUnitStringList* ImplGetCleanedFieldUnits()
{
    ImplSVData* pSVData = ImplGetSVData();
    if( ! pSVData->maCtrlData.mpCleanUnitStrings )
    {
        FieldUnitStringList* pUnits = ImplGetFieldUnits();
        if( pUnits )
        {
            size_t nUnits = pUnits->size();
            pSVData->maCtrlData.mpCleanUnitStrings = new FieldUnitStringList();
            pSVData->maCtrlData.mpCleanUnitStrings->reserve( nUnits );
            for( size_t i = 0; i < nUnits; ++i )
            {
                OUString aUnit( (*pUnits)[i].first );
                aUnit = comphelper::string::remove(aUnit, ' ');
                aUnit = aUnit.toAsciiLowerCase();
                std::pair< OUString, FieldUnit > aElement( aUnit, (*pUnits)[i].second );
                pSVData->maCtrlData.mpCleanUnitStrings->push_back( aElement );
            }
        }
    }
    return pSVData->maCtrlData.mpCleanUnitStrings;
}

DockingManager* ImplGetDockingManager()
{
    ImplSVData* pSVData = ImplGetSVData();
    if ( !pSVData->mpDockingManager )
        pSVData->mpDockingManager = new DockingManager();

    return pSVData->mpDockingManager;
}

BlendFrameCache* ImplGetBlendFrameCache()
{
    ImplSVData* pSVData = ImplGetSVData();
    if ( !pSVData->mpBlendFrameCache)
        pSVData->mpBlendFrameCache= new BlendFrameCache();

    return pSVData->mpBlendFrameCache;
}

#ifdef _WIN32
bool HasAtHook();
#endif

bool ImplInitAccessBridge()
{
    ImplSVData* pSVData = ImplGetSVData();
    if( ! pSVData->mxAccessBridge.is() )
    {
        css::uno::Reference< XComponentContext > xContext(comphelper::getProcessComponentContext());

#ifdef _WIN32
        if (!HasAtHook() && !getenv("SAL_FORCE_IACCESSIBLE2"))
        {
            SAL_INFO("vcl", "Apparently no running AT -> "
                     "not enabling IAccessible2 integration");
        }
        else
        {
            try {
                 pSVData->mxAccessBridge
                     = css::accessibility::MSAAService::create(xContext);
                 SAL_INFO("vcl", "got IAccessible2 bridge");
                 return true;
             } catch (css::uno::DeploymentException & e) {
                 SAL_WARN(
                    "vcl",
                    "got no IAccessible2 bridge" << e.Message);
                 return false;
             }
        }
#endif
    }

    return true;
}

vcl::Window* ImplFindWindow( const SalFrame* pFrame, ::Point& rSalFramePos )
{
    ImplSVData* pSVData = ImplGetSVData();
    vcl::Window*     pFrameWindow = pSVData->maWinData.mpFirstFrame;
    while ( pFrameWindow )
    {
        if ( pFrameWindow->ImplGetFrame() == pFrame )
        {
            vcl::Window* pWindow = pFrameWindow->ImplFindWindow( rSalFramePos );
            if ( !pWindow )
                pWindow = pFrameWindow->ImplGetWindow();
            rSalFramePos = pWindow->ImplFrameToOutput( rSalFramePos );
            return pWindow;
        }
        pFrameWindow = pFrameWindow->ImplGetFrameData()->mpNextFrame;
    }

    return NULL;
}

void LocaleConfigurationListener::ConfigurationChanged( utl::ConfigurationBroadcaster*, sal_uInt32 nHint )
{
    AllSettings::LocaleSettingsChanged( nHint );
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
