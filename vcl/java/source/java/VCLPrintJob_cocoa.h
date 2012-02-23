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

#ifndef __VCLPRINTJOB_COCOA_H__
#define __VCLPRINTJOB_COCOA_H__

#include <sal/types.h>

#ifdef __cplusplus
typedef void* id;
#endif

#ifdef __cplusplus
BEGIN_C
#endif
SAL_DLLPRIVATE BOOL NSPrintInfo_pageRange( id pNSPrintInfo, int *nFirst, int *nLast );
SAL_DLLPRIVATE float NSPrintInfo_scale( id pNSPrintInfo );
SAL_DLLPRIVATE id NSPrintInfo_showPrintDialog( id pNSPrintInfo, id pNSWindow, CFStringRef aJobName );
SAL_DLLPRIVATE BOOL NSPrintPanel_finished( id pDialog );
SAL_DLLPRIVATE id NSPrintPanel_printOperation( id pDialog );
SAL_DLLPRIVATE void NSPrintPanel_release( id pDialog );
#ifdef __cplusplus
END_C
#endif

#endif
