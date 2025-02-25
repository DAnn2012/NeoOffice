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

#include <svtools/headbar.hxx>
#include <vclxaccessibleheaderbar.hxx>
#include <tools/debug.hxx>

#include <vcl/svapp.hxx>
#include <vcl/help.hxx>
#include <vcl/image.hxx>
#include <vcl/salnativewidgets.hxx>
#include <vcl/settings.hxx>
#include <com/sun/star/accessibility/AccessibleRole.hpp>
#include <com/sun/star/accessibility/XAccessible.hpp>

class ImplHeadItem
{
public:
    sal_uInt16          mnId;
    HeaderBarItemBits   mnBits;
    long                mnSize;
    OString             maHelpId;
    Image               maImage;
    OUString            maOutText;
    OUString            maText;
    OUString            maHelpText;
};



#define HEAD_ARROWSIZE1             4
#define HEAD_ARROWSIZE2             7

#define HEADERBAR_TEXTOFF           2
#define HEADERBAR_ARROWOFF          5
#define HEADERBAR_SPLITOFF          3

#define HEADERBAR_DRAGOUTOFF        15

#define HEAD_HITTEST_ITEM           ((sal_uInt16)0x0001)
#define HEAD_HITTEST_DIVIDER        ((sal_uInt16)0x0002)



void HeaderBar::ImplInit( WinBits nWinStyle )
{
    mpItemList      = new ImplHeadItemList;
    mnBorderOff1    = 0;
    mnBorderOff2    = 0;
    mnOffset        = 0;
    mnDX            = 0;
    mnDY            = 0;
    mnDragSize      = 0;
    mnStartPos      = 0;
    mnDragPos       = 0;
    mnMouseOff      = 0;
    mnCurItemId     = 0;
    mnItemDragPos   = HEADERBAR_ITEM_NOTFOUND;
    mbDrag          = false;
    mbItemDrag      = false;
    mbOutDrag       = false;
    mbItemMode      = false;

    m_pVCLXHeaderBar = NULL;
    // StyleBits auswerten
    if ( nWinStyle & WB_DRAG )
        mbDragable = true;
    else
        mbDragable = false;
    if ( nWinStyle & WB_BUTTONSTYLE )
        mbButtonStyle = true;
    else
        mbButtonStyle = false;
    if ( nWinStyle & WB_BORDER )
    {
        mnBorderOff1 = 1;
        mnBorderOff2 = 1;
    }
    else
    {
        if ( nWinStyle & WB_BOTTOMBORDER )
            mnBorderOff2 = 1;
    }

    ImplInitSettings( true, true, true );
}



HeaderBar::HeaderBar( vcl::Window* pParent, WinBits nWinStyle ) :
    Window( pParent, nWinStyle & WB_3DLOOK )
{
    ImplInit( nWinStyle );
    SetSizePixel( CalcWindowSizePixel() );
}



HeaderBar::~HeaderBar()
{
    // Alle Items loeschen
    for ( size_t i = 0, n = mpItemList->size(); i < n; ++i ) {
        delete (*mpItemList)[ i ];
    }
    mpItemList->clear();
    delete mpItemList;
}



void HeaderBar::ImplInitSettings( bool bFont,
                                  bool bForeground, bool bBackground )
{
    const StyleSettings& rStyleSettings = GetSettings().GetStyleSettings();

    if ( bFont )
    {
        vcl::Font aFont;
        aFont = rStyleSettings.GetToolFont();
        if ( IsControlFont() )
            aFont.Merge( GetControlFont() );
        SetZoomedPointFont( aFont );
    }

    if ( bForeground || bFont )
    {
        Color aColor;
        if ( IsControlForeground() )
            aColor = GetControlForeground();
        else
            aColor = rStyleSettings.GetButtonTextColor();
        SetTextColor( aColor );
        SetTextFillColor();
    }

    if ( bBackground )
    {
        Color aColor;
        if ( IsControlBackground() )
            aColor = GetControlBackground();
        else
            aColor = rStyleSettings.GetFaceColor();
        SetBackground( aColor );
    }
}



long HeaderBar::ImplGetItemPos( sal_uInt16 nPos ) const
{
    long nX = -mnOffset;
    for ( size_t i = 0; i < nPos; i++ )
        nX += (*mpItemList)[ i ]->mnSize;
    return nX;
}



Rectangle HeaderBar::ImplGetItemRect( sal_uInt16 nPos ) const
{
    Rectangle aRect( ImplGetItemPos( nPos ), 0, 0, mnDY-1 );
    aRect.Right() = aRect.Left() + (*mpItemList)[ nPos ]->mnSize - 1;
    // check for overflow on various systems
    if ( aRect.Right() > 16000 )
        aRect.Right() = 16000;
    return aRect;
}



sal_uInt16 HeaderBar::ImplHitTest( const Point& rPos,
                               long& nMouseOff, sal_uInt16& nPos ) const
{
    ImplHeadItem*   pItem;
    size_t          nCount = (sal_uInt16)mpItemList->size();
    bool            bLastFixed = true;
    long            nX = -mnOffset;

    for ( size_t i = 0; i < nCount; i++ )
    {
        pItem = (*mpItemList)[ i ];

        if ( rPos.X() < (nX+pItem->mnSize) )
        {
            sal_uInt16 nMode;

            if ( !bLastFixed && (rPos.X() < (nX+HEADERBAR_SPLITOFF)) )
            {
                nMode = HEAD_HITTEST_DIVIDER;
                nPos = i-1;
                nMouseOff = rPos.X()-nX+1;
            }
            else
            {
                nPos = i;

                if ( !(pItem->mnBits & HIB_FIXED) && (rPos.X() >= (nX+pItem->mnSize-HEADERBAR_SPLITOFF)) )
                {
                    nMode = HEAD_HITTEST_DIVIDER;
                    nMouseOff = rPos.X()-(nX+pItem->mnSize);
                }
                else
                {
                    nMode = HEAD_HITTEST_ITEM;
                    nMouseOff = rPos.X()-nX;
                }
            }

            return nMode;
        }

        if ( pItem->mnBits & HIB_FIXED )
            bLastFixed = true;
        else
            bLastFixed = false;

        nX += pItem->mnSize;
    }

    if ( !bLastFixed )
    {
        pItem = (*mpItemList)[ nCount-1 ];
        if ( (pItem->mnSize < 4)  && (rPos.X() < (nX+HEADERBAR_SPLITOFF)) )
        {
            nPos = nCount-1;
            nMouseOff = rPos.X()-nX+1;
            return HEAD_HITTEST_DIVIDER;
        }
    }

    return 0;
}



