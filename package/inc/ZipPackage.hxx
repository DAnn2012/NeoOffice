/*************************************************************************
 *
 * Copyright 2008 by Sun Microsystems, Inc.
 *
 * $RCSfile$
 * $Revision$
 *
 * This file is part of NeoOffice.
 *
 * NeoOffice is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3
 * only, as published by the Free Software Foundation.
 *
 * NeoOffice is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License version 3 for more details
 * (a copy is included in the LICENSE file that accompanied this code).
 *
 * You should have received a copy of the GNU General Public License
 * version 3 along with NeoOffice.  If not, see
 * <http://www.gnu.org/licenses/gpl-3.0.txt>
 * for a copy of the GPLv3 License.
 *
 * Modified March 2013 by Patrick Luby. NeoOffice is distributed under
 * GPL only under modification term 2 of the LGPL.
 *
 ************************************************************************/
#ifndef _ZIP_PACKAGE_HXX
#define _ZIP_PACKAGE_HXX

#include <cppuhelper/implbase7.hxx>
#include <com/sun/star/lang/XInitialization.hpp>
#include <com/sun/star/container/XHierarchicalNameAccess.hpp>
#include <com/sun/star/lang/XSingleServiceFactory.hpp>
#include <com/sun/star/util/XChangesBatch.hpp>
#include <com/sun/star/lang/XUnoTunnel.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#ifndef _COM_SUN_STAR_LANG_XPSERVICEINFO_HPP_
#include <com/sun/star/lang/XServiceInfo.hpp>
#endif
#include <HashMaps.hxx>
#include <com/sun/star/lang/IllegalArgumentException.hpp>
#include <osl/file.h>
class ZipOutputStream;
class ZipPackageFolder;
class ZipFile;
class ByteGrabber;
namespace com { namespace sun { namespace star {
	namespace container { class XNameContainer; }
	namespace io { class XStream; class XOutputStream; class XInputStream; class XSeekable; class XActiveDataStreamer; }
	namespace lang { class XMultiServiceFactory; }
	namespace task { class XInteractionHandler; }
} } }
enum SegmentEnum
{
	e_Aborted = -1000,
	e_Retry,
	e_Finished,
	e_Success = 0
};

enum InitialisationMode
{
	e_IMode_None,
	e_IMode_URL,
	e_IMode_XInputStream,
	e_IMode_XStream
};

