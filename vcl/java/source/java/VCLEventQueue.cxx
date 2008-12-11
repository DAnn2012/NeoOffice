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

#define _SV_COM_SUN_STAR_VCL_VCLEVENTQUEUE_CXX

#ifndef _SV_COM_SUN_STAR_VCL_VCLEVENTQUEUE_HXX
#include <com/sun/star/vcl/VCLEventQueue.hxx>
#endif
#ifndef _SV_COM_SUN_STAR_VCL_VCLEVENT_HXX
#include <com/sun/star/vcl/VCLEvent.hxx>
#endif
#ifndef _SV_COM_SUN_STAR_VCL_VCLFRAME_HXX
#include <com/sun/star/vcl/VCLFrame.hxx>
#endif
#ifndef _SV_SALDATA_HXX
#include <saldata.hxx>
#endif
#ifndef _SV_SALFRAME_H
#include <salframe.h>
#endif
#ifndef _SV_SVAPP_HXX
#include <vcl/svapp.hxx>
#endif
#ifndef _VCL_UNOHELP_HXX
#include <vcl/unohelp.hxx>
#endif
#ifndef _SV_WINDOW_HXX
#include <vcl/window.hxx>
#endif
#ifndef _COMPHELPER_PROCESSFACTORY_HXX_
#include <comphelper/processfactory.hxx>
#endif
#ifndef _TOOLKIT_HELPER_VCLUNOHELPER_HXX_
#include <toolkit/helper/vclunohelper.hxx>
#endif
#ifndef _VOS_MUTEX_HXX_
#include <vos/mutex.hxx>
#endif
#ifndef _VOS_MODULE_HXX_
#include <vos/module.hxx>
#endif
#ifndef _COM_SUN_STAR_AWT_XWINDOW_HDL_
#include <com/sun/star/awt/XWindow.hpp>
#endif
#ifndef _COM_SUN_STAR_DATATRANSFER_XTRANSFERABLE_HDL_
#include <com/sun/star/datatransfer/XTransferable.hpp>
#endif
#ifndef _COM_SUN_STAR_DATATRANSFER_CLIPBOARD_XCLIPBOARD_HDL_
#include <com/sun/star/datatransfer/clipboard/XClipboard.hpp>
#endif
#ifndef _COM_SUN_STAR_FRAME_XDESKTOP_HDL_
#include <com/sun/star/frame/XDesktop.hpp>
#endif
#ifndef _COM_SUN_STAR_FRAME_XFRAME_HDL_
#include <com/sun/star/frame/XFrame.hpp>
#endif

#include <premac.h>
#include <Carbon/Carbon.h>
#include <postmac.h>

#include "VCLEventQueue_cocoa.h"

static ::vos::OModule aAWTFontModule;

using namespace com::sun::star::awt;
using namespace com::sun::star::datatransfer;
using namespace com::sun::star::datatransfer::clipboard;
using namespace com::sun::star::frame;
using namespace com::sun::star::uno;
using namespace vcl;
using namespace vos;

// ============================================================================

JNIEXPORT jboolean JNICALL Java_com_sun_star_vcl_VCLEventQueue_hasApplicationDelegate( JNIEnv *pEnv, jobject object )
{
	return ( NSApplication_hasDelegate() ? JNI_TRUE : JNI_FALSE );
}

// ----------------------------------------------------------------------------

JNIEXPORT jboolean JNICALL Java_com_sun_star_vcl_VCLEventQueue_isApplicationActive( JNIEnv *pEnv, jobject object )
{
	return ( NSApplication_isActive() ? JNI_TRUE : JNI_FALSE );
}

// ----------------------------------------------------------------------------

JNIEXPORT jboolean JNICALL Java_com_sun_star_vcl_VCLEventQueue_isApplicationMainThread( JNIEnv *pEnv, jobject object )
{
	return ( GetCurrentEventLoop() == GetMainEventLoop() ? JNI_TRUE : JNI_FALSE );
}