void HeaderBar::ImplInvertDrag( sal_uInt16 nStartPos, sal_uInt16 nEndPos )
{
    Rectangle aRect1 = ImplGetItemRect( nStartPos );
    Rectangle aRect2 = ImplGetItemRect( nEndPos );
    Point     aStartPos = aRect1.Center();
    Point     aEndPos = aStartPos;
    Rectangle aStartRect( aStartPos.X()-2, aStartPos.Y()-2,
                          aStartPos.X()+2, aStartPos.Y()+2 );

    if ( nEndPos > nStartPos )
    {
        aStartPos.X() += 3;
        aEndPos.X() = aRect2.Right()-6;
    }
    else
    {
        aStartPos.X() -= 3;
        aEndPos.X() = aRect2.Left()+6;
    }

    SetRasterOp( ROP_INVERT );
    DrawRect( aStartRect );
    DrawLine( aStartPos, aEndPos );
    if ( nEndPos > nStartPos )
    {
        DrawLine( Point( aEndPos.X()+1, aEndPos.Y()-3 ),
                  Point( aEndPos.X()+1, aEndPos.Y()+3 ) );
        DrawLine( Point( aEndPos.X()+2, aEndPos.Y()-2 ),
                  Point( aEndPos.X()+2, aEndPos.Y()+2 ) );
        DrawLine( Point( aEndPos.X()+3, aEndPos.Y()-1 ),
                  Point( aEndPos.X()+3, aEndPos.Y()+1 ) );
        DrawPixel( Point( aEndPos.X()+4, aEndPos.Y() ) );
    }
    else
    {
        DrawLine( Point( aEndPos.X()-1, aEndPos.Y()-3 ),
                  Point( aEndPos.X()-1, aEndPos.Y()+3 ) );
        DrawLine( Point( aEndPos.X()-2, aEndPos.Y()-2 ),
                  Point( aEndPos.X()-2, aEndPos.Y()+2 ) );
        DrawLine( Point( aEndPos.X()-3, aEndPos.Y()-1 ),
                  Point( aEndPos.X()-3, aEndPos.Y()+1 ) );
        DrawPixel( Point( aEndPos.X()-4, aEndPos.Y() ) );
    }
    SetRasterOp( ROP_OVERPAINT );
}



