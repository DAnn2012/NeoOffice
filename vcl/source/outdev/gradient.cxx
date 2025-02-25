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

#include <tools/poly.hxx>

#include <vcl/gradient.hxx>
#include <vcl/virdev.hxx>
#include <vcl/outdev.hxx>
#include <vcl/settings.hxx>

#include "salgdi.hxx"

#define GRADIENT_DEFAULT_STEPCOUNT  0

void OutputDevice::DrawGradient( const Rectangle& rRect,
                                 const Gradient& rGradient )
{
    // Convert rectangle to a tools::PolyPolygon by first converting to a Polygon
    Polygon aPolygon ( rRect );
    tools::PolyPolygon aPolyPoly ( aPolygon );

    DrawGradient ( aPolyPoly, rGradient );
}

void OutputDevice::DrawGradient( const tools::PolyPolygon& rPolyPoly,
                                 const Gradient& rGradient )
{
    if ( mnDrawMode & DRAWMODE_NOGRADIENT )
        return;     // nothing to draw!

    if ( mbInitClipRegion )
        InitClipRegion();

    if ( mbOutputClipped )
        return;

    if ( rPolyPoly.Count() && rPolyPoly[ 0 ].GetSize() )
    {
        if ( mnDrawMode & ( DRAWMODE_BLACKGRADIENT | DRAWMODE_WHITEGRADIENT | DRAWMODE_SETTINGSGRADIENT) )
        {
            Color aColor = GetSingleColorGradientFill();

            Push( PushFlags::LINECOLOR | PushFlags::FILLCOLOR );
            SetLineColor( aColor );
            SetFillColor( aColor );
            DrawPolyPolygon( rPolyPoly );
            Pop();
            return;
        }

        Gradient aGradient( rGradient );

        if ( mnDrawMode & ( DRAWMODE_GRAYGRADIENT | DRAWMODE_GHOSTEDGRADIENT ) )
        {
            SetGrayscaleColors( aGradient );
        }

        DrawGradientToMetafile( rPolyPoly, rGradient );

        if( !IsDeviceOutputNecessary() || ImplIsRecordLayout() )
            return;

        // Clip and then draw the gradient
        if( !Rectangle( PixelToLogic( Point() ), GetOutputSize() ).IsEmpty() )
        {
            const Rectangle aBoundRect( rPolyPoly.GetBoundRect() );

            // convert rectangle to pixels
            Rectangle aRect( ImplLogicToDevicePixel( aBoundRect ) );
            aRect.Justify();

            // do nothing if the rectangle is empty
            if ( !aRect.IsEmpty() )
            {
                tools::PolyPolygon aClipPolyPoly( ImplLogicToDevicePixel( rPolyPoly ) );
                bool bDrawn = false;

                if( !mpGraphics && !AcquireGraphics() )
                    return;

                // secure clip region
                Push( PushFlags::CLIPREGION );
                IntersectClipRegion( aBoundRect );

                if( mbInitClipRegion )
                    InitClipRegion();

                // try to draw gradient natively
                bDrawn = mpGraphics->DrawGradient( aClipPolyPoly, aGradient, this );

                if( !bDrawn && !mbOutputClipped )
                {
                    // draw gradients without border
                    if( mbLineColor || mbInitLineColor )
                    {
                        mpGraphics->SetLineColor();
                        mbInitLineColor = true;
                    }

                    mbInitFillColor = true;

                    // calculate step count if necessary
                    if ( !aGradient.GetSteps() )
                        aGradient.SetSteps( GRADIENT_DEFAULT_STEPCOUNT );

                    if ( rPolyPoly.IsRect() )
                    {
                        // because we draw with no border line, we have to expand gradient
                        // rect to avoid missing lines on the right and bottom edge
                        aRect.Left()--;
                        aRect.Top()--;
                        aRect.Right()++;
                        aRect.Bottom()++;
                    }

                    // if the clipping polypolygon is a rectangle, then it's the same size as the bounding of the
                    // polypolygon, so pass in a NULL for the clipping parameter
                    if( aGradient.GetStyle() == GradientStyle_LINEAR || rGradient.GetStyle() == GradientStyle_AXIAL )
                        DrawLinearGradient( aRect, aGradient, aClipPolyPoly.IsRect() ? NULL : &aClipPolyPoly );
                    else
                        DrawComplexGradient( aRect, aGradient, aClipPolyPoly.IsRect() ? NULL : &aClipPolyPoly );
                }

                Pop();
            }
        }
    }

    if( mpAlphaVDev )
        mpAlphaVDev->DrawPolyPolygon( rPolyPoly );
}

void OutputDevice::ClipAndDrawGradientMetafile ( const Gradient &rGradient, const tools::PolyPolygon &rPolyPoly )
{
    const Rectangle aBoundRect( rPolyPoly.GetBoundRect() );
    const bool  bOldOutput = IsOutputEnabled();

    EnableOutput( false );
#if defined USE_JAVA && defined MACOSX
    // Avoid expensive XORing to draw transparent objects
    DrawGradient( aBoundRect, rGradient );
#else	// USE_JAVA && MACOSX
    Push( PushFlags::RASTEROP );
    SetRasterOp( ROP_XOR );
    DrawGradient( aBoundRect, rGradient );
    SetFillColor( COL_BLACK );
    SetRasterOp( ROP_0 );
    DrawPolyPolygon( rPolyPoly );
    SetRasterOp( ROP_XOR );
    DrawGradient( aBoundRect, rGradient );
    Pop();
#endif	// USE_JAVA && MACOSX
    EnableOutput( bOldOutput );
}

