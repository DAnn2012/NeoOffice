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
 *		 - GNU General Public License Version 2.1
 *
 *  GNU General Public License Version 2.1
 *  =============================================
 *  Copyright 2008 by Planamesa Inc.
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
 *************************************************************************/

#include "neomobileappevent.hxx"
#include <org/neooffice/XNeoOfficeMobile.hpp>
#include <unistd.h>

#ifndef _RTL_USTRING_HXX_
#include <rtl/ustring.hxx>
#endif

#ifndef _CPPUHELPER_QUERYINTERFACE_HXX_
#include <cppuhelper/queryinterface.hxx> // helper for queryInterface() impl
#endif
#ifndef _CPPUHELPER_FACTORY_HXX_
#include <cppuhelper/factory.hxx> // helper for component factory
#endif
#ifndef _CPPUHELPER_IMPLEMENATIONENTRY_HXX_
#include <cppuhelper/implementationentry.hxx>
#endif

// generated c++ interfaces

#ifndef _COM_SUN_STAR_LANG_XSINGLESERVICEFACTORY_HPP_
#include <com/sun/star/lang/XSingleServiceFactory.hpp>
#endif
#ifndef _COM_SUN_STAR_LANG_XMULTISERVICEFACTORY_HPP_
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#endif
#ifndef _COM_SUN_STAR_LANG_XSERVICEINFO_HPP_
#include <com/sun/star/lang/XServiceInfo.hpp>
#endif
#ifndef _COM_SUN_STAR_REGISTRY_XREGISTRYKEY_HPP_
#include <com/sun/star/registry/XRegistryKey.hpp>
#endif
#include <comphelper/processfactory.hxx>


#include "premac.h"
#import <Foundation/Foundation.h>
#include "postmac.h"


using namespace ::rtl;
using namespace ::osl;
using namespace ::cppu;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::registry;
using namespace ::org::neooffice;


NeoMobilExportFileAppEvent::NeoMobilExportFileAppEvent() :
	mbFinished( false )
{
}

IMPL_LINK( NeoMobilExportFileAppEvent, ExportFile, void*, EMPTY_ARG )
{
	if ( !mbFinished )
	{
		if ( Application::IsInMain() )
		{
			// get reference to our NeoOfficeMobile service to be used to
			// perform our conversions
			
			Reference< XComponentContext > component( comphelper_getProcessComponentContext() );
			Reference< XMultiComponentFactory > rServiceManager = component->getServiceManager();
			Reference< XInterface > rNeoOfficeMobile = rServiceManager->createInstanceWithContext(OUString::createFromAscii("org.neooffice.NeoOfficeMobile"), component);
			
			if(!rNeoOfficeMobile.is())
			{
				fprintf( stderr, "NeoMobilExportFileAppEvent::ExportFile unable to get NeoOfficeMobile service reference\n" );
				return 0;
			}
			
			Reference< XNeoOfficeMobile > neoOfficeMobile(rNeoOfficeMobile, UNO_QUERY);
			if(!neoOfficeMobile.is())
			{
				fprintf( stderr, "NeoMobilExportFileAppEvent::ExportFile unable to cast NeoOfficeMobile reference to service\n" );
				return 0;
			}
			
			// embed the UUID within the current document
			
			// +++ FIXME
			// need to pass in UUID
			// use neoOfficeMobile->setPropertyValue() with appropriate key/value
			
			// get a unique temporary base filename
			
			NSAutoreleasePool *pool=[[NSAutoreleasePool alloc] init];
			
			NSString *basePath = NSTemporaryDirectory();
			NSString *filePath = [basePath stringByAppendingPathComponent:@"_nm_export"];
			while ([[NSFileManager defaultManager] fileExistsAtPath:filePath]) {
				filePath = [basePath stringByAppendingPathComponent:[NSString stringWithFormat:@"_nm_export_%d", rand()]];
			}
			
			fprintf(stderr, "NeoMobilExportFileAppEvent::ExportFile exporting to '%s'\n", [filePath UTF8String]);
			
			// perform an opendocument export
			
			OUString docExtension=neoOfficeMobile->getOpenDocumentExtension();
			
			OUString openDocExportURL=OUString::createFromAscii("file://");
			openDocExportURL+=OUString::createFromAscii([filePath UTF8String]);
			openDocExportURL+=docExtension;
			
			if(!neoOfficeMobile->saveAsOpenDocument(openDocExportURL))
			{
				[pool release];
				fprintf( stderr, "NeoMobilExportFileAppEvent::ExportFile unable to perform OpenDocument export\n" );
				return 0;
			}
			
			// perform a PDF export
			
			OUString pdfExportURL=OUString::createFromAscii("file://");
			pdfExportURL+=OUString::createFromAscii([filePath UTF8String]);
			pdfExportURL+=OUString::createFromAscii(".pdf");
			
			if(!neoOfficeMobile->saveAsPDF(pdfExportURL))
			{
				[pool release];
				fprintf( stderr, "NeoMobilExportFileAppEvent::ExportFile unable to perform PDF export\n" );
				return 0;
			}
			
			// perform an HTML export.  Note that we need to do this in a
			// temporary directory and then zip the directory contents into
			// a single file.  We'll just use our file's base path
			// as the temporary directory name.
			
			if(![[NSFileManager defaultManager] createDirectoryAtPath: filePath attributes: nil])
			{
				[pool release];
				fprintf( stderr, "NeoMobilExportFileAppEvent::ExportFile unable to create export directory\n" );
				return 0;
			}
			
			OUString htmlExportURL=OUString::createFromAscii("file://");
			htmlExportURL+=OUString::createFromAscii([filePath UTF8String]);
			htmlExportURL+=OUString::createFromAscii("/_nm_export.html");
			
			if(!neoOfficeMobile->saveAsHTML(htmlExportURL))
			{
				[pool release];
				fprintf( stderr, "NeoMobilExportFileAppEvent::ExportFile unable to perform HTML export\n" );
				return 0;
			}
			
			OUString htmlExportZipFile=OUString::createFromAscii([filePath UTF8String]);
			htmlExportZipFile+=OUString::createFromAscii(".zip");
			
			if(!neoOfficeMobile->zipDirectory(OUString::createFromAscii([filePath UTF8String]), htmlExportZipFile))
			{
				[pool release];
				fprintf( stderr, "NeoMobilExportFileAppEvent::ExportFile unable to create HTML zip file\n" );
				return 0;
			}
			
			// upload files
			
			// +++ FIXME
			//
			// Note that at this point the exported trio is in the following
			// OUStrings:
			//
			//	PDF (as file:// URL) - pdfExportURL
			//	HTML (as full absolute path) - htmlExportZipFile
			//	OpenDocument (as file:// URL) - openDocExportURL
			
			// free our autorelease pool
			
			[pool release];
		}
		else
		{
			fprintf( stderr, "NeoMobilExportFileAppEvent::ExportFile : I'm notn Main!\n");
		}

		fprintf( stderr, "NeoMobilExportFileAppEvent::ExportFile not implemented\n" );
		mbFinished = true;
	}

	return 0;
}