void HeaderBar::ImplDrawItem( OutputDevice* pDev,
                              sal_uInt16 nPos, bool bHigh, bool bDrag,
                              const Rectangle& rItemRect,
                              const Rectangle* pRect,
                              sal_uLong )
{
    vcl::Window *const pWin = (pDev->GetOutDevType()==OUTDEV_WINDOW) ? static_cast<vcl::Window*>(pDev) : NULL;
    ImplControlValue aControlValue(0);
    Rectangle aCtrlRegion;
    ControlState nState(0);

    Rectangle aRect = rItemRect;

    // do not display if there is no space
    if ( aRect.GetWidth() <= 1 )
        return;

    // check of rectangle is visible
    if ( pRect )
    {
        if ( aRect.Right() < pRect->Left() )
            return;
        else if ( aRect.Left() > pRect->Right() )
            return;
    }
    else
    {
        if ( aRect.Right() < 0 )
            return;
        else if ( aRect.Left() > mnDX )
            return;
    }

    ImplHeadItem*           pItem  = (*mpItemList)[ nPos ];
    HeaderBarItemBits       nBits = pItem->mnBits;
    const StyleSettings&    rStyleSettings = GetSettings().GetStyleSettings();

    if( pWin && pWin->IsNativeControlSupported(CTRL_WINDOW_BACKGROUND, PART_ENTIRE_CONTROL) )
    {
        aCtrlRegion=aRect;
        pWin->DrawNativeControl( CTRL_WINDOW_BACKGROUND, PART_ENTIRE_CONTROL,
                                 aCtrlRegion, nState, aControlValue,
                                 OUString() );

    }
    else
    {
        // do not draw border
        aRect.Top()     += mnBorderOff1;
        aRect.Bottom()  -= mnBorderOff2;

        // delete background
        if ( !pRect || bDrag )
        {
            if ( bDrag )
            {
                pDev->SetLineColor();
                pDev->SetFillColor( rStyleSettings.GetCheckedColor() );
                pDev->DrawRect( aRect );
            }
            else
                pDev->DrawWallpaper( aRect, GetBackground() );
        }
    }

    Color aSelectionTextColor( COL_TRANSPARENT );

#ifdef USE_JAVA
    if ( pWin && pWin->IsNativeControlSupported( CTRL_LISTVIEWHEADER, PART_ENTIRE_CONTROL ) )
    {
        ControlState nState = CTRL_STATE_ENABLED;
        if ( !IsEnabled() )
            nState &= ~CTRL_STATE_ENABLED;
        if ( HasFocus() )
            nState |= CTRL_STATE_FOCUSED;

        ListViewHeaderValue aHeaderValue;
        aHeaderValue.mbPrimarySortColumn = ( bHigh || nBits & ( HIB_UPARROW | HIB_DOWNARROW | HIB_FLAT ) );
        if ( nBits & HIB_UPARROW )
            aHeaderValue.mnSortDirection = LISTVIEWHEADER_SORT_ASCENDING;
        else if ( nBits & HIB_DOWNARROW )
            aHeaderValue.mnSortDirection = LISTVIEWHEADER_SORT_DESCENDING;
        else if ( bHigh || nBits & HIB_FLAT )
            aHeaderValue.mnSortDirection = LISTVIEWHEADER_SORT_DONTKNOW;

        pWin->DrawNativeControl( CTRL_LISTVIEWHEADER, PART_ENTIRE_CONTROL, aRect, nState, aHeaderValue, pItem->maOutText );
    }
    else
    {
#endif	// USE_JAVA
    if( pWin && pWin->IsNativeControlSupported(CTRL_LISTHEADER, PART_BUTTON) )
    {
        aCtrlRegion=aRect;
        aControlValue.setTristateVal(BUTTONVALUE_ON);
        nState|=CTRL_STATE_ENABLED;
        if(bHigh)
            nState|=CTRL_STATE_PRESSED;
        pWin->DrawNativeControl( CTRL_LISTHEADER, PART_BUTTON,
                                 aCtrlRegion, nState, aControlValue,
                                 OUString() );
    }
    else
    {
        // draw separation line
        pDev->SetLineColor( rStyleSettings.GetDarkShadowColor() );
        pDev->DrawLine( Point( aRect.Right(), aRect.Top() ),
                        Point( aRect.Right(), aRect.Bottom() ) );

        // draw ButtonStyle
        // avoid 3D borders
        if( bHigh )
            DrawSelectionBackground( aRect, 1, true, false, false, &aSelectionTextColor );
        else if ( !mbButtonStyle || (nBits & HIB_FLAT) )
            DrawSelectionBackground( aRect, 0, true, false, false, &aSelectionTextColor );
    }
#ifdef USE_JAVA
    }
#endif	// USE_JAVA

    // do not draw if there is no space
    if ( aRect.GetWidth() < 1 )
        return;

    // calculate size and position and draw content
    pItem->maOutText = pItem->maText;
    Size aImageSize = pItem->maImage.GetSizePixel();
    Size aTxtSize( pDev->GetTextWidth( pItem->maOutText ), 0  );
    if (!pItem->maOutText.isEmpty())
        aTxtSize.Height() = pDev->GetTextHeight();
    long nArrowWidth = 0;
    if ( nBits & (HIB_UPARROW | HIB_DOWNARROW) )
        nArrowWidth = HEAD_ARROWSIZE2+HEADERBAR_ARROWOFF;

    // do not draw if there is not enough space for the image
    long nTestHeight = aImageSize.Height();
    if ( !(nBits & (HIB_LEFTIMAGE | HIB_RIGHTIMAGE)) )
        nTestHeight += aTxtSize.Height();
    if ( (aImageSize.Width() > aRect.GetWidth()) || (nTestHeight > aRect.GetHeight()) )
    {
        aImageSize.Width() = 0;
        aImageSize.Height() = 0;
    }

    // cut text to correct length
    bool bLeftText = false;
    long nMaxTxtWidth = aRect.GetWidth()-(HEADERBAR_TEXTOFF*2)-nArrowWidth;
    if ( nBits & (HIB_LEFTIMAGE | HIB_RIGHTIMAGE) )
        nMaxTxtWidth -= aImageSize.Width();
    long nTxtWidth = aTxtSize.Width();
    if ( nTxtWidth > nMaxTxtWidth )
    {
        bLeftText = true;
        OUStringBuffer aBuf(pItem->maOutText);
        aBuf.append("...");
        do
        {
            aBuf.remove(aBuf.getLength()-3-1, 1);
            nTxtWidth = pDev->GetTextWidth( aBuf.toString() );
        }
        while ( (nTxtWidth > nMaxTxtWidth) && (aBuf.getLength() > 3) );
        pItem->maOutText = aBuf.makeStringAndClear();
        if ( pItem->maOutText.getLength() == 3 )
        {
            nTxtWidth = 0;
            (pItem->maOutText).clear();
        }
    }

    // calculate text/imageposition
    long nTxtPos;
    if ( !bLeftText && (nBits & HIB_RIGHT) )
    {
        nTxtPos = aRect.Right()-nTxtWidth-HEADERBAR_TEXTOFF;
        if ( nBits & HIB_RIGHTIMAGE )
            nTxtPos -= aImageSize.Width();
    }
    else if ( !bLeftText && (nBits & HIB_CENTER) )
    {
        long nTempWidth = nTxtWidth;
        if ( nBits & (HIB_LEFTIMAGE | HIB_RIGHTIMAGE) )
            nTempWidth += aImageSize.Width();
        nTxtPos = aRect.Left()+(aRect.GetWidth()-nTempWidth)/2;
        if ( nBits & HIB_LEFTIMAGE )
            nTxtPos += aImageSize.Width();
        if ( nArrowWidth )
        {
            if ( nTxtPos+nTxtWidth+nArrowWidth >= aRect.Right() )
            {
                nTxtPos = aRect.Left()+HEADERBAR_TEXTOFF;
                if ( nBits & HIB_LEFTIMAGE )
                    nTxtPos += aImageSize.Width();
            }
        }
    }
    else
    {
        nTxtPos = aRect.Left()+HEADERBAR_TEXTOFF;
        if ( nBits & HIB_LEFTIMAGE )
            nTxtPos += aImageSize.Width();
        if ( nBits & HIB_RIGHT )
            nTxtPos += nArrowWidth;
    }

    // calculate text/imageposition
    long nTxtPosY = 0;
    if ( !pItem->maOutText.isEmpty() || (nArrowWidth && aTxtSize.Height()) )
    {
        if ( nBits & HIB_TOP )
        {
            nTxtPosY = aRect.Top();
            if ( !(nBits & (HIB_LEFTIMAGE | HIB_RIGHTIMAGE)) )
                nTxtPosY += aImageSize.Height();
        }
        else if ( nBits & HIB_BOTTOM )
            nTxtPosY = aRect.Bottom()-aTxtSize.Height();
        else
        {
            long nTempHeight = aTxtSize.Height();
            if ( !(nBits & (HIB_LEFTIMAGE | HIB_RIGHTIMAGE)) )
                nTempHeight += aImageSize.Height();
            nTxtPosY = aRect.Top()+((aRect.GetHeight()-nTempHeight)/2);
            if ( !(nBits & (HIB_LEFTIMAGE | HIB_RIGHTIMAGE)) )
                nTxtPosY += aImageSize.Height();
        }
    }

    // display text
    if (!pItem->maOutText.isEmpty())
    {
        if( aSelectionTextColor != Color( COL_TRANSPARENT ) )
        {
            pDev->Push( PushFlags::TEXTCOLOR );
            pDev->SetTextColor( aSelectionTextColor );
        }
        if ( IsEnabled() )
            pDev->DrawText( Point( nTxtPos, nTxtPosY ), pItem->maOutText );
        else
            pDev->DrawCtrlText( Point( nTxtPos, nTxtPosY ), pItem->maOutText, 0, pItem->maOutText.getLength(), TEXT_DRAW_DISABLE );
        if( aSelectionTextColor != Color( COL_TRANSPARENT ) )
            pDev->Pop();
    }

    // calculate the position and draw image if it is available
    long nImagePosY = 0;
    if ( aImageSize.Width() && aImageSize.Height() )
    {
        long nImagePos = nTxtPos;
        if ( nBits & HIB_LEFTIMAGE )
        {
            nImagePos -= aImageSize.Width();
            if ( nBits & HIB_RIGHT )
                nImagePos -= nArrowWidth;
        }
        else if ( nBits & HIB_RIGHTIMAGE )
        {
            nImagePos += nTxtWidth;
            if ( !(nBits & HIB_RIGHT) )
                nImagePos += nArrowWidth;
        }
        else
        {
            if ( nBits & HIB_RIGHT )
                nImagePos = aRect.Right()-aImageSize.Width();
            else if ( nBits & HIB_CENTER )
                nImagePos = aRect.Left()+(aRect.GetWidth()-aImageSize.Width())/2;
            else
                nImagePos = aRect.Left()+HEADERBAR_TEXTOFF;
        }

        if ( nBits & HIB_TOP )
            nImagePosY = aRect.Top();
        else if ( nBits & HIB_BOTTOM )
        {
            nImagePosY = aRect.Bottom()-aImageSize.Height();
            if ( !(nBits & (HIB_LEFTIMAGE | HIB_RIGHTIMAGE)) )
                nImagePosY -= aTxtSize.Height();
        }
        else
        {
            long nTempHeight = aImageSize.Height();
            if ( !(nBits & (HIB_LEFTIMAGE | HIB_RIGHTIMAGE)) )
                nTempHeight += aTxtSize.Height();
            nImagePosY = aRect.Top()+((aRect.GetHeight()-nTempHeight)/2);
        }
        if ( nImagePos+aImageSize.Width() <= aRect.Right() )
        {
            sal_uInt16 nStyle = 0;
            if ( !IsEnabled() )
                nStyle |= IMAGE_DRAW_DISABLE;
            pDev->DrawImage( Point( nImagePos, nImagePosY ), pItem->maImage, nStyle );
        }
    }

#ifdef USE_JAVA
    // if native control supports the sort indicator, it should already have
    // been drawn with the call to DrawEntireControl
    if ( !IsNativeControlSupported( CTRL_LISTVIEWHEADER, PART_LISTVIEWHEADER_SORT_MARK ) )
    {
#endif	// USE_JAVA
    if ( nBits & (HIB_UPARROW | HIB_DOWNARROW) )
    {
        long nArrowX = nTxtPos;
        if ( nBits & HIB_RIGHT )
            nArrowX -= nArrowWidth;
        else
            nArrowX += nTxtWidth+HEADERBAR_ARROWOFF;
        if ( !(nBits & (HIB_LEFTIMAGE | HIB_RIGHTIMAGE)) && pItem->maText.isEmpty() )
        {
            if ( nBits & HIB_RIGHT )
                nArrowX -= aImageSize.Width();
            else
                nArrowX += aImageSize.Width();
        }

        // is there enough space to draw the item?
        bool bDraw = true;
        if ( nArrowX < aRect.Left()+HEADERBAR_TEXTOFF )
            bDraw = false;
        else if ( nArrowX+HEAD_ARROWSIZE2 > aRect.Right() )
            bDraw = false;

        if ( bDraw )
        {
            if( pWin && pWin->IsNativeControlSupported(CTRL_LISTHEADER, PART_ARROW) )
            {
                aCtrlRegion=Rectangle(Point(nArrowX,aRect.Top()),Size(nArrowWidth,aRect.GetHeight()));
                // control value passes 1 if arrow points down, 0 otherwise
                aControlValue.setNumericVal((nBits&HIB_DOWNARROW)?1:0);
                nState|=CTRL_STATE_ENABLED;
                if(bHigh)
                    nState|=CTRL_STATE_PRESSED;
                pWin->DrawNativeControl( CTRL_LISTHEADER, PART_ARROW,
                                         aCtrlRegion, nState, aControlValue,
                                         OUString() );
            }
            else
            {
                long nArrowY;
                if ( aTxtSize.Height() )
                    nArrowY = nTxtPosY+(aTxtSize.Height()/2);
                else if ( aImageSize.Width() && aImageSize.Height() )
                    nArrowY = nImagePosY+(aImageSize.Height()/2);
                else
                {
                    if ( nBits & HIB_TOP )
                        nArrowY = aRect.Top()+1;
                    else if ( nBits & HIB_BOTTOM )
                        nArrowY = aRect.Bottom()-HEAD_ARROWSIZE2-1;
                    else
                        nArrowY = aRect.Top()+((aRect.GetHeight()-HEAD_ARROWSIZE2)/2);
                }
                nArrowY -= HEAD_ARROWSIZE1-1;
                if ( nBits & HIB_DOWNARROW )
                {
                    pDev->SetLineColor( rStyleSettings.GetLightColor() );
                    pDev->DrawLine( Point( nArrowX, nArrowY ),
                                    Point( nArrowX+HEAD_ARROWSIZE2, nArrowY ) );
                    pDev->DrawLine( Point( nArrowX, nArrowY ),
                                    Point( nArrowX+HEAD_ARROWSIZE1, nArrowY+HEAD_ARROWSIZE2 ) );
                    pDev->SetLineColor( rStyleSettings.GetShadowColor() );
                    pDev->DrawLine( Point( nArrowX+HEAD_ARROWSIZE1, nArrowY+HEAD_ARROWSIZE2 ),
                                    Point( nArrowX+HEAD_ARROWSIZE2, nArrowY ) );
                }
                else
                {
                    pDev->SetLineColor( rStyleSettings.GetLightColor() );
                    pDev->DrawLine( Point( nArrowX, nArrowY+HEAD_ARROWSIZE2 ),
                                    Point( nArrowX+HEAD_ARROWSIZE1, nArrowY ) );
                    pDev->SetLineColor( rStyleSettings.GetShadowColor() );
                    pDev->DrawLine( Point( nArrowX, nArrowY+HEAD_ARROWSIZE2 ),
                                    Point( nArrowX+HEAD_ARROWSIZE2, nArrowY+HEAD_ARROWSIZE2 ) );
                    pDev->DrawLine( Point( nArrowX+HEAD_ARROWSIZE2, nArrowY+HEAD_ARROWSIZE2 ),
                                    Point( nArrowX+HEAD_ARROWSIZE1, nArrowY ) );
                }
            }
        }
    }
#ifdef USE_JAVA
    }
#endif	// USE_JAVA

    // all UserDraw if required
    if ( nBits & HIB_USERDRAW )
    {
        vcl::Region aRegion( aRect );
        if ( pRect )
            aRegion.Intersect( *pRect );
        pDev->SetClipRegion( aRegion );
        UserDrawEvent aODEvt( pDev, aRect, pItem->mnId );
        UserDraw( aODEvt );
        pDev->SetClipRegion();
    }
}