// ----------------------------------------------------------------------------

JNIEXPORT void JNICALL Java_com_sun_star_vcl_VCLEventQueue_runApplicationMainThreadTimers( JNIEnv *pEnv, jobject object )
{
	if ( GetCurrentEventLoop() == GetMainEventLoop() )
		ReceiveNextEvent( 0, NULL, 0, false, NULL );
}

// ============================================================================

void VCLEventQueue_getTextSelection( CFStringRef *pTextSelection, CFDataRef *pRTFSelection )
{
	if ( !pTextSelection && !pRTFSelection )
		return;

	if ( pTextSelection && *pTextSelection )
	{
		CFRelease( *pTextSelection );
		*pTextSelection = NULL;
	}

	if ( pRTFSelection && *pRTFSelection )
	{
		CFRelease( *pRTFSelection );
		*pRTFSelection = NULL;
	}

	if ( !Application::IsShutDown() )
	{
		IMutex& rSolarMutex = Application::GetSolarMutex();
		rSolarMutex.acquire();

		if ( !Application::IsShutDown() )
		{
			Reference< XDesktop > xDesktop( ::comphelper::getProcessServiceFactory()->createInstance( OUString( RTL_CONSTASCII_USTRINGPARAM( "com.sun.star.frame.Desktop" ) ) ), UNO_QUERY );
			if ( xDesktop.is() )
			{
				Reference< XFrame > xFrame = xDesktop->getCurrentFrame();
				if ( xFrame.is() )
				{
					Reference< XWindow > xWindow = xFrame->getComponentWindow();
					if ( xWindow.is() )
					{
						Window *pWindow = VCLUnoHelper::GetWindow( xWindow );
						if ( pWindow )
						{
							Reference< XClipboard > xClipboard = pWindow->GetPrimarySelection();
							if ( xClipboard.is() )
							{
								Reference< XTransferable > xTransferable = xClipboard->getContents();
								if ( xTransferable.is() )
								{
									DataFlavor aFlavor;

									// Handle string selection
									if ( pTextSelection )
									{
										Type aType( getCppuType( ( OUString* )0 ) );
										aFlavor.MimeType = OUString( RTL_CONSTASCII_USTRINGPARAM( "text/plain;charset=utf-16" ) );
										aFlavor.DataType = aType;
										if ( xTransferable->isDataFlavorSupported( aFlavor ) )
										{
											Any aValue = xTransferable->getTransferData( aFlavor );
											if ( aValue.getValueType().equals( aType ) )
											{
												OUString aText;
												aValue >>= aText;
												if ( aText.getLength() )
													*pTextSelection = CFStringCreateWithCharacters( NULL, aText.getStr(), aText.getLength() );
											}
										}
									}

									// Handle RTF selection
									if ( pRTFSelection )
									{
										Type aType( getCppuType( ( ::com::sun::star::uno::Sequence< sal_Int8 >* )0 ) );
										aFlavor.MimeType = OUString( RTL_CONSTASCII_USTRINGPARAM( "text/richtext" ) );
										aFlavor.DataType = aType;
										if ( xTransferable->isDataFlavorSupported( aFlavor ) )
										{
											Any aValue = xTransferable->getTransferData( aFlavor );
											if ( aValue.getValueType().equals( aType ) )
											{
												Sequence< sal_Int8 > aData;
												aValue >>= aData;
												if ( aData.getLength() )
												{
													*pRTFSelection = CFDataCreate( NULL, (const UInt8 *)aData.getArray(), aData.getLength() );
												}
											}
										}
									}
								}
							}
						}
					}
				}
			}
		}

		rSolarMutex.release();
	}
}

// ----------------------------------------------------------------------------

void VCLEventQueue_postCommandEvent( jobject aPeer, short nKey, short nModifiers )
{
	if ( aPeer )
		com_sun_star_vcl_VCLEventQueue::postCommandEvent( aPeer, nKey, nModifiers & KEY_SHIFT, nModifiers & KEY_MOD1, nModifiers & KEY_MOD2, nModifiers & KEY_MOD3 );
}

