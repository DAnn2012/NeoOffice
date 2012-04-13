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
 *  Patrick Luby, June 2003
 *
 *  GNU General Public License Version 2.1
 *  =============================================
 *  Copyright 2003 Planamesa Inc.
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

#include <saldata.hxx>
#include <salframe.h>
#include <com/sun/star/vcl/VCLPrintJob.hxx>
#include <com/sun/star/vcl/VCLGraphics.hxx>
#include <com/sun/star/vcl/VCLPageFormat.hxx>
#include <java/lang/Class.hxx>
#include <tools/string.hxx>
#include <vcl/svapp.hxx>
#include <vcl/window.hxx>

#include "VCLPrintJob_cocoa.h"

#ifndef USE_NATIVE_PRINTING

using namespace vcl;

// ============================================================================

static jboolean JNICALL Java_sun_print_CUPSPrinter_canConnect( JNIEnv *pEnv, jobject object, jobject _par0, jint _par1 )
{
	return JNI_FALSE;
}

// ============================================================================

jclass com_sun_star_vcl_VCLPrintJob::theClass = NULL;

// ----------------------------------------------------------------------------

jclass com_sun_star_vcl_VCLPrintJob::getMyClass()
{
	if ( !theClass )
	{
		VCLThreadAttach t;
		if ( !t.pEnv ) return (jclass)NULL;

		// Override the CUPSPrinter.canConnect() method to explicity
		// disable hanging with networked CUPS printers
		jclass cCUPSPrinterClass = t.pEnv->FindClass( "sun/print/CUPSPrinter" );
		if ( cCUPSPrinterClass )
		{
			JNINativeMethod aMethod;
			aMethod.name = "canConnect";
			aMethod.signature = "(Ljava/lang/String;I)Z";
			aMethod.fnPtr = (void *)Java_sun_print_CUPSPrinter_canConnect;
			t.pEnv->RegisterNatives( cCUPSPrinterClass, &aMethod, 1 );
		}

		// Fix bug 3597 by handling Java versions that do not have a
		// CUPSPrinter class
		if ( t.pEnv->ExceptionCheck() )
			t.pEnv->ExceptionClear();

		jclass tempClass = t.pEnv->FindClass( "com/sun/star/vcl/VCLPrintJob" );
		OSL_ENSURE( tempClass, "Java : FindClass not found!" );
		theClass = (jclass)t.pEnv->NewGlobalRef( tempClass );
	}
	return theClass;
}

// ----------------------------------------------------------------------------

com_sun_star_vcl_VCLPrintJob::com_sun_star_vcl_VCLPrintJob() : java_lang_Object( (jobject)NULL )
{
	static jmethodID mID = NULL;
	VCLThreadAttach t;
	if ( !t.pEnv )
		return;
	if ( !mID )
	{
		char *cSignature = "(Lcom/sun/star/vcl/VCLEventQueue;)V";
		mID = t.pEnv->GetMethodID( getMyClass(), "<init>", cSignature );
	}
	OSL_ENSURE( mID, "Unknown method id!" );
	jvalue args[1];
	args[0].l = GetSalData()->mpEventQueue->getJavaObject();
	jobject tempObj = t.pEnv->NewObjectA( getMyClass(), mID, args );
	saveRef( tempObj );
}

// ----------------------------------------------------------------------------

void com_sun_star_vcl_VCLPrintJob::abortJob()
{
	static jmethodID mID = NULL;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "()V";
			mID = t.pEnv->GetMethodID( getMyClass(), "abortJob", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
		{
			ULONG nCount = Application::ReleaseSolarMutex();
			t.pEnv->CallNonvirtualVoidMethod( object, getMyClass(), mID );
			Application::AcquireSolarMutex( nCount );
		}
	}
}

// ----------------------------------------------------------------------------

void com_sun_star_vcl_VCLPrintJob::dispose()
{
	static jmethodID mID = NULL;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "()V";
			mID = t.pEnv->GetMethodID( getMyClass(), "dispose", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
			t.pEnv->CallNonvirtualVoidMethod( object, getMyClass(), mID );
	}
}

// ----------------------------------------------------------------------------

void com_sun_star_vcl_VCLPrintJob::endJob()
{
	static jmethodID mID = NULL;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "()V";
			mID = t.pEnv->GetMethodID( getMyClass(), "endJob", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
		{
			ULONG nCount = Application::ReleaseSolarMutex();
			t.pEnv->CallNonvirtualVoidMethod( object, getMyClass(), mID );
			Application::AcquireSolarMutex( nCount );
		}
	}
}

// ----------------------------------------------------------------------------

void com_sun_star_vcl_VCLPrintJob::endPage()
{
	static jmethodID mID = NULL;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "()V";
			mID = t.pEnv->GetMethodID( getMyClass(), "endPage", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
		{
			ULONG nCount = Application::ReleaseSolarMutex();
			t.pEnv->CallNonvirtualVoidMethod( object, getMyClass(), mID );
			Application::AcquireSolarMutex( nCount );
		}
	}
}

// ----------------------------------------------------------------------------

XubString com_sun_star_vcl_VCLPrintJob::getPageRange( com_sun_star_vcl_VCLPageFormat *_par0 )
{
	XubString out;
	int nFirst;
	int nLast;
	if ( NSPrintInfo_pageRange( _par0->getNativePrinterJob(), &nFirst, &nLast ) )
	{
		out = XubString::CreateFromInt32( nFirst );
		out += '-';
		out += XubString::CreateFromInt32( nLast );
	}
	return out;
}

