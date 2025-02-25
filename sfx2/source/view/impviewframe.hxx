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

#ifndef INCLUDED_SFX2_SOURCE_VIEW_IMPVIEWFRAME_HXX
#define INCLUDED_SFX2_SOURCE_VIEW_IMPVIEWFRAME_HXX

#include <sfx2/viewfrm.hxx>

#include <svtools/asynclink.hxx>
#include <vcl/window.hxx>

#include <boost/optional.hpp>

#if defined USE_JAVA && defined MACOSX
#include <vcl/timer.hxx>
#endif	// USE_JAVA && MACOSX

struct SfxViewFrame_Impl
{
    SvBorder            aBorder;
    Size                aMargin;
    Size                aSize;
    OUString            aActualURL;
    SfxFrame&           rFrame;
    svtools::AsynchronLink* pReloader;
    vcl::Window*             pWindow;
    SfxViewFrame*       pActiveChild;
    vcl::Window*             pFocusWin;
    sal_uInt16          nDocViewNo;
    sal_uInt16          nCurViewId;
    bool            bResizeInToOut:1;
    bool            bDontOverwriteResizeInToOut:1;
    bool            bObjLocked:1;
    bool            bReloading:1;
    bool            bIsDowning:1;
    bool            bModal:1;
    bool            bEnabled:1;
    bool            bWindowWasEnabled:1;
    bool            bActive;
    OUString            aFactoryName;
    ::boost::optional< bool >
                        aHasToolPanels;
#if defined USE_JAVA && defined MACOSX
    sal_Bool            bNeedsUpdateTitle;
    Timer               aTimer;
#endif	// USE_JAVA && MACOSX

    SfxViewFrame_Impl(SfxFrame& i_rFrame)
        : rFrame(i_rFrame)
        , pReloader(0)
        , pWindow(0)
        , pActiveChild(0)
        , pFocusWin(0)
        , nDocViewNo(0)
        , nCurViewId(0)
        , bResizeInToOut(false)
        , bDontOverwriteResizeInToOut(false)
        , bObjLocked(false)
        , bReloading(false)
        , bIsDowning(false)
        , bModal(false)
        , bEnabled(false)
        , bWindowWasEnabled(true)
        , bActive(false)
#if defined USE_JAVA && defined MACOSX
        , bNeedsUpdateTitle(sal_True)
#endif	// USE_JAVA && MACOSX
    {
    }

    ~SfxViewFrame_Impl()
    {
        delete pReloader;
    }
};

class SfxFrameViewWindow_Impl : public vcl::Window
{
    SfxViewFrame*   pFrame;

public:
                        SfxFrameViewWindow_Impl( SfxViewFrame* p, vcl::Window& rParent, WinBits nBits=0 ) :
                            Window( &rParent, nBits | WB_CLIPCHILDREN ),
                            pFrame( p )
                        {
                            p->GetFrame().GetWindow().SetBorderStyle( WindowBorderStyle::NOBORDER );
                        }

    virtual void        Resize() SAL_OVERRIDE;
    virtual void        StateChanged( StateChangedType nStateChange ) SAL_OVERRIDE;
};

#endif // INCLUDED_SFX2_SOURCE_VIEW_IMPVIEWFRAME_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