void OutputDevice::DrawGradientToMetafile ( const tools::PolyPolygon& rPolyPoly,
                                            const Gradient& rGradient )
{
    if ( !mpMetaFile )
        return;

    if ( rPolyPoly.Count() && rPolyPoly[ 0 ].GetSize() )
    {
        Gradient aGradient( rGradient );

        if ( mnDrawMode & ( DRAWMODE_GRAYGRADIENT | DRAWMODE_GHOSTEDGRADIENT ) )
        {
            SetGrayscaleColors( aGradient );
        }

        const Rectangle aBoundRect( rPolyPoly.GetBoundRect() );

        if ( rPolyPoly.IsRect() )
        {
            mpMetaFile->AddAction( new MetaGradientAction( aBoundRect, aGradient ) );
        }
        else
        {
            mpMetaFile->AddAction( new MetaCommentAction( "XGRAD_SEQ_BEGIN" ) );
#if defined USE_JAVA && defined MACOSX
            // Fix stray lines drawn at top of transparent gradients that
            // occurs when running slideshows on a Retina display by passing
            // the gradient's shape to OutputDevice::DrawTransparent via the
            // clip region
            mpMetaFile->AddAction( new MetaCommentAction( "XTRANSGRADPUSHCLIP_SEQ_BEGIN" ) );
            mpMetaFile->AddAction( new MetaPushAction( PushFlags::CLIPREGION ) );
            mpMetaFile->AddAction( new MetaISectRegionClipRegionAction( vcl::Region( rPolyPoly ) ) );
            mpMetaFile->AddAction( new MetaCommentAction( "XTRANSGRADPUSHCLIP_SEQ_END" ) );
#endif	// USE_JAVA && MACOSX
            mpMetaFile->AddAction( new MetaGradientExAction( rPolyPoly, rGradient ) );

            ClipAndDrawGradientMetafile ( rGradient, rPolyPoly );

#if defined USE_JAVA && defined MACOSX
            // Append action to pop clip
            mpMetaFile->AddAction( new MetaCommentAction( "XTRANSGRADPOPCLIP_SEQ_BEGIN" ) );
            mpMetaFile->AddAction( new MetaPopAction() );
            mpMetaFile->AddAction( new MetaCommentAction( "XTRANSGRADPOPCLIP_SEQ_END" ) );
#endif	// USE_JAVA && MACOSX
            mpMetaFile->AddAction( new MetaCommentAction( "XGRAD_SEQ_END" ) );
        }

        if( !IsDeviceOutputNecessary() || ImplIsRecordLayout() )
            return;

        // Clip and then draw the gradient
        if( !Rectangle( PixelToLogic( Point() ), GetOutputSize() ).IsEmpty() )
        {
            // convert rectangle to pixels
            Rectangle aRect( ImplLogicToDevicePixel( aBoundRect ) );
            aRect.Justify();

            // do nothing if the rectangle is empty
            if ( !aRect.IsEmpty() )
            {
                if( !mbOutputClipped )
                {
                    // calculate step count if necessary
                    if ( !aGradient.GetSteps() )
                        aGradient.SetSteps( GRADIENT_DEFAULT_STEPCOUNT );

                    if ( rPolyPoly.IsRect() )
                    {
                        // because we draw with no border line, we have to expand gradient
                        // rect to avoid missing lines on the right and bottom edge
                        aRect.Left()--;
                        aRect.Top()--;
                        aRect.Right()++;
                        aRect.Bottom()++;
                    }

                    // if the clipping polypolygon is a rectangle, then it's the same size as the bounding of the
                    // polypolygon, so pass in a NULL for the clipping parameter
                    if( aGradient.GetStyle() == GradientStyle_LINEAR || rGradient.GetStyle() == GradientStyle_AXIAL )
                        DrawLinearGradientToMetafile( aRect, aGradient );
                    else
                        DrawComplexGradientToMetafile( aRect, aGradient );
                }
            }
        }
    }
}

namespace
{
    inline sal_uInt8 GetGradientColorValue( long nValue )
    {
        if ( nValue < 0 )
            return 0;
        else if ( nValue > 0xFF )
            return 0xFF;
        else
            return (sal_uInt8)nValue;
    }
}

