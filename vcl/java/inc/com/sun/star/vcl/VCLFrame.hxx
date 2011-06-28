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

#ifndef _SV_COM_SUN_STAR_VCL_VCLFRAME_HXX
#define	_SV_COM_SUN_STAR_VCL_VCLFRAME_HXX

#ifndef _SV_JAVA_LANG_OBJECT_HXX
#include <java/lang/Object.hxx>
#endif
#ifndef _SV_GEN_HXX
#include <tools/gen.hxx>
#endif

class JavaSalFrame;

namespace vcl {

class com_sun_star_vcl_VCLGraphics;

class com_sun_star_vcl_VCLFrame : public java_lang_Object
{
protected:
	static jclass		theClass;

public:
	static jclass		getMyClass();
	static void			flushAllFrames();

						com_sun_star_vcl_VCLFrame( jobject myObj ) : java_lang_Object( myObj ) {}
						com_sun_star_vcl_VCLFrame( ULONG nSalFrameStyle, const JavaSalFrame *pFrame, const JavaSalFrame *pParent, sal_Bool bShowOnlyMenus, sal_Bool bUtilityWindow );
	virtual				~com_sun_star_vcl_VCLFrame() {}

	void				addChild( JavaSalFrame *_par0 );
	void				dispose();
	const Rectangle		getBounds( sal_Bool *_par0 = NULL, sal_Bool _par1 = sal_False );
	com_sun_star_vcl_VCLGraphics*	getGraphics();
	const Rectangle		getInsets();
	::rtl::OUString		getKeyName( USHORT _par0 );
	void*				getNativeWindow();
	void*				getNativeWindowContentView();
	void*				getPeer();
	ULONG				getState();
	void				makeModal();
	void				removeChild( JavaSalFrame *_par0 );
	sal_Bool			requestFocus();
	void				setAllowKeyBindings( sal_Bool _par0 );
	void				setBounds( long _par0, long _par1, long _par2, long _par3 );
	void				setFullScreenMode( sal_Bool _par0 );
	void				setMinClientSize( long _par0, long _par1 );
	void				setPointer( USHORT _par0 );
	void				setState( ULONG _par0 );
	void				setTitle( ::rtl::OUString _par0 );
	void				setVisible( sal_Bool _par0, sal_Bool _par1 );
	void				sync();
	sal_Bool			toFront();
};

} // namespace vcl

#endif // _SV_COM_SUN_STAR_VCL_VCLFRAME_HXX
