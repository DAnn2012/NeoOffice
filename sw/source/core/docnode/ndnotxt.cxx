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

#include <hintids.hxx>
#include <tools/poly.hxx>
#include <tools/fract.hxx>
#include <svl/stritem.hxx>
#include <svx/contdlg.hxx>
#include <vcl/svapp.hxx>
#include <docary.hxx>
#include <doc.hxx>
#include <fmtcol.hxx>
#include <ndnotxt.hxx>
#include <ndgrf.hxx>
#include <ndole.hxx>
#include <ndindex.hxx>
#include <hints.hxx>
#include <istyleaccess.hxx>
#include <SwStyleNameMapper.hxx>

#include <frmfmt.hxx>

SwNoTxtNode::SwNoTxtNode( const SwNodeIndex & rWhere,
                  const sal_uInt8 nNdType,
                  SwGrfFmtColl *pGrfColl,
                  SwAttrSet* pAutoAttr ) :
    SwCntntNode( rWhere, nNdType, pGrfColl ),
    pContour( 0 ),
    bAutomaticContour( false ),
    bContourMapModeValid( true ),
    bPixelContour( false )
{
    // Should this set a hard attribute?
    if( pAutoAttr )
        SetAttr( *pAutoAttr );
}

SwNoTxtNode::~SwNoTxtNode()
{
    delete pContour;
}

/// Creates an AttrSet for all derivations with ranges for frame-
/// and graphics-attributes.
void SwNoTxtNode::NewAttrSet( SwAttrPool& rPool )
{
    OSL_ENSURE( !mpAttrSet.get(), "AttrSet is already set" );
    SwAttrSet aNewAttrSet( rPool, aNoTxtNodeSetRange );

    // put names of parent style and conditional style:
    const SwFmtColl* pFmtColl = GetFmtColl();
    OUString sVal;
    SwStyleNameMapper::FillProgName( pFmtColl->GetName(), sVal, nsSwGetPoolIdFromName::GET_POOLID_TXTCOLL, true );
    SfxStringItem aFmtColl( RES_FRMATR_STYLE_NAME, sVal );
    aNewAttrSet.Put( aFmtColl );

    aNewAttrSet.SetParent( &GetFmtColl()->GetAttrSet() );
    mpAttrSet = GetDoc()->GetIStyleAccess().getAutomaticStyle( aNewAttrSet, IStyleAccess::AUTO_STYLE_NOTXT );
}

/// Dummies for loading/saving of persistent data
/// when working with graphics and OLE objects
bool SwNoTxtNode::RestorePersistentData()
{
    return true;
}

bool SwNoTxtNode::SavePersistentData()
{
    return true;
}

void SwNoTxtNode::SetContour( const tools::PolyPolygon *pPoly, bool bAutomatic )
{
    delete pContour;
    if ( pPoly )
        pContour = new tools::PolyPolygon( *pPoly );
    else
        pContour = 0;
    bAutomaticContour = bAutomatic;
    bContourMapModeValid = true;
    bPixelContour = false;
}

void SwNoTxtNode::CreateContour()
{
    OSL_ENSURE( !pContour, "Contour available." );
    pContour = new tools::PolyPolygon(SvxContourDlg::CreateAutoContour(GetGraphic()));
    bAutomaticContour = true;
    bContourMapModeValid = true;
    bPixelContour = false;
}

