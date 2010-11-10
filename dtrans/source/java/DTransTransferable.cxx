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

#define _DTRANSCLIPBOARD_CXX

#include <list>

#ifndef _DTRANSCLIPBOARD_HXX
#include "DTransClipboard.hxx"
#endif
#ifndef _DTRANSTRANSFERABLE_HXX
#include "DTransTransferable.hxx"
#endif
#ifndef _SV_SVAPP_HXX
#include <vcl/svapp.hxx>
#endif
#ifndef _VOS_MUTEX_HXX_
#include <vos/mutex.hxx>
#endif

#include "HtmlFmtFlt.hxx"

#include <premac.h>
#include <Carbon/Carbon.h>
#include <QuickTime/QuickTime.h>
#include <postmac.h>

using namespace com::sun::star::datatransfer;
using namespace com::sun::star::io;
using namespace com::sun::star::uno;
using namespace java;
using namespace rtl;
using namespace vcl;
using namespace vos;

static UInt32 nSupportedTypes = 9;

// List of supported native types in priority order
static FourCharCode aSupportedNativeTypes[] = {
	// Mark 'furl' as text to ensure that it is the preferred flavor
	'furl',
	'RTF ',
	'HTML',
	'utxt',
	kQTFileTypeText,
	kQTFileTypePDF,
	kQTFileTypePNG,
	kQTFileTypeTIFF,
	kQTFileTypePicture
};

// List of supported types that are text
static bool aSupportedTextTypes[] = {
	// Mark 'furl' as text to ensure that it is the preferred flavor
	true,
	true,
	true,
	true,
	true,
	false,
	false,
	false,
	false
};

// List of supported mime types in priority order
static OUString aSupportedMimeTypes[] = {
	OUString::createFromAscii( "application/x-openoffice-file;windows_formatname=\"FileName\"" ),
	OUString::createFromAscii( "text/richtext" ),
	OUString::createFromAscii( "text/html" ),
	OUString::createFromAscii( "text/plain;charset=utf-16" ),
	OUString::createFromAscii( "text/plain;charset=utf-16" ),
	OUString::createFromAscii( "image/bmp" ),
	OUString::createFromAscii( "image/bmp" ),
	OUString::createFromAscii( "image/bmp" ),
	OUString::createFromAscii( "image/bmp" )
};

// List of supported data types in priority order
static ::com::sun::star::uno::Type aSupportedDataTypes[] = {
	getCppuType( ( ::com::sun::star::uno::Sequence< sal_Int8 >* )0 ),
	getCppuType( ( ::com::sun::star::uno::Sequence< sal_Int8 >* )0 ),
	getCppuType( ( ::com::sun::star::uno::Sequence< sal_Int8 >* )0 ),
	getCppuType( ( OUString* )0 ),
	getCppuType( ( OUString* )0 ),
	getCppuType( ( ::com::sun::star::uno::Sequence< sal_Int8 >* )0 ),
	getCppuType( ( ::com::sun::star::uno::Sequence< sal_Int8 >* )0 ),
	getCppuType( ( ::com::sun::star::uno::Sequence< sal_Int8 >* )0 ),
	getCppuType( ( ::com::sun::star::uno::Sequence< sal_Int8 >* )0 ),
};

static ::std::list< DTransTransferable* > aTransferableList;
static DragSendDataUPP pDragSendDataUPP = NULL;
static ScrapPromiseKeeperUPP pScrapPromiseKeeperUPP = NULL;

// ============================================================================