void HeaderBar::ImplDrawItem( sal_uInt16 nPos, bool bHigh, bool bDrag,
                              const Rectangle* pRect )
{
    Rectangle aRect = ImplGetItemRect( nPos );
    ImplDrawItem( this, nPos, bHigh, bDrag, aRect, pRect, 0 );
}



void HeaderBar::ImplUpdate( sal_uInt16 nPos, bool bEnd, bool bDirect )
{
    if ( IsVisible() && IsUpdateMode() )
    {
        if ( !bDirect )
        {
            Rectangle   aRect;
            size_t      nItemCount = mpItemList->size();
            if ( nPos < nItemCount )
                aRect = ImplGetItemRect( nPos );
            else
            {
                aRect.Bottom() = mnDY-1;
                if ( nItemCount )
                    aRect.Left() = ImplGetItemRect( nItemCount-1 ).Right();
            }
            if ( bEnd )
                aRect.Right() = mnDX-1;
            aRect.Top()     += mnBorderOff1;
            aRect.Bottom()  -= mnBorderOff2;
            Invalidate( aRect );
        }
        else
        {
            for ( size_t i = nPos; i < mpItemList->size(); i++ )
                ImplDrawItem( i );
            if ( bEnd )
            {
                Rectangle aRect = ImplGetItemRect( (sal_uInt16)mpItemList->size() );
                aRect.Left()  = aRect.Right();
                aRect.Right() = mnDX-1;
                if ( aRect.Left() < aRect.Right() )
                {
                    aRect.Top()     += mnBorderOff1;
                    aRect.Bottom()  -= mnBorderOff2;
                    Erase( aRect );
                }
            }
        }
    }
}