const tools::PolyPolygon *SwNoTxtNode::HasContour() const
{
    if( !bContourMapModeValid )
    {
        const MapMode aGrfMap( GetGraphic().GetPrefMapMode() );
#ifdef USE_JAVA
        // Fix bug 3593 by treating MAP_POINT the same as MAP_PIXEL when
        // calculating contours
        bool bPixelGrf = ( aGrfMap.GetMapUnit() == MAP_PIXEL || aGrfMap.GetMapUnit() == MAP_POINT );
#else	// USE_JAVA
        bool bPixelGrf = aGrfMap.GetMapUnit() == MAP_PIXEL;
#endif	// USE_JAVA
        const MapMode aContourMap( bPixelGrf ? MAP_PIXEL : MAP_100TH_MM );
        if( bPixelGrf ? !bPixelContour : aGrfMap != aContourMap )
        {
            // #i102238#
            double nGrfDPIx = 0.0;
            double nGrfDPIy = 0.0;
            {
                if ( !bPixelGrf && bPixelContour )
                {
                    const Size aGrfPixelSize( GetGraphic().GetSizePixel() );
                    const Size aGrfPrefMapModeSize( GetGraphic().GetPrefSize() );
                    if ( aGrfMap.GetMapUnit() == MAP_INCH )
                    {
                        nGrfDPIx = aGrfPixelSize.Width() / ( (double)aGrfMap.GetScaleX() * aGrfPrefMapModeSize.Width() );
                        nGrfDPIy = aGrfPixelSize.Height() / ( (double)aGrfMap.GetScaleY() * aGrfPrefMapModeSize.Height() );
                    }
                    else
                    {
                        const Size aGrf1000thInchSize =
                            OutputDevice::LogicToLogic( aGrfPrefMapModeSize,
                                                        aGrfMap, MAP_1000TH_INCH );
                        nGrfDPIx = 1000.0 * aGrfPixelSize.Width() / aGrf1000thInchSize.Width();
                        nGrfDPIy = 1000.0 * aGrfPixelSize.Height() / aGrf1000thInchSize.Height();
                    }
                }
            }
            OSL_ENSURE( !bPixelGrf || aGrfMap == aContourMap,
                        "scale factor for pixel unsupported" );
            OutputDevice* pOutDev =
                (bPixelGrf || bPixelContour) ? Application::GetDefaultDevice()
                                             : 0;
            sal_uInt16 nPolyCount = pContour->Count();
            for( sal_uInt16 j=0; j<nPolyCount; j++ )
            {
                Polygon& rPoly = (*pContour)[j];

                sal_uInt16 nCount = rPoly.GetSize();
                for( sal_uInt16 i=0 ; i<nCount; i++ )
                {
                    if( bPixelGrf )
                        rPoly[i] = pOutDev->LogicToPixel( rPoly[i],
                                                          aContourMap );
                    else if( bPixelContour )
                    {
                        rPoly[i] = pOutDev->PixelToLogic( rPoly[i], aGrfMap );
                        // #i102238#
                        if ( nGrfDPIx != 0 && nGrfDPIy != 0 )
                        {
                            rPoly[i] = Point( rPoly[i].getX() * pOutDev->GetDPIX() / nGrfDPIx,
                                              rPoly[i].getY() * pOutDev->GetDPIY() / nGrfDPIy );
                        }
                    }
                    else
                        rPoly[i] = OutputDevice::LogicToLogic( rPoly[i],
                                                                 aContourMap,
                                                                 aGrfMap );
                }
            }
        }
        ((SwNoTxtNode *)this)->bContourMapModeValid = true;
        ((SwNoTxtNode *)this)->bPixelContour = false;
    }

    return pContour;
}

void SwNoTxtNode::GetContour( tools::PolyPolygon &rPoly ) const
{
    OSL_ENSURE( pContour, "Contour not available." );
    rPoly = *HasContour();
}

void SwNoTxtNode::SetContourAPI( const tools::PolyPolygon *pPoly )
{
    delete pContour;
    if ( pPoly )
        pContour = new tools::PolyPolygon( *pPoly );
    else
        pContour = 0;
    bContourMapModeValid = false;
}