class ZipPackage : public cppu::WeakImplHelper7
					<
					   com::sun::star::lang::XInitialization,
					   com::sun::star::lang::XSingleServiceFactory,
					   com::sun::star::lang::XUnoTunnel,
					   com::sun::star::lang::XServiceInfo,
					   com::sun::star::container::XHierarchicalNameAccess,
					   com::sun::star::util::XChangesBatch,
					   com::sun::star::beans::XPropertySet
					>
{
protected:
	::com::sun::star::uno::Sequence < sal_Int8 > aEncryptionKey;
#ifndef NO_OOO_3_4_1_AES_ENCRYPTION
	::com::sun::star::uno::Sequence < sal_Int8 > aEncryptionKeySHA256;
#endif	// !NO_OOO_3_4_1_AES_ENCRYPTION
	FolderHash 		 aRecent;
	::rtl::OUString	 sURL;
	sal_Bool 		 bHasEncryptedEntries;
	sal_Bool 		 bUseManifest;
	sal_Bool		 bForceRecovery;
	
	sal_Bool		m_bMediaTypeFallbackUsed;
	sal_Int16		m_nFormat;
	sal_Bool		m_bAllowRemoveOnInsert;
	
	InitialisationMode eMode;

	::com::sun::star::uno::Reference < com::sun::star::container::XNameContainer > xRootFolder;
	::com::sun::star::uno::Reference < com::sun::star::io::XStream > xStream;
	::com::sun::star::uno::Reference < com::sun::star::io::XInputStream > xContentStream;
	::com::sun::star::uno::Reference < com::sun::star::io::XSeekable > xContentSeek;
	const ::com::sun::star::uno::Reference < com::sun::star::lang::XMultiServiceFactory > xFactory;

	ZipPackageFolder *pRootFolder;
	ZipFile 		 *pZipFile;

	void parseManifest();
	void parseContentType();
	void getZipFileContents();
	sal_Bool writeFileIsTemp();
	::com::sun::star::uno::Reference < ::com::sun::star::io::XActiveDataStreamer > openOriginalForOutput();
	void WriteMimetypeMagicFile( ZipOutputStream& aZipOut );
	void DisconnectFromTargetAndThrowException_Impl(
			const ::com::sun::star::uno::Reference< ::com::sun::star::io::XInputStream >& xTempStream );

public:
	ZipPackage (const ::com::sun::star::uno::Reference < com::sun::star::lang::XMultiServiceFactory > &xNewFactory);
	virtual ~ZipPackage( void );
	ZipFile& getZipFile() { return *pZipFile;}
	const com::sun::star::uno::Sequence < sal_Int8 > & getEncryptionKey ( ) {return aEncryptionKey;}
	sal_Int16 getFormat() const { return m_nFormat; }

	// XInitialization
    virtual void SAL_CALL initialize( const ::com::sun::star::uno::Sequence< ::com::sun::star::uno::Any >& aArguments ) 
		throw(::com::sun::star::uno::Exception, ::com::sun::star::uno::RuntimeException);
	// XHierarchicalNameAccess
    virtual ::com::sun::star::uno::Any SAL_CALL getByHierarchicalName( const ::rtl::OUString& aName ) 
		throw(::com::sun::star::container::NoSuchElementException, ::com::sun::star::uno::RuntimeException);
    virtual sal_Bool SAL_CALL hasByHierarchicalName( const ::rtl::OUString& aName ) 
		throw(::com::sun::star::uno::RuntimeException);
	// XSingleServiceFactory
    virtual ::com::sun::star::uno::Reference< ::com::sun::star::uno::XInterface > SAL_CALL createInstance(  ) 
		throw(::com::sun::star::uno::Exception, ::com::sun::star::uno::RuntimeException);
    virtual ::com::sun::star::uno::Reference< ::com::sun::star::uno::XInterface > SAL_CALL createInstanceWithArguments( const ::com::sun::star::uno::Sequence< ::com::sun::star::uno::Any >& aArguments ) 
		throw(::com::sun::star::uno::Exception, ::com::sun::star::uno::RuntimeException);
	// XChangesBatch
    virtual void SAL_CALL commitChanges(  ) 
		throw(::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException);
    virtual sal_Bool SAL_CALL hasPendingChanges(  ) 
		throw(::com::sun::star::uno::RuntimeException);
    virtual ::com::sun::star::uno::Sequence< ::com::sun::star::util::ElementChange > SAL_CALL getPendingChanges(  ) 
		throw(::com::sun::star::uno::RuntimeException);
	// XUnoTunnel
    virtual sal_Int64 SAL_CALL getSomething( const ::com::sun::star::uno::Sequence< sal_Int8 >& aIdentifier ) 
		throw(::com::sun::star::uno::RuntimeException);
	com::sun::star::uno::Sequence < sal_Int8 > getUnoTunnelImplementationId( void ) 
		throw(::com::sun::star::uno::RuntimeException);
	// XPropertySet
    virtual ::com::sun::star::uno::Reference< ::com::sun::star::beans::XPropertySetInfo > SAL_CALL getPropertySetInfo(  ) 
		throw(::com::sun::star::uno::RuntimeException);
    virtual void SAL_CALL setPropertyValue( const ::rtl::OUString& aPropertyName, const ::com::sun::star::uno::Any& aValue ) 
		throw(::com::sun::star::beans::UnknownPropertyException, ::com::sun::star::beans::PropertyVetoException, ::com::sun::star::lang::IllegalArgumentException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException);
    virtual ::com::sun::star::uno::Any SAL_CALL getPropertyValue( const ::rtl::OUString& PropertyName ) 
		throw(::com::sun::star::beans::UnknownPropertyException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException);
    virtual void SAL_CALL addPropertyChangeListener( const ::rtl::OUString& aPropertyName, const ::com::sun::star::uno::Reference< ::com::sun::star::beans::XPropertyChangeListener >& xListener ) 
		throw(::com::sun::star::beans::UnknownPropertyException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException);
    virtual void SAL_CALL removePropertyChangeListener( const ::rtl::OUString& aPropertyName, const ::com::sun::star::uno::Reference< ::com::sun::star::beans::XPropertyChangeListener >& aListener ) 
		throw(::com::sun::star::beans::UnknownPropertyException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException);
    virtual void SAL_CALL addVetoableChangeListener( const ::rtl::OUString& PropertyName, const ::com::sun::star::uno::Reference< ::com::sun::star::beans::XVetoableChangeListener >& aListener ) 
		throw(::com::sun::star::beans::UnknownPropertyException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException);
    virtual void SAL_CALL removeVetoableChangeListener( const ::rtl::OUString& PropertyName, const ::com::sun::star::uno::Reference< ::com::sun::star::beans::XVetoableChangeListener >& aListener ) 
		throw(::com::sun::star::beans::UnknownPropertyException, ::com::sun::star::lang::WrappedTargetException, ::com::sun::star::uno::RuntimeException);

	// XServiceInfo
    virtual ::rtl::OUString SAL_CALL getImplementationName(  ) 
		throw (::com::sun::star::uno::RuntimeException);
    virtual sal_Bool SAL_CALL supportsService( const ::rtl::OUString& ServiceName ) 
		throw (::com::sun::star::uno::RuntimeException);
    virtual ::com::sun::star::uno::Sequence< ::rtl::OUString > SAL_CALL getSupportedServiceNames(  ) 
		throw (::com::sun::star::uno::RuntimeException);

	// Uno componentiseralation
	static ::rtl::OUString static_getImplementationName();	
	static ::com::sun::star::uno::Sequence < ::rtl::OUString > static_getSupportedServiceNames();	
	static ::com::sun::star::uno::Reference < com::sun::star::lang::XSingleServiceFactory > createServiceFactory( com::sun::star::uno::Reference < com::sun::star::lang::XMultiServiceFactory > const & rServiceFactory );
	sal_Bool SAL_CALL static_supportsService(rtl::OUString const & rServiceName);
};
#endif