void HeaderBar::ImplStartDrag( const Point& rMousePos, bool bCommand )
{
    sal_uInt16  nPos;
    sal_uInt16  nHitTest = ImplHitTest( rMousePos, mnMouseOff, nPos );
    if ( nHitTest )
    {
        mbDrag = false;
        ImplHeadItem* pItem = (*mpItemList)[ nPos ];
        if ( nHitTest & HEAD_HITTEST_DIVIDER )
            mbDrag = true;
        else
        {
            if ( ((pItem->mnBits & HIB_CLICKABLE) && !(pItem->mnBits & HIB_FLAT)) ||
                 (mbDragable && !(pItem->mnBits & HIB_FIXEDPOS)) )
            {
                mbItemMode = true;
                mbDrag = true;
                if ( bCommand )
                {
                    if ( mbDragable )
                        mbItemDrag = true;
                    else
                    {
                        mbItemMode = false;
                        mbDrag = false;
                    }
                }
            }
            else
            {
                if ( !bCommand )
                {
                    mnCurItemId = pItem->mnId;
                    Select();
                    mnCurItemId = 0;
                }
            }
        }

        if ( mbDrag )
        {
            mbOutDrag = false;
            mnCurItemId = pItem->mnId;
            mnItemDragPos = nPos;
            StartTracking();
            mnStartPos = rMousePos.X()-mnMouseOff;
            mnDragPos = mnStartPos;
            StartDrag();
            if ( mbItemMode )
                ImplDrawItem( nPos, true, mbItemDrag );
            else
            {
                Rectangle aSizeRect( mnDragPos, 0, mnDragPos, mnDragSize+mnDY );
                ShowTracking( aSizeRect, SHOWTRACK_SPLIT );
            }
        }
        else
            mnMouseOff = 0;
    }
}



void HeaderBar::ImplDrag( const Point& rMousePos )
{
    bool        bNewOutDrag;
    sal_uInt16  nPos = GetItemPos( mnCurItemId );

    mnDragPos = rMousePos.X()-mnMouseOff;
    if ( mbItemMode )
    {
        Rectangle aItemRect = ImplGetItemRect( nPos );
        if ( aItemRect.IsInside( rMousePos ) )
            bNewOutDrag = false;
        else
            bNewOutDrag = true;

        //  if needed switch on ItemDrag
        if ( bNewOutDrag && mbDragable && !mbItemDrag &&
             !((*mpItemList)[ nPos ]->mnBits & HIB_FIXEDPOS) )
        {
            if ( (rMousePos.Y() >= aItemRect.Top()) && (rMousePos.Y() <= aItemRect.Bottom()) )
            {
                mbItemDrag = true;
                ImplDrawItem( nPos, true, mbItemDrag );
            }
        }

        sal_uInt16 nOldItemDragPos = mnItemDragPos;
        if ( mbItemDrag )
        {
            if ( (rMousePos.Y() < -HEADERBAR_DRAGOUTOFF) || (rMousePos.Y() > mnDY+HEADERBAR_DRAGOUTOFF) )
                bNewOutDrag = true;
            else
                bNewOutDrag = false;

            if ( bNewOutDrag )
                mnItemDragPos = HEADERBAR_ITEM_NOTFOUND;
            else
            {
                sal_uInt16 nTempId = GetItemId( Point( rMousePos.X(), 2 ) );
                if ( nTempId )
                    mnItemDragPos = GetItemPos( nTempId );
                else
                {
                    if ( rMousePos.X() <= 0 )
                        mnItemDragPos = 0;
                    else
                        mnItemDragPos = GetItemCount()-1;
                }

                // do not use non-movable items
                if ( mnItemDragPos < nPos )
                {
                    while ( ((*mpItemList)[ mnItemDragPos ]->mnBits & HIB_FIXEDPOS) &&
                            (mnItemDragPos < nPos) )
                        mnItemDragPos++;
                }
                else if ( mnItemDragPos > nPos )
                {
                    while ( ((*mpItemList)[ mnItemDragPos ]->mnBits & HIB_FIXEDPOS) &&
                            (mnItemDragPos > nPos) )
                        mnItemDragPos--;
                }
            }

            if ( (mnItemDragPos != nOldItemDragPos) &&
                 (nOldItemDragPos != nPos) &&
                 (nOldItemDragPos != HEADERBAR_ITEM_NOTFOUND) )
            {
                ImplInvertDrag( nPos, nOldItemDragPos );
                ImplDrawItem( nOldItemDragPos );
            }
        }

        if ( bNewOutDrag != mbOutDrag )
            ImplDrawItem( nPos, !bNewOutDrag, mbItemDrag );

        if ( mbItemDrag  )
        {
            if ( (mnItemDragPos != nOldItemDragPos) &&
                 (mnItemDragPos != nPos) &&
                 (mnItemDragPos != HEADERBAR_ITEM_NOTFOUND) )
            {
                ImplDrawItem( mnItemDragPos, false, true );
                ImplInvertDrag( nPos, mnItemDragPos );
            }
        }

        mbOutDrag = bNewOutDrag;
    }
    else
    {
        Rectangle aItemRect = ImplGetItemRect( nPos );
        if ( mnDragPos < aItemRect.Left() )
            mnDragPos = aItemRect.Left();
        if ( (mnDragPos < 0) || (mnDragPos > mnDX-1) )
            HideTracking();
        else
        {
            Rectangle aSizeRect( mnDragPos, 0, mnDragPos, mnDragSize+mnDY );
            ShowTracking( aSizeRect, SHOWTRACK_SPLIT );
        }
    }

    Drag();
}



void HeaderBar::ImplEndDrag( bool bCancel )
{
    HideTracking();

    if ( bCancel || mbOutDrag )
    {
        if ( mbItemMode && (!mbOutDrag || mbItemDrag) )
        {
            sal_uInt16 nPos = GetItemPos( mnCurItemId );
            ImplDrawItem( nPos );
        }

        mnCurItemId = 0;
    }
    else
    {
        sal_uInt16 nPos = GetItemPos( mnCurItemId );
        if ( mbItemMode )
        {
            if ( mbItemDrag )
            {
                Pointer aPointer( POINTER_ARROW );
                SetPointer( aPointer );
                if ( (mnItemDragPos != nPos) &&
                     (mnItemDragPos != HEADERBAR_ITEM_NOTFOUND) )
                {
                    ImplInvertDrag( nPos, mnItemDragPos );
                    MoveItem( mnCurItemId, mnItemDragPos );
                }
                else
                    ImplDrawItem( nPos );
            }
            else
            {
                Select();
                ImplUpdate( nPos );
            }
        }
        else
        {
            long nDelta = mnDragPos - mnStartPos;
            if ( nDelta )
            {
                ImplHeadItem* pItem = (*mpItemList)[ nPos ];
                pItem->mnSize += nDelta;
                ImplUpdate( nPos, true );
            }
        }
    }

    mbDrag          = false;
    EndDrag();
    mnCurItemId     = 0;
    mnItemDragPos   = HEADERBAR_ITEM_NOTFOUND;
    mbOutDrag       = false;
    mbItemMode      = false;
    mbItemDrag      = false;
}



