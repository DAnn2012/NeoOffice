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
 *   Modified December 2016 by Patrick Luby. NeoOffice is only distributed
 *   under the GNU General Public License, Version 3 as allowed by Section 3.3
 *   of the Mozilla Public License, v. 2.0.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <tools/rc.h>

#include <vcl/event.hxx>
#include <vcl/layout.hxx>
#include <vcl/svapp.hxx>
#include <vcl/tabpage.hxx>
#include <vcl/tabctrl.hxx>
#include <vcl/bitmapex.hxx>
#include <vcl/settings.hxx>

#include <svdata.hxx>

#include <com/sun/star/accessibility/XAccessible.hpp>

void TabPage::ImplInit( vcl::Window* pParent, WinBits nStyle )
{
    if ( !(nStyle & WB_NODIALOGCONTROL) )
        nStyle |= WB_DIALOGCONTROL;

    Window::ImplInit( pParent, nStyle, NULL );

    ImplInitSettings();

    // if the tabpage is drawn (ie filled) by a native widget, make sure all contols will have transparent background
    // otherwise they will paint with a wrong background
    if( IsNativeControlSupported(CTRL_TAB_BODY, PART_ENTIRE_CONTROL) && GetParent() && (GetParent()->GetType() == WINDOW_TABCONTROL) )
        EnableChildTransparentMode( true );
#if defined USE_JAVA && defined MACOSX
    else if ( IsNativeControlSupported( CTRL_TAB_PANE, PART_ENTIRE_CONTROL ) && GetParent() )
    {
        if ( GetParent()->GetType() == WINDOW_CONTAINER )
        {
            SetMouseTransparent( true );
            EnableChildTransparentMode( true );
            SetParentClipMode( PARENTCLIPMODE_NOCLIP );
            SetPaintTransparent( true );
            SetBackground();
        }
        else
        {
            // dialogs will implement tabpages as peers of their borders, not
            // as contained
            for ( sal_uInt16 i = 0; i < GetParent()->GetChildCount(); i++ )
            {
                Window* pChild = GetParent()->GetChild( i );
                if ( pChild->GetType() == WINDOW_CONTAINER )
                {
                    SetMouseTransparent( true );
                    EnableChildTransparentMode( true );
                    SetParentClipMode( PARENTCLIPMODE_NOCLIP );
                    SetPaintTransparent( true );
                    SetBackground();
                    break;
                }
            }
        }
    }
#endif	// USE_JAVA && MACOSX
}

void TabPage::ImplInitSettings()
{
    vcl::Window* pParent = GetParent();
    if ( pParent->IsChildTransparentModeEnabled() && !IsControlBackground() )
    {
        EnableChildTransparentMode( true );
        SetParentClipMode( PARENTCLIPMODE_NOCLIP );
        SetPaintTransparent( true );
        SetBackground();
    }
    else
    {
        EnableChildTransparentMode( false );
        SetParentClipMode( 0 );
        SetPaintTransparent( false );

        if ( IsControlBackground() )
            SetBackground( GetControlBackground() );
        else
            SetBackground( pParent->GetBackground() );
    }
}

TabPage::TabPage( vcl::Window* pParent, WinBits nStyle ) :
    Window( WINDOW_TABPAGE )
{
    ImplInit( pParent, nStyle );
}

TabPage::TabPage(vcl::Window *pParent, const OString& rID, const OUString& rUIXMLDescription)
    : Window(WINDOW_TABPAGE)
{
    ImplInit(pParent, 0);
    m_pUIBuilder = new VclBuilder(this, getUIRootDir(), rUIXMLDescription, rID);
    set_hexpand(true);
    set_vexpand(true);
    set_expand(true);
}

void TabPage::StateChanged( StateChangedType nType )
{
    Window::StateChanged( nType );

    if ( nType == StateChangedType::INITSHOW )
    {
        if ( GetSettings().GetStyleSettings().GetAutoMnemonic() )
            ImplWindowAutoMnemonic( this );
        // FIXME: no layouting, workaround some clipping issues
        ImplAdjustNWFSizes();
    }
    else if ( nType == StateChangedType::CONTROLBACKGROUND )
    {
        ImplInitSettings();
        Invalidate();
    }
}