// ----------------------------------------------------------------------------

void VCLEventQueue_postMouseWheelEvent( jobject aPeer, long nX, long nY, long nRotationX, long nRotationY, BOOL bShiftDown, BOOL bMetaDown, BOOL bAltDown, BOOL bControlDown )
{
	if ( aPeer )
		com_sun_star_vcl_VCLEventQueue::postMouseWheelEvent( aPeer, nX, nY, nRotationX, nRotationY, bShiftDown, bMetaDown, bAltDown, bControlDown );
}

// ----------------------------------------------------------------------------

void VCLEventQueue_postWindowMoveSessionEvent( jobject aPeer, long nX, long nY, BOOL bStartSession )
{
	if ( aPeer )
		com_sun_star_vcl_VCLEventQueue::postWindowMoveSessionEvent( aPeer, nX, nY, bStartSession );
}

// ============================================================================

jclass com_sun_star_vcl_VCLEventQueue::theClass = NULL;

// ----------------------------------------------------------------------------

jclass com_sun_star_vcl_VCLEventQueue::getMyClass()
{
	if ( !theClass )
	{
		VCLThreadAttach t;
		if ( !t.pEnv ) return (jclass)NULL;

		// Determine if fixes for Java 1.4.x and early versions of Java 1.5
		// are needed
		BOOL bUseKeyEntryFix = FALSE;
		BOOL bUsePartialKeyEntryFix = FALSE;
		if ( IsRunningTiger() )
		{
			bUsePartialKeyEntryFix = TRUE;

			jclass systemClass = t.pEnv->FindClass( "java/lang/System" );
			if ( systemClass )
			{
				jmethodID mID = NULL;
				OUString aJavaHomePath;
				if ( !mID )
				{
					char *cSignature = "(Ljava/lang/String;)Ljava/lang/String;";
					mID = t.pEnv->GetStaticMethodID( systemClass, "getProperty", cSignature );
				}
				OSL_ENSURE( mID, "Unknown method id!" );
				if ( mID )
				{
					jvalue args[1];
					args[0].l = StringToJavaString( t.pEnv, OUString::createFromAscii( "java.specification.version" ) );
					jstring tempJavaSpecVersion = (jstring)t.pEnv->CallStaticObjectMethodA( systemClass, mID, args );
					if ( tempJavaSpecVersion )
					{
						OUString aJavaSpecVersion = JavaString2String( t.pEnv, tempJavaSpecVersion );
						if ( aJavaSpecVersion.getLength() )
						{
							if ( aJavaSpecVersion == OUString::createFromAscii( "1.4" ) )
							{
								bUseKeyEntryFix = TRUE;
							}
							else if ( aJavaSpecVersion == OUString::createFromAscii( "1.5" ) )
							{
								args[0].l = StringToJavaString( t.pEnv, OUString::createFromAscii( "java.version" ) );
								jstring tempJavaVersion = (jstring)t.pEnv->CallStaticObjectMethodA( systemClass, mID, args );
								if ( tempJavaVersion )
								{
									OUString aJavaVersion = JavaString2String( t.pEnv, tempJavaVersion );
									if ( aJavaVersion.compareTo( OUString::createFromAscii( "1.5.0_06" ) ) < 0 )
										bUseKeyEntryFix = TRUE;
								}
							}
						}
					}
				}
			}
		}

		// Fix bugs 1390 and 1619 by inserting our own Cocoa class in place of
		// the NSView class. We need to do this because the JVM does not
		// properly handle key events where a single key press generates more
		// than one Unicode character.
		VCLEventQueue_installVCLEventQueueClasses( bUseKeyEntryFix, bUsePartialKeyEntryFix );

		// Load our AWTFont replacement class
		OUString aVCLJavaLibName;
		if ( IsRunningTiger() )
			aVCLJavaLibName = OUString::createFromAscii( "libvcljava1.dylib" );
		else
			aVCLJavaLibName = OUString::createFromAscii( "libvcljava2.dylib" );
		aAWTFontModule.load( aVCLJavaLibName, SAL_LOADMODULE_GLOBAL | SAL_LOADMODULE_NOW );

		jclass tempClass = t.pEnv->FindClass( "com/sun/star/vcl/VCLEventQueue" );
		OSL_ENSURE( tempClass, "Java : FindClass not found!" );

		if ( tempClass )
		{
			// Register the native methods for our class
			JNINativeMethod pMethods[4]; 
			pMethods[0].name = "hasApplicationDelegate";
			pMethods[0].signature = "()Z";
			pMethods[0].fnPtr = (void *)Java_com_sun_star_vcl_VCLEventQueue_hasApplicationDelegate;
			pMethods[1].name = "isApplicationActive";
			pMethods[1].signature = "()Z";
			pMethods[1].fnPtr = (void *)Java_com_sun_star_vcl_VCLEventQueue_isApplicationActive;
			pMethods[2].name = "isApplicationMainThread";
			pMethods[2].signature = "()Z";
			pMethods[2].fnPtr = (void *)Java_com_sun_star_vcl_VCLEventQueue_isApplicationMainThread;
			pMethods[3].name = "runApplicationMainThreadTimers";
			pMethods[3].signature = "()V";
			pMethods[3].fnPtr = (void *)Java_com_sun_star_vcl_VCLEventQueue_runApplicationMainThreadTimers;
			t.pEnv->RegisterNatives( tempClass, pMethods, 4 );
		}

		theClass = (jclass)t.pEnv->NewGlobalRef( tempClass );
	}
	return theClass;
}