void OutputDevice::DrawLinearGradient( const Rectangle& rRect,
                                       const Gradient& rGradient,
                                       const tools::PolyPolygon* pClipPolyPoly )
{
    // get BoundRect of rotated rectangle
    Rectangle aRect;
    Point     aCenter;
    sal_uInt16    nAngle = rGradient.GetAngle() % 3600;

    rGradient.GetBoundRect( rRect, aRect, aCenter );

#if defined USE_JAVA && defined MACOSX
    Rectangle aFullRect = aRect;
#endif	// USE_JAVA && MACOSX
    bool bLinear = (rGradient.GetStyle() == GradientStyle_LINEAR);
    double fBorder = rGradient.GetBorder() * aRect.GetHeight() / 100.0;
    if ( !bLinear )
    {
        fBorder /= 2.0;
    }
    Rectangle aMirrorRect = aRect; // used in style axial
    aMirrorRect.Top() = ( aRect.Top() + aRect.Bottom() ) / 2;
    if ( !bLinear )
    {
        aRect.Bottom() = aMirrorRect.Top();
    }

    // colour-intensities of start- and finish; change if needed
    long    nFactor;
    Color   aStartCol   = rGradient.GetStartColor();
    Color   aEndCol     = rGradient.GetEndColor();
    long    nStartRed   = aStartCol.GetRed();
    long    nStartGreen = aStartCol.GetGreen();
    long    nStartBlue  = aStartCol.GetBlue();
    long    nEndRed     = aEndCol.GetRed();
    long    nEndGreen   = aEndCol.GetGreen();
    long    nEndBlue    = aEndCol.GetBlue();
            nFactor     = rGradient.GetStartIntensity();
            nStartRed   = (nStartRed   * nFactor) / 100;
            nStartGreen = (nStartGreen * nFactor) / 100;
            nStartBlue  = (nStartBlue  * nFactor) / 100;
            nFactor     = rGradient.GetEndIntensity();
            nEndRed     = (nEndRed   * nFactor) / 100;
            nEndGreen   = (nEndGreen * nFactor) / 100;
            nEndBlue    = (nEndBlue  * nFactor) / 100;

    // gradient style axial has exchanged start and end colors
    if ( !bLinear)
    {
        long nTempColor = nStartRed;
        nStartRed = nEndRed;
        nEndRed = nTempColor;
        nTempColor = nStartGreen;
        nStartGreen = nEndGreen;
        nEndGreen = nTempColor;
        nTempColor = nStartBlue;
        nStartBlue = nEndBlue;
        nEndBlue = nTempColor;
    }

    sal_uInt8   nRed;
    sal_uInt8   nGreen;
    sal_uInt8   nBlue;

    // Create border
    Rectangle aBorderRect = aRect;
    Polygon     aPoly( 4 );
    if (fBorder > 0.0)
    {
        nRed        = (sal_uInt8)nStartRed;
        nGreen      = (sal_uInt8)nStartGreen;
        nBlue       = (sal_uInt8)nStartBlue;

        mpGraphics->SetFillColor( MAKE_SALCOLOR( nRed, nGreen, nBlue ) );

        aBorderRect.Bottom() = (long)( aBorderRect.Top() + fBorder );
        aRect.Top() = aBorderRect.Bottom();
        aPoly[0] = aBorderRect.TopLeft();
        aPoly[1] = aBorderRect.TopRight();
#if defined USE_JAVA && defined MACOSX
        // Fix printing bug reported in the following NeoOffice forum post by
        // underlapping all successive stripes with the current color:
        // http://trinity.neooffice.org/modules.php?name=Forums&file=viewtopic&p=63688#63688
        if ( meRasterOp == ROP_OVERPAINT )
        {
            aPoly[2] = aFullRect.BottomRight();
            aPoly[3] = aFullRect.BottomLeft();
        }
        else
        {
#endif	// USE_JAVA && MACOSX
        aPoly[2] = aBorderRect.BottomRight();
        aPoly[3] = aBorderRect.BottomLeft();
#if defined USE_JAVA && defined MACOSX
        }
#endif	// USE_JAVA && MACOSX
        aPoly.Rotate( aCenter, nAngle );

        ImplDrawPolygon( aPoly, pClipPolyPoly );

        if ( !bLinear)
        {
            aBorderRect = aMirrorRect;
            aBorderRect.Top() = (long) ( aBorderRect.Bottom() - fBorder );
            aMirrorRect.Bottom() = aBorderRect.Top();
            aPoly[0] = aBorderRect.TopLeft();
            aPoly[1] = aBorderRect.TopRight();
            aPoly[2] = aBorderRect.BottomRight();
            aPoly[3] = aBorderRect.BottomLeft();
            aPoly.Rotate( aCenter, nAngle );

            ImplDrawPolygon( aPoly, pClipPolyPoly );
        }
    }

    // calculate step count
    bool    bMtf = false;
    long    nStepCount  = GetGradientSteps( rGradient, aRect, bMtf );

    // minimal three steps and maximal as max color steps
    long   nAbsRedSteps   = std::abs( nEndRed   - nStartRed );
    long   nAbsGreenSteps = std::abs( nEndGreen - nStartGreen );
    long   nAbsBlueSteps  = std::abs( nEndBlue  - nStartBlue );
    long   nMaxColorSteps = std::max( nAbsRedSteps , nAbsGreenSteps );
    nMaxColorSteps = std::max( nMaxColorSteps, nAbsBlueSteps );
    long nSteps = std::min( nStepCount, nMaxColorSteps );
    if ( nSteps < 3)
    {
        nSteps = 3;
    }

    double fScanInc = ((double)aRect.GetHeight()) / (double) nSteps;
    double fGradientLine = (double)aRect.Top();
    double fMirrorGradientLine = (double) aMirrorRect.Bottom();

    double fAlpha = 0.0;
    const double fStepsMinus1 = ((double)nSteps) - 1.0;
    double fTempColor;
    if ( !bLinear)
    {
        nSteps -= 1; // draw middle polygons as one polygon after loop to avoid gap
    }
    for ( long i = 0; i < nSteps; i++ )
    {
        // linear interpolation of color
        fAlpha = ((double)i) / fStepsMinus1;
        fTempColor = ((double)nStartRed) * (1.0-fAlpha) + ((double)nEndRed) * fAlpha;
        nRed = GetGradientColorValue((long)fTempColor);
        fTempColor = ((double)nStartGreen) * (1.0-fAlpha) + ((double)nEndGreen) * fAlpha;
        nGreen = GetGradientColorValue((long)fTempColor);
        fTempColor = ((double)nStartBlue) * (1.0-fAlpha) + ((double)nEndBlue) * fAlpha;
        nBlue = GetGradientColorValue((long)fTempColor);

        mpGraphics->SetFillColor( MAKE_SALCOLOR( nRed, nGreen, nBlue ) );

        // Polygon for this color step
        aRect.Top() = (long)( fGradientLine + ((double) i) * fScanInc );
        aRect.Bottom() = (long)( fGradientLine + ( ((double) i) + 1.0 ) * fScanInc );
        aPoly[0] = aRect.TopLeft();
        aPoly[1] = aRect.TopRight();
#if defined USE_JAVA && defined MACOSX
        // Fix printing bug reported in the following NeoOffice forum post by
        // underlapping all successive stripes with the current color:
        // http://trinity.neooffice.org/modules.php?name=Forums&file=viewtopic&p=63688#63688
        if ( meRasterOp == ROP_OVERPAINT )
        {
            if ( bLinear )
            {
                aPoly[2] = aFullRect.BottomRight();
                aPoly[3] = aFullRect.BottomLeft();
            }
            else
            {
                aMirrorRect.Bottom() = (long)( fMirrorGradientLine - ((double) i) * fScanInc );
                aMirrorRect.Top() = (long)( fMirrorGradientLine - (((double) i) + 1.0)* fScanInc );
                aPoly[2] = aMirrorRect.BottomRight();
                aPoly[3] = aMirrorRect.BottomLeft();
            }
        }
        else
        {
#endif	// USE_JAVA && MACOSX
        aPoly[2] = aRect.BottomRight();
        aPoly[3] = aRect.BottomLeft();
#if defined USE_JAVA && defined MACOSX
        }
#endif	// USE_JAVA && MACOSX
        aPoly.Rotate( aCenter, nAngle );

        ImplDrawPolygon( aPoly, pClipPolyPoly );

        if ( !bLinear )
        {
            aMirrorRect.Bottom() = (long)( fMirrorGradientLine - ((double) i) * fScanInc );
            aMirrorRect.Top() = (long)( fMirrorGradientLine - (((double) i) + 1.0)* fScanInc );
            aPoly[0] = aMirrorRect.TopLeft();
            aPoly[1] = aMirrorRect.TopRight();
            aPoly[2] = aMirrorRect.BottomRight();
            aPoly[3] = aMirrorRect.BottomLeft();
            aPoly.Rotate( aCenter, nAngle );

            ImplDrawPolygon( aPoly, pClipPolyPoly );
        }
    }
    if ( !bLinear)
    {
        // draw middle polygon with end color
        nRed = GetGradientColorValue(nEndRed);
        nGreen = GetGradientColorValue(nEndGreen);
        nBlue = GetGradientColorValue(nEndBlue);

        mpGraphics->SetFillColor( MAKE_SALCOLOR( nRed, nGreen, nBlue ) );

        aRect.Top() = (long)( fGradientLine + ((double)nSteps) * fScanInc );
        aRect.Bottom() = (long)( fMirrorGradientLine - ((double) nSteps) * fScanInc );
        aPoly[0] = aRect.TopLeft();
        aPoly[1] = aRect.TopRight();
        aPoly[2] = aRect.BottomRight();
        aPoly[3] = aRect.BottomLeft();
        aPoly.Rotate( aCenter, nAngle );

        ImplDrawPolygon( aPoly, pClipPolyPoly );
    }
}

