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

#include "tools/rc.h"

#include "vcl/event.hxx"
#include "vcl/decoview.hxx"
#include "vcl/spin.h"
#include "vcl/spinfld.hxx"
#include "vcl/settings.hxx"

#include "controldata.hxx"
#include "svdata.hxx"

namespace {

void ImplGetSpinbuttonValue( vcl::Window *pWin, const Rectangle& rUpperRect,
                            const Rectangle& rLowerRect,
                            bool bUpperIn, bool bLowerIn,
                            bool bUpperEnabled, bool bLowerEnabled, bool bHorz,
                            SpinbuttonValue& rValue )
{
    // convert spinbutton data to a SpinbuttonValue structure for native painting

    rValue.maUpperRect = rUpperRect;
    rValue.maLowerRect = rLowerRect;

    Point aPointerPos = pWin->GetPointerPosPixel();

    ControlState nState = CTRL_STATE_ENABLED;
    if ( bUpperIn )
        nState |= CTRL_STATE_PRESSED;
    if ( !pWin->IsEnabled() || !bUpperEnabled )
        nState &= ~CTRL_STATE_ENABLED;
    if ( pWin->HasFocus() )
        nState |= CTRL_STATE_FOCUSED;
    if( pWin->IsMouseOver() && rUpperRect.IsInside( aPointerPos ) )
        nState |= CTRL_STATE_ROLLOVER;
    rValue.mnUpperState = nState;

    nState = CTRL_STATE_ENABLED;
    if ( bLowerIn )
        nState |= CTRL_STATE_PRESSED;
    if ( !pWin->IsEnabled() || !bLowerEnabled )
        nState &= ~CTRL_STATE_ENABLED;
    if ( pWin->HasFocus() )
        nState |= CTRL_STATE_FOCUSED;
    // for overlapping spins: highlight only one
    if( pWin->IsMouseOver() && rLowerRect.IsInside( aPointerPos ) &&
                              !rUpperRect.IsInside( aPointerPos ) )
        nState |= CTRL_STATE_ROLLOVER;
    rValue.mnLowerState = nState;

    rValue.mnUpperPart = bHorz ? PART_BUTTON_LEFT : PART_BUTTON_UP;
    rValue.mnLowerPart = bHorz ? PART_BUTTON_RIGHT : PART_BUTTON_DOWN;
}

#ifdef USE_JAVA
bool ImplDrawNativeSpinfield( vcl::Window *pWin, const SpinbuttonValue& rSpinbuttonValue, bool bInDropDown = false )
#else	// USE_JAVA
bool ImplDrawNativeSpinfield( vcl::Window *pWin, const SpinbuttonValue& rSpinbuttonValue )
#endif	// USE_JAVA
{
    bool bNativeOK = false;

    if( pWin->IsNativeControlSupported(CTRL_SPINBOX, PART_ENTIRE_CONTROL) &&
        // there is just no useful native support for spinfields with dropdown
        !(pWin->GetStyle() & WB_DROPDOWN) )
    {
        if( pWin->IsNativeControlSupported(CTRL_SPINBOX, rSpinbuttonValue.mnUpperPart) &&
            pWin->IsNativeControlSupported(CTRL_SPINBOX, rSpinbuttonValue.mnLowerPart) )
        {
            // only paint the embedded spin buttons, all buttons are painted at once
#ifdef USE_JAVA
            bNativeOK = pWin->DrawNativeControl( CTRL_SPINBOX, PART_ALL_BUTTONS, Rectangle(), ( ( pWin->IsEnabled() ) ? CTRL_STATE_ENABLED : 0 ),
#else	// USE_JAVA
            bNativeOK = pWin->DrawNativeControl( CTRL_SPINBOX, PART_ALL_BUTTONS, Rectangle(), CTRL_STATE_ENABLED,
#endif	// USE_JAVA
                        rSpinbuttonValue, OUString() );
        }
        else
        {
            // paint the spinbox as a whole, use borderwindow to have proper clipping
            vcl::Window *pBorder = pWin->GetWindow( WINDOW_BORDER );

            // to not overwrite everything, set the button region as clipregion to the border window
            Rectangle aClipRect( rSpinbuttonValue.maLowerRect );
            aClipRect.Union( rSpinbuttonValue.maUpperRect );

            // convert from screen space to borderwin space
            aClipRect.SetPos( pBorder->ScreenToOutputPixel(pWin->OutputToScreenPixel(aClipRect.TopLeft())) );

            vcl::Region oldRgn( pBorder->GetClipRegion() );
            pBorder->SetClipRegion( vcl::Region( aClipRect ) );

            Point aPt;
            Size aSize( pBorder->GetOutputSizePixel() );    // the size of the border window, i.e., the whole control
            Rectangle aBound, aContent;
            Rectangle aNatRgn( aPt, aSize );
            if( ! ImplGetSVData()->maNWFData.mbCanDrawWidgetAnySize &&
                pBorder->GetNativeControlRegion( CTRL_SPINBOX, PART_ENTIRE_CONTROL,
                                                 aNatRgn, 0, rSpinbuttonValue, OUString(), aBound, aContent) )
            {
                aSize = aContent.GetSize();
            }

            Rectangle aRgn( aPt, aSize );
#ifdef USE_JAVA
            bNativeOK = pBorder->DrawNativeControl( CTRL_SPINBOX, PART_ENTIRE_CONTROL, aRgn, ( ( pWin->IsEnabled() ) ? CTRL_STATE_ENABLED : 0 ),
#else	// USE_JAVA
            bNativeOK = pBorder->DrawNativeControl( CTRL_SPINBOX, PART_ENTIRE_CONTROL, aRgn, CTRL_STATE_ENABLED,
#endif	// USE_JAVA
                        rSpinbuttonValue, OUString() );

            pBorder->SetClipRegion(vcl::Region(oldRgn));
        }
    }
#ifdef USE_JAVA
    // render spinfields with dropdowns as combo boxes
    else if( (pWin->GetStyle() & WB_DROPDOWN) && pWin->IsNativeControlSupported(CTRL_COMBOBOX, PART_ENTIRE_CONTROL) )
    {
        vcl::Window *pBorder = pWin->GetWindow( WINDOW_BORDER );
        Point aPt;
        Size aSize( pBorder->GetOutputSizePixel() );
        Rectangle aRgn( aPt, aSize );
        ImplControlValue aControlValue;
        ControlState nState = 0;
        if( pWin->IsEnabled() )
            nState |= CTRL_STATE_ENABLED;
        if( bInDropDown )
            nState |= CTRL_STATE_PRESSED;
        bNativeOK = pBorder->DrawNativeControl( CTRL_COMBOBOX, PART_ENTIRE_CONTROL, aRgn, nState, aControlValue, OUString() );
    }
#endif	// USE_JAVA
    return bNativeOK;
}

bool ImplDrawNativeSpinbuttons( vcl::Window *pWin, const SpinbuttonValue& rSpinbuttonValue )
{
    bool bNativeOK = false;

    if( pWin->IsNativeControlSupported(CTRL_SPINBUTTONS, PART_ENTIRE_CONTROL) )
    {
        // only paint the standalone spin buttons, all buttons are painted at once
#ifdef USE_JAVA
        Point aPt;
        Size aSize( pWin->GetOutputSizePixel() );
        Rectangle aRgn( aPt, aSize );
        bNativeOK = pWin->DrawNativeControl( CTRL_SPINBUTTONS, PART_ALL_BUTTONS, aRgn, CTRL_STATE_ENABLED,
#else	// USE_JAVA
        bNativeOK = pWin->DrawNativeControl( CTRL_SPINBUTTONS, PART_ALL_BUTTONS, Rectangle(), CTRL_STATE_ENABLED,
#endif	// USE_JAVA
                    rSpinbuttonValue, OUString() );
    }
    return bNativeOK;
}

}

#ifdef USE_JAVA

static void ImplDrawSpinButton( OutputDevice* pOutDev,
                         const Rectangle& rUpperRect,
                         const Rectangle& rLowerRect,
                         bool bUpperIn, bool bLowerIn,
                         bool bUpperEnabled, bool bLowerEnabled, bool bHorz, bool bMirrorHorz, bool bInDropDown = false );

#endif	// USE_JAVA

void ImplDrawSpinButton( OutputDevice* pOutDev,
                         const Rectangle& rUpperRect,
                         const Rectangle& rLowerRect,
                         bool bUpperIn, bool bLowerIn,
                         bool bUpperEnabled, bool bLowerEnabled, bool bHorz, bool bMirrorHorz )
#ifdef USE_JAVA
{
    ImplDrawSpinButton( pOutDev, rUpperRect, rLowerRect, bUpperIn, bLowerIn, bUpperEnabled, bLowerEnabled, bHorz, bMirrorHorz, false );
}

static void ImplDrawSpinButton( OutputDevice* pOutDev,
                         const Rectangle& rUpperRect,
                         const Rectangle& rLowerRect,
                         bool bUpperIn, bool bLowerIn,
                         bool bUpperEnabled, bool bLowerEnabled, bool bHorz, bool bMirrorHorz, bool bInDropDown )
#endif	// USE_JAVA
{
    DecorationView aDecoView( pOutDev );

    sal_uInt16 nStyle = BUTTON_DRAW_NOLEFTLIGHTBORDER;
    sal_uInt16 nSymStyle = 0;

    SymbolType eType1, eType2;

    const StyleSettings& rStyleSettings = pOutDev->GetSettings().GetStyleSettings();
    if ( rStyleSettings.GetOptions() & STYLE_OPTION_SPINARROW )
    {
        // arrows are only use in OS/2 look
        if ( bHorz )
        {
            eType1 = bMirrorHorz ? SymbolType::ARROW_RIGHT : SymbolType::ARROW_LEFT;
            eType2 = bMirrorHorz ? SymbolType::ARROW_LEFT : SymbolType::ARROW_RIGHT;
        }
        else
        {
            eType1 = SymbolType::ARROW_UP;
            eType2 = SymbolType::ARROW_DOWN;
        }
    }
    else
    {
        if ( bHorz )
        {
            eType1 = bMirrorHorz ? SymbolType::SPIN_RIGHT : SymbolType::SPIN_LEFT;
            eType2 = bMirrorHorz ? SymbolType::SPIN_LEFT : SymbolType::SPIN_RIGHT;
        }
        else
        {
            eType1 = SymbolType::SPIN_UP;
            eType2 = SymbolType::SPIN_DOWN;
        }
    }

    // draw upper/left Button
    sal_uInt16 nTempStyle = nStyle;
    if ( bUpperIn )
        nTempStyle |= BUTTON_DRAW_PRESSED;

    bool bNativeOK = false;
    Rectangle aUpRect;

    if( pOutDev->GetOutDevType() == OUTDEV_WINDOW )
    {
        vcl::Window *pWin = static_cast<vcl::Window*>(pOutDev);

        // are we drawing standalone spin buttons or members of a spinfield ?
        ControlType aControl = CTRL_SPINBUTTONS;
        switch( pWin->GetType() )
        {
            case WINDOW_EDIT:
            case WINDOW_MULTILINEEDIT:
            case WINDOW_PATTERNFIELD:
            case WINDOW_METRICFIELD:
            case WINDOW_CURRENCYFIELD:
            case WINDOW_DATEFIELD:
            case WINDOW_TIMEFIELD:
            case WINDOW_LONGCURRENCYFIELD:
            case WINDOW_NUMERICFIELD:
            case WINDOW_SPINFIELD:
                aControl = CTRL_SPINBOX;
                break;
            default:
                aControl = CTRL_SPINBUTTONS;
                break;
        }

        SpinbuttonValue aValue;
        ImplGetSpinbuttonValue( pWin, rUpperRect, rLowerRect,
                                bUpperIn, bLowerIn, bUpperEnabled, bLowerEnabled,
                                bHorz, aValue );

        if( aControl == CTRL_SPINBOX )
#ifdef USE_JAVA
            bNativeOK = ImplDrawNativeSpinfield( pWin, aValue, bInDropDown );
#else	// USE_JAVA
            bNativeOK = ImplDrawNativeSpinfield( pWin, aValue );
#endif	// USE_JAVA
        else if( aControl == CTRL_SPINBUTTONS )
            bNativeOK = ImplDrawNativeSpinbuttons( pWin, aValue );
    }

    if( !bNativeOK )
        aUpRect = aDecoView.DrawButton( rUpperRect, nTempStyle );

    // draw lower/right Button
    if ( bLowerIn )
        nStyle |= BUTTON_DRAW_PRESSED;
    Rectangle aLowRect;
    if( !bNativeOK )
        aLowRect = aDecoView.DrawButton( rLowerRect, nStyle );

     // make use of additional default edge
    aUpRect.Left()--;
    aUpRect.Top()--;
    aUpRect.Right()++;
    aUpRect.Bottom()++;
    aLowRect.Left()--;
    aLowRect.Top()--;
    aLowRect.Right()++;
    aLowRect.Bottom()++;

    // draw into the edge, so that something is visible if the rectangle is too small
    if ( aUpRect.GetHeight() < 4 )
    {
        aUpRect.Right()++;
        aUpRect.Bottom()++;
        aLowRect.Right()++;
        aLowRect.Bottom()++;
    }

    // calculate Symbol size
    long nTempSize1 = aUpRect.GetWidth();
    long nTempSize2 = aLowRect.GetWidth();
    if ( std::abs( nTempSize1-nTempSize2 ) == 1 )
    {
        if ( nTempSize1 > nTempSize2 )
            aUpRect.Left()++;
        else
            aLowRect.Left()++;
    }
    nTempSize1 = aUpRect.GetHeight();
    nTempSize2 = aLowRect.GetHeight();
    if ( std::abs( nTempSize1-nTempSize2 ) == 1 )
    {
        if ( nTempSize1 > nTempSize2 )
            aUpRect.Top()++;
        else
            aLowRect.Top()++;
    }

    nTempStyle = nSymStyle;
    if ( !bUpperEnabled )
        nTempStyle |= SYMBOL_DRAW_DISABLE;
    if( !bNativeOK )
        aDecoView.DrawSymbol( aUpRect, eType1, rStyleSettings.GetButtonTextColor(), nTempStyle );

    if ( !bLowerEnabled )
        nSymStyle |= SYMBOL_DRAW_DISABLE;
    if( !bNativeOK )
        aDecoView.DrawSymbol( aLowRect, eType2, rStyleSettings.GetButtonTextColor(), nSymStyle );
}

void SpinField::ImplInitSpinFieldData()
{
    mpEdit          = NULL;
    mbSpin          = false;
    mbRepeat        = false;
    mbUpperIn       = false;
    mbLowerIn       = false;
    mbInitialUp     = false;
    mbInitialDown   = false;
    mbNoSelect      = false;
    mbInDropDown    = false;
}

void SpinField::ImplInit( vcl::Window* pParent, WinBits nWinStyle )
{
    Edit::ImplInit( pParent, nWinStyle );

    if ( nWinStyle & (WB_SPIN|WB_DROPDOWN) )
    {
        mbSpin = true;

        // Some themes want external spin buttons, therefore the main
        // spinfield should not overdraw the border between its encapsulated
        // edit field and the spin buttons
#ifdef USE_JAVA
        if ( ImplUseNativeBorder( nWinStyle ) ) 
#else	// USE_JAVA
        if ( (nWinStyle & WB_SPIN) && ImplUseNativeBorder( nWinStyle ) )
#endif	// USE_JAVA
        {
            SetBackground();
            mpEdit = new Edit( this, WB_NOBORDER );
            mpEdit->SetBackground();
        }
        else
            mpEdit = new Edit( this, WB_NOBORDER );

        mpEdit->EnableRTL( false );
        mpEdit->SetPosPixel( Point() );
        mpEdit->Show();
        SetSubEdit( mpEdit );

        maRepeatTimer.SetTimeoutHdl( LINK( this, SpinField, ImplTimeout ) );
        maRepeatTimer.SetTimeout( GetSettings().GetMouseSettings().GetButtonStartRepeat() );
        if ( nWinStyle & WB_REPEAT )
            mbRepeat = true;

        SetCompoundControl( true );
    }
}

SpinField::SpinField( WindowType nTyp ) :
    Edit( nTyp )
{
    ImplInitSpinFieldData();
}

SpinField::SpinField( vcl::Window* pParent, WinBits nWinStyle ) :
    Edit( WINDOW_SPINFIELD )
{
    ImplInitSpinFieldData();
    ImplInit( pParent, nWinStyle );
}

SpinField::SpinField( vcl::Window* pParent, const ResId& rResId ) :
    Edit( WINDOW_SPINFIELD )
{
    ImplInitSpinFieldData();
    rResId.SetRT( RSC_SPINFIELD );
    WinBits nStyle = ImplInitRes( rResId );
    ImplInit( pParent, nStyle );
    ImplLoadRes( rResId );

    if ( !(nStyle & WB_HIDE) )
        Show();
}

SpinField::~SpinField()
{
    delete mpEdit;
}

void SpinField::Up()
{
    ImplCallEventListenersAndHandler( VCLEVENT_SPINFIELD_UP, maUpHdlLink, this );
#ifdef USE_JAVA
    if ( IsNativeControlSupported( CTRL_SPINBOX, PART_ENTIRE_CONTROL ) )
        ImplInvalidateOutermostBorder( this );
#endif	// USE_JAVA
}

void SpinField::Down()
{
    ImplCallEventListenersAndHandler( VCLEVENT_SPINFIELD_DOWN, maDownHdlLink, this );
#ifdef USE_JAVA
    if ( IsNativeControlSupported( CTRL_SPINBOX, PART_ENTIRE_CONTROL ) )
        ImplInvalidateOutermostBorder( this );
#endif	// USE_JAVA
}

void SpinField::First()
{
    ImplCallEventListenersAndHandler( VCLEVENT_SPINFIELD_FIRST, maFirstHdlLink, this );
#ifdef USE_JAVA
    if ( IsNativeControlSupported( CTRL_SPINBOX, PART_ENTIRE_CONTROL ) )
        ImplInvalidateOutermostBorder( this );
#endif	// USE_JAVA
}

void SpinField::Last()
{
    ImplCallEventListenersAndHandler( VCLEVENT_SPINFIELD_LAST, maLastHdlLink, this );
#ifdef USE_JAVA
    if ( IsNativeControlSupported( CTRL_SPINBOX, PART_ENTIRE_CONTROL ) )
        ImplInvalidateOutermostBorder( this );
#endif	// USE_JAVA
}

void SpinField::MouseButtonDown( const MouseEvent& rMEvt )
{
    if ( !HasFocus() && ( !mpEdit || !mpEdit->HasFocus() ) )
    {
        mbNoSelect = true;
        GrabFocus();
    }

    if ( !IsReadOnly() )
    {
        if ( maUpperRect.IsInside( rMEvt.GetPosPixel() ) )
        {
            mbUpperIn   = true;
            mbInitialUp = true;
#ifdef USE_JAVA
            if ( IsNativeControlSupported( CTRL_SPINBOX, PART_ENTIRE_CONTROL ) )
                ImplInvalidateOutermostBorder( this );
            else
#endif	// USE_JAVA
            Invalidate( maUpperRect );
        }
        else if ( maLowerRect.IsInside( rMEvt.GetPosPixel() ) )
        {
            mbLowerIn    = true;
            mbInitialDown = true;
#ifdef USE_JAVA
            if ( IsNativeControlSupported( CTRL_SPINBOX, PART_ENTIRE_CONTROL ) )
                ImplInvalidateOutermostBorder( this );
            else
#endif	// USE_JAVA
            Invalidate( maLowerRect );
        }
        else if ( maDropDownRect.IsInside( rMEvt.GetPosPixel() ) )
        {
            // put DropDownButton to the right
            mbInDropDown = ShowDropDown( mbInDropDown ? false : true );
#ifdef USE_JAVA
            if ( IsNativeControlSupported( CTRL_COMBOBOX, PART_ENTIRE_CONTROL ) )
            {
                GetParent()->Invalidate( Rectangle( GetPosPixel(), GetSizePixel() ) );
                if ( GetParent()->IsInPaint() )
                    GetParent()->Update();
            }
            else
#endif	// USE_JAVA
            Paint( Rectangle( Point(), GetOutputSizePixel() ) );
        }

        if ( mbUpperIn || mbLowerIn )
        {
            Update();
            CaptureMouse();
            if ( mbRepeat )
                maRepeatTimer.Start();
            return;
        }
    }

    Edit::MouseButtonDown( rMEvt );
}

void SpinField::MouseButtonUp( const MouseEvent& rMEvt )
{
    ReleaseMouse();
    mbInitialUp = mbInitialDown = false;
    maRepeatTimer.Stop();
    maRepeatTimer.SetTimeout( GetSettings().GetMouseSettings().GetButtonStartRepeat() );

    if ( mbUpperIn )
    {
        mbUpperIn = false;
#ifdef USE_JAVA
        if ( IsNativeControlSupported( CTRL_SPINBOX, PART_ENTIRE_CONTROL ) )
        {
            ImplInvalidateOutermostBorder( this );
            if ( GetParent()->IsInPaint() )
                GetParent()->Update();
        }
        else
        {
#endif	// USE_JAVA
        Invalidate( maUpperRect );
        Update();
#ifdef USE_JAVA
        }
#endif	// USE_JAVA
        Up();
    }
    else if ( mbLowerIn )
    {
        mbLowerIn = false;
#ifdef USE_JAVA
        if ( IsNativeControlSupported( CTRL_SPINBOX, PART_ENTIRE_CONTROL ) )
        {
            ImplInvalidateOutermostBorder( this );
            if ( GetParent()->IsInPaint() )
                GetParent()->Update();
        }
        else
        {
#endif	// USE_JAVA
        Invalidate( maLowerRect );
        Update();
#ifdef USE_JAVA
        }
#endif	// USE_JAVA
        Down();
    }

    Edit::MouseButtonUp( rMEvt );
}

void SpinField::MouseMove( const MouseEvent& rMEvt )
{
    if ( rMEvt.IsLeft() )
    {
        if ( mbInitialUp )
        {
            bool bNewUpperIn = maUpperRect.IsInside( rMEvt.GetPosPixel() );
            if ( bNewUpperIn != mbUpperIn )
            {
                if ( bNewUpperIn )
                {
                    if ( mbRepeat )
                        maRepeatTimer.Start();
                }
                else
                    maRepeatTimer.Stop();

                mbUpperIn = bNewUpperIn;
#ifdef USE_JAVA
                if ( IsNativeControlSupported( CTRL_SPINBOX, PART_ENTIRE_CONTROL ) )
                {
                    ImplInvalidateOutermostBorder( this );
                    if ( GetParent()->IsInPaint() )
                        GetParent()->Update();
                }
                else
                {
#endif	// USE_JAVA
                Invalidate( maUpperRect );
                Update();
#ifdef USE_JAVA
                }
#endif	// USE_JAVA
            }
        }
        else if ( mbInitialDown )
        {
            bool bNewLowerIn = maLowerRect.IsInside( rMEvt.GetPosPixel() );
            if ( bNewLowerIn != mbLowerIn )
            {
                if ( bNewLowerIn )
                {
                    if ( mbRepeat )
                        maRepeatTimer.Start();
                }
                else
                    maRepeatTimer.Stop();

                mbLowerIn = bNewLowerIn;
#ifdef USE_JAVA
                if ( IsNativeControlSupported( CTRL_SPINBOX, PART_ENTIRE_CONTROL ) )
                {
                    ImplInvalidateOutermostBorder( this );
                    if ( GetParent()->IsInPaint() )
                        GetParent()->Update();
                }
                else
                {
#endif	// USE_JAVA
                Invalidate( maLowerRect );
                Update();
#ifdef USE_JAVA
                }
#endif	// USE_JAVA
            }
        }
    }

    Edit::MouseMove( rMEvt );
}

bool SpinField::Notify( NotifyEvent& rNEvt )
{
    bool nDone = false;
    if( rNEvt.GetType() == EVENT_KEYINPUT )
    {
        const KeyEvent& rKEvt = *rNEvt.GetKeyEvent();
        if ( !IsReadOnly() )
        {
            sal_uInt16 nMod = rKEvt.GetKeyCode().GetModifier();
            switch ( rKEvt.GetKeyCode().GetCode() )
            {
                case KEY_UP:
                {
                    if ( !nMod )
                    {
                        Up();
                        nDone = true;
                    }
                }
                break;
                case KEY_DOWN:
                {
                    if ( !nMod )
                    {
                        Down();
                        nDone = true;
                    }
                    else if ( ( nMod == KEY_MOD2 ) && !mbInDropDown && ( GetStyle() & WB_DROPDOWN ) )
                    {
                        mbInDropDown = ShowDropDown( true );
#ifdef USE_JAVA
                        if ( IsNativeControlSupported( CTRL_SPINBOX, PART_ENTIRE_CONTROL ) )
                        {
                            ImplInvalidateOutermostBorder( this );
                            if ( GetParent()->IsInPaint() )
                                GetParent()->Update();
                        }
#else	// USE_JAVA
                        Paint( Rectangle( Point(), GetOutputSizePixel() ) );
#endif	// USE_JAVA
                        nDone = true;
                    }
                }
                break;
                case KEY_PAGEUP:
                {
                    if ( !nMod )
                    {
                        Last();
                        nDone = true;
                    }
                }
                break;
                case KEY_PAGEDOWN:
                {
                    if ( !nMod )
                    {
                        First();
                        nDone = true;
                    }
                }
                break;
            }
        }
    }

    if ( rNEvt.GetType() == EVENT_COMMAND )
    {
        if ( ( rNEvt.GetCommandEvent()->GetCommand() == COMMAND_WHEEL ) && !IsReadOnly() )
        {
            sal_uInt16 nWheelBehavior( GetSettings().GetMouseSettings().GetWheelBehavior() );
            if  (   ( nWheelBehavior == MOUSE_WHEEL_ALWAYS )
                ||  (   ( nWheelBehavior == MOUSE_WHEEL_FOCUS_ONLY )
                    &&  HasChildPathFocus()
                    )
                )
            {
                const CommandWheelData* pData = rNEvt.GetCommandEvent()->GetWheelData();
                if ( pData->GetMode() == CommandWheelMode::SCROLL )
                {
                    if ( pData->GetDelta() < 0L )
                        Down();
                    else
                        Up();
                    nDone = true;
                }
            }
            else
                nDone = false;  // don't eat this event, let the default handling happen (i.e. scroll the context)
        }
    }

    return nDone || Edit::Notify( rNEvt );
}

void SpinField::Command( const CommandEvent& rCEvt )
{
    Edit::Command( rCEvt );
}

void SpinField::FillLayoutData() const
{
    if( mbSpin )
    {
        mpControlData->mpLayoutData = new vcl::ControlLayoutData();
        AppendLayoutData( *GetSubEdit() );
        GetSubEdit()->SetLayoutDataParent( this );
    }
    else
        Edit::FillLayoutData();
}

void SpinField::Paint( const Rectangle& rRect )
{
    if ( mbSpin )
    {
        bool    bEnable = IsEnabled();
        ImplDrawSpinButton( this, maUpperRect, maLowerRect,
                            mbUpperIn, mbLowerIn, bEnable, bEnable );
    }

    if ( GetStyle() & WB_DROPDOWN )
    {
#ifdef USE_JAVA
		if( IsNativeControlSupported( CTRL_COMBOBOX, PART_ENTIRE_CONTROL ) )
		{
			bool bEnable = IsEnabled();
			ImplDrawSpinButton( this, maUpperRect, maLowerRect, mbUpperIn, mbLowerIn, bEnable, bEnable, false, false, mbInDropDown );
		}
		else
		{
#endif	// USE_JAVA
        DecorationView aView( this );

        sal_uInt16 nStyle = BUTTON_DRAW_NOLIGHTBORDER;
        if ( mbInDropDown )
            nStyle |= BUTTON_DRAW_PRESSED;
        Rectangle aInnerRect = aView.DrawButton( maDropDownRect, nStyle );

        SymbolType eSymbol = SymbolType::SPIN_DOWN;
        if ( GetSettings().GetStyleSettings().GetOptions() & STYLE_OPTION_SPINUPDOWN )
            eSymbol = SymbolType::SPIN_UPDOWN;

        nStyle = IsEnabled() ? 0 : SYMBOL_DRAW_DISABLE;
        aView.DrawSymbol( aInnerRect, eSymbol, GetSettings().GetStyleSettings().GetButtonTextColor(), nStyle );
#ifdef USE_JAVA
        }
#endif	// USE_JAVA
    }

    Edit::Paint( rRect );
}

void SpinField::ImplCalcButtonAreas( OutputDevice* pDev, const Size& rOutSz, Rectangle& rDDArea, Rectangle& rSpinUpArea, Rectangle& rSpinDownArea )
{
    const StyleSettings& rStyleSettings = pDev->GetSettings().GetStyleSettings();

    Size aSize = rOutSz;
    Size aDropDownSize;

    if ( GetStyle() & WB_DROPDOWN )
    {
        long nW = rStyleSettings.GetScrollBarSize();
        nW = GetDrawPixel( pDev, nW );
        aDropDownSize = Size( CalcZoom( nW ), aSize.Height() );
        aSize.Width() -= aDropDownSize.Width();
        rDDArea = Rectangle( Point( aSize.Width(), 0 ), aDropDownSize );
        rDDArea.Top()--;
    }
    else
        rDDArea.SetEmpty();

    // calcuate sizes according to the height
    if ( GetStyle() & WB_SPIN )
    {
        long nBottom1 = aSize.Height()/2;
        long nBottom2 = aSize.Height()-1;
        long nTop2 = nBottom1;
        long nTop1 = 0;
        if ( !(aSize.Height() & 0x01) )
            nBottom1--;

        bool bNativeRegionOK = false;
        Rectangle aContentUp, aContentDown;

        if ( (pDev->GetOutDevType() == OUTDEV_WINDOW) &&
            // there is just no useful native support for spinfields with dropdown
            ! (GetStyle() & WB_DROPDOWN) &&
            IsNativeControlSupported(CTRL_SPINBOX, PART_ENTIRE_CONTROL) )
        {
            vcl::Window *pWin = static_cast<vcl::Window*>(pDev);
            vcl::Window *pBorder = pWin->GetWindow( WINDOW_BORDER );

            // get the system's spin button size
            ImplControlValue aControlValue;
            Rectangle aBound;
            Point aPoint;

            // use the full extent of the control
            Rectangle aArea( aPoint, pBorder->GetOutputSizePixel() );

            bNativeRegionOK =
                pWin->GetNativeControlRegion(CTRL_SPINBOX, PART_BUTTON_UP,
                    aArea, 0, aControlValue, OUString(), aBound, aContentUp) &&
                pWin->GetNativeControlRegion(CTRL_SPINBOX, PART_BUTTON_DOWN,
                    aArea, 0, aControlValue, OUString(), aBound, aContentDown);

            if( bNativeRegionOK )
            {
                // convert back from border space to local coordinates
                aPoint = pBorder->ScreenToOutputPixel( pWin->OutputToScreenPixel( aPoint ) );
                aContentUp.Move(-aPoint.X(), -aPoint.Y());
                aContentDown.Move(-aPoint.X(), -aPoint.Y());
            }
        }

        if( bNativeRegionOK )
        {
            rSpinUpArea = aContentUp;
            rSpinDownArea = aContentDown;
        }
        else
        {
            aSize.Width() -= CalcZoom( GetDrawPixel( pDev, rStyleSettings.GetSpinSize() ) );

            rSpinUpArea = Rectangle( aSize.Width(), nTop1, rOutSz.Width()-aDropDownSize.Width()-1, nBottom1 );
            rSpinDownArea = Rectangle( rSpinUpArea.Left(), nTop2, rSpinUpArea.Right(), nBottom2 );
        }
    }
    else
    {
        rSpinUpArea.SetEmpty();
        rSpinDownArea.SetEmpty();
    }
}

void SpinField::Resize()
{
    if ( mbSpin )
    {
        Control::Resize();
        Size aSize = GetOutputSizePixel();
        bool bSubEditPositioned = false;

        if ( GetStyle() & (WB_SPIN|WB_DROPDOWN) )
        {
            ImplCalcButtonAreas( this, aSize, maDropDownRect, maUpperRect, maLowerRect );

            ImplControlValue aControlValue;
            Point aPoint;
            Rectangle aContent, aBound;

            // use the full extent of the control
            vcl::Window *pBorder = GetWindow( WINDOW_BORDER );
            Rectangle aArea( aPoint, pBorder->GetOutputSizePixel() );

            // adjust position and size of the edit field
            if ( GetNativeControlRegion(CTRL_SPINBOX, PART_SUB_EDIT,
                        aArea, 0, aControlValue, OUString(), aBound, aContent) )
            {
                // convert back from border space to local coordinates
                aPoint = pBorder->ScreenToOutputPixel( OutputToScreenPixel( aPoint ) );
                aContent.Move(-aPoint.X(), -aPoint.Y());

                // use the themes drop down size
                mpEdit->SetPosPixel( aContent.TopLeft() );
                bSubEditPositioned = true;
                aSize = aContent.GetSize();
            }
            else
            {
                if ( maUpperRect.IsEmpty() )
                {
                    DBG_ASSERT( !maDropDownRect.IsEmpty(), "SpinField::Resize: SPIN && DROPDOWN, but all empty rects?" );
                    aSize.Width() = maDropDownRect.Left();
                }
                else
                    aSize.Width() = maUpperRect.Left();
            }
        }

        if( ! bSubEditPositioned )
        {
            // this moves our sub edit if RTL gets switched
            mpEdit->SetPosPixel( Point() );
        }
        mpEdit->SetSizePixel( aSize );

#ifdef USE_JAVA
        if ( IsNativeControlSupported( CTRL_SPINBOX, PART_ENTIRE_CONTROL ) )
        {
            ImplInvalidateOutermostBorder( this );
        }
        else
        {
#endif	// USE_JAVA
        if ( GetStyle() & WB_SPIN )
            Invalidate( Rectangle( maUpperRect.TopLeft(), maLowerRect.BottomRight() ) );
        if ( GetStyle() & WB_DROPDOWN )
            Invalidate( maDropDownRect );
#ifdef USE_JAVA
        }
#endif	// USE_JAVA
    }
}

void SpinField::StateChanged( StateChangedType nType )
{
    Edit::StateChanged( nType );

    if ( nType == StateChangedType::ENABLE )
    {
        if ( mbSpin || ( GetStyle() & WB_DROPDOWN ) )
        {
            mpEdit->Enable( IsEnabled() );

#ifdef USE_JAVA
            if ( IsNativeControlSupported( CTRL_SPINBOX, PART_ENTIRE_CONTROL ) )
            {
                ImplInvalidateOutermostBorder( this );
            }
            else
            {
#endif	// USE_JAVA
            if ( mbSpin )
            {
                Invalidate( maLowerRect );
                Invalidate( maUpperRect );
            }
            if ( GetStyle() & WB_DROPDOWN )
                Invalidate( maDropDownRect );
#ifdef USE_JAVA
            }
#endif	// USE_JAVA
        }
    }
    else if ( nType == StateChangedType::STYLE )
    {
        if ( GetStyle() & WB_REPEAT )
            mbRepeat = true;
        else
            mbRepeat = false;
    }
    else if ( nType == StateChangedType::ZOOM )
    {
        Resize();
        if ( mpEdit )
            mpEdit->SetZoom( GetZoom() );
        Invalidate();
    }
    else if ( nType == StateChangedType::CONTROLFONT )
    {
        if ( mpEdit )
            mpEdit->SetControlFont( GetControlFont() );
        ImplInitSettings( true, false, false );
        Invalidate();
    }
    else if ( nType == StateChangedType::CONTROLFOREGROUND )
    {
        if ( mpEdit )
            mpEdit->SetControlForeground( GetControlForeground() );
        ImplInitSettings( false, true, false );
        Invalidate();
    }
    else if ( nType == StateChangedType::CONTROLBACKGROUND )
    {
        if ( mpEdit )
            mpEdit->SetControlBackground( GetControlBackground() );
        ImplInitSettings( false, false, true );
        Invalidate();
    }
    else if( nType == StateChangedType::MIRRORING )
    {
        if( mpEdit )
            mpEdit->StateChanged( StateChangedType::MIRRORING );
        Resize();
    }
}

void SpinField::DataChanged( const DataChangedEvent& rDCEvt )
{
    Edit::DataChanged( rDCEvt );

    if ( (rDCEvt.GetType() == DATACHANGED_SETTINGS) &&
         (rDCEvt.GetFlags() & SETTINGS_STYLE) )
    {
        Resize();
        Invalidate();
    }
}

Rectangle* SpinField::ImplFindPartRect( const Point& rPt )
{
    if( maUpperRect.IsInside( rPt ) )
        return &maUpperRect;
    else if( maLowerRect.IsInside( rPt ) )
        return &maLowerRect;
    else
        return NULL;
}

bool SpinField::PreNotify( NotifyEvent& rNEvt )
{
    const MouseEvent* pMouseEvt = NULL;

    if( (rNEvt.GetType() == EVENT_MOUSEMOVE) && (pMouseEvt = rNEvt.GetMouseEvent()) != NULL )
    {
        if( !pMouseEvt->GetButtons() && !pMouseEvt->IsSynthetic() && !pMouseEvt->IsModifierChanged() )
        {
            // trigger redraw if mouse over state has changed
            if( IsNativeControlSupported(CTRL_SPINBOX, PART_ENTIRE_CONTROL) ||
                IsNativeControlSupported(CTRL_SPINBOX, PART_ALL_BUTTONS) )
            {
                Rectangle* pRect = ImplFindPartRect( GetPointerPosPixel() );
                Rectangle* pLastRect = ImplFindPartRect( GetLastPointerPosPixel() );
                if( pRect != pLastRect || (pMouseEvt->IsLeaveWindow() || pMouseEvt->IsEnterWindow()) )
                {
#ifdef USE_JAVA
                    if ( IsNativeControlSupported( CTRL_SPINBOX, PART_ENTIRE_CONTROL ) )
                    {
                        ImplInvalidateOutermostBorder( this );
                    }
#else	// USE_JAVA
                    // FIXME: this is currently only on OS X
                    // check for other platforms that need similar handling
                    if( ImplGetSVData()->maNWFData.mbNoFocusRects &&
                        IsNativeWidgetEnabled() &&
                        IsNativeControlSupported( CTRL_EDITBOX, PART_ENTIRE_CONTROL ) )
                    {
                        ImplInvalidateOutermostBorder( this );
                    }
#endif	// USE_JAVA
                    else
                    {
                        // paint directly
                        vcl::Region aRgn( GetActiveClipRegion() );
                        if( pLastRect )
                        {
                            SetClipRegion(vcl::Region(*pLastRect));
                            Paint( *pLastRect );
                            SetClipRegion( aRgn );
                        }
                        if( pRect )
                        {
                            SetClipRegion(vcl::Region(*pRect));
                            Paint( *pRect );
                            SetClipRegion( aRgn );
                        }
                    }
                }
            }
        }
    }

    return Edit::PreNotify(rNEvt);
}

void SpinField::EndDropDown()
{
    mbInDropDown = false;
    Paint( Rectangle( Point(), GetOutputSizePixel() ) );
}

bool SpinField::ShowDropDown( bool )
{
    return false;
}

Size SpinField::CalcMinimumSizeForText(const OUString &rString) const
{
    Size aSz = Edit::CalcMinimumSizeForText(rString);

    if ( GetStyle() & WB_DROPDOWN )
        aSz.Width() += GetSettings().GetStyleSettings().GetScrollBarSize();
    if ( GetStyle() & WB_SPIN )
    {
        ImplControlValue aControlValue;
        Rectangle aArea( Point(), Size(100, aSz.Height()));
        Rectangle aEntireBound, aEntireContent, aEditBound, aEditContent;
        if (
               GetNativeControlRegion(CTRL_SPINBOX, PART_ENTIRE_CONTROL,
                   aArea, 0, aControlValue, OUString(), aEntireBound, aEntireContent) &&
               GetNativeControlRegion(CTRL_SPINBOX, PART_SUB_EDIT,
                   aArea, 0, aControlValue, OUString(), aEditBound, aEditContent)
           )
        {
            aSz.Width() += (aEntireContent.GetWidth() - aEditContent.GetWidth());
        }
        else
        {
            aSz.Width() += maUpperRect.GetWidth();
        }
    }

    return aSz;
}

Size SpinField::CalcMinimumSize() const
{
    return CalcMinimumSizeForText(GetText());
}

Size SpinField::GetOptimalSize() const
{
    return CalcMinimumSize();
}

Size SpinField::CalcSize(sal_Int32 nChars) const
{
    Size aSz = Edit::CalcSize( nChars );

    if ( GetStyle() & WB_DROPDOWN )
        aSz.Width() += GetSettings().GetStyleSettings().GetScrollBarSize();
    if ( GetStyle() & WB_SPIN )
        aSz.Width() += GetSettings().GetStyleSettings().GetSpinSize();

    return aSz;
}

IMPL_LINK( SpinField, ImplTimeout, Timer*, pTimer )
{
    if ( pTimer->GetTimeout() == GetSettings().GetMouseSettings().GetButtonStartRepeat() )
    {
        pTimer->SetTimeout( GetSettings().GetMouseSettings().GetButtonRepeat() );
        pTimer->Start();
    }
    else
    {
        if ( mbInitialUp )
            Up();
        else
            Down();

#ifdef USE_JAVA
        if ( IsNativeControlSupported( CTRL_SPINBOX, PART_ENTIRE_CONTROL ) )
            ImplInvalidateOutermostBorder( this );
#endif	// USE_JAVA
    }
    return 0;
}

void SpinField::Draw( OutputDevice* pDev, const Point& rPos, const Size& rSize, sal_uLong nFlags )
{
    Edit::Draw( pDev, rPos, rSize, nFlags );

    WinBits nFieldStyle = GetStyle();
    if ( !(nFlags & WINDOW_DRAW_NOCONTROLS ) && ( nFieldStyle & (WB_SPIN|WB_DROPDOWN) ) )
    {
        Point aPos = pDev->LogicToPixel( rPos );
        Size aSize = pDev->LogicToPixel( rSize );
        OutDevType eOutDevType = pDev->GetOutDevType();
        AllSettings aOldSettings = pDev->GetSettings();

        pDev->Push();
        pDev->SetMapMode();

        if ( eOutDevType == OUTDEV_PRINTER )
        {
            StyleSettings aStyleSettings = aOldSettings.GetStyleSettings();
            aStyleSettings.SetFaceColor( COL_LIGHTGRAY );
            aStyleSettings.SetButtonTextColor( COL_BLACK );
            AllSettings aSettings( aOldSettings );
            aSettings.SetStyleSettings( aStyleSettings );
            pDev->SetSettings( aSettings );
        }

        Rectangle aDD, aUp, aDown;
        ImplCalcButtonAreas( pDev, aSize, aDD, aUp, aDown );
        aDD.Move( aPos.X(), aPos.Y() );
        aUp.Move( aPos.X(), aPos.Y() );
        aUp.Top()++;
        aDown.Move( aPos.X(), aPos.Y() );

        Color aButtonTextColor;
        if ( ( nFlags & WINDOW_DRAW_MONO ) || ( eOutDevType == OUTDEV_PRINTER ) )
            aButtonTextColor = Color( COL_BLACK );
        else
            aButtonTextColor = GetSettings().GetStyleSettings().GetButtonTextColor();

        if ( GetStyle() & WB_DROPDOWN )
        {
#ifdef USE_JAVA
            ImplDrawSpinButton( pDev, aUp, aDown, false, false, true, true, mbInDropDown );
#else	// USE_JAVA
            DecorationView aView( pDev );
            sal_uInt16 nStyle = BUTTON_DRAW_NOLIGHTBORDER;
            Rectangle aInnerRect = aView.DrawButton( aDD, nStyle );
            SymbolType eSymbol = SymbolType::SPIN_DOWN;
            if ( GetSettings().GetStyleSettings().GetOptions() & STYLE_OPTION_SPINUPDOWN )
                eSymbol = SymbolType::SPIN_UPDOWN;

            nStyle = ( IsEnabled() || ( nFlags & WINDOW_DRAW_NODISABLE ) ) ? 0 : SYMBOL_DRAW_DISABLE;
            aView.DrawSymbol( aInnerRect, eSymbol, aButtonTextColor, nStyle );
#endif	// USE_JAVA
        }

        if ( GetStyle() & WB_SPIN )
        {
            ImplDrawSpinButton( pDev, aUp, aDown, false, false, true, true );
        }

        pDev->Pop();
        pDev->SetSettings( aOldSettings );
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
