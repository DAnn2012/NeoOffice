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

#include <vcl/outdev.hxx>
#include <vcl/window.hxx>

#include <vcl/salnativewidgets.hxx>
#include <vcl/pdfextoutdevdata.hxx>

#include <salgdi.hxx>

static bool EnableNativeWidget( const OutputDevice& i_rDevice )
{
    const OutDevType eType( i_rDevice.GetOutDevType() );
    switch ( eType )
    {

    case OUTDEV_WINDOW:
        {
            const vcl::Window* pWindow = dynamic_cast< const vcl::Window* >( &i_rDevice );
            if (pWindow)
            {
                return pWindow->IsNativeWidgetEnabled();
            }
            else
            {
                SAL_WARN ("vcl.gdi", "Could not cast i_rDevice to Window");
                assert (pWindow);
                return false;
            }
        }

    case OUTDEV_VIRDEV:
    {
        const ::vcl::ExtOutDevData* pOutDevData( i_rDevice.GetExtOutDevData() );
        const ::vcl::PDFExtOutDevData* pPDFData( dynamic_cast< const ::vcl::PDFExtOutDevData* >( pOutDevData ) );
        if ( pPDFData != NULL )
            return false;
        return true;
    }

    default:
        return false;
    }
}

ImplControlValue::~ImplControlValue()
{
}

ImplControlValue* ImplControlValue::clone() const
{
    assert( typeid( const ImplControlValue ) == typeid( *this ));
    return new ImplControlValue( *this );
}

ScrollbarValue::~ScrollbarValue()
{
}

ScrollbarValue* ScrollbarValue::clone() const
{
    assert( typeid( const ScrollbarValue ) == typeid( *this ));
    return new ScrollbarValue( *this );
}

SliderValue::~SliderValue()
{
}

SliderValue* SliderValue::clone() const
{
    assert( typeid( const SliderValue ) == typeid( *this ));
    return new SliderValue( *this );
}

TabitemValue::~TabitemValue()
{
}

TabitemValue* TabitemValue::clone() const
{
    assert( typeid( const TabitemValue ) == typeid( *this ));
    return new TabitemValue( *this );
}

SpinbuttonValue::~SpinbuttonValue()
{
}

SpinbuttonValue* SpinbuttonValue::clone() const
{
    assert( typeid( const SpinbuttonValue ) == typeid( *this ));
    return new SpinbuttonValue( *this );
}

ToolbarValue::~ToolbarValue()
{
}

ToolbarValue* ToolbarValue::clone() const
{
    assert( typeid( const ToolbarValue ) == typeid( *this ));
    return new ToolbarValue( *this );
}

MenubarValue::~MenubarValue()
{
}

MenubarValue* MenubarValue::clone() const
{
    assert( typeid( const MenubarValue ) == typeid( *this ));
    return new MenubarValue( *this );
}

MenupopupValue::~MenupopupValue()
{
}

MenupopupValue* MenupopupValue::clone() const
{
    assert( typeid( const MenupopupValue ) == typeid( *this ));
    return new MenupopupValue( *this );
}

PushButtonValue::~PushButtonValue()
{
}

PushButtonValue* PushButtonValue::clone() const
{
    assert( typeid( const PushButtonValue ) == typeid( *this ));
    return new PushButtonValue( *this );
}

#ifdef USE_JAVA

ListViewHeaderValue* ListViewHeaderValue::clone() const
{
    assert( typeid( const ListViewHeaderValue ) == typeid( *this ));
    return new ListViewHeaderValue( *this );
}

#endif	// USE_JAVA

// These functions are mainly passthrough functions that allow access to
// the SalFrame behind a Window object for native widget rendering purposes.

bool OutputDevice::IsNativeControlSupported( ControlType nType, ControlPart nPart ) const
{
    if( !EnableNativeWidget( *this ) )
        return false;

    if ( !mpGraphics )
        if ( !AcquireGraphics() )
            return false;

    return( mpGraphics->IsNativeControlSupported(nType, nPart) );
}

bool OutputDevice::HitTestNativeControl( ControlType nType,
                              ControlPart nPart,
                              const Rectangle& rControlRegion,
                              const Point& aPos,
                              bool& rIsInside ) const
{
    if( !EnableNativeWidget( *this ) )
        return false;

    if ( !mpGraphics )
        if ( !AcquireGraphics() )
            return false;

    Point aWinOffs( mnOutOffX, mnOutOffY );
    Rectangle screenRegion( rControlRegion );
    screenRegion.Move( aWinOffs.X(), aWinOffs.Y());

    return( mpGraphics->HitTestNativeControl(nType, nPart, screenRegion, Point( aPos.X() + mnOutOffX, aPos.Y() + mnOutOffY ),
        rIsInside, this ) );
}

