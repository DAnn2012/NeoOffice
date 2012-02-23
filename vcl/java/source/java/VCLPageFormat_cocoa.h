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

#ifndef __VCLPAGEFORMAT_COCOA_H__
#define __VCLPAGEFORMAT_COCOA_H__

#ifdef __OBJC__
@interface VCLPrintOperation : NSPrintOperation
+ (NSPrintOperation *)printOperationWithView:(NSView *)pView;
+ (NSPrintOperation *)printOperationWithView:(NSView *)pView printInfo:(NSPrintInfo *)pPrintInfo;
+ (NSPrintOperation *)poseAsPrintOperationWithView:(NSView *)pView;
+ (NSPrintOperation *)poseAsPrintOperationWithView:(NSView *)pView printInfo:(NSPrintInfo *)pPrintInfo;
@end
#else	// __OBJC__
typedef void* id;
#endif	// __OBJC__

#ifdef __cplusplus
BEGIN_C
#endif
SAL_DLLPRIVATE sal_Bool NSPageLayout_finished( id pDialog );
SAL_DLLPRIVATE sal_Bool NSPageLayout_result( id pDialog, sal_Bool *bLandscape );
SAL_DLLPRIVATE id NSPrintInfo_create();
SAL_DLLPRIVATE void NSPrintInfo_getPrintInfoDimensions( id pNSPrintInfo, float *pWidth, float *pHeight, float *pImageableX, float *pImageableY, float *pImageableWidth, float *pImageableHeight );
SAL_DLLPRIVATE sal_Bool NSPrintInfo_setPaperSize( id pNSPrintInfo, long nWidth, long nHeight );
SAL_DLLPRIVATE void NSPrintInfo_setSharedPrintInfo( id pNSPrintInfo );
SAL_DLLPRIVATE id NSPrintInfo_showPageLayoutDialog( id pNSPrintInfo, id pNSWindow, sal_Bool bLandscape );
SAL_DLLPRIVATE CFStringRef VCLPrintInfo_getVCLPrintInfoDictionaryKey();
void VCLPrintInfo_installVCLPrintClasses();
#ifdef __cplusplus
END_C
#endif

#endif