// ----------------------------------------------------------------------------

sal_Bool com_sun_star_vcl_VCLPrintJob::isFinished()
{
	static jmethodID mID = NULL;
	sal_Bool out = sal_False;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "()Z";
			mID = t.pEnv->GetMethodID( getMyClass(), "isFinished", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
			out = (sal_Bool)t.pEnv->CallNonvirtualBooleanMethod( object, getMyClass(), mID );
	}
	return out;
}

// ----------------------------------------------------------------------------

sal_Bool com_sun_star_vcl_VCLPrintJob::startJob( com_sun_star_vcl_VCLPageFormat *_par0, ::rtl::OUString _par1, float _par2, sal_Bool _par3 ) 
{
	static jmethodID mID = NULL;
	sal_Bool out = sal_False;

	if ( _par3 )
	{
		VCLThreadAttach t;
		if ( t.pEnv )
		{
			if ( !mID )
			{
				char *cSignature = "(Lcom/sun/star/vcl/VCLPageFormat;Ljava/lang/String;F)Z";
				mID = t.pEnv->GetMethodID( getMyClass(), "startJob", cSignature );
			}
			OSL_ENSURE( mID, "Unknown method id!" );
			if ( mID )
			{
				jvalue args[3];
				args[0].l = _par0->getJavaObject();
				args[1].l = StringToJavaString( t.pEnv, _par1 );
				args[2].f = jfloat( _par2 );
				out = (sal_Bool)t.pEnv->CallNonvirtualBooleanMethodA( object, getMyClass(), mID, args );
			}
		}
	}
	else
	{
		SalData *pSalData = GetSalData();

		JavaSalFrame *pFocusFrame = NULL;

		// Get the active document window
		Window *pWindow = Application::GetActiveTopWindow();
		if ( pWindow )
			pFocusFrame = (JavaSalFrame *)pWindow->ImplGetFrame();

		if ( !pFocusFrame )
			pFocusFrame = pSalData->mpFocusFrame;

		// Fix bug 3294 by not attaching to utility windows
		while ( pFocusFrame && ( pFocusFrame->IsFloatingFrame() || pFocusFrame->IsUtilityWindow() || pFocusFrame->mbShowOnlyMenus ) )
			pFocusFrame = pFocusFrame->mpParent;

		// Fix bug 1106 If the focus frame is not set or is not visible, find
		// the first visible non-floating, non-utility frame
		if ( !pFocusFrame || !pFocusFrame->mbVisible || pFocusFrame->IsFloatingFrame() || pFocusFrame->IsUtilityWindow() || pFocusFrame->mbShowOnlyMenus )
		{
			pFocusFrame = NULL;
			for ( ::std::list< JavaSalFrame* >::const_iterator it = pSalData->maFrameList.begin(); it != pSalData->maFrameList.end(); ++it )
			{
				if ( (*it)->mbVisible && !(*it)->IsFloatingFrame() && !(*it)->IsUtilityWindow() && !(*it)->mbShowOnlyMenus )
				{
					pFocusFrame = *it;
					break;
				}
			}
		}

		// Ignore any AWT events while the print dialog is showing to
		// emulate a modal dialog
		pSalData->mpNativeModalSheetFrame = pFocusFrame;
		pSalData->mbInNativeModalSheet = true;
		void *pNSPrintInfo = _par0->getNativePrinterJob();
		ULONG nCount = pFocusFrame ? 0 : Application::ReleaseSolarMutex();
		CFStringRef aString = CFStringCreateWithCharactersNoCopy( NULL, _par1.getStr(), _par1.getLength(), kCFAllocatorNull );
		void *pDialog = NSPrintInfo_showPrintDialog( pNSPrintInfo, pFocusFrame ? pFocusFrame->GetNativeWindow() : NULL, aString );
		if ( aString )
			CFRelease( aString );
		Application::AcquireSolarMutex( nCount );

		while ( !NSPrintPanel_finished( pDialog ) )
			Application::Yield();
		pSalData->mbInNativeModalSheet = false;
		pSalData->mpNativeModalSheetFrame = NULL;
		out = NSPrintPanel_result( pDialog );
	}

	return out;
}

// ----------------------------------------------------------------------------

com_sun_star_vcl_VCLGraphics *com_sun_star_vcl_VCLPrintJob::startPage( Orientation _par0 )
{
	static jmethodID mID = NULL;
	com_sun_star_vcl_VCLGraphics *out = NULL;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "(I)Lcom/sun/star/vcl/VCLGraphics;";
			mID = t.pEnv->GetMethodID( getMyClass(), "startPage", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
		{
			jvalue args[1];
			args[0].i = jint( _par0 );
			jobject tempObj = t.pEnv->CallNonvirtualObjectMethodA( object, getMyClass(), mID, args );
			if ( tempObj )
			{
				ULONG nCount = Application::ReleaseSolarMutex();
				out = new com_sun_star_vcl_VCLGraphics( tempObj );
				Application::AcquireSolarMutex( nCount );
			}
		}
	}
	return out;
}

#endif	// !USE_NATIVE_PRINTING