// ----------------------------------------------------------------------------

void com_sun_star_vcl_VCLEventQueue::postCommandEvent( jobject _par0, short _par1, sal_Bool _par2, sal_Bool _par3, sal_Bool _par4, sal_Bool _par5 )
{
	static jmethodID mID = NULL;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "(Ljava/lang/Object;IZZZZ)V";
			mID = t.pEnv->GetStaticMethodID( getMyClass(), "postCommandEvent", cSignature );	
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
		{
			jvalue args[6];
			args[0].l = _par0;
			args[1].i = jint( _par1 );
			args[2].z = jboolean( _par2 );
			args[3].z = jboolean( _par3 );
			args[4].z = jboolean( _par4 );
			args[5].z = jboolean( _par5 );
			t.pEnv->CallStaticVoidMethodA( getMyClass(), mID, args );
		}
	}
}

// ----------------------------------------------------------------------------

void com_sun_star_vcl_VCLEventQueue::postMouseWheelEvent( jobject _par0, long _par1, long _par2, long _par3, long _par4, sal_Bool _par5, sal_Bool _par6, sal_Bool _par7, sal_Bool _par8 )
{
	static jmethodID mID = NULL;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "(Ljava/lang/Object;IIIIZZZZ)V";
			mID = t.pEnv->GetStaticMethodID( getMyClass(), "postMouseWheelEvent", cSignature );	
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
		{
			jvalue args[9];
			args[0].l = _par0;
			args[1].i = jint( _par1 );
			args[2].i = jint( _par2 );
			args[3].i = jint( _par3 );
			args[4].i = jint( _par4 );
			args[5].z = jboolean( _par5 );
			args[6].z = jboolean( _par6 );
			args[7].z = jboolean( _par7 );
			args[8].z = jboolean( _par8 );
			t.pEnv->CallStaticVoidMethodA( getMyClass(), mID, args );
		}
	}
}

// ----------------------------------------------------------------------------

void com_sun_star_vcl_VCLEventQueue::postWindowMoveSessionEvent( jobject _par0, long _par1, long _par2, sal_Bool _par3 )
{
	static jmethodID mID = NULL;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "(Ljava/lang/Object;IIZ)V";
			mID = t.pEnv->GetStaticMethodID( getMyClass(), "postWindowMoveSessionEvent", cSignature );	
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
		{
			jvalue args[4];
			args[0].l = _par0;
			args[1].i = jint( _par1 );
			args[2].i = jint( _par2 );
			args[3].z = jboolean( _par3 );
			t.pEnv->CallStaticVoidMethodA( getMyClass(), mID, args );
		}
	}
}