static OSStatus ImplSetTransferableData( void *pNativeTransferable, int nTransferableType, FlavorType nType, void *pData )
{
	OSStatus nErr;
	if ( nTransferableType == TRANSFERABLE_TYPE_CLIPBOARD )
		nErr = noTypeErr;
	else if ( nTransferableType == TRANSFERABLE_TYPE_DRAG )
		nErr = cantGetFlavorErr;
	else
		nErr = noTypeErr;

	if ( !Application::IsShutDown() )
	{
		IMutex& rSolarMutex = Application::GetSolarMutex();
		rSolarMutex.acquire();
		if ( !Application::IsShutDown() )
		{
			DTransTransferable *pTransferable = (DTransTransferable *)pData;

			bool bTransferableFound = false;
			if ( pTransferable )
			{
				for ( ::std::list< DTransTransferable* >::const_iterator it = aTransferableList.begin(); it != aTransferableList.end(); ++it )
				{
					if ( pTransferable == *it )
					{
						bTransferableFound = true;
						break;
					}
				}
			}

			if ( bTransferableFound )
			{
				bool bFlavorFound = false;
				bool bFlavorIsText = false;
				DataFlavor aFlavor;
				for ( USHORT i = 0; i < nSupportedTypes; i++ ) {
					if ( nType == aSupportedNativeTypes[ i ] )
					{
						aFlavor.MimeType = aSupportedMimeTypes[ i ];
						aFlavor.DataType = aSupportedDataTypes[ i ];
						if ( pTransferable->isDataFlavorSupported( aFlavor ) )
						{
							bFlavorFound = true;
							bFlavorIsText = aSupportedTextTypes[ i ];
							break;
						}
					}
				}

				if ( bFlavorFound )
				{
					bool bDataFound = false;
					Any aValue;
					try {
						aValue = pTransferable->getTransferData( aFlavor );
						bDataFound = true;
					}
					catch ( ... )
					{
					}

					if ( bDataFound )
					{
						if ( aValue.getValueType().equals( getCppuType( ( OUString* )0 ) ) )
						{
							OUString aString;
							aValue >>= aString;
							sal_Unicode *pArray = (sal_Unicode *)aString.getStr();
							sal_Int32 nLen = aString.getLength();
							if ( pArray && nLen )
							{
								if ( nType == kQTFileTypeText )
								{
									CFStringRef aCFString = CFStringCreateWithCharactersNoCopy( kCFAllocatorDefault, pArray, nLen, kCFAllocatorNull );
									if ( aCFString )
									{
										CFIndex nBufLen;
										CFRange aRange;
										aRange.location = 0;
										aRange.length = CFStringGetLength( aCFString );
										if ( CFStringGetBytes( aCFString, aRange, CFStringGetSystemEncoding(), '?', false, NULL, 0, &nBufLen ) )
										{
											CFIndex nRealLen = nBufLen;
											// Place on to heap, not stack.
											UInt8 *aBuf = new UInt8[ nBufLen + 1 ];
											if ( aBuf && CFStringGetBytes( aCFString, aRange, CFStringGetSystemEncoding(), '?', false, aBuf, nBufLen, &nRealLen ) && nRealLen == nBufLen )
											{
												aBuf[ nBufLen ] = '\0';

												// Replace line feeds with
												// carriage returns but for only
												// very old applications
												for ( int j = 0; j < nBufLen; j++ )
												{
													if ( aBuf[ j ] == '\n' )
														aBuf[ j ] = '\r';
												}

												if ( nTransferableType == TRANSFERABLE_TYPE_CLIPBOARD )
													nErr = PutScrapFlavor( (ScrapRef)pNativeTransferable, nType, kScrapFlavorMaskNone, nBufLen, (const void *)aBuf );
												else if ( nTransferableType == TRANSFERABLE_TYPE_DRAG )
													nErr = SetDragItemFlavorData( (DragRef)pNativeTransferable, (DragItemRef)pData, nType, (const void *)aBuf, nBufLen, 0 );
												delete aBuf;
											}
										}

										CFRelease( aCFString );
									}
								}
								else
								{
 									nLen *= sizeof( sal_Unicode );
									if ( nTransferableType == TRANSFERABLE_TYPE_CLIPBOARD )
										nErr = PutScrapFlavor( (ScrapRef)pNativeTransferable, nType, kScrapFlavorMaskNone, nLen, (const void *)aString.getStr() );
									else if ( nTransferableType == TRANSFERABLE_TYPE_DRAG )
										nErr = SetDragItemFlavorData( (DragRef)pNativeTransferable, (DragItemRef)pData, nType, (const void *)aString.getStr(), nLen, 0 );
								}
							}
						}
						else if ( aValue.getValueType().equals( getCppuType( ( Sequence< sal_Int8 >* )0 ) ) )
						{
							Sequence< sal_Int8 > aData;
							aValue >>= aData;

							// Fix bug 3640 by not adding the Microsoft Office
							// HTML headers. Microsoft Office 2004 and 2008 do
							// appear to need those headers and the headers are
							// not understood by other applications.
							sal_Int8 *pArray = aData.getArray();
							sal_Int32 nLen = aData.getLength();
							if ( pArray && nLen )
							{
								if ( !bFlavorIsText )
								{
									// Convert from our BMP data
									ComponentInstance aImporter;
									if ( OpenADefaultComponent( GraphicsImporterComponentType, kQTFileTypeBMP, &aImporter ) == noErr )
									{
										Handle hData;
										if ( PtrToHand( pArray, &hData, nLen ) == noErr )
										{
											// Free the source data
											aData = Sequence< sal_Int8 >();

											Rect aBounds;
											if ( GetHandleSize( hData ) == nLen && GraphicsImportSetDataHandle( aImporter, hData ) == noErr && GraphicsImportGetNaturalBounds( aImporter, &aBounds ) == noErr )
											{
												if ( nType == kQTFileTypePicture )
												{
													Handle hPict;
													if ( GraphicsImportGetAsPicture( aImporter, (PicHandle *)&hPict ) == noErr )
													{
														HLock( hPict );
														if ( nTransferableType == TRANSFERABLE_TYPE_CLIPBOARD )
															nErr = PutScrapFlavor( (ScrapRef)pNativeTransferable, nType, kScrapFlavorMaskNone, GetHandleSize( (Handle)hPict ), (const void *)*hPict );
														else if ( nTransferableType == TRANSFERABLE_TYPE_DRAG )
															nErr = SetDragItemFlavorData( (DragRef)pNativeTransferable, (DragItemRef)pData, nType, (const void *)*hPict, GetHandleSize( (Handle)hPict ), 0 );
														HUnlock( hPict );

														DisposeHandle( hPict );
													}
												}
												else
												{
													GWorldPtr aGWorld;
													if ( QTNewGWorld( &aGWorld, k32ARGBPixelFormat, &aBounds, NULL, NULL, 0 ) == noErr )
													{
														if ( GraphicsImportSetGWorld( aImporter, aGWorld, NULL ) == noErr && GraphicsImportDraw( aImporter ) == noErr )
														{
															ComponentInstance aExporter;
															if ( OpenADefaultComponent( GraphicsExporterComponentType, nType, &aExporter ) == noErr )
															{
																if ( GraphicsExportSetInputGWorld( aExporter, aGWorld ) == noErr )
																{
																	Handle hExportData = NewHandle( 0 );
																	if ( GraphicsExportSetOutputHandle( aExporter, hExportData ) == noErr )
																	{
																		ULONG nDataLen;
																		if ( GraphicsExportDoExport( aExporter, &nDataLen ) == noErr )
																		{
																			Sequence< sal_Int8 > aExportData( nDataLen );
																			HLock( hExportData );
																			if ( nTransferableType == TRANSFERABLE_TYPE_CLIPBOARD )
																				nErr = PutScrapFlavor( (ScrapRef)pNativeTransferable, nType, kScrapFlavorMaskNone, nDataLen, (const void *)*hExportData );
																			else if ( nTransferableType == TRANSFERABLE_TYPE_DRAG )
																				nErr = SetDragItemFlavorData( (DragRef)pNativeTransferable, (DragItemRef)pData, nType, (const void *)*hExportData, nDataLen, 0 );
																			HUnlock( hExportData );
																		}
																	}

																	DisposeHandle( hExportData );
																}

																CloseComponent( aExporter );
															}
														}

														DisposeGWorld( aGWorld );
													}
												}
											}

											DisposeHandle( hData );
										}

										CloseComponent( aImporter );
									}
								}
								else
								{
									if ( pArray && nLen )
									{
										if ( nTransferableType == TRANSFERABLE_TYPE_CLIPBOARD )
											nErr = PutScrapFlavor( (ScrapRef)pNativeTransferable, nType, kScrapFlavorMaskNone, nLen, (const void *)pArray );
										else if ( nTransferableType == TRANSFERABLE_TYPE_DRAG )
											nErr = SetDragItemFlavorData( (DragRef)pNativeTransferable, (DragItemRef)pData, nType, (const void *)pArray, nLen, 0 );
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

	return nErr;
}

// ----------------------------------------------------------------------------

static OSErr ImplDragSendDataCallback( FlavorType nType, void *pData, DragItemRef aItem, DragRef aDrag )
{
	return ImplSetTransferableData( (void *)aDrag, TRANSFERABLE_TYPE_DRAG, nType, pData );
}

// ----------------------------------------------------------------------------

static OSStatus ImplScrapPromiseKeeperCallback( ScrapRef aScrap, ScrapFlavorType nType, void *pData )
{
	return ImplSetTransferableData( (void *)aScrap, TRANSFERABLE_TYPE_CLIPBOARD, nType, pData );
}

// ============================================================================

void DTransTransferable::flush()
{
	if ( hasOwnership() )
		CallInScrapPromises();
}

// ============================================================================

Any DTransTransferable::getTransferData( const DataFlavor& aFlavor ) throw ( UnsupportedFlavorException, IOException, RuntimeException )
{
	if ( mxTransferable.is() )
		return mxTransferable->getTransferData( aFlavor );

	Any out;

	FourCharCode nRequestedType;
	memset( &nRequestedType, 0, sizeof( FourCharCode ) );
	bool bRequestedTypeIsText = false;
	OSStatus nErr = noErr;

	// Run a loop so that if data type fails, we can try another
	for ( USHORT i = 0; i < nSupportedTypes; i++ )
	{
		if ( aFlavor.MimeType.equalsIgnoreAsciiCase( aSupportedMimeTypes[ i ] ) )
		{
			nRequestedType = aSupportedNativeTypes[ i ];
			bRequestedTypeIsText = aSupportedTextTypes[ i ];
		}
		else
		{
			continue;
		}

		MacOSSize nSize;
		if ( mnTransferableType == TRANSFERABLE_TYPE_CLIPBOARD )
		{
			nErr = GetScrapFlavorSize( (ScrapRef)mpNativeTransferable, nRequestedType, &nSize );
		}
		else if ( mnTransferableType == TRANSFERABLE_TYPE_DRAG && mnItem )
		{
			DragItemRef aItem;
			if ( GetDragItemReferenceNumber( (DragRef)mpNativeTransferable, mnItem, &aItem ) == noErr )
				nErr = GetFlavorDataSize( (DragRef)mpNativeTransferable, aItem, nRequestedType, &nSize );
		}
		else
		{
			nErr = noTypeErr;
			nSize = 0;
		}

		if ( nErr == noErr && nSize > 0 )
		{
			bool bDataFound = false;
			Handle hData = NewHandle( nSize );

			HLock( hData );
			if ( mnTransferableType == TRANSFERABLE_TYPE_CLIPBOARD )
			{
				bDataFound = ( GetScrapFlavorData( (ScrapRef)mpNativeTransferable, nRequestedType, &nSize, *hData ) == noErr );
			}
			else if ( mnTransferableType == TRANSFERABLE_TYPE_DRAG && mnItem )
			{
				DragItemRef aItem;
				if ( GetDragItemReferenceNumber( (DragRef)mpNativeTransferable, mnItem, &aItem ) == noErr )
					bDataFound = ( GetFlavorData( (DragRef)mpNativeTransferable, aItem, nRequestedType, *hData, &nSize, 0 ) == noErr );
			}
			HUnlock( hData );

			if ( bDataFound )
			{
				if ( aFlavor.DataType.equals( getCppuType( ( OUString* )0 ) ) )
				{
					OUString aString;
					sal_Int32 nLen = nSize;
					if ( nRequestedType == kQTFileTypeText )
					{
						HLock( hData );
						if ( ( (sal_Char *)*hData )[ nLen - 1 ] == 0 )
							nLen--;
						CFStringRef aCFString = CFStringCreateWithBytes( kCFAllocatorDefault, (const UInt8 *)*hData, nLen, CFStringGetSystemEncoding(), false );
						HUnlock( hData );

						if ( aCFString )
						{
							CFRange aRange;
							aRange.location = 0;
							aRange.length = CFStringGetLength( aCFString );
							// [ed 3/24/07 Place on to heap, not stack.  Bug #2171
							UniChar *aBuf = new UniChar[ aRange.length ];
							if ( aBuf )
							{
								CFStringGetCharacters( aCFString, aRange, aBuf );
								aString = OUString( (sal_Unicode *)aBuf, aRange.length );
								CFRelease( aCFString );
								delete[] aBuf;
							}
						}
					}
					else
					{
						HLock( hData );
						nLen = nSize / 2;
						if ( ( (sal_Unicode *)*hData )[ nLen - 1 ] == 0 )
							nLen--;
						aString = OUString( (sal_Unicode *)*hData, nLen );
						HUnlock( hData );
					}

					// Replace carriage returns with line feeds
					aString = aString.replace( (sal_Unicode)'\r', (sal_Unicode)'\n' );

					out <<= aString;
				}
				else if ( aFlavor.DataType.equals( getCppuType( ( Sequence< sal_Int8 >* )0 ) ) )
				{
					if ( !bRequestedTypeIsText )
					{
						// Convert to PNG format
						if ( nRequestedType == kQTFileTypePNG )
						{
							HLock( hData );
							Sequence< sal_Int8 > aExportData( nSize );
							memcpy( aExportData.getArray(), *hData, nSize );
							HUnlock( hData );
							out <<= aExportData;
						}
						else if ( nRequestedType == kQTFileTypePicture )
						{
							ComponentInstance aExporter;
							if ( OpenADefaultComponent( GraphicsExporterComponentType, kQTFileTypePNG, &aExporter ) == noErr )
							{
								if ( GraphicsExportSetInputPicture( aExporter, (PicHandle)hData ) == noErr )
								{
									Handle hExportData = NewHandle( 0 );
									if ( GraphicsExportSetOutputHandle( aExporter, hExportData ) == noErr )
									{
										ULONG nDataLen;
										if ( GraphicsExportDoExport( aExporter, &nDataLen ) == noErr )
										{
											Sequence< sal_Int8 > aExportData( nDataLen );
											HLock( hExportData );
											memcpy( aExportData.getArray(), *hExportData, nDataLen );
											HUnlock( hExportData );
											out <<= aExportData;
										}
									}

									DisposeHandle( hExportData );
								}

								CloseComponent( aExporter );
							}
						}
						else
						{
							ComponentInstance aImporter;
							if ( OpenADefaultComponent( GraphicsImporterComponentType, nRequestedType, &aImporter ) == noErr )
							{
								Rect aBounds;
								if ( GraphicsImportSetDataHandle( aImporter, hData ) == noErr && GraphicsImportGetNaturalBounds( aImporter, &aBounds ) == noErr )
								{
									GWorldPtr aGWorld;
									if ( QTNewGWorld( &aGWorld, k32ARGBPixelFormat, &aBounds, NULL, NULL, 0 ) == noErr )
									{
										if ( GraphicsImportSetGWorld( aImporter, aGWorld, NULL ) == noErr && GraphicsImportDraw( aImporter ) == noErr )
										{
											ComponentInstance aExporter;
											if ( OpenADefaultComponent( GraphicsExporterComponentType, kQTFileTypePNG, &aExporter ) == noErr )
											{
												if ( GraphicsExportSetInputGWorld( aExporter, aGWorld ) == noErr )
												{
													Handle hExportData = NewHandle( 0 );
													if ( GraphicsExportSetOutputHandle( aExporter, hExportData ) == noErr )
													{
														ULONG nDataLen;
														if ( GraphicsExportDoExport( aExporter, &nDataLen ) == noErr )
														{
															Sequence< sal_Int8 > aExportData( nDataLen );
															HLock( hExportData );
															memcpy( aExportData.getArray(), *hExportData, nDataLen );
															HUnlock( hExportData );
															out <<= aExportData;
														}
													}

													DisposeHandle( hExportData );
												}

												CloseComponent( aExporter );
											}
										}

										DisposeGWorld( aGWorld );
									}
								}

								CloseComponent( aImporter );
							}
						}
					}
					else
					{
						HLock( hData );
						sal_Int32 nLen = nSize;
						if ( ( (sal_Char *)*hData )[ nLen - 1 ] == 0 )
							nLen--;
						Sequence< sal_Int8 > aExportData( nLen );
						memcpy( aExportData.getArray(), *hData, nLen );
						HUnlock( hData );

						// Replace carriage returns with line feeds
						sal_Char *pArray = (sal_Char *)aExportData.getArray();
						if ( pArray )
						{
							for ( int j = 0; j < nLen; j++ )
							{
								if ( pArray[ j ] == '\r' )
									pArray[ j ] = '\n';
							}
						}

						// Strip out HTML Microsoft Office headers
						if ( nRequestedType == 'HTML' && isHTMLFormat( aExportData ) )
							aExportData = HTMLFormatToTextHtml( aExportData );

						out <<= aExportData;
					}
				}

				// Force a break from the loop
				i = nSupportedTypes;
			}

			DisposeHandle( hData );
		}
	}

	if ( !nRequestedType )
	{
		if ( nErr == noTypeErr || nErr == cantGetFlavorErr )
			throw UnsupportedFlavorException( aFlavor.MimeType, static_cast< XTransferable * >( this ) );
		else
			throw IOException( aFlavor.MimeType, static_cast< XTransferable * >( this ) );
	}

	return out;
}

// ----------------------------------------------------------------------------

DTransTransferable::DTransTransferable( void *pNativeTransferable, int nTransferableType, sal_uInt16 nItem ) :
	mpNativeTransferable( pNativeTransferable ),
	mnTransferableType( nTransferableType ),
	mnItem( 0 )
{
	if ( mnTransferableType == TRANSFERABLE_TYPE_DRAG && nItem )
	{
		UInt16 nCount = 0;
		if ( CountDragItems( (DragRef)mpNativeTransferable, &nCount ) == noErr && nItem <= nCount )
			mnItem = nItem;
	}
}

// ----------------------------------------------------------------------------

DTransTransferable::~DTransTransferable()
{
	aTransferableList.remove( this );
}

// ----------------------------------------------------------------------------

Sequence< DataFlavor > DTransTransferable::getTransferDataFlavors() throw ( RuntimeException )
{
	if ( mxTransferable.is() )
		return mxTransferable->getTransferDataFlavors();

	Sequence< DataFlavor > out;

	if ( mnTransferableType == TRANSFERABLE_TYPE_CLIPBOARD )
	{
		UInt32 nCount;
		if ( GetScrapFlavorCount( (ScrapRef)mpNativeTransferable, &nCount ) == noErr && nCount > 0 )
		{
			ScrapFlavorInfo *pInfo = new ScrapFlavorInfo[ nCount ];

			if ( GetScrapFlavorInfoList( (ScrapRef)mpNativeTransferable, &nCount, pInfo ) == noErr )
			{
				for ( USHORT i = 0; i < nSupportedTypes; i++ )
				{
					for ( UInt32 j = 0; j < nCount; j++ )
					{
						if ( aSupportedNativeTypes[ i ] == pInfo[ j ].flavorType )
						{
							DataFlavor aFlavor;
							aFlavor.MimeType = aSupportedMimeTypes[ i ];
							aFlavor.DataType = aSupportedDataTypes[ i ];
							sal_Int32 nLen = out.getLength();
							out.realloc( nLen + 1 );
							out[ nLen ] = aFlavor;
						}
					}
				}
			}

			delete[] pInfo;
		}
	}
	else if ( mnTransferableType == TRANSFERABLE_TYPE_DRAG && mnItem )
	{
		DragItemRef aItem;
		UInt16 nCount;
		if ( GetDragItemReferenceNumber( (DragRef)mpNativeTransferable, mnItem, &aItem ) == noErr && CountDragItemFlavors( (DragRef)mpNativeTransferable, aItem, &nCount ) == noErr && nCount > 0 )
		{
			for ( USHORT i = 0; i < nSupportedTypes; i++ )
			{
				for ( UInt16 j = 0; j < nCount; j++ )
				{
					FlavorType nFlavor;
					if ( GetFlavorType( (DragRef)mpNativeTransferable, aItem, j, &nFlavor ) == noErr && aSupportedNativeTypes[ i ] == nFlavor )
					{
						DataFlavor aFlavor;
						aFlavor.MimeType = aSupportedMimeTypes[ i ];
						aFlavor.DataType = aSupportedDataTypes[ i ];
						sal_Int32 nLen = out.getLength();
						out.realloc( nLen + 1 );
						out[ nLen ] = aFlavor;
					}
				}
			}
		}
	}

	return out;
}

// ----------------------------------------------------------------------------

sal_Bool DTransTransferable::hasOwnership()
{
	sal_Bool out = sal_False;

	if ( mnTransferableType == TRANSFERABLE_TYPE_CLIPBOARD )
	{
		ScrapRef aScrap;

		if ( GetCurrentScrap( &aScrap ) == noErr && aScrap == (ScrapRef)mpNativeTransferable )
			out = sal_True;
	}

	return out;
}

// ----------------------------------------------------------------------------

sal_Bool DTransTransferable::isDataFlavorSupported( const DataFlavor& aFlavor ) throw ( RuntimeException )
{
	if ( mxTransferable.is() )
		return mxTransferable->isDataFlavorSupported( aFlavor );

	sal_Bool out = sal_False;

	FourCharCode nRequestedType;
	memset( &nRequestedType, 0, sizeof( FourCharCode ) );
	Type aRequestedDataType;
	for ( USHORT i = 0; i < nSupportedTypes; i++ )
	{
		if ( aFlavor.MimeType.equalsIgnoreAsciiCase( aSupportedMimeTypes[ i ] ) )
		{
			nRequestedType = aSupportedNativeTypes[ i ];
			aRequestedDataType = aSupportedDataTypes[ i ];
			break;
		}
	}

	if ( nRequestedType )
	{
		if ( mnTransferableType == TRANSFERABLE_TYPE_CLIPBOARD )
		{
			UInt32 nCount;
			if ( GetScrapFlavorCount( (ScrapRef)mpNativeTransferable, &nCount ) == noErr && nCount > 0 )
			{
				ScrapFlavorInfo *pInfo = new ScrapFlavorInfo[ nCount ];

				if ( GetScrapFlavorInfoList( (ScrapRef)mpNativeTransferable, &nCount, pInfo ) == noErr )
				{
					for ( UInt32 i = 0; i < nCount; i++ )
					{
						if ( pInfo[ i ].flavorType == nRequestedType && aFlavor.DataType.equals( aRequestedDataType ) )
						{
							out = sal_True;
							break;
						}
					}
				}

				delete[] pInfo;
			}
		}
		else if ( mnTransferableType == TRANSFERABLE_TYPE_DRAG )
		{
			DragItemRef aItem;
			UInt16 nCount;
			if ( GetDragItemReferenceNumber( (DragRef)mpNativeTransferable, 1, &aItem ) == noErr && CountDragItemFlavors( (DragRef)mpNativeTransferable, aItem, &nCount ) == noErr && nCount > 0 )
			{
				for ( UInt16 i = 0; i < nCount; i++ )
				{
					FlavorType nFlavor;
					if ( GetFlavorType( (DragRef)mpNativeTransferable, aItem, i, &nFlavor ) == noErr && nFlavor == nRequestedType && aFlavor.DataType.equals( aRequestedDataType ) )
					{
						out = sal_True;
						break;
					}
				}
			}
		}
	}

	return out;
}

// ----------------------------------------------------------------------------

sal_Bool DTransTransferable::setContents( const Reference< XTransferable > &xTransferable )
{
	sal_Bool out = sal_False;

	mxTransferable = xTransferable;
	if ( mxTransferable.is() )
	{
		if ( mnTransferableType == TRANSFERABLE_TYPE_CLIPBOARD )
		{
			ScrapRef aScrap;
			if ( ClearCurrentScrap() == noErr && GetCurrentScrap( &aScrap ) == noErr )
			{
				// We have now cleared the scrap so we now own it
				mpNativeTransferable = aScrap;
				out = sal_True;

				Sequence< DataFlavor > xFlavors;
				try
				{
					xFlavors = mxTransferable->getTransferDataFlavors();
				}
				catch ( ... )
				{
				}

				// Check if text flavors are supported, if so, exclude any
				// image flavors since we would be just passing a picture
				// of text
				sal_Int32 nLen = xFlavors.getLength();
				bool bTextOnly = false;
				sal_Int32 i;
				for ( i = 0; i < nLen; i++ )
				{
					for ( USHORT j = 0; j < nSupportedTypes; j++ )
					{
						if ( xFlavors[ i ].MimeType.equalsIgnoreAsciiCase( aSupportedMimeTypes[ j ] ) && aSupportedTextTypes[ j ] )
						{
							bTextOnly = true;
							break;
						}
					}
				}

				aTransferableList.push_back( this );

				bool bRenderImmediately = false;
				if ( !pScrapPromiseKeeperUPP )
					pScrapPromiseKeeperUPP = NewScrapPromiseKeeperUPP( (ScrapPromiseKeeperProcPtr)ImplScrapPromiseKeeperCallback );
				if ( !pScrapPromiseKeeperUPP || SetScrapPromiseKeeper( (ScrapRef)mpNativeTransferable, pScrapPromiseKeeperUPP, (const void *)this ) != noErr )
					bRenderImmediately = true;

				for ( i = 0; i < nLen; i++ )
				{ 
					for ( USHORT j = 0; j < nSupportedTypes; j++ )
					{
						if ( xFlavors[ i ].MimeType.equalsIgnoreAsciiCase( aSupportedMimeTypes[ j ] ) )
						{
							if ( bTextOnly && !aSupportedTextTypes[ j ] )
								continue;

							// Converting to PDF doesn't work (only converting
							// from PDF works) so don't add the PDF flavor
							if ( aSupportedNativeTypes[ j ] == kQTFileTypePDF )
								continue;

							if ( bRenderImmediately )
								ImplScrapPromiseKeeperCallback( (ScrapRef)mpNativeTransferable, aSupportedNativeTypes[ j ], (void *)this );
							else
								 PutScrapFlavor( (ScrapRef)mpNativeTransferable, aSupportedNativeTypes[ j ], kScrapFlavorMaskNone, kScrapFlavorSizeUnknown, NULL );
						}
					}
				}
			}
		}
		else if ( mnTransferableType == TRANSFERABLE_TYPE_DRAG )
		{
			out = sal_True;

			Sequence< DataFlavor > xFlavors;
			try
			{
				xFlavors = mxTransferable->getTransferDataFlavors();
			}
			catch ( ... )
			{
			}

			// Check if text flavors are supported, if so, exclude any
			// image flavors since we would be just passing a picture
			// of text
			sal_Int32 nLen = xFlavors.getLength();
			bool bTextOnly = false;
			sal_Int32 i;
			for ( i = 0; i < nLen; i++ )
			{
				for ( USHORT j = 0; j < nSupportedTypes; j++ )
				{
					if ( xFlavors[ i ].MimeType.equalsIgnoreAsciiCase( aSupportedMimeTypes[ j ] ) && aSupportedTextTypes[ j ] )
					{
						bTextOnly = true;
						break;
					}
				}
			}

			aTransferableList.push_back( this );

			bool bRenderImmediately = false;
			if ( !pDragSendDataUPP )
				pDragSendDataUPP = NewDragSendDataUPP( (DragSendDataProcPtr)ImplDragSendDataCallback );
			if ( !pDragSendDataUPP || SetDragSendProc( (DragRef)mpNativeTransferable, pDragSendDataUPP, (void *)this ) != noErr )
				bRenderImmediately = true;

			for ( i = 0; i < nLen; i++ )
			{ 
				for ( USHORT j = 0; j < nSupportedTypes; j++ )
				{
					if ( xFlavors[ i ].MimeType.equalsIgnoreAsciiCase( aSupportedMimeTypes[ j ] ) )
					{
						if ( bTextOnly && !aSupportedTextTypes[ j ] )
							continue;

						AddDragItemFlavor( (DragRef)mpNativeTransferable, (DragItemRef)this, aSupportedNativeTypes[ j ], NULL, 0, 0 );

						// Set item to the last item
						UInt16 nCount = 0;
						if ( CountDragItems( (DragRef)mpNativeTransferable, &nCount ) == noErr && nCount )
							mnItem = nCount;

						if ( bRenderImmediately )
							ImplDragSendDataCallback( aSupportedNativeTypes[ j ], (void *)this, (DragItemRef)this, (DragRef)mpNativeTransferable );
					}
				}
			}
		}
	}

	return out;
}
