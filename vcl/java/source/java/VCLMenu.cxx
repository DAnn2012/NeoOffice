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
 *  Copyright 2003 by Patrick Luby (patrick.luby@planamesa.com)
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
 
#define _SV_COM_SUN_STAR_VCL_VCLMENU_CXX

#ifndef _SV_COM_SUN_STAR_VCL_VCLMENU_HXX
#include <com/sun/star/vcl/VCLMenu.hxx>
#endif
#ifndef _SV_COM_SUN_STAR_VCL_VCLMENUITEMDATA_HXX
#include <com/sun/star/vcl/VCLMenuItemData.hxx>
#endif

using namespace vcl;

// ============================================================================

jclass com_sun_star_vcl_VCLMenu::theClass = NULL;

// ----------------------------------------------------------------------------

jclass com_sun_star_vcl_VCLMenu::getMyClass()
{
	if ( !theClass )
	{
		VCLThreadAttach t;
		if ( !t.pEnv ) return (jclass)NULL;
		jclass tempClass = t.pEnv->FindClass( "com/sun/star/vcl/VCLMenu" );
		OSL_ENSURE( tempClass, "Java : FindClass not found!" );
		theClass = (jclass)t.pEnv->NewGlobalRef( tempClass );
	}
	return theClass;
}

// ----------------------------------------------------------------------------

com_sun_star_vcl_VCLMenu::com_sun_star_vcl_VCLMenu( ) : java_lang_Object( (jobject)NULL )
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
	jobject tempObj;
	tempObj = t.pEnv->NewObjectA( getMyClass(), mID, NULL );
	saveRef( tempObj );
}

// ----------------------------------------------------------------------------

com_sun_star_vcl_VCLMenuItemData * com_sun_star_vcl_VCLMenu::getMenuItemDataObject( void )
{
	static jmethodID mID = NULL;
	com_sun_star_vcl_VCLMenuItemData *out = NULL;
	VCLThreadAttach t;
	if ( t.pEnv )
	{
		if ( !mID )
		{
			char *cSignature = "()Lcom/sun/star/vcl/VCLMenuItemData;";
			mID = t.pEnv->GetMethodID( getMyClass(), "getMenuItemDataObject", cSignature );
		}
		OSL_ENSURE( mID, "Unknown method id!" );
		if ( mID )
		{
			jobject tempObj = t.pEnv->CallNonvirtualObjectMethod( object, getMyClass(), mID );
			if ( tempObj )
				out = new com_sun_star_vcl_VCLMenuItemData( tempObj );
		}
	}
	return out;
}

// ----------------------------------------------------------------------------

void com_sun_star_vcl_VCLMenu::insertItem( com_sun_star_vcl_VCLMenuItemData *_par0, int _par1 )
{
        static jmethodID mID = NULL;
        VCLThreadAttach t;
        if ( t.pEnv )
        {
                if ( !mID )
                {
                        char *cSignature = "(Lcom/sun/star/vcl/VCLMenuItemData;I)V";
                        mID = t.pEnv->GetMethodID( getMyClass(), "insertItem", cSignature );
                }
                OSL_ENSURE( mID, "Unknown method id!" );
                if ( mID )
                {
                        jvalue args[2];
                        if ( _par0 )
                            args[0].l = _par0->getJavaObject();
                        else
                            args[0].l = NULL;
                        args[1].i = jint( _par1 );
                        t.pEnv->CallNonvirtualVoidMethodA( object, getMyClass(), mID, args );
                }
        }
}

// ----------------------------------------------------------------------------

void com_sun_star_vcl_VCLMenu::removeItem( int _par0 )
{
        static jmethodID mID = NULL;
        VCLThreadAttach t;
        if ( t.pEnv )
        {
                if ( !mID )
                {
                        char *cSignature = "(I)V";
                        mID = t.pEnv->GetMethodID( getMyClass(), "removeItem", cSignature );
                }
                OSL_ENSURE( mID, "Unknown method id!" );
                if ( mID )
                {
                        jvalue args[1];
                        args[0].i = jint( _par0 );
                        t.pEnv->CallNonvirtualVoidMethodA( object, getMyClass(), mID, args );
                }
        }
}

// ----------------------------------------------------------------------------

void com_sun_star_vcl_VCLMenu::checkItem( int _par0, bool _par1 )
{
        static jmethodID mID = NULL;
        VCLThreadAttach t;
        if ( t.pEnv )
        {
                if ( !mID )
                {
                        char *cSignature = "(IZ)V";
                        mID = t.pEnv->GetMethodID( getMyClass(), "checkItem", cSignature );
                }
                OSL_ENSURE( mID, "Unknown method id!" );
                if ( mID )
                {
                        jvalue args[2];
                        args[0].i = jint( _par0 );
                        args[1].z = jboolean( _par1 );
                        t.pEnv->CallNonvirtualVoidMethodA( object, getMyClass(), mID, args );
                }
        }
}

// ----------------------------------------------------------------------------

void com_sun_star_vcl_VCLMenu::enableItem( int _par0, bool _par1 )
{
        static jmethodID mID = NULL;
        VCLThreadAttach t;
        if ( t.pEnv )
        {
                if ( !mID )
                {
                        char *cSignature = "(IZ)V";
                        mID = t.pEnv->GetMethodID( getMyClass(), "enableItem", cSignature );
                }
                OSL_ENSURE( mID, "Unknown method id!" );
                if ( mID )
                {
                        jvalue args[2];
                        args[0].i = jint( _par0 );
                        args[1].z = jboolean( _par1 );
                        t.pEnv->CallNonvirtualVoidMethodA( object, getMyClass(), mID, args );
                }
        }
}

// ----------------------------------------------------------------------------

void com_sun_star_vcl_VCLMenu::attachSubmenu( com_sun_star_vcl_VCLMenuItemData *_par0, int _par1 )
{
        static jmethodID mID = NULL;
        VCLThreadAttach t;
        if ( t.pEnv )
        {
                if ( !mID )
                {
                        char *cSignature = "(Lcom/sun/star/vcl/VCLMenuItemData;I)V";
                        mID = t.pEnv->GetMethodID( getMyClass(), "attachSubmenu", cSignature );
                }
                OSL_ENSURE( mID, "Unknown method id!" );
                if ( mID )
                {
                        jvalue args[2];
                        if ( _par0 )
                            args[0].l = _par0->getJavaObject();
                        else
                            args[0].l = NULL;
                        args[1].i = jint( _par1 );
                        t.pEnv->CallNonvirtualVoidMethodA( object, getMyClass(), mID, args );
                }
        }
}