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

#ifndef _JAVA_CLIPBOARD_HXX_
#define _JAVA_CLIPBOARD_HXX_

#include <hash_map>
#include <list>

#include <cppuhelper/factory.hxx>
#include <cppuhelper/compbase1.hxx>
#include <cppuhelper/compbase4.hxx>
#include <com/sun/star/datatransfer/XTransferable.hpp>
#include <com/sun/star/datatransfer/clipboard/XClipboardEx.hpp>
#include <com/sun/star/datatransfer/clipboard/XClipboardOwner.hpp>
#include <com/sun/star/datatransfer/clipboard/XClipboardListener.hpp>
#include <com/sun/star/datatransfer/clipboard/XClipboardNotifier.hpp>
#include <com/sun/star/datatransfer/clipboard/XFlushableClipboard.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>

#include <rtl/ustring.hxx>
#include <sal/types.h>
#include "DTransClipboard.hxx"

namespace java {

class JavaClipboard : public ::cppu::WeakComponentImplHelper4< ::com::sun::star::datatransfer::clipboard::XClipboardEx, ::com::sun::star::datatransfer::clipboard::XFlushableClipboard, ::com::sun::star::datatransfer::clipboard::XClipboardNotifier, ::com::sun::star::lang::XServiceInfo >
{
	::com::sun::star::uno::Reference< ::com::sun::star::datatransfer::XTransferable >	maContents;
	::com::sun::star::uno::Reference< ::com::sun::star::datatransfer::clipboard::XClipboardOwner >	maOwner;
	::std::list< ::com::sun::star::uno::Reference< ::com::sun::star::datatransfer::clipboard::XClipboardListener > >	maListeners;
	::com::sun::star::uno::Reference< ::com::sun::star::datatransfer::XTransferable >	maPrivateContents;
	::com::sun::star::uno::Reference< ::com::sun::star::datatransfer::clipboard::XClipboardOwner >	maPrivateOwner;
	::osl::Mutex			maMutex;
	bool					mbSystemClipboard;
	sal_Bool				mbPrivateClipboard;

public:
							JavaClipboard( bool bSystemClipboard );
	virtual					~JavaClipboard();

    virtual void			SAL_CALL flushClipboard( ) throw( ::com::sun::star::uno::RuntimeException );
	virtual ::com::sun::star::uno::Reference< ::com::sun::star::datatransfer::XTransferable >	SAL_CALL getContents() throw( ::com::sun::star::uno::RuntimeException );
	virtual void			SAL_CALL setContents( const ::com::sun::star::uno::Reference< ::com::sun::star::datatransfer::XTransferable >& xTransferable, const ::com::sun::star::uno::Reference< ::com::sun::star::datatransfer::clipboard::XClipboardOwner >& xClipboardOwner ) throw( ::com::sun::star::uno::RuntimeException );
	virtual ::rtl::OUString	SAL_CALL getName() throw( ::com::sun::star::uno::RuntimeException );
	virtual sal_Int8		SAL_CALL getRenderingCapabilities() throw( ::com::sun::star::uno::RuntimeException );
	virtual void			SAL_CALL addClipboardListener( const ::com::sun::star::uno::Reference< ::com::sun::star::datatransfer::clipboard::XClipboardListener >& listener ) throw( ::com::sun::star::uno::RuntimeException );
	virtual void			SAL_CALL removeClipboardListener( const ::com::sun::star::uno::Reference< ::com::sun::star::datatransfer::clipboard::XClipboardListener >& listener ) throw( ::com::sun::star::uno::RuntimeException );
	virtual ::rtl::OUString	SAL_CALL getImplementationName() throw( ::com::sun::star::uno::RuntimeException );
	virtual sal_Bool		SAL_CALL supportsService( const ::rtl::OUString& ServiceName ) throw( ::com::sun::star::uno::RuntimeException );

	virtual ::com::sun::star::uno::Sequence< ::rtl::OUString >	SAL_CALL getSupportedServiceNames() throw( ::com::sun::star::uno::RuntimeException );
	virtual void			SAL_CALL initialize( const com::sun::star::uno::Sequence<com::sun::star::uno::Any>& xAny ) throw( ::com::sun::star::uno::RuntimeException );

	void					setPrivateClipboard( sal_Bool bPrivateClipboard );
};

class JavaClipboardFactory : public ::cppu::WeakComponentImplHelper1< ::com::sun::star::lang::XSingleServiceFactory >
{
	::std::hash_map< ::rtl::OUString, ::com::sun::star::uno::Reference< XInterface >, ::rtl::OUStringHash >	maInstances;
	::osl::Mutex maMutex;

public:
							JavaClipboardFactory();
	virtual					~JavaClipboardFactory();

	virtual ::com::sun::star::uno::Reference< XInterface >	SAL_CALL createInstance() throw();
	virtual ::com::sun::star::uno::Reference< XInterface >	SAL_CALL createInstanceWithArguments( const ::com::sun::star::uno::Sequence< ::com::sun::star::uno::Any >& rArgs ) throw();
};

::com::sun::star::uno::Sequence< ::rtl::OUString > SAL_CALL JavaClipboard_getSupportedServiceNames();
}

#endif