void OutputDevice::DrawComplexGradient( const Rectangle& rRect,
                                        const Gradient& rGradient,
                                        const tools::PolyPolygon* pClipPolyPoly )
{
    // Determine if we output via Polygon or PolyPolygon
    // For all rasteroperations other then Overpaint always use PolyPolygon,
    // as we will get wrong results if we output multiple times on top of each other.
    // Also for printers always use PolyPolygon, as not all printers
    // can print polygons on top of each other.

    boost::scoped_ptr<tools::PolyPolygon> pPolyPoly;
    Rectangle       aRect;
    Point           aCenter;
    Color           aStartCol( rGradient.GetStartColor() );
    Color           aEndCol( rGradient.GetEndColor() );
    long            nStartRed = ( (long) aStartCol.GetRed() * rGradient.GetStartIntensity() ) / 100;
    long            nStartGreen = ( (long) aStartCol.GetGreen() * rGradient.GetStartIntensity() ) / 100;
    long            nStartBlue = ( (long) aStartCol.GetBlue() * rGradient.GetStartIntensity() ) / 100;
    long            nEndRed = ( (long) aEndCol.GetRed() * rGradient.GetEndIntensity() ) / 100;
    long            nEndGreen = ( (long) aEndCol.GetGreen() * rGradient.GetEndIntensity() ) / 100;
    long            nEndBlue = ( (long) aEndCol.GetBlue() * rGradient.GetEndIntensity() ) / 100;
    long            nRedSteps = nEndRed - nStartRed;
    long            nGreenSteps = nEndGreen - nStartGreen;
    long            nBlueSteps = nEndBlue   - nStartBlue;
    sal_uInt16      nAngle = rGradient.GetAngle() % 3600;

    rGradient.GetBoundRect( rRect, aRect, aCenter );

    if ( UsePolyPolygonForComplexGradient() )
        pPolyPoly.reset(new tools::PolyPolygon( 2 ));

    bool bMtf = false;
    bool bComplex = true;
    long nStepCount = GetGradientSteps( rGradient, rRect, bMtf, bComplex );

    // at least three steps and at most the number of colour differences
    long nSteps = std::max( nStepCount, 2L );
    long nCalcSteps  = std::abs( nRedSteps );
    long nTempSteps = std::abs( nGreenSteps );
    if ( nTempSteps > nCalcSteps )
        nCalcSteps = nTempSteps;
    nTempSteps = std::abs( nBlueSteps );
    if ( nTempSteps > nCalcSteps )
        nCalcSteps = nTempSteps;
    if ( nCalcSteps < nSteps )
        nSteps = nCalcSteps;
    if ( !nSteps )
        nSteps = 1;

    // determine output limits and stepsizes for all directions
    Polygon aPoly;
    double  fScanLeft = aRect.Left();
    double  fScanTop = aRect.Top();
    double  fScanRight = aRect.Right();
    double  fScanBottom = aRect.Bottom();
    double fScanIncX = (double) aRect.GetWidth() / (double) nSteps * 0.5;
    double fScanIncY = (double) aRect.GetHeight() / (double) nSteps * 0.5;

    // all gradients are rendered as nested rectangles which shrink
    // equally in each dimension - except for 'square' gradients
    // which shrink to a central vertex but are not per-se square.
    if( rGradient.GetStyle() != GradientStyle_SQUARE )
    {
        fScanIncY = std::min( fScanIncY, fScanIncX );
        fScanIncX = fScanIncY;
    }
    sal_uInt8   nRed = (sal_uInt8) nStartRed, nGreen = (sal_uInt8) nStartGreen, nBlue = (sal_uInt8) nStartBlue;
    bool    bPaintLastPolygon( false ); // #107349# Paint last polygon only if loop has generated any output

    mpGraphics->SetFillColor( MAKE_SALCOLOR( nRed, nGreen, nBlue ) );

    if( pPolyPoly )
    {
        pPolyPoly->Insert( aPoly = rRect );
        pPolyPoly->Insert( aPoly );

#if defined USE_JAVA && defined MACOSX
        // Fix bug when drawing radial gradients to the printer or exporting to
        // PDF found in the attachment in the following NeoOffice forum post by
        // drawing the starting color to the intersection of the gradient and
        // clip regions:
        // http://trinity.neooffice.org/modules.php?name=Forums&file=viewtopic&p=63684#63684
        if ( meRasterOp == ROP_OVERPAINT )
            ImplDrawPolygon( aPoly, pClipPolyPoly );
#endif	// USE_JAVA && MACOSX
    }
    else
    {
        // extend rect, to avoid missing bounding line
        Rectangle aExtRect( rRect );

        aExtRect.Left() -= 1;
        aExtRect.Top() -= 1;
        aExtRect.Right() += 1;
        aExtRect.Bottom() += 1;

        ImplDrawPolygon( aPoly = aExtRect, pClipPolyPoly );
    }

    // loop to output Polygone/PolyPolygone sequentially
    for( long i = 1; i < nSteps; i++ )
    {
        // calculate new Polygon
        aRect.Left() = (long)( fScanLeft += fScanIncX );
        aRect.Top() = (long)( fScanTop += fScanIncY );
        aRect.Right() = (long)( fScanRight -= fScanIncX );
        aRect.Bottom() = (long)( fScanBottom -= fScanIncY );

        if( ( aRect.GetWidth() < 2 ) || ( aRect.GetHeight() < 2 ) )
            break;

        if( rGradient.GetStyle() == GradientStyle_RADIAL || rGradient.GetStyle() == GradientStyle_ELLIPTICAL )
            aPoly = Polygon( aRect.Center(), aRect.GetWidth() >> 1, aRect.GetHeight() >> 1 );
        else
            aPoly = Polygon( aRect );

        aPoly.Rotate( aCenter, nAngle );

        // adapt colour accordingly
        const long nStepIndex = ( ( pPolyPoly ) ? i : ( i + 1 ) );
        nRed = GetGradientColorValue( nStartRed + ( ( nRedSteps * nStepIndex ) / nSteps ) );
        nGreen = GetGradientColorValue( nStartGreen + ( ( nGreenSteps * nStepIndex ) / nSteps ) );
        nBlue = GetGradientColorValue( nStartBlue + ( ( nBlueSteps * nStepIndex ) / nSteps ) );

        // either slow tools::PolyPolygon output or fast Polygon-Paiting
        if( pPolyPoly )
        {
            bPaintLastPolygon = true; // #107349# Paint last polygon only if loop has generated any output

            pPolyPoly->Replace( pPolyPoly->GetObject( 1 ), 0 );
            pPolyPoly->Replace( aPoly, 1 );

#if defined USE_JAVA && defined MACOSX
            // Fix printing bug reported in the following NeoOffice forum post
            // by drawing entire polygon so that there are no gaps between
            // bands in elliptical or radial gradients:
            // http://trinity.neooffice.org/modules.php?name=Forums&file=viewtopic&p=63688#63688
            if ( meRasterOp == ROP_OVERPAINT )
            {
                ImplDrawPolygon( aPoly, pClipPolyPoly );
            }
            else
            {
#endif	// USE_JAVA && MACOSX
            ImplDrawPolyPolygon( *pPolyPoly, pClipPolyPoly );
#if defined USE_JAVA && defined MACOSX
            }
#endif	// USE_JAVA && MACOSX

            // #107349# Set fill color _after_ geometry painting:
            // pPolyPoly's geometry is the band from last iteration's
            // aPoly to current iteration's aPoly. The window outdev
            // path (see else below), on the other hand, paints the
            // full aPoly. Thus, here, we're painting the band before
            // the one painted in the window outdev path below. To get
            // matching colors, have to delay color setting here.
            mpGraphics->SetFillColor( MAKE_SALCOLOR( nRed, nGreen, nBlue ) );
        }
        else
        {
            // #107349# Set fill color _before_ geometry painting
            mpGraphics->SetFillColor( MAKE_SALCOLOR( nRed, nGreen, nBlue ) );

            ImplDrawPolygon( aPoly, pClipPolyPoly );
        }
    }

    // we should draw last inner Polygon if we output PolyPolygon
    if( pPolyPoly )
    {
        const Polygon& rPoly = pPolyPoly->GetObject( 1 );

        if( !rPoly.GetBoundRect().IsEmpty() )
        {
            // #107349# Paint last polygon with end color only if loop
            // has generated output. Otherwise, the current
            // (i.e. start) color is taken, to generate _any_ output.
            if( bPaintLastPolygon )
            {
                nRed = GetGradientColorValue( nEndRed );
                nGreen = GetGradientColorValue( nEndGreen );
                nBlue = GetGradientColorValue( nEndBlue );
            }

            mpGraphics->SetFillColor( MAKE_SALCOLOR( nRed, nGreen, nBlue ) );
            ImplDrawPolygon( rPoly, pClipPolyPoly );
        }
    }
}