void HeaderBar::MouseButtonDown( const MouseEvent& rMEvt )
{
    if ( rMEvt.IsLeft() )
    {
        if ( rMEvt.GetClicks() == 2 )
        {
            long    nTemp;
            sal_uInt16  nPos;
            sal_uInt16  nHitTest = ImplHitTest( rMEvt.GetPosPixel(), nTemp, nPos );
            if ( nHitTest )
            {
                ImplHeadItem* pItem = (*mpItemList)[ nPos ];
                if ( nHitTest & HEAD_HITTEST_DIVIDER )
                    mbItemMode = false;
                else
                    mbItemMode = true;
                mnCurItemId = pItem->mnId;
                DoubleClick();
                mbItemMode = false;
                mnCurItemId = 0;
            }
        }
        else
            ImplStartDrag( rMEvt.GetPosPixel(), false );
    }
}



void HeaderBar::MouseMove( const MouseEvent& rMEvt )
{
    long            nTemp1;
    sal_uInt16          nTemp2;
    PointerStyle    eStyle = POINTER_ARROW;
    sal_uInt16          nHitTest = ImplHitTest( rMEvt.GetPosPixel(), nTemp1, nTemp2 );

    if ( nHitTest & HEAD_HITTEST_DIVIDER )
        eStyle = POINTER_HSIZEBAR;
    Pointer aPtr( eStyle );
    SetPointer( aPtr );
}



void HeaderBar::Tracking( const TrackingEvent& rTEvt )
{
    Point aMousePos = rTEvt.GetMouseEvent().GetPosPixel();

    if ( rTEvt.IsTrackingEnded() )
        ImplEndDrag( rTEvt.IsTrackingCanceled() );
    else
        ImplDrag( aMousePos );
}



void HeaderBar::Paint( const Rectangle& rRect )
{
    if ( mnBorderOff1 || mnBorderOff2 )
    {
        SetLineColor( GetSettings().GetStyleSettings().GetDarkShadowColor() );
        if ( mnBorderOff1 )
            DrawLine( Point( 0, 0 ), Point( mnDX-1, 0 ) );
        if ( mnBorderOff2 )
            DrawLine( Point( 0, mnDY-1 ), Point( mnDX-1, mnDY-1 ) );
        // #i40393# draw left and right border, if WB_BORDER was set in ImplInit()
        if ( mnBorderOff1 && mnBorderOff2 )
        {
            DrawLine( Point( 0, 0 ), Point( 0, mnDY-1 ) );
            DrawLine( Point( mnDX-1, 0 ), Point( mnDX-1, mnDY-1 ) );
        }
    }

    sal_uInt16 nCurItemPos;
    if ( mbDrag )
        nCurItemPos = GetItemPos( mnCurItemId );
    else
        nCurItemPos = HEADERBAR_ITEM_NOTFOUND;
    sal_uInt16 nItemCount = (sal_uInt16)mpItemList->size();
    for ( sal_uInt16 i = 0; i < nItemCount; i++ )
        ImplDrawItem( i, (i == nCurItemPos), false, &rRect );
}



void HeaderBar::Draw( OutputDevice* pDev, const Point& rPos, const Size& rSize,
                      sal_uLong nFlags )
{
    Point       aPos  = pDev->LogicToPixel( rPos );
    Size        aSize = pDev->LogicToPixel( rSize );
    Rectangle   aRect( aPos, aSize );
    vcl::Font   aFont = GetDrawPixelFont( pDev );

    pDev->Push();
    pDev->SetMapMode();
    pDev->SetFont( aFont );
    if ( nFlags & WINDOW_DRAW_MONO )
        pDev->SetTextColor( Color( COL_BLACK ) );
    else
        pDev->SetTextColor( GetTextColor() );
    pDev->SetTextFillColor();

    if ( !(nFlags & WINDOW_DRAW_NOBACKGROUND) )
    {
        pDev->DrawWallpaper( aRect, GetBackground() );
        if ( mnBorderOff1 || mnBorderOff2 )
        {
            pDev->SetLineColor( GetSettings().GetStyleSettings().GetDarkShadowColor() );
            if ( mnBorderOff1 )
                pDev->DrawLine( aRect.TopLeft(), Point( aRect.Right(), aRect.Top() ) );
            if ( mnBorderOff2 )
                pDev->DrawLine( Point( aRect.Left(), aRect.Bottom() ), Point( aRect.Right(), aRect.Bottom() ) );
            // #i40393# draw left and right border, if WB_BORDER was set in ImplInit()
            if ( mnBorderOff1 && mnBorderOff2 )
            {
                pDev->DrawLine( aRect.TopLeft(), Point( aRect.Left(), aRect.Bottom() ) );
                pDev->DrawLine( Point( aRect.Right(), aRect.Top() ), Point( aRect.Right(), aRect.Bottom() ) );
            }
        }
    }

    Rectangle aItemRect( aRect );
    size_t nItemCount = mpItemList->size();
    for ( size_t i = 0; i < nItemCount; i++ )
    {
        aItemRect.Left() = aRect.Left()+ImplGetItemPos( i );
        aItemRect.Right() = aItemRect.Left() + (*mpItemList)[ i ]->mnSize - 1;
        // check for overflow on some systems
        if ( aItemRect.Right() > 16000 )
            aItemRect.Right() = 16000;
        vcl::Region aRegion( aRect );
        pDev->SetClipRegion( aRegion );
        ImplDrawItem( pDev, i, false, false, aItemRect, &aRect, nFlags );
        pDev->SetClipRegion();
    }

    pDev->Pop();
}



void HeaderBar::Resize()
{
    Size aSize = GetOutputSizePixel();
    if ( IsVisible() && (mnDY != aSize.Height()) )
        Invalidate();
    mnDX = aSize.Width();
    mnDY = aSize.Height();
}



void HeaderBar::Command( const CommandEvent& rCEvt )
{
    if ( rCEvt.IsMouseEvent() && (rCEvt.GetCommand() == COMMAND_STARTDRAG) && !mbDrag )
    {
        ImplStartDrag( rCEvt.GetMousePosPixel(), true );
        return;
    }

    Window::Command( rCEvt );
}



