/**************************************************************
 * 
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 * 
 *************************************************************/



/**
 * hwpio.h
 * (C) 1999 Mizi Research, All rights are reserved
 *
 * $Id$
 */

#ifndef _HIODEV_H_
#define _HIODEV_H_

#ifdef __GNUG__
#pragma interface
#endif

#include <stdio.h>
#include "hwplib.h"
/**
 * @short Abstract IO class
 */
class DLLEXPORT HIODev
{
    protected:
        bool compressed;
        virtual void init();
    public:
        HIODev();
        virtual ~HIODev();

        virtual bool open() = 0;
        virtual void close() = 0;
        virtual void flush() = 0;
        virtual int  state() const = 0;
/* gzip routine wrapper */
        virtual bool setCompressed( bool ) = 0;

        virtual int read1b() = 0;
        virtual int read2b() = 0;
        virtual long read4b() = 0;
        virtual int readBlock( void *ptr, int size ) = 0;
        virtual int skipBlock( int size ) = 0;

        virtual int read1b( void *ptr, int nmemb );
        virtual int read2b( void *ptr, int nmemb );
        virtual int read4b( void *ptr, int nmemb );
};

struct gz_stream;

/* 파일 입출력 장치 */

/**
 * This controls the HStream given by constructor
 * @short Stream IO device
 */
class HStreamIODev : public HIODev
{
    private:
/* zlib으로 압축을 풀기 위한 자료 구조 */
        gz_stream *_gzfp;
        HStream& _stream;
    public:
        HStreamIODev(HStream& stream);
        virtual ~HStreamIODev();
/**
 * Check whether the stream is available
 */
        virtual bool open();
/**
 * Free stream object
 */
        virtual void close();
/**
 * If the stream is gzipped, flush the stream.
 */
        virtual void flush();
/**
 * Not implemented.
 */
        virtual int  state() const;
/**
 * Set whether the stream is compressed or not
 */
        virtual bool setCompressed( bool );
/**
 * Read one byte from stream
 */
        using HIODev::read1b;
        virtual int read1b();
/**
 * Read 2 bytes from stream
 */
        using HIODev::read2b;
        virtual int read2b();
/**
 * Read 4 bytes from stream
 */
        using HIODev::read4b;
        virtual long read4b();
/**
 * Read some bytes from stream to given pointer as amount of size
 */
        virtual int readBlock( void *ptr, int size );
/**
 * Move current pointer of stream as amount of size
 */
        virtual int skipBlock( int size );
    protected:
/**
 * Initialize this object
 */
        virtual void init();
};

/* 메모리 입출력 장치 */
/**
 * The HMemIODev class controls the Input/Output device.
 * @short Memory IO device
 */
class HMemIODev : public HIODev
{
    uchar *ptr;
    int pos, length;
    public:
        HMemIODev(char *s, int len);
        virtual ~HMemIODev();

        virtual bool open();
        virtual void close();
        virtual void flush();
        virtual int  state() const;
/* gzip routine wrapper */
        virtual bool setCompressed( bool );
        using HIODev::read1b;
        virtual int read1b();
        using HIODev::read2b;
        virtual int read2b();
        using HIODev::read4b;
        virtual long read4b();
        virtual int readBlock( void *ptr, int size );
        virtual int skipBlock( int size );
    protected:
        virtual void init();
};
#endif                                            /* _HIODEV_H_*/