void TabPage::DataChanged( const DataChangedEvent& rDCEvt )
{
    Window::DataChanged( rDCEvt );

    if ( (rDCEvt.GetType() == DATACHANGED_SETTINGS) &&
         (rDCEvt.GetFlags() & SETTINGS_STYLE) )
    {
        ImplInitSettings();
        Invalidate();
    }
}

void TabPage::Paint( const Rectangle& )
{
    // draw native tabpage only inside tabcontrols, standalone tabpages look ugly (due to bad dialog design)
    if( IsNativeControlSupported(CTRL_TAB_BODY, PART_ENTIRE_CONTROL) && GetParent() && (GetParent()->GetType() == WINDOW_TABCONTROL) )
    {
        const ImplControlValue aControlValue;

        ControlState nState = CTRL_STATE_ENABLED;
        int part = PART_ENTIRE_CONTROL;
        if ( !IsEnabled() )
            nState &= ~CTRL_STATE_ENABLED;
        if ( HasFocus() )
            nState |= CTRL_STATE_FOCUSED;
        Point aPoint;
        // pass the whole window region to NWF as the tab body might be a gradient or bitmap
        // that has to be scaled properly, clipping makes sure that we do not paint too much
        Rectangle aCtrlRegion( aPoint, GetOutputSizePixel() );
        DrawNativeControl( CTRL_TAB_BODY, part, aCtrlRegion, nState,
                aControlValue, OUString() );
    }
}

void TabPage::Draw( OutputDevice* pDev, const Point& rPos, const Size& rSize, sal_uLong )
{
    Point aPos = pDev->LogicToPixel( rPos );
    Size aSize = pDev->LogicToPixel( rSize );

    Wallpaper aWallpaper = GetBackground();
    if ( !aWallpaper.IsBitmap() )
        ImplInitSettings();

    pDev->Push();
    pDev->SetMapMode();
    pDev->SetLineColor();

    if ( aWallpaper.IsBitmap() )
        pDev->DrawBitmapEx( aPos, aSize, aWallpaper.GetBitmap() );
    else
    {
        if( aWallpaper.GetColor() == COL_AUTO )
            pDev->SetFillColor( GetSettings().GetStyleSettings().GetDialogColor() );
        else
            pDev->SetFillColor( aWallpaper.GetColor() );
        pDev->DrawRect( Rectangle( aPos, aSize ) );
    }

    pDev->Pop();
}

void TabPage::ActivatePage()
{
}

void TabPage::DeactivatePage()
{
}

OString TabPage::GetConfigId() const
{
    OString sId(GetHelpId());
    if (sId.isEmpty() && isLayoutEnabled(this))
        sId = GetWindow(WINDOW_FIRSTCHILD)->GetHelpId();
    return sId;
}

Size TabPage::GetOptimalSize() const
{
    if (isLayoutEnabled(this))
        return VclContainer::getLayoutRequisition(*GetWindow(WINDOW_FIRSTCHILD));
    return getLegacyBestSizeForChildren(*this);
}

void TabPage::SetPosSizePixel(const Point& rAllocPos, const Size& rAllocation)
{
    Window::SetPosSizePixel(rAllocPos, rAllocation);
    if (isLayoutEnabled(this) && rAllocation.Width() && rAllocation.Height())
        VclContainer::setLayoutAllocation(*GetWindow(WINDOW_FIRSTCHILD), Point(0, 0), rAllocation);
}

void TabPage::SetSizePixel(const Size& rAllocation)
{
    Window::SetSizePixel(rAllocation);
    if (isLayoutEnabled(this) && rAllocation.Width() && rAllocation.Height())
        VclContainer::setLayoutAllocation(*GetWindow(WINDOW_FIRSTCHILD), Point(0, 0), rAllocation);
}

void TabPage::SetPosPixel(const Point& rAllocPos)
{
    Window::SetPosPixel(rAllocPos);
    Size aAllocation(GetOutputSizePixel());
    if (isLayoutEnabled(this) && aAllocation.Width() && aAllocation.Height())
    {
        VclContainer::setLayoutAllocation(*GetWindow(WINDOW_FIRSTCHILD), Point(0, 0), aAllocation);
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