void OutputDevice::DrawLinearGradientToMetafile( const Rectangle& rRect,
                                                 const Gradient& rGradient )
{
    // get BoundRect of rotated rectangle
    Rectangle aRect;
    Point     aCenter;
    sal_uInt16    nAngle = rGradient.GetAngle() % 3600;

    rGradient.GetBoundRect( rRect, aRect, aCenter );

#if defined USE_JAVA && defined MACOSX
    Rectangle aFullRect = aRect;
#endif	// USE_JAVA && MACOSX
    bool bLinear = (rGradient.GetStyle() == GradientStyle_LINEAR);
    double fBorder = rGradient.GetBorder() * aRect.GetHeight() / 100.0;
    if ( !bLinear )
    {
        fBorder /= 2.0;
    }
    Rectangle aMirrorRect = aRect; // used in style axial
    aMirrorRect.Top() = ( aRect.Top() + aRect.Bottom() ) / 2;
    if ( !bLinear )
    {
        aRect.Bottom() = aMirrorRect.Top();
    }

    // colour-intensities of start- and finish; change if needed
    long    nFactor;
    Color   aStartCol   = rGradient.GetStartColor();
    Color   aEndCol     = rGradient.GetEndColor();
    long    nStartRed   = aStartCol.GetRed();
    long    nStartGreen = aStartCol.GetGreen();
    long    nStartBlue  = aStartCol.GetBlue();
    long    nEndRed     = aEndCol.GetRed();
    long    nEndGreen   = aEndCol.GetGreen();
    long    nEndBlue    = aEndCol.GetBlue();
            nFactor     = rGradient.GetStartIntensity();
            nStartRed   = (nStartRed   * nFactor) / 100;
            nStartGreen = (nStartGreen * nFactor) / 100;
            nStartBlue  = (nStartBlue  * nFactor) / 100;
            nFactor     = rGradient.GetEndIntensity();
            nEndRed     = (nEndRed   * nFactor) / 100;
            nEndGreen   = (nEndGreen * nFactor) / 100;
            nEndBlue    = (nEndBlue  * nFactor) / 100;

    // gradient style axial has exchanged start and end colors
    if ( !bLinear)
    {
        long nTempColor = nStartRed;
        nStartRed = nEndRed;
        nEndRed = nTempColor;
        nTempColor = nStartGreen;
        nStartGreen = nEndGreen;
        nEndGreen = nTempColor;
        nTempColor = nStartBlue;
        nStartBlue = nEndBlue;
        nEndBlue = nTempColor;
    }

    sal_uInt8   nRed;
    sal_uInt8   nGreen;
    sal_uInt8   nBlue;

    // Create border
    Rectangle aBorderRect = aRect;
    Polygon     aPoly( 4 );
    if (fBorder > 0.0)
    {
        nRed        = (sal_uInt8)nStartRed;
        nGreen      = (sal_uInt8)nStartGreen;
        nBlue       = (sal_uInt8)nStartBlue;

        mpMetaFile->AddAction( new MetaFillColorAction( Color( nRed, nGreen, nBlue ), true ) );

        aBorderRect.Bottom() = (long)( aBorderRect.Top() + fBorder );
        aRect.Top() = aBorderRect.Bottom();
        aPoly[0] = aBorderRect.TopLeft();
        aPoly[1] = aBorderRect.TopRight();
#if defined USE_JAVA && defined MACOSX
        // Fix printing bug reported in the following NeoOffice forum post by
        // underlapping all successive stripes with the current color:
        // http://trinity.neooffice.org/modules.php?name=Forums&file=viewtopic&p=63688#63688
        if ( meRasterOp == ROP_OVERPAINT )
        {
            aPoly[2] = aFullRect.BottomRight();
            aPoly[3] = aFullRect.BottomLeft();
        }
        else
        {
#endif	// USE_JAVA && MACOSX
        aPoly[2] = aBorderRect.BottomRight();
        aPoly[3] = aBorderRect.BottomLeft();
#if defined USE_JAVA && defined MACOSX
        }
#endif	// USE_JAVA && MACOSX
        aPoly.Rotate( aCenter, nAngle );

        mpMetaFile->AddAction( new MetaPolygonAction( aPoly ) );

        if ( !bLinear)
        {
            aBorderRect = aMirrorRect;
            aBorderRect.Top() = (long) ( aBorderRect.Bottom() - fBorder );
            aMirrorRect.Bottom() = aBorderRect.Top();
            aPoly[0] = aBorderRect.TopLeft();
            aPoly[1] = aBorderRect.TopRight();
            aPoly[2] = aBorderRect.BottomRight();
            aPoly[3] = aBorderRect.BottomLeft();
            aPoly.Rotate( aCenter, nAngle );

            mpMetaFile->AddAction( new MetaPolygonAction( aPoly ) );
        }
    }

    bool    bMtf = true;
    long    nStepCount  = GetGradientSteps( rGradient, aRect, bMtf );

    // minimal three steps and maximal as max color steps
    long   nAbsRedSteps   = std::abs( nEndRed   - nStartRed );
    long   nAbsGreenSteps = std::abs( nEndGreen - nStartGreen );
    long   nAbsBlueSteps  = std::abs( nEndBlue  - nStartBlue );
    long   nMaxColorSteps = std::max( nAbsRedSteps , nAbsGreenSteps );
    nMaxColorSteps = std::max( nMaxColorSteps, nAbsBlueSteps );
    long nSteps = std::min( nStepCount, nMaxColorSteps );
    if ( nSteps < 3)
    {
        nSteps = 3;
    }

    double fScanInc = ((double)aRect.GetHeight()) / (double) nSteps;
    double fGradientLine = (double)aRect.Top();
    double fMirrorGradientLine = (double) aMirrorRect.Bottom();

    double fAlpha = 0.0;
    const double fStepsMinus1 = ((double)nSteps) - 1.0;
    double fTempColor;
    if ( !bLinear)
    {
        nSteps -= 1; // draw middle polygons as one polygon after loop to avoid gap
    }
    for ( long i = 0; i < nSteps; i++ )
    {
        // linear interpolation of color
        fAlpha = ((double)i) / fStepsMinus1;
        fTempColor = ((double)nStartRed) * (1.0-fAlpha) + ((double)nEndRed) * fAlpha;
        nRed = GetGradientColorValue((long)fTempColor);
        fTempColor = ((double)nStartGreen) * (1.0-fAlpha) + ((double)nEndGreen) * fAlpha;
        nGreen = GetGradientColorValue((long)fTempColor);
        fTempColor = ((double)nStartBlue) * (1.0-fAlpha) + ((double)nEndBlue) * fAlpha;
        nBlue = GetGradientColorValue((long)fTempColor);

        mpMetaFile->AddAction( new MetaFillColorAction( Color( nRed, nGreen, nBlue ), true ) );

        // Polygon for this color step
        aRect.Top() = (long)( fGradientLine + ((double) i) * fScanInc );
        aRect.Bottom() = (long)( fGradientLine + ( ((double) i) + 1.0 ) * fScanInc );
        aPoly[0] = aRect.TopLeft();
        aPoly[1] = aRect.TopRight();
#if defined USE_JAVA && defined MACOSX
        // Fix printing bug reported in the following NeoOffice forum post by
        // underlapping all successive stripes with the current color:
        // http://trinity.neooffice.org/modules.php?name=Forums&file=viewtopic&p=63688#63688
        if ( meRasterOp == ROP_OVERPAINT )
        {
            if ( bLinear )
            {
                aPoly[2] = aFullRect.BottomRight();
                aPoly[3] = aFullRect.BottomLeft();
            }
            else
            {
                aMirrorRect.Bottom() = (long)( fMirrorGradientLine - ((double) i) * fScanInc );
                aMirrorRect.Top() = (long)( fMirrorGradientLine - (((double) i) + 1.0)* fScanInc );
                aPoly[2] = aMirrorRect.BottomRight();
                aPoly[3] = aMirrorRect.BottomLeft();
            }
            aPoly.Rotate( aCenter, nAngle );

            mpMetaFile->AddAction( new MetaPolygonAction( aPoly ) );
        }
        else
        {
#endif	// USE_JAVA && MACOSX
        aPoly[2] = aRect.BottomRight();
        aPoly[3] = aRect.BottomLeft();
#if defined USE_JAVA && defined MACOSX
        }
#endif	// USE_JAVA && MACOSX
        aPoly.Rotate( aCenter, nAngle );

        mpMetaFile->AddAction( new MetaPolygonAction( aPoly ) );

        if ( !bLinear )
        {
            aMirrorRect.Bottom() = (long)( fMirrorGradientLine - ((double) i) * fScanInc );
            aMirrorRect.Top() = (long)( fMirrorGradientLine - (((double) i) + 1.0)* fScanInc );
            aPoly[0] = aMirrorRect.TopLeft();
            aPoly[1] = aMirrorRect.TopRight();
            aPoly[2] = aMirrorRect.BottomRight();
            aPoly[3] = aMirrorRect.BottomLeft();
            aPoly.Rotate( aCenter, nAngle );

            mpMetaFile->AddAction( new MetaPolygonAction( aPoly ) );
        }
    }
    if ( !bLinear)
    {
        // draw middle polygon with end color
        nRed = GetGradientColorValue(nEndRed);
        nGreen = GetGradientColorValue(nEndGreen);
        nBlue = GetGradientColorValue(nEndBlue);

        mpMetaFile->AddAction( new MetaFillColorAction( Color( nRed, nGreen, nBlue ), true ) );

        aRect.Top() = (long)( fGradientLine + ((double)nSteps) * fScanInc );
        aRect.Bottom() = (long)( fMirrorGradientLine - ((double) nSteps) * fScanInc );
        aPoly[0] = aRect.TopLeft();
        aPoly[1] = aRect.TopRight();
        aPoly[2] = aRect.BottomRight();
        aPoly[3] = aRect.BottomLeft();
        aPoly.Rotate( aCenter, nAngle );

        mpMetaFile->AddAction( new MetaPolygonAction( aPoly ) );
    }
}