void HeaderBar::RequestHelp( const HelpEvent& rHEvt )
{
    sal_uInt16 nItemId = GetItemId( ScreenToOutputPixel( rHEvt.GetMousePosPixel() ) );
    if ( nItemId )
    {
        if ( rHEvt.GetMode() & (HelpEventMode::QUICK | HelpEventMode::BALLOON) )
        {
            Rectangle aItemRect = GetItemRect( nItemId );
            Point aPt = OutputToScreenPixel( aItemRect.TopLeft() );
            aItemRect.Left()   = aPt.X();
            aItemRect.Top()    = aPt.Y();
            aPt = OutputToScreenPixel( aItemRect.BottomRight() );
            aItemRect.Right()  = aPt.X();
            aItemRect.Bottom() = aPt.Y();

            OUString aStr = GetHelpText( nItemId );
            if ( aStr.isEmpty() || !(rHEvt.GetMode() & HelpEventMode::BALLOON) )
            {
                ImplHeadItem* pItem = (*mpItemList)[ GetItemPos( nItemId ) ];
                // Quick-help is only displayed if the text is not fully visible.
                // Otherwise we display Helptext only if the items do not contain text
                if ( pItem->maOutText != pItem->maText )
                    aStr = pItem->maText;
                else if (!pItem->maText.isEmpty())
                    aStr.clear();
            }

            if (!aStr.isEmpty())
            {
                if ( rHEvt.GetMode() & HelpEventMode::BALLOON )
                    Help::ShowBalloon( this, aItemRect.Center(), aItemRect, aStr );
                else
                    Help::ShowQuickHelp( this, aItemRect, aStr );
                return;
            }
        }
        else if ( rHEvt.GetMode() & HelpEventMode::EXTENDED )
        {
            OUString aHelpId( OStringToOUString( GetHelpId( nItemId ), RTL_TEXTENCODING_UTF8 ) );
            if ( !aHelpId.isEmpty() )
            {
                // display it if help is available
                Help* pHelp = Application::GetHelp();
                if ( pHelp )
                    pHelp->Start( aHelpId, this );
                return;
            }
        }
    }

    Window::RequestHelp( rHEvt );
}



void HeaderBar::StateChanged( StateChangedType nType )
{
    Window::StateChanged( nType );

    if ( nType == StateChangedType::ENABLE )
        Invalidate();
    else if ( (nType == StateChangedType::ZOOM) ||
              (nType == StateChangedType::CONTROLFONT) )
    {
        ImplInitSettings( true, false, false );
        Invalidate();
    }
    else if ( nType == StateChangedType::CONTROLFOREGROUND )
    {
        ImplInitSettings( false, true, false );
        Invalidate();
    }
    else if ( nType == StateChangedType::CONTROLBACKGROUND )
    {
        ImplInitSettings( false, false, true );
        Invalidate();
    }
}



void HeaderBar::DataChanged( const DataChangedEvent& rDCEvt )
{
    Window::DataChanged( rDCEvt );

    if ( (rDCEvt.GetType() == DATACHANGED_FONTS) ||
         (rDCEvt.GetType() == DATACHANGED_FONTSUBSTITUTION) ||
         ((rDCEvt.GetType() == DATACHANGED_SETTINGS) &&
          (rDCEvt.GetFlags() & SETTINGS_STYLE)) )
    {
        ImplInitSettings( true, true, true );
        Invalidate();
    }
}



void HeaderBar::UserDraw( const UserDrawEvent& )
{
}



void HeaderBar::StartDrag()
{
    maStartDragHdl.Call( this );
}



void HeaderBar::Drag()
{
    maDragHdl.Call( this );
}



void HeaderBar::EndDrag()
{
    maEndDragHdl.Call( this );
}



void HeaderBar::Select()
{
    maSelectHdl.Call( this );
}



void HeaderBar::DoubleClick()
{
    maDoubleClickHdl.Call( this );
}



void HeaderBar::InsertItem( sal_uInt16 nItemId, const OUString& rText,
                            long nSize, HeaderBarItemBits nBits, sal_uInt16 nPos )
{
    DBG_ASSERT( nItemId, "HeaderBar::InsertItem(): ItemId == 0" );
    DBG_ASSERT( GetItemPos( nItemId ) == HEADERBAR_ITEM_NOTFOUND,
                "HeaderBar::InsertItem(): ItemId already exists" );

    // create item and insert in the list
    ImplHeadItem* pItem = new ImplHeadItem;
    pItem->mnId         = nItemId;
    pItem->mnBits       = nBits;
    pItem->mnSize       = nSize;
    pItem->maText       = rText;
    if ( nPos < mpItemList->size() ) {
        ImplHeadItemList::iterator it = mpItemList->begin();
        ::std::advance( it, nPos );
        mpItemList->insert( it, pItem );
    } else {
        mpItemList->push_back( pItem );
    }

    // update display
    ImplUpdate( nPos, true );
}



void HeaderBar::RemoveItem( sal_uInt16 nItemId )
{
    sal_uInt16 nPos = GetItemPos( nItemId );
    if ( nPos != HEADERBAR_ITEM_NOTFOUND )
    {
        if ( nPos < mpItemList->size() ) {
            ImplHeadItemList::iterator it = mpItemList->begin();
            ::std::advance( it, nPos );
            delete *it;
            mpItemList->erase( it );
        }
    }
}



void HeaderBar::MoveItem( sal_uInt16 nItemId, sal_uInt16 nNewPos )
{
    sal_uInt16 nPos = GetItemPos( nItemId );
    if ( nPos != HEADERBAR_ITEM_NOTFOUND )
    {
        if ( nPos != nNewPos )
        {
            ImplHeadItemList::iterator it = mpItemList->begin();
            ::std::advance( it, nPos );
            ImplHeadItem* pItem = *it;
            mpItemList->erase( it );
            if ( nNewPos < nPos )
                nPos = nNewPos;
            it = mpItemList->begin();
            ::std::advance( it, nNewPos );
            mpItemList->insert( it, pItem );
            ImplUpdate( nPos, true);
        }
    }
}



void HeaderBar::Clear()
{
    // delete all items
    for ( size_t i = 0, n = mpItemList->size(); i < n; ++i ) {
        delete (*mpItemList)[ i ];
    }
    mpItemList->clear();

    ImplUpdate( 0, true );
}



void HeaderBar::SetOffset( long nNewOffset )
{
    // move area
    Rectangle aRect( 0, mnBorderOff1, mnDX-1, mnDY-mnBorderOff1-mnBorderOff2-1 );
    long nDelta = mnOffset-nNewOffset;
    mnOffset = nNewOffset;
    Scroll( nDelta, 0, aRect );
}



sal_uInt16 HeaderBar::GetItemCount() const
{
    return (sal_uInt16)mpItemList->size();
}