bool SwNoTxtNode::GetContourAPI( tools::PolyPolygon &rContour ) const
{
    if( !pContour )
        return false;

    rContour = *pContour;
    if( bContourMapModeValid )
    {
        const MapMode aGrfMap( GetGraphic().GetPrefMapMode() );
        const MapMode aContourMap( MAP_100TH_MM );
        OSL_ENSURE( aGrfMap.GetMapUnit() != MAP_PIXEL ||
                aGrfMap == MapMode( MAP_PIXEL ),
                    "scale factor for pixel unsupported" );
        if( aGrfMap.GetMapUnit() != MAP_PIXEL &&
            aGrfMap != aContourMap )
        {
            sal_uInt16 nPolyCount = rContour.Count();
            for( sal_uInt16 j=0; j<nPolyCount; j++ )
            {
                // --> OD #i102238# - use the right <tools::PolyPolygon> instance
                Polygon& rPoly = rContour[j];
                // <--

                sal_uInt16 nCount = rPoly.GetSize();
                for( sal_uInt16 i=0 ; i<nCount; i++ )
                {
                    rPoly[i] = OutputDevice::LogicToLogic( rPoly[i], aGrfMap,
                                                           aContourMap );
                }
            }
        }
    }

    return true;
}

bool SwNoTxtNode::IsPixelContour() const
{
    bool bRet;
    if( bContourMapModeValid )
    {
        const MapMode aGrfMap( GetGraphic().GetPrefMapMode() );
        bRet = aGrfMap.GetMapUnit() == MAP_PIXEL;
    }
    else
    {
        bRet = bPixelContour;
    }

    return bRet;
}

Graphic SwNoTxtNode::GetGraphic() const
{
    Graphic aRet;
    if ( GetGrfNode() )
    {
        aRet = static_cast<const SwGrfNode*>(this)->GetGrf(true);
    }
    else
    {
        OSL_ENSURE( GetOLENode(), "new type of Node?" );
        aRet = *const_cast<SwOLENode*>(static_cast<const SwOLENode*>(this))->SwOLENode::GetGraphic();
    }
    return aRet;
}

// #i73249#
void SwNoTxtNode::SetTitle( const OUString& rTitle, bool bBroadcast )
{
    // Title attribute of <SdrObject> replaces own AlternateText attribute
    SwFlyFrmFmt* pFlyFmt = dynamic_cast<SwFlyFrmFmt*>(GetFlyFmt());
    OSL_ENSURE( pFlyFmt, "<SwNoTxtNode::SetTitle(..)> - missing <SwFlyFrmFmt> instance" );
    if ( !pFlyFmt )
    {
        return;
    }

    pFlyFmt->SetObjTitle( rTitle, bBroadcast );
}

OUString SwNoTxtNode::GetTitle() const
{
    const SwFlyFrmFmt* pFlyFmt = dynamic_cast<const SwFlyFrmFmt*>(GetFlyFmt());
    OSL_ENSURE( pFlyFmt, "<SwNoTxtNode::GetTitle(..)> - missing <SwFlyFrmFmt> instance" );
    if ( !pFlyFmt )
    {
        return OUString();
    }

    return pFlyFmt->GetObjTitle();
}

void SwNoTxtNode::SetDescription( const OUString& rDescription, bool bBroadcast )
{
    SwFlyFrmFmt* pFlyFmt = dynamic_cast<SwFlyFrmFmt*>(GetFlyFmt());
    OSL_ENSURE( pFlyFmt, "<SwNoTxtNode::SetDescription(..)> - missing <SwFlyFrmFmt> instance" );
    if ( !pFlyFmt )
    {
        return;
    }

    pFlyFmt->SetObjDescription( rDescription, bBroadcast );
}

OUString SwNoTxtNode::GetDescription() const
{
    const SwFlyFrmFmt* pFlyFmt = dynamic_cast<const SwFlyFrmFmt*>(GetFlyFmt());
    OSL_ENSURE( pFlyFmt, "<SwNoTxtNode::GetDescription(..)> - missing <SwFlyFrmFmt> instance" );
    if ( !pFlyFmt )
    {
        return OUString();
    }

    return pFlyFmt->GetObjDescription();
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
