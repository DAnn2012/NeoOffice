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
 *   Modified October 2016 by Patrick Luby. NeoOffice is only distributed
 *   under the GNU General Public License, Version 3 as allowed by Section 3.3
 *   of the Mozilla Public License, v. 2.0.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "framegrabber.hxx"
#include "player.hxx"

#include <tools/stream.hxx>
#include <vcl/graph.hxx>
#include <vcl/cvtgrf.hxx>
#include <unotools/localfilehelper.hxx>

#ifndef USE_JAVA
#define AVMEDIA_QUICKTIME_FRAMEGRABBER_IMPLEMENTATIONNAME "com.sun.star.comp.avmedia.FrameGrabber_Quicktime"
#define AVMEDIA_QUICKTIME_FRAMEGRABBER_SERVICENAME "com.sun.star.media.FrameGrabber_Quicktime"
#endif	// USE_JAVA

using namespace ::com::sun::star;

namespace avmedia { namespace quicktime {


// - FrameGrabber -


FrameGrabber::FrameGrabber( const uno::Reference< lang::XMultiServiceFactory >& rxMgr ) :
    mxMgr( rxMgr )
{
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    mpMovie = [QTMovie movie];
    [mpMovie retain];
    mbInitialized = true;
    [pool release];
}



FrameGrabber::~FrameGrabber()
{
    if( mbInitialized )
    {
        if( mpMovie )
        {
            [mpMovie release];
            mpMovie = nil;
        }
    }
}



bool FrameGrabber::create( const ::rtl::OUString& rURL )
{
    bool bRet = false;
    maURL = rURL;
    NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
    NSString* aNSStr = [[[NSString alloc] initWithCharacters: rURL.getStr() length: rURL.getLength()]stringByAddingPercentEscapesUsingEncoding:NSUTF8StringEncoding] ;
    NSURL* aURL = [NSURL URLWithString:aNSStr ];

    // create the Movie

        mpMovie = [mpMovie initWithURL:aURL error:nil];
        if(mpMovie)
        {
            [mpMovie retain];
            bRet = true;
        }

    [pool release];

    return( bRet );
}



uno::Reference< graphic::XGraphic > SAL_CALL FrameGrabber::grabFrame( double fMediaTime )
    throw (uno::RuntimeException)
{
    uno::Reference< graphic::XGraphic > xRet;

    NSImage* pImage = [mpMovie frameImageAtTime: QTMakeTimeWithTimeInterval(fMediaTime)];
    NSData *pBitmap = [pImage TIFFRepresentation];
    long nSize = [pBitmap length];
    const void* pBitmapData = [pBitmap bytes];
    SvMemoryStream  aMemStm( (char *)pBitmapData, nSize, STREAM_READ | STREAM_WRITE );
    Graphic aGraphic;
    if ( GraphicConverter::Import( aMemStm, aGraphic, CVT_TIF ) == ERRCODE_NONE )
    {
        xRet = aGraphic.GetXGraphic();
    }

    return xRet;
}



::rtl::OUString SAL_CALL FrameGrabber::getImplementationName(  )
    throw (uno::RuntimeException)
{
    return ::rtl::OUString( AVMEDIA_QUICKTIME_FRAMEGRABBER_IMPLEMENTATIONNAME );
}



sal_Bool SAL_CALL FrameGrabber::supportsService( const ::rtl::OUString& ServiceName )
    throw (uno::RuntimeException)
{
    return ( ServiceName == AVMEDIA_QUICKTIME_FRAMEGRABBER_SERVICENAME );
}



uno::Sequence< ::rtl::OUString > SAL_CALL FrameGrabber::getSupportedServiceNames(  )
    throw (uno::RuntimeException)
{
    uno::Sequence< ::rtl::OUString > aRet(1);
    aRet[0] = ::rtl::OUString( AVMEDIA_QUICKTIME_FRAMEGRABBER_SERVICENAME );

    return aRet;
}

} // namespace quicktime
} // namespace avmedia

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