static boost::shared_ptr< ImplControlValue > TransformControlValue( const ImplControlValue& rVal, const OutputDevice& rDev )
{
    boost::shared_ptr< ImplControlValue > aResult;
    switch( rVal.getType() )
    {
    case CTRL_SLIDER:
        {
            const SliderValue* pSlVal = static_cast<const SliderValue*>(&rVal);
            SliderValue* pNew = new SliderValue( *pSlVal );
            aResult.reset( pNew );
            pNew->maThumbRect = rDev.ImplLogicToDevicePixel( pSlVal->maThumbRect );
        }
        break;
    case CTRL_SCROLLBAR:
        {
            const ScrollbarValue* pScVal = static_cast<const ScrollbarValue*>(&rVal);
            ScrollbarValue* pNew = new ScrollbarValue( *pScVal );
            aResult.reset( pNew );
            pNew->maThumbRect = rDev.ImplLogicToDevicePixel( pScVal->maThumbRect );
            pNew->maButton1Rect = rDev.ImplLogicToDevicePixel( pScVal->maButton1Rect );
            pNew->maButton2Rect = rDev.ImplLogicToDevicePixel( pScVal->maButton2Rect );
        }
        break;
    case CTRL_SPINBUTTONS:
        {
            const SpinbuttonValue* pSpVal = static_cast<const SpinbuttonValue*>(&rVal);
            SpinbuttonValue* pNew = new SpinbuttonValue( *pSpVal );
            aResult.reset( pNew );
            pNew->maUpperRect = rDev.ImplLogicToDevicePixel( pSpVal->maUpperRect );
            pNew->maLowerRect = rDev.ImplLogicToDevicePixel( pSpVal->maLowerRect );
        }
        break;
    case CTRL_TOOLBAR:
        {
            const ToolbarValue* pTVal = static_cast<const ToolbarValue*>(&rVal);
            ToolbarValue* pNew = new ToolbarValue( *pTVal );
            aResult.reset( pNew );
            pNew->maGripRect = rDev.ImplLogicToDevicePixel( pTVal->maGripRect );
        }
        break;
    case CTRL_TAB_ITEM:
        {
            const TabitemValue* pTIVal = static_cast<const TabitemValue*>(&rVal);
            TabitemValue* pNew = new TabitemValue( *pTIVal );
            aResult.reset( pNew );
        }
        break;
    case CTRL_MENUBAR:
        {
            const MenubarValue* pMVal = static_cast<const MenubarValue*>(&rVal);
            MenubarValue* pNew = new MenubarValue( *pMVal );
            aResult.reset( pNew );
        }
        break;
    case CTRL_PUSHBUTTON:
        {
            const PushButtonValue* pBVal = static_cast<const PushButtonValue*>(&rVal);
            PushButtonValue* pNew = new PushButtonValue( *pBVal );
            aResult.reset( pNew );
        }
        break;
    case CTRL_GENERIC:
            aResult.reset( new ImplControlValue( rVal ) );
            break;
    case CTRL_MENU_POPUP:
        {
            const MenupopupValue* pMVal = static_cast<const MenupopupValue*>(&rVal);
            MenupopupValue* pNew = new MenupopupValue( *pMVal );
            pNew->maItemRect = rDev.ImplLogicToDevicePixel( pMVal->maItemRect );
            aResult.reset( pNew );
        }
        break;
#ifdef USE_JAVA
    case CTRL_LISTVIEWHEADER:
        {
            const ListViewHeaderValue* pLVal = static_cast<const ListViewHeaderValue*>(&rVal);
            ListViewHeaderValue* pNew = new ListViewHeaderValue( *pLVal );
            aResult.reset( pNew );
        }
        break;
#endif	// USE_JAVA
    default:
        OSL_FAIL( "unknown ImplControlValue type !" );
        break;
    }
    return aResult;
}
bool OutputDevice::DrawNativeControl( ControlType nType,
                            ControlPart nPart,
                            const Rectangle& rControlRegion,
                            ControlState nState,
                            const ImplControlValue& aValue,
                            const OUString& aCaption )
{
    if( !EnableNativeWidget( *this ) )
        return false;

    // make sure the current clip region is initialized correctly
    if ( !mpGraphics )
        if ( !AcquireGraphics() )
            return false;

    if ( mbInitClipRegion )
        InitClipRegion();
    if ( mbOutputClipped )
        return true;

    if ( mbInitLineColor )
        InitLineColor();
    if ( mbInitFillColor )
        InitFillColor();

    // Convert the coordinates from relative to Window-absolute, so we draw
    // in the correct place in platform code
    boost::shared_ptr< ImplControlValue > aScreenCtrlValue( TransformControlValue( aValue, *this ) );
    Rectangle screenRegion( ImplLogicToDevicePixel( rControlRegion ) );

    vcl::Region aTestRegion( GetActiveClipRegion() );
    aTestRegion.Intersect( rControlRegion );
    if (aTestRegion == vcl::Region(rControlRegion))
        nState |= CTRL_CACHING_ALLOWED;   // control is not clipped, caching allowed

    bool bRet = mpGraphics->DrawNativeControl(nType, nPart, screenRegion, nState, *aScreenCtrlValue, aCaption, this );

    return bRet;
}

bool OutputDevice::GetNativeControlRegion(  ControlType nType,
                                ControlPart nPart,
                                const Rectangle& rControlRegion,
                                ControlState nState,
                                const ImplControlValue& aValue,
                                const OUString& aCaption,
                                Rectangle &rNativeBoundingRegion,
                                Rectangle &rNativeContentRegion ) const
{
    if( !EnableNativeWidget( *this ) )
        return false;

    if ( !mpGraphics )
        if ( !AcquireGraphics() )
            return false;

    // Convert the coordinates from relative to Window-absolute, so we draw
    // in the correct place in platform code
    boost::shared_ptr< ImplControlValue > aScreenCtrlValue( TransformControlValue( aValue, *this ) );
    Rectangle screenRegion( ImplLogicToDevicePixel( rControlRegion ) );

    bool bRet = mpGraphics->GetNativeControlRegion(nType, nPart, screenRegion, nState, *aScreenCtrlValue,
                                aCaption, rNativeBoundingRegion,
                                rNativeContentRegion, this );
    if( bRet )
    {
        // transform back native regions
        rNativeBoundingRegion = ImplDevicePixelToLogic( rNativeBoundingRegion );
        rNativeContentRegion = ImplDevicePixelToLogic( rNativeContentRegion );
    }

    return bRet;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