sal_uInt16 HeaderBar::GetItemPos( sal_uInt16 nItemId ) const
{
    for ( size_t i = 0, n = mpItemList->size(); i < n; ++i ) {
        ImplHeadItem* pItem = (*mpItemList)[ i ];
        if ( pItem->mnId == nItemId )
            return (sal_uInt16)i;
    }
    return HEADERBAR_ITEM_NOTFOUND;
}



sal_uInt16 HeaderBar::GetItemId( sal_uInt16 nPos ) const
{
    ImplHeadItem* pItem = (nPos < mpItemList->size() ) ? (*mpItemList)[ nPos ] : NULL;
    if ( pItem )
        return pItem->mnId;
    else
        return 0;
}



sal_uInt16 HeaderBar::GetItemId( const Point& rPos ) const
{
    for ( size_t i = 0, n = mpItemList->size(); i < n; ++i ) {
        if ( ImplGetItemRect( i ).IsInside( rPos ) ) {
            return GetItemId( i );
        }
    }
    return 0;
}



Rectangle HeaderBar::GetItemRect( sal_uInt16 nItemId ) const
{
    Rectangle aRect;
    sal_uInt16 nPos = GetItemPos( nItemId );
    if ( nPos != HEADERBAR_ITEM_NOTFOUND )
        aRect = ImplGetItemRect( nPos );
    return aRect;
}



void HeaderBar::SetItemSize( sal_uInt16 nItemId, long nNewSize )
{
    sal_uInt16 nPos = GetItemPos( nItemId );
    if ( nPos != HEADERBAR_ITEM_NOTFOUND )
    {
        ImplHeadItem* pItem = (*mpItemList)[ nPos ];
        if ( pItem->mnSize != nNewSize )
        {
            pItem->mnSize = nNewSize;
            ImplUpdate( nPos, true );
        }
    }
}



long HeaderBar::GetItemSize( sal_uInt16 nItemId ) const
{
    sal_uInt16 nPos = GetItemPos( nItemId );
    if ( nPos != HEADERBAR_ITEM_NOTFOUND )
        return (*mpItemList)[ nPos ]->mnSize;
    else
        return 0;
}



void HeaderBar::SetItemBits( sal_uInt16 nItemId, HeaderBarItemBits nNewBits )
{
    sal_uInt16 nPos = GetItemPos( nItemId );
    if ( nPos != HEADERBAR_ITEM_NOTFOUND )
    {
        ImplHeadItem* pItem = (*mpItemList)[ nPos ];
        if ( pItem->mnBits != nNewBits )
        {
            pItem->mnBits = nNewBits;
            ImplUpdate( nPos );
        }
    }
}



HeaderBarItemBits HeaderBar::GetItemBits( sal_uInt16 nItemId ) const
{
    sal_uInt16 nPos = GetItemPos( nItemId );
    if ( nPos != HEADERBAR_ITEM_NOTFOUND )
        return (*mpItemList)[ nPos ]->mnBits;
    else
        return 0;
}



void HeaderBar::SetItemText( sal_uInt16 nItemId, const OUString& rText )
{
    sal_uInt16 nPos = GetItemPos( nItemId );
    if ( nPos != HEADERBAR_ITEM_NOTFOUND )
    {
        (*mpItemList)[ nPos ]->maText = rText;
        ImplUpdate( nPos );
    }
}



OUString HeaderBar::GetItemText( sal_uInt16 nItemId ) const
{
    sal_uInt16 nPos = GetItemPos( nItemId );
    if ( nPos != HEADERBAR_ITEM_NOTFOUND )
        return (*mpItemList)[ nPos ]->maText;
    return OUString();
}



OUString HeaderBar::GetHelpText( sal_uInt16 nItemId ) const
{
    sal_uInt16 nPos = GetItemPos( nItemId );
    if ( nPos != HEADERBAR_ITEM_NOTFOUND )
    {
        ImplHeadItem* pItem = (*mpItemList)[ nPos ];
        if ( pItem->maHelpText.isEmpty() && !pItem->maHelpId.isEmpty() )
        {
            Help* pHelp = Application::GetHelp();
            if ( pHelp )
                pItem->maHelpText = pHelp->GetHelpText( OStringToOUString( pItem->maHelpId, RTL_TEXTENCODING_UTF8 ), this );
        }

        return pItem->maHelpText;
    }

    return OUString();
}



OString HeaderBar::GetHelpId( sal_uInt16 nItemId ) const
{
    sal_uInt16 nPos = GetItemPos( nItemId );
    OString aRet;
    if ( nPos != HEADERBAR_ITEM_NOTFOUND )
        return (*mpItemList)[ nPos ]->maHelpId;
    return aRet;
}



Size HeaderBar::CalcWindowSizePixel() const
{
    long nMaxImageSize = 0;
    Size aSize( 0, GetTextHeight() );

    for ( size_t i = 0, n = mpItemList->size(); i < n; ++i )
    {
        ImplHeadItem* pItem = (*mpItemList)[ i ];
        // take image size into account
        long nImageHeight = pItem->maImage.GetSizePixel().Height();
        if ( !(pItem->mnBits & (HIB_LEFTIMAGE | HIB_RIGHTIMAGE)) && !pItem->maText.isEmpty() )
            nImageHeight += aSize.Height();
        if ( nImageHeight > nMaxImageSize )
            nMaxImageSize = nImageHeight;

        // add width
        aSize.Width() += pItem->mnSize;
    }

    if ( nMaxImageSize > aSize.Height() )
        aSize.Height() = nMaxImageSize;

    // add border
    if ( mbButtonStyle )
        aSize.Height() += 4;
    else
        aSize.Height() += 2;
    aSize.Height() += mnBorderOff1+mnBorderOff2;

    return aSize;
}

::com::sun::star::uno::Reference< ::com::sun::star::accessibility::XAccessible > HeaderBar::CreateAccessible()
{
    if ( !mxAccessible.is() )
    {
        if ( maCreateAccessibleHdl.IsSet() )
            maCreateAccessibleHdl.Call( this );

        if ( !mxAccessible.is() )
            mxAccessible = Window::CreateAccessible();
    }

    return mxAccessible;
}

void HeaderBar::SetAccessible( ::com::sun::star::uno::Reference< ::com::sun::star::accessibility::XAccessible > _xAccessible )
{
    mxAccessible = _xAccessible;
}

::com::sun::star::uno::Reference< ::com::sun::star::awt::XWindowPeer > HeaderBar::GetComponentInterface( bool bCreate )
{
    ::com::sun::star::uno::Reference< ::com::sun::star::awt::XWindowPeer > xPeer
        (Window::GetComponentInterface(false));
    if ( !xPeer.is() && bCreate )
    {
        ::com::sun::star::awt::XWindowPeer* mxPeer = new VCLXHeaderBar(this);
        m_pVCLXHeaderBar = static_cast<VCLXHeaderBar*>(mxPeer);
        SetComponentInterface(mxPeer);
        return mxPeer;
    }
    else
        return xPeer;
}


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