void OutputDevice::DrawComplexGradientToMetafile( const Rectangle& rRect,
                                                  const Gradient& rGradient )
{
    // Determine if we output via Polygon or PolyPolygon
    // For all rasteroperations other then Overpaint always use PolyPolygon,
    // as we will get wrong results if we output multiple times on top of each other.
    // Also for printers always use PolyPolygon, as not all printers
    // can print polygons on top of each other.

    boost::scoped_ptr<tools::PolyPolygon> pPolyPoly;
    Rectangle       aRect;
    Point           aCenter;
    Color           aStartCol( rGradient.GetStartColor() );
    Color           aEndCol( rGradient.GetEndColor() );
    long            nStartRed = ( (long) aStartCol.GetRed() * rGradient.GetStartIntensity() ) / 100;
    long            nStartGreen = ( (long) aStartCol.GetGreen() * rGradient.GetStartIntensity() ) / 100;
    long            nStartBlue = ( (long) aStartCol.GetBlue() * rGradient.GetStartIntensity() ) / 100;
    long            nEndRed = ( (long) aEndCol.GetRed() * rGradient.GetEndIntensity() ) / 100;
    long            nEndGreen = ( (long) aEndCol.GetGreen() * rGradient.GetEndIntensity() ) / 100;
    long            nEndBlue = ( (long) aEndCol.GetBlue() * rGradient.GetEndIntensity() ) / 100;
    long            nRedSteps = nEndRed - nStartRed;
    long            nGreenSteps = nEndGreen - nStartGreen;
    long            nBlueSteps = nEndBlue   - nStartBlue;
    sal_uInt16      nAngle = rGradient.GetAngle() % 3600;

    rGradient.GetBoundRect( rRect, aRect, aCenter );

    pPolyPoly.reset(new tools::PolyPolygon( 2 ));

    // last parameter - true if complex gradient, false if linear
    long nStepCount = GetGradientSteps( rGradient, rRect, true, true );

    // at least three steps and at most the number of colour differences
    long nSteps = std::max( nStepCount, 2L );
    long nCalcSteps  = std::abs( nRedSteps );
    long nTempSteps = std::abs( nGreenSteps );
    if ( nTempSteps > nCalcSteps )
        nCalcSteps = nTempSteps;
    nTempSteps = std::abs( nBlueSteps );
    if ( nTempSteps > nCalcSteps )
        nCalcSteps = nTempSteps;
    if ( nCalcSteps < nSteps )
        nSteps = nCalcSteps;
    if ( !nSteps )
        nSteps = 1;

    // determine output limits and stepsizes for all directions
    Polygon aPoly;
    double  fScanLeft = aRect.Left();
    double  fScanTop = aRect.Top();
    double  fScanRight = aRect.Right();
    double  fScanBottom = aRect.Bottom();
    double fScanIncX = (double) aRect.GetWidth() / (double) nSteps * 0.5;
    double fScanIncY = (double) aRect.GetHeight() / (double) nSteps * 0.5;

    // all gradients are rendered as nested rectangles which shrink
    // equally in each dimension - except for 'square' gradients
    // which shrink to a central vertex but are not per-se square.
    if( rGradient.GetStyle() != GradientStyle_SQUARE )
    {
        fScanIncY = std::min( fScanIncY, fScanIncX );
        fScanIncX = fScanIncY;
    }
    sal_uInt8   nRed = (sal_uInt8) nStartRed, nGreen = (sal_uInt8) nStartGreen, nBlue = (sal_uInt8) nStartBlue;
    bool    bPaintLastPolygon( false ); // #107349# Paint last polygon only if loop has generated any output

    mpMetaFile->AddAction( new MetaFillColorAction( Color( nRed, nGreen, nBlue ), true ) );

    pPolyPoly->Insert( aPoly = rRect );
    pPolyPoly->Insert( aPoly );

#if defined USE_JAVA && defined MACOSX
    // Fix bug when drawing radial gradients to the printer or exporting to
    // PDF found in the attachment in the following NeoOffice forum post by
    // drawing the starting color to the intersection of the gradient and
    // clip regions:
    // http://trinity.neooffice.org/modules.php?name=Forums&file=viewtopic&p=63684#63684
    if ( meRasterOp == ROP_OVERPAINT )
        mpMetaFile->AddAction( new MetaPolygonAction( aPoly ) );
#endif	// USE_JAVA && MACOSX

    // loop to output Polygone/PolyPolygone sequentially
    for( long i = 1; i < nSteps; i++ )
    {
        // calculate new Polygon
        aRect.Left() = (long)( fScanLeft += fScanIncX );
        aRect.Top() = (long)( fScanTop += fScanIncY );
        aRect.Right() = (long)( fScanRight -= fScanIncX );
        aRect.Bottom() = (long)( fScanBottom -= fScanIncY );

        if( ( aRect.GetWidth() < 2 ) || ( aRect.GetHeight() < 2 ) )
            break;

        if( rGradient.GetStyle() == GradientStyle_RADIAL || rGradient.GetStyle() == GradientStyle_ELLIPTICAL )
            aPoly = Polygon( aRect.Center(), aRect.GetWidth() >> 1, aRect.GetHeight() >> 1 );
        else
            aPoly = Polygon( aRect );

        aPoly.Rotate( aCenter, nAngle );

        // adapt colour accordingly
        const long nStepIndex = ( ( pPolyPoly ) ? i : ( i + 1 ) );
        nRed = GetGradientColorValue( nStartRed + ( ( nRedSteps * nStepIndex ) / nSteps ) );
        nGreen = GetGradientColorValue( nStartGreen + ( ( nGreenSteps * nStepIndex ) / nSteps ) );
        nBlue = GetGradientColorValue( nStartBlue + ( ( nBlueSteps * nStepIndex ) / nSteps ) );

        bPaintLastPolygon = true; // #107349# Paint last polygon only if loop has generated any output

        pPolyPoly->Replace( pPolyPoly->GetObject( 1 ), 0 );
        pPolyPoly->Replace( aPoly, 1 );

#if defined USE_JAVA && defined MACOSX
        // Fix printing bug reported in the following NeoOffice forum post
        // by drawing entire polygon so that there are no gaps between
        // bands in elliptical or radial gradients:
        // http://trinity.neooffice.org/modules.php?name=Forums&file=viewtopic&p=63688#63688
        if ( meRasterOp == ROP_OVERPAINT )
        {
            mpMetaFile->AddAction( new MetaPolygonAction( aPoly ) );
        }
        else
        {
#endif	// USE_JAVA && MACOSX
        mpMetaFile->AddAction( new MetaPolyPolygonAction( *pPolyPoly ) );
#if defined USE_JAVA && defined MACOSX
        }
#endif	// USE_JAVA && MACOSX

        // #107349# Set fill color _after_ geometry painting:
        // pPolyPoly's geometry is the band from last iteration's
        // aPoly to current iteration's aPoly. The window outdev
        // path (see else below), on the other hand, paints the
        // full aPoly. Thus, here, we're painting the band before
        // the one painted in the window outdev path below. To get
        // matching colors, have to delay color setting here.
        mpMetaFile->AddAction( new MetaFillColorAction( Color( nRed, nGreen, nBlue ), true ) );
    }

    const Polygon& rPoly = pPolyPoly->GetObject( 1 );

    if( !rPoly.GetBoundRect().IsEmpty() )
    {
        // #107349# Paint last polygon with end color only if loop
        // has generated output. Otherwise, the current
        // (i.e. start) color is taken, to generate _any_ output.
        if( bPaintLastPolygon )
        {
            nRed = GetGradientColorValue( nEndRed );
            nGreen = GetGradientColorValue( nEndGreen );
            nBlue = GetGradientColorValue( nEndBlue );
        }

        mpMetaFile->AddAction( new MetaFillColorAction( Color( nRed, nGreen, nBlue ), true ) );
        mpMetaFile->AddAction( new MetaPolygonAction( rPoly ) );
    }
}