// ----------------------------------------------------------------------------

com_sun_star_vcl_VCLEventQueue::com_sun_star_vcl_VCLEventQueue( jobject myObj ) : java_lang_Object( (jobject)NULL )
{
	static jmethodID mID = NULL;
	VCLThreadAttach t;
	if ( !t.pEnv )
		return;
	if ( !mID )
	{
		char *cSignature = "()V";
		mID = t.pEnv->GetMethodID( getMyClass(), "<init>", cSignature );
	}
	OSL_ENSURE( mID, "Unknown method id!" );

	jobject tempObj = t.pEnv->NewObject( getMyClass(), mID );
	saveRef( tempObj );
}

// ----------------------------------------------------------------------------

sal_Bool com_sun_star_vcl_VCLEventQueue::anyCachedEvent( USHORT _par0 )
{
	static jmethodID mID = NULL;
	sal_Bool out = sal_False;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "(I)Z";
			mID = t.pEnv->GetMethodID( getMyClass(), "anyCachedEvent", cSignature );	
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
		{
			jvalue args[1];
			args[0].i = jint( _par0 );
			out = (sal_Bool)t.pEnv->CallNonvirtualBooleanMethodA( object, getMyClass(), mID, args );
		}
	}
	return out;
}

// ----------------------------------------------------------------------------

void com_sun_star_vcl_VCLEventQueue::dispatchNextEvent()
{
	static jmethodID mID = NULL;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "()V";
			mID = t.pEnv->GetMethodID( getMyClass(), "dispatchNextEvent", cSignature );	
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
			t.pEnv->CallNonvirtualVoidMethod( object, getMyClass(), mID );
	}
}

// ----------------------------------------------------------------------------

com_sun_star_vcl_VCLEvent *com_sun_star_vcl_VCLEventQueue::getNextCachedEvent( ULONG _par0, sal_Bool _par1 )
{
	static jmethodID mID = NULL;
	com_sun_star_vcl_VCLEvent *out = NULL;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "(JZ)Lcom/sun/star/vcl/VCLEvent;";
			mID = t.pEnv->GetMethodID( getMyClass(), "getNextCachedEvent", cSignature );	
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
		{
			jvalue args[2];
			args[0].j = jlong( _par0 );
			args[1].z = jboolean( _par1 );
			jobject tempObj;
			tempObj = t.pEnv->CallNonvirtualObjectMethodA( object, getMyClass(), mID, args );
			if ( tempObj )
				out = new com_sun_star_vcl_VCLEvent( tempObj );
		}
	}
	return out;
}

// ----------------------------------------------------------------------------

void com_sun_star_vcl_VCLEventQueue::postCachedEvent( const com_sun_star_vcl_VCLEvent *_par0 )
{
	static jmethodID mID = NULL;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "(Lcom/sun/star/vcl/VCLEvent;)V";
			mID = t.pEnv->GetMethodID( getMyClass(), "postCachedEvent", cSignature );	
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
		{
			jvalue args[1];
			args[0].l = _par0->getJavaObject();
			t.pEnv->CallNonvirtualVoidMethodA( object, getMyClass(), mID, args );
		}
	}
}

// ----------------------------------------------------------------------------

void com_sun_star_vcl_VCLEventQueue::setShutdownDisabled( sal_Bool _par0 )
{
	static jmethodID mID = NULL;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "(Z)V";
			mID = t.pEnv->GetMethodID( getMyClass(), "setShutdownDisabled", cSignature );	
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
		{
			jvalue args[1];
			args[0].z = jboolean( _par0 );
			t.pEnv->CallNonvirtualVoidMethodA( object, getMyClass(), mID, args );
		}
	}
}
