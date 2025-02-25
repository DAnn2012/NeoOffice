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

#include "svx/dialcontrol.hxx"
#include "bmpmask.hrc"
#include <svx/dialmgr.hxx>
#include <tools/rcid.h>
#include <cmath>
#include <vcl/virdev.hxx>
#include <vcl/svapp.hxx>
#include <vcl/bitmap.hxx>
#include <vcl/field.hxx>
#include <vcl/settings.hxx>
#include <svtools/colorcfg.hxx>
#include <vcl/builder.hxx>

namespace svx {



const long DIAL_OUTER_WIDTH = 8;



DialControlBmp::DialControlBmp( vcl::Window& rParent ) :
    VirtualDevice( rParent, 0, 0 ),
    mbEnabled( true ),
    mrParent( rParent ),
    mnCenterX(0),
    mnCenterY(0)
{
    EnableRTL( false );
}

void DialControlBmp::InitBitmap(const vcl::Font& rFont)
{
    Init();
    SetFont(rFont);
}

void DialControlBmp::CopyBackground( const DialControlBmp& rSrc )
{
#ifdef USE_JAVA
    // Fix drawing artifacts on Retina displays reported in the following
    // NeoOffice forum by not layer the control on top of previous drawings
    // of the control:
    // http://trinity.neooffice.org/modules.php?name=Forums&file=viewtopic&t=8666
    DrawBackground( rSrc.maRect.GetSize(), rSrc.mbEnabled );
#else	// USE_JAVA
    Init();
    SetSize(rSrc.maRect.GetSize());
    mbEnabled = rSrc.mbEnabled;
    Point aPos;
    DrawBitmapEx( aPos, rSrc.GetBitmapEx( aPos, maRect.GetSize() ) );
#endif	// USE_JAVA
}

void DialControlBmp::DrawBackground( const Size& rSize, bool bEnabled )
{
    Init();
    SetSize(rSize);
    mbEnabled = bEnabled;
    DrawBackground();
}

void DialControlBmp::DrawElements( const OUString& rText, sal_Int32 nAngle )
{
    double fAngle = nAngle * F_PI180 / 100.0;
    double fSin = sin( fAngle );
    double fCos = cos( fAngle );
    double fWidth = GetTextWidth( rText ) / 2.0;
    double fHeight = GetTextHeight() / 2.0;

    if ( !rText.isEmpty() )
    {
        // rotated text
        vcl::Font aFont( GetFont() );
        aFont.SetColor( GetTextColor() );
        aFont.SetOrientation( static_cast< short >( (nAngle + 5) / 10 ) );  // Font uses 1/10 degrees
        aFont.SetWeight( WEIGHT_BOLD );
        SetFont( aFont );

        long nX = static_cast< long >( mnCenterX - fWidth * fCos - fHeight * fSin );
        long nY = static_cast< long >( mnCenterY + fWidth * fSin - fHeight * fCos );
        Rectangle aRect( nX, nY, 2 * mnCenterX - nX, 2 * mnCenterY - nY );
        DrawText( aRect, rText, mbEnabled ? 0 : TEXT_DRAW_DISABLE );
    }
    else
    {
        // only a line
        const sal_Int32 nDx (fCos * (maRect.GetWidth()-4) / 2);
        const sal_Int32 nDy (-fSin * (maRect.GetHeight()-4) / 2);
        Point pt1( maRect.Center() );
        Point pt2( pt1.X() + nDx, pt1.Y() + nDy);

        SetLineColor( GetTextColor() );
        DrawLine( pt1, pt2 );
    }

    // *** drag button ***

    bool bMain = (nAngle % 4500) != 0;
    SetLineColor( GetButtonLineColor() );
    SetFillColor( GetButtonFillColor( bMain ) );

    long nX = mnCenterX - static_cast< long >( (DIAL_OUTER_WIDTH / 2 - mnCenterX) * fCos );
    long nY = mnCenterY - static_cast< long >( (mnCenterY - DIAL_OUTER_WIDTH / 2) * fSin );
    long nSize = bMain ? (DIAL_OUTER_WIDTH / 4) : (DIAL_OUTER_WIDTH / 2 - 1);
    DrawEllipse( Rectangle( nX - nSize, nY - nSize, nX + nSize, nY + nSize ) );
}

const Color& DialControlBmp::GetBackgroundColor() const
{
    return GetSettings().GetStyleSettings().GetDialogColor();
}

const Color& DialControlBmp::GetTextColor() const
{
    return GetSettings().GetStyleSettings().GetLabelTextColor();
}

const Color& DialControlBmp::GetScaleLineColor() const
{
    const StyleSettings& rSett = GetSettings().GetStyleSettings();
    return mbEnabled ? rSett.GetButtonTextColor() : rSett.GetDisableColor();
}

const Color& DialControlBmp::GetButtonLineColor() const
{
    const StyleSettings& rSett = GetSettings().GetStyleSettings();
    return mbEnabled ? rSett.GetButtonTextColor() : rSett.GetDisableColor();
}

const Color& DialControlBmp::GetButtonFillColor( bool bMain ) const
{
    const StyleSettings& rSett = GetSettings().GetStyleSettings();
    return mbEnabled ? (bMain ? rSett.GetMenuColor() : rSett.GetHighlightColor()) : rSett.GetDisableColor();
}

void DialControlBmp::Init()
{
    SetSettings(mrParent.GetSettings());
    SetBackground();
}

void DialControlBmp::SetSize( const Size& rSize )
{
    maRect.SetPos( Point( 0, 0 ) );
    maRect.SetSize( rSize );
    mnCenterX = rSize.Width() / 2;
    mnCenterY = rSize.Height() / 2;
    SetOutputSize( rSize );
}

void DialControlBmp::DrawBackground()
{
    // *** background with 3D effect ***

    SetLineColor();
    SetFillColor();
    Erase();

    EnableRTL( true ); // draw 3D effect in correct direction

    sal_uInt8 nDiff = mbEnabled ? 0x18 : 0x10;
    Color aColor;

    aColor = GetBackgroundColor();
    SetFillColor( aColor );
    DrawPie( maRect, maRect.TopRight(), maRect.TopCenter() );
    DrawPie( maRect, maRect.BottomLeft(), maRect.BottomCenter() );

    aColor.DecreaseLuminance( nDiff );
    SetFillColor( aColor );
    DrawPie( maRect, maRect.BottomCenter(), maRect.TopRight() );

    aColor.DecreaseLuminance( nDiff );
    SetFillColor( aColor );
    DrawPie( maRect, maRect.BottomRight(), maRect.RightCenter() );

    aColor = GetBackgroundColor();
    aColor.IncreaseLuminance( nDiff );
    SetFillColor( aColor );
    DrawPie( maRect, maRect.TopCenter(), maRect.BottomLeft() );

    aColor.IncreaseLuminance( nDiff );
    SetFillColor( aColor );
    DrawPie( maRect, maRect.TopLeft(), maRect.LeftCenter() );

    EnableRTL( false );

    // *** calibration ***

    Point aStartPos( mnCenterX, mnCenterY );
    Color aFullColor( GetScaleLineColor() );
    Color aLightColor( GetBackgroundColor() );
    aLightColor.Merge( aFullColor, 128 );

    for( int nAngle = 0; nAngle < 360; nAngle += 15 )
    {
        SetLineColor( (nAngle % 45) ? aLightColor : aFullColor );
        double fAngle = nAngle * F_PI180;
        long nX = static_cast< long >( -mnCenterX * cos( fAngle ) );
        long nY = static_cast< long >( mnCenterY * sin( fAngle ) );
        DrawLine( aStartPos, Point( mnCenterX - nX, mnCenterY - nY ) );
    }

    // *** clear inner area ***

    SetLineColor();
    SetFillColor( GetBackgroundColor() );
    DrawEllipse( Rectangle( maRect.Left() + DIAL_OUTER_WIDTH, maRect.Top() + DIAL_OUTER_WIDTH,
        maRect.Right() - DIAL_OUTER_WIDTH, maRect.Bottom() - DIAL_OUTER_WIDTH ) );
}



DialControl::DialControl_Impl::DialControl_Impl ( vcl::Window& rParent ) :
    mpBmpEnabled(new DialControlBmp(rParent)),
    mpBmpDisabled(new DialControlBmp(rParent)),
    mpBmpBuffered(new DialControlBmp(rParent)),
    mpLinkField( 0 ),
    mnLinkedFieldValueMultiplyer( 0 ),
    mnAngle( 0 ),
    mnInitialAngle( 0 ),
    mnOldAngle( 0 ),
    mnCenterX( 0 ),
    mnCenterY( 0 ),
    mbNoRot( false )
{
}

void DialControl::DialControl_Impl::Init( const Size& rWinSize, const vcl::Font& rWinFont )
{
    maWinFont = rWinFont;
    maWinFont.SetTransparent(true);
    mpBmpBuffered->InitBitmap(maWinFont);
    SetSize(rWinSize);
}

void DialControl::DialControl_Impl::SetSize( const Size& rWinSize )
{
    // make the control squared, and adjusted so that we have a well-defined
    // center ["(x - 1) | 1" creates odd value <= x]
    long nMin = (std::min(rWinSize.Width(), rWinSize.Height()) - 1) | 1;

    maWinSize = Size( nMin, nMin );

    mnCenterX = maWinSize.Width() / 2;
    mnCenterY = maWinSize.Height() / 2;

    mpBmpEnabled->DrawBackground( maWinSize, true );
    mpBmpDisabled->DrawBackground( maWinSize, false );
    mpBmpBuffered->SetSize( maWinSize );
}



DialControl::DialControl( vcl::Window* pParent, WinBits nBits ) :
    Control( pParent, nBits ),
     mpImpl( new DialControl_Impl( *this ) )
{
    Init( GetOutputSizePixel() );
}

DialControl::~DialControl()
{
}

extern "C" SAL_DLLPUBLIC_EXPORT vcl::Window* SAL_CALL makeDialControl(vcl::Window *pParent, VclBuilder::stringmap &)
{
    return new DialControl(pParent, WB_TABSTOP);
}

void DialControl::Resize()
{
    mpImpl->SetSize(GetOutputSizePixel());
    InvalidateControl();
}

void DialControl::Paint( const Rectangle&  )
{
    Point aPos;
    DrawBitmapEx( aPos, mpImpl->mpBmpBuffered->GetBitmapEx( aPos, mpImpl->maWinSize ) );
}

void DialControl::StateChanged( StateChangedType nStateChange )
{
    if( nStateChange == StateChangedType::ENABLE )
        InvalidateControl();

    // update the linked edit field
    if( mpImpl->mpLinkField )
    {
        NumericField& rField = *mpImpl->mpLinkField;
        switch( nStateChange )
        {
            case StateChangedType::VISIBLE:  rField.Show( IsVisible() );     break;
            case StateChangedType::ENABLE:   rField.Enable( IsEnabled() );   break;
            default:;
        }
    }

    Control::StateChanged( nStateChange );
}

void DialControl::DataChanged( const DataChangedEvent& rDCEvt )
{
    if( (rDCEvt.GetType() == DATACHANGED_SETTINGS) && (rDCEvt.GetFlags() & SETTINGS_STYLE) )
    {
        Init( mpImpl->maWinSize, mpImpl->maWinFont );
        InvalidateControl();
    }
    Control::DataChanged( rDCEvt );
}

void DialControl::MouseButtonDown( const MouseEvent& rMEvt )
{
    if( rMEvt.IsLeft() )
    {
        GrabFocus();
        CaptureMouse();
        mpImpl->mnOldAngle = mpImpl->mnAngle;
        HandleMouseEvent( rMEvt.GetPosPixel(), true );
    }
    Control::MouseButtonDown( rMEvt );
}

void DialControl::MouseMove( const MouseEvent& rMEvt )
{
    if( IsMouseCaptured() && rMEvt.IsLeft() )
        HandleMouseEvent( rMEvt.GetPosPixel(), false );
    Control::MouseMove(rMEvt );
}

void DialControl::MouseButtonUp( const MouseEvent& rMEvt )
{
    if( IsMouseCaptured() )
    {
        ReleaseMouse();
        if( mpImpl->mpLinkField )
            mpImpl->mpLinkField->GrabFocus();
    }
    Control::MouseButtonUp( rMEvt );
}

void DialControl::KeyInput( const KeyEvent& rKEvt )
{
    const vcl::KeyCode& rKCode = rKEvt.GetKeyCode();
    if( !rKCode.GetModifier() && (rKCode.GetCode() == KEY_ESCAPE) )
        HandleEscapeEvent();
    else
        Control::KeyInput( rKEvt );
}

void DialControl::LoseFocus()
{
    // release captured mouse
    HandleEscapeEvent();
    Control::LoseFocus();
}

bool DialControl::HasRotation() const
{
    return !mpImpl->mbNoRot;
}

void DialControl::SetNoRotation()
{
    if( !mpImpl->mbNoRot )
    {
        mpImpl->mbNoRot = true;
        InvalidateControl();
        if( mpImpl->mpLinkField )
            mpImpl->mpLinkField->SetText( "" );
    }
}

sal_Int32 DialControl::GetRotation() const
{
    return mpImpl->mnAngle;
}

Size DialControl::GetOptimalSize() const
{
    return LogicToPixel(Size(42 , 43), MAP_APPFONT);
}

void DialControl::SetRotation( sal_Int32 nAngle )
{
    SetRotation( nAngle, false );
}

void DialControl::SetLinkedField( NumericField* pField, sal_Int32 nDecimalPlaces )
{
    mpImpl->mnLinkedFieldValueMultiplyer = 100 / std::pow(10.0, double(nDecimalPlaces));

    // remove modify handler from old linked field
    ImplSetFieldLink( Link() );
    // remember the new linked field
    mpImpl->mpLinkField = pField;
    // set modify handler at new linked field
    ImplSetFieldLink( LINK( this, DialControl, LinkedFieldModifyHdl ) );
}

void DialControl::SaveValue()
{
    mpImpl->mnInitialAngle = mpImpl->mnAngle;
}

bool DialControl::IsValueModified()
{
    return mpImpl->mnInitialAngle != mpImpl->mnAngle;
}

void DialControl::SetModifyHdl( const Link& rLink )
{
    mpImpl->maModifyHdl = rLink;
}

void DialControl::Init( const Size& rWinSize, const vcl::Font& rWinFont )
{
    mpImpl->Init( rWinSize, rWinFont );
    EnableRTL( false ); // don't mirror mouse handling
    SetOutputSizePixel( mpImpl->maWinSize );
    SetBackground();
}

void DialControl::Init( const Size& rWinSize )
{
    //hidpi TODO: GetDefaultFont() picks a font size too small, so fix it here.
    vcl::Font aDefaultSize = GetFont();

    vcl::Font aFont( OutputDevice::GetDefaultFont(
        DEFAULTFONT_UI_SANS, Application::GetSettings().GetUILanguageTag().getLanguageType(), DEFAULTFONT_FLAGS_ONLYONE ) );

    aFont.SetHeight(aDefaultSize.GetHeight());
    Init( rWinSize, aFont );
}

void DialControl::InvalidateControl()
{
    mpImpl->mpBmpBuffered->CopyBackground( IsEnabled() ? *mpImpl->mpBmpEnabled : *mpImpl->mpBmpDisabled );
    if( !mpImpl->mbNoRot )
        mpImpl->mpBmpBuffered->DrawElements( GetText(), mpImpl->mnAngle );
    Invalidate();
}

void DialControl::SetRotation( sal_Int32 nAngle, bool bBroadcast )
{
    bool bOldSel = mpImpl->mbNoRot;
    mpImpl->mbNoRot = false;

    while( nAngle < 0 )
        nAngle += 36000;

    if( !bOldSel || (mpImpl->mnAngle != nAngle) )
    {
        mpImpl->mnAngle = nAngle;
        InvalidateControl();
        if( mpImpl->mpLinkField )
            mpImpl->mpLinkField->SetValue( static_cast< long >( GetRotation() / mpImpl->mnLinkedFieldValueMultiplyer ) );
        if( bBroadcast )
            mpImpl->maModifyHdl.Call( this );
    }
}

void DialControl::ImplSetFieldLink( const Link& rLink )
{
    if( mpImpl->mpLinkField )
    {
        NumericField& rField = *mpImpl->mpLinkField;
        rField.SetModifyHdl( rLink );
        rField.SetUpHdl( rLink );
        rField.SetDownHdl( rLink );
        rField.SetFirstHdl( rLink );
        rField.SetLastHdl( rLink );
        rField.SetLoseFocusHdl( rLink );
    }
}

void DialControl::HandleMouseEvent( const Point& rPos, bool bInitial )
{
    long nX = rPos.X() - mpImpl->mnCenterX;
    long nY = mpImpl->mnCenterY - rPos.Y();
    double fH = sqrt( static_cast< double >( nX ) * nX + static_cast< double >( nY ) * nY );
    if( fH != 0.0 )
    {
        double fAngle = acos( nX / fH );
        sal_Int32 nAngle = static_cast< sal_Int32 >( fAngle / F_PI180 * 100.0 );
        if( nY < 0 )
            nAngle = 36000 - nAngle;
        if( bInitial )  // round to entire 15 degrees
            nAngle = ((nAngle + 750) / 1500) * 1500;
        // Round up to 1 degree
        nAngle = (((nAngle + 50) / 100) * 100) % 36000;
        SetRotation( nAngle, true );
    }
}

void DialControl::HandleEscapeEvent()
{
    if( IsMouseCaptured() )
    {
        ReleaseMouse();
        SetRotation( mpImpl->mnOldAngle, true );
        if( mpImpl->mpLinkField )
            mpImpl->mpLinkField->GrabFocus();
    }
}

IMPL_LINK( DialControl, LinkedFieldModifyHdl, NumericField*, pField )
{
    if( pField )
        SetRotation( static_cast< sal_Int32 >( pField->GetValue() * mpImpl->mnLinkedFieldValueMultiplyer ), false );
    return 0;
}



DialControlWrapper::DialControlWrapper( DialControl& rDial ) :
    SingleControlWrapperType( rDial )
{
}

bool DialControlWrapper::IsControlDontKnow() const
{
    return !GetControl().HasRotation();
}

void DialControlWrapper::SetControlDontKnow( bool bSet )
{
    if( bSet )
        GetControl().SetNoRotation();
}

sal_Int32 DialControlWrapper::GetControlValue() const
{
    return GetControl().GetRotation();
}

void DialControlWrapper::SetControlValue( sal_Int32 nValue )
{
    GetControl().SetRotation( nValue );
}



} // namespace svx

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