long OutputDevice::GetGradientStepCount( long nMinRect )
{
    long nInc = (nMinRect < 50) ? 2 : 4;

    return nInc;
}

long OutputDevice::GetGradientSteps( const Gradient& rGradient, const Rectangle& rRect, bool bMtf, bool bComplex )
{
    // calculate step count
    long nStepCount  = rGradient.GetSteps();
    long nMinRect;

    // generate nStepCount, if not passed
    if (bComplex)
        nMinRect = std::min( rRect.GetWidth(), rRect.GetHeight() );
    else
        nMinRect = rRect.GetHeight();

    if ( !nStepCount )
    {
        long nInc;

        nInc = GetGradientStepCount (nMinRect);
        if ( !nInc || bMtf )
            nInc = 1;
        nStepCount = nMinRect / nInc;
    }

    return nStepCount;
}

Color OutputDevice::GetSingleColorGradientFill()
{
    Color aColor;

    // we should never call on this function if any of these aren't set!
    assert( mnDrawMode & ( DRAWMODE_BLACKGRADIENT | DRAWMODE_WHITEGRADIENT | DRAWMODE_SETTINGSGRADIENT) );

    if ( mnDrawMode & DRAWMODE_BLACKGRADIENT )
        aColor = Color( COL_BLACK );
    else if ( mnDrawMode & DRAWMODE_WHITEGRADIENT )
        aColor = Color( COL_WHITE );
    else if ( mnDrawMode & DRAWMODE_SETTINGSGRADIENT )
        aColor = GetSettings().GetStyleSettings().GetWindowColor();

    if ( mnDrawMode & DRAWMODE_GHOSTEDGRADIENT )
    {
        aColor = Color( ( aColor.GetRed() >> 1 ) | 0x80,
                        ( aColor.GetGreen() >> 1 ) | 0x80,
                        ( aColor.GetBlue() >> 1 ) | 0x80 );
    }

    return aColor;
}

