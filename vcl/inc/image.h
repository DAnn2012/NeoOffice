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

#ifndef INCLUDED_VCL_INC_IMAGE_H
#define INCLUDED_VCL_INC_IMAGE_H

#include <vcl/bitmapex.hxx>

#include <boost/unordered_map.hpp>

// - ImplImageBmp -

class ImplImageBmp
{
public:

                ImplImageBmp();
                ~ImplImageBmp();

    void        Create( const BitmapEx& rBmpEx, long nItemWidth, long nItemHeight,sal_uInt16 nInitSize );
    void        ColorTransform();
    void        Draw( sal_uInt16 nPos, OutputDevice* pDev, const Point& rPos, sal_uInt16 nStyle, const Size* pSize = NULL );

private:

    BitmapEx    maBmpEx;
    BitmapEx    maDisabledBmpEx;
    BitmapEx*   mpDisplayBmp;
    Size        maSize;
    sal_uInt8*      mpInfoAry;
    sal_uInt16      mnSize;

    void        ImplUpdateDisplayBmp( OutputDevice* pOutDev );
    void        ImplUpdateDisabledBmpEx( int nPos );

private:    // prevent assignment and copy construction
    ImplImageBmp( const ImplImageBmp& );
    void operator=( const ImplImageBmp& );
};

// - ImageTypes -

enum ImageType { IMAGETYPE_BITMAP, IMAGETYPE_IMAGE };

// - ImplImageList -

struct ImageAryData
{
    OUString maName;
    // Images identified by either name, or by id
    sal_uInt16          mnId;
    BitmapEx        maBitmapEx;

    ImageAryData( const OUString &aName,
                  sal_uInt16 nId, const BitmapEx &aBitmap );
    ImageAryData( const ImageAryData& rData );
    ~ImageAryData();

    bool IsLoadable() { return maBitmapEx.IsEmpty() && !maName.isEmpty(); }
    void Load(const OUString &rPrefix);

    ImageAryData&   operator=( const ImageAryData& rData );
};

struct ImplImageList
{
    typedef std::vector<ImageAryData *> ImageAryDataVec;
    typedef boost::unordered_map< OUString, ImageAryData *, OUStringHash >
        ImageAryDataNameHash;

    ImageAryDataVec             maImages;
    ImageAryDataNameHash        maNameHash;
    OUString               maPrefix;
    Size                        maImageSize;
    sal_uIntPtr                       mnRefCount;
#ifdef USE_JAVA
    OUString maIconTheme;
#endif	// USE_JAVA

    ImplImageList();
    ImplImageList( const ImplImageList &aSrc );
    ~ImplImageList();

    void AddImage( const OUString &aName,
                   sal_uInt16 nId, const BitmapEx &aBitmapEx );
    void RemoveImage( sal_uInt16 nPos );
#ifdef USE_JAVA
    void SetBitmapsToEmptyIfIconThemeChanged();
#endif	// USE_JAVA
};

// - ImplImageRefData -

struct ImplImageRefData
{
    ImplImageList*  mpImplData;
    sal_uInt16          mnIndex;

                    ImplImageRefData() {}    // Um Warning zu umgehen
                    ~ImplImageRefData();

    bool            IsEqual( const ImplImageRefData& rData );
};

// - ImpImageData -

struct ImplImageData
{
    ImplImageBmp*   mpImageBitmap;
    BitmapEx        maBmpEx;

                    ImplImageData( const BitmapEx& rBmpEx );
                    ~ImplImageData();

    bool            IsEqual( const ImplImageData& rData );
};

// - ImplImage -

struct ImplImage
{
    sal_uIntPtr         mnRefCount;
    // TODO: use inheritance to get rid of meType+mpData
    void*           mpData;
    ImageType       meType;

                    ImplImage();
                    ~ImplImage();

private:    // prevent assignment and copy construction
            ImplImage( const ImplImage&);
    void    operator=( const ImplImage&);
};

#endif // INCLUDED_VCL_INC_IMAGE_H

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
