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
 */

#ifndef INCLUDED_SC_SOURCE_UI_INC_NOTEMARK_HXX
#define INCLUDED_SC_SOURCE_UI_INC_NOTEMARK_HXX

#include <vcl/mapmod.hxx>
#include <vcl/timer.hxx>
#include "global.hxx"
#include "address.hxx"
#ifndef NO_LIBO_5_4_4_POSTIT_FIXES
#include "postit.hxx"
#endif	// !NO_LIBO_5_4_4_POSTIT_FIXES

class SdrModel;
class SdrObject;

class ScNoteMarker
{
private:
    vcl::Window*     pWindow;
    vcl::Window*     pRightWin;
    vcl::Window*     pBottomWin;
    vcl::Window*     pDiagWin;
    ScDocument* pDoc;
    ScAddress   aDocPos;
    OUString    aUserText;
    Rectangle   aVisRect;
    Timer       aTimer;
    MapMode     aMapMode;
    bool        bLeft;
    bool        bByKeyboard;

    Rectangle       aRect;
    SdrModel*       pModel;
#ifdef NO_LIBO_5_4_4_POSTIT_FIXES
    SdrObject*      pObject;
#else	// NO_LIBO_5_4_4_POSTIT_FIXES
    ScCaptionPtr    mxObject;
#endif	// NO_LIBO_5_4_4_POSTIT_FIXES
    bool            bVisible;
    Point           aGridOff;
    DECL_LINK( TimeHdl, void* );

public:
                ScNoteMarker( vcl::Window* pWin, vcl::Window* pRight, vcl::Window* pBottom, vcl::Window* pDiagonal,
                                ScDocument* pD, ScAddress aPos, const OUString& rUser,
                                const MapMode& rMap, bool bLeftEdge, bool bForce, bool bKeyboard );
                ~ScNoteMarker();

    void        Draw();
    void        InvalidateWin();

    ScAddress   GetDocPos() const       { return aDocPos; }
    bool        IsByKeyboard() const    { return bByKeyboard; }
    void        SetGridOff( const Point& rOff ) { aGridOff = rOff; }
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