void OutputDevice::SetGrayscaleColors( Gradient &rGradient )
{
    // this should only be called with the drawing mode is for grayscale or ghosted gradients
    assert ( mnDrawMode & ( DRAWMODE_GRAYGRADIENT | DRAWMODE_GHOSTEDGRADIENT ) );

    Color aStartCol( rGradient.GetStartColor() );
    Color aEndCol( rGradient.GetEndColor() );

    if ( mnDrawMode & DRAWMODE_GRAYGRADIENT )
    {
        sal_uInt8 cStartLum = aStartCol.GetLuminance(), cEndLum = aEndCol.GetLuminance();
        aStartCol = Color( cStartLum, cStartLum, cStartLum );
        aEndCol = Color( cEndLum, cEndLum, cEndLum );
    }

    if ( mnDrawMode & DRAWMODE_GHOSTEDGRADIENT )
    {
        aStartCol = Color( ( aStartCol.GetRed() >> 1 ) | 0x80,
                           ( aStartCol.GetGreen() >> 1 ) | 0x80,
                           ( aStartCol.GetBlue() >> 1 ) | 0x80 );

        aEndCol = Color( ( aEndCol.GetRed() >> 1 ) | 0x80,
                         ( aEndCol.GetGreen() >> 1 ) | 0x80,
                         ( aEndCol.GetBlue() >> 1 ) | 0x80 );
    }

    rGradient.SetStartColor( aStartCol );
    rGradient.SetEndColor( aEndCol );
}

void OutputDevice::AddGradientActions( const Rectangle& rRect, const Gradient& rGradient,
                                       GDIMetaFile& rMtf )
{

    Rectangle aRect( rRect );

    aRect.Justify();

    // do nothing if the rectangle is empty
    if ( !aRect.IsEmpty() )
    {
        Gradient        aGradient( rGradient );
        GDIMetaFile*    pOldMtf = mpMetaFile;

        mpMetaFile = &rMtf;
        mpMetaFile->AddAction( new MetaPushAction( PushFlags::ALL ) );
        mpMetaFile->AddAction( new MetaISectRectClipRegionAction( aRect ) );
        mpMetaFile->AddAction( new MetaLineColorAction( Color(), false ) );

        // because we draw with no border line, we have to expand gradient
        // rect to avoid missing lines on the right and bottom edge
        aRect.Left()--;
        aRect.Top()--;
        aRect.Right()++;
        aRect.Bottom()++;

        // calculate step count if necessary
        if ( !aGradient.GetSteps() )
            aGradient.SetSteps( GRADIENT_DEFAULT_STEPCOUNT );

        if( aGradient.GetStyle() == GradientStyle_LINEAR || aGradient.GetStyle() == GradientStyle_AXIAL )
            DrawLinearGradientToMetafile( aRect, aGradient );
        else
            DrawComplexGradientToMetafile( aRect, aGradient );

        mpMetaFile->AddAction( new MetaPopAction() );
        mpMetaFile = pOldMtf;
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
