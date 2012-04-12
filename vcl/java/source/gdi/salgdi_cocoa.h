/*************************************************************************
 *
 *  $RCSfile$
 *
 *  $Revision$
 *
 *  last change: $Author$ $Date$
 *
 *  The Contents of this file are made available subject to the terms of
 *  either of the following licenses
 *
 *         - GNU General Public License Version 2.1
 *
 *  Patrick Luby, September 2005
 *
 *  GNU General Public License Version 2.1
 *  =============================================
 *  Copyright 2005 Planamesa Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU General Public
 *  License version 2.1, as published by the Free Software Foundation.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 *  MA  02111-1307  USA
 *
 ************************************************************************/

#ifndef __SALGDI_COCOA_H__
#define __SALGDI_COCOA_H__

#include <salgdi.h>

#if !defined USE_NATIVE_WINDOW || !defined USE_NATIVE_VIRTUAL_DEVICE || !defined USE_NATIVE_PRINTING

#ifdef __cplusplus
#include <premac.h>
#endif
#include <ApplicationServices/ApplicationServices.h>
#ifdef __cplusplus
#include <postmac.h>
#endif

SAL_DLLPRIVATE BOOL NSEPSImageRep_drawInBitmap( void *pEPSPtr, unsigned nEPSSize, int *pDestPtr, int nDestWidth, int nDestHeight );

#endif	// !USE_NATIVE_WINDOW || !USE_NATIVE_VIRTUAL_DEVICE || !USE_NATIVE_PRINTING

#endif
