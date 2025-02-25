/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 *
 * This file incorporates work covered by the following license notice:
 *
 *   Licensed to the Apache Software Foundation (ASF) under one or more
 *   contributor license agreements. See the NOTICE file distributed
 *   with this work for additional information regarding copyright
 *   ownership. The ASF licenses this file to you under the Apache
 *   License, Version 2.0 (the "License"); you may not use this file
 *   except in compliance with the License. You may obtain a copy of
 *   the License at http://www.apache.org/licenses/LICENSE-2.0 .
 * 
 *   Modified November 2016 by Patrick Luby. NeoOffice is only distributed
 *   under the GNU General Public License, Version 3 as allowed by Section 3.3
 *   of the Mozilla Public License, v. 2.0.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifdef UNX
#include <pwd.h>
#include <sys/types.h>
#endif

#include <svtools/inettbc.hxx>
#include <com/sun/star/uno/Any.hxx>
#include <com/sun/star/uno/Reference.hxx>
#include <com/sun/star/beans/Property.hpp>
#include <com/sun/star/beans/PropertyValue.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>
#include <com/sun/star/sdbc/XResultSet.hpp>
#include <com/sun/star/sdbc/XRow.hpp>
#include <com/sun/star/task/XInteractionHandler.hpp>
#include <com/sun/star/ucb/NumberedSortingInfo.hpp>
#include <com/sun/star/ucb/UniversalContentBroker.hpp>
#include <com/sun/star/ucb/XAnyCompareFactory.hpp>
#include <com/sun/star/ucb/XCommandProcessor2.hpp>
#include <com/sun/star/ucb/XProgressHandler.hpp>
#include <com/sun/star/ucb/XContentAccess.hpp>
#include <com/sun/star/ucb/SortedDynamicResultSetFactory.hpp>
#include <comphelper/processfactory.hxx>
#include <comphelper/string.hxx>
#include <rtl/instance.hxx>
#include <salhelper/thread.hxx>
#include <osl/mutex.hxx>
#include <vcl/builder.hxx>
#include <vcl/svapp.hxx>
#include <vcl/toolbox.hxx>
#include <unotools/historyoptions.hxx>
#include <svl/eitem.hxx>
#include <svl/stritem.hxx>
#include <svl/itemset.hxx>
#include <svl/urihelper.hxx>
#include <unotools/pathoptions.hxx>
#include <ucbhelper/commandenvironment.hxx>
#include <ucbhelper/content.hxx>
#include <unotools/localfilehelper.hxx>
#include <unotools/ucbhelper.hxx>
#include <svtools/asynclink.hxx>
#include <svl/urlfilter.hxx>

#include <vector>
#include <algorithm>

#if defined USE_JAVA && defined MACOSX

#include <osl/file.hxx>
#include <sys/stat.h>

#endif	// USE_JAVA && MACOSX

using namespace ::ucbhelper;
using namespace ::utl;
using namespace ::com::sun::star;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::sdbc;
using namespace ::com::sun::star::task;
using namespace ::com::sun::star::ucb;
using namespace ::com::sun::star::uno;
using namespace ::boost::placeholders;

class SvtURLBox_Impl
{
public:
    std::vector<OUString>      aURLs;
    std::vector<OUString>      aCompletions;
    std::vector<WildCard>      m_aFilters;

    static bool TildeParsing( OUString& aText, OUString& aBaseUrl );

    inline SvtURLBox_Impl( )
    {
        FilterMatch::createWildCardFilterList(OUString(),m_aFilters);
    }
};

class SvtMatchContext_Impl: public salhelper::Thread
{
    static ::osl::Mutex*            pDirMutex;

    std::vector<OUString>      aPickList;
    std::vector<OUString>      aCompletions;
    std::vector<OUString>      aURLs;
    svtools::AsynchronLink          aLink;
    OUString                        aBaseURL;
    OUString                        aText;
    SvtURLBox*                      pBox;
    bool                            bOnlyDirectories;
    bool                            bNoSelection;

    osl::Mutex mutex_;
    bool stopped_;
    css::uno::Reference< css::ucb::XCommandProcessor > processor_;
    sal_Int32 commandId_;

    DECL_STATIC_LINK(               SvtMatchContext_Impl, Select_Impl, void* );

    virtual                         ~SvtMatchContext_Impl();
    virtual void                    execute() SAL_OVERRIDE;
    void                            doExecute();
    void                            Insert( const OUString& rCompletion, const OUString& rURL, bool bForce = false);
    void                            ReadFolder( const OUString& rURL, const OUString& rMatch, bool bSmart );
    void                            FillPicklist(std::vector<OUString>& rPickList);

public:
                                    SvtMatchContext_Impl( SvtURLBox* pBoxP, const OUString& rText );
    void                            Stop();
};

namespace
{
    struct theSvtMatchContextMutex
        : public rtl::Static< ::osl::Mutex, theSvtMatchContextMutex > {};
}

SvtMatchContext_Impl::SvtMatchContext_Impl(
    SvtURLBox* pBoxP, const OUString& rText )
    : Thread( "SvtMatchContext_Impl" )
    , aLink( STATIC_LINK( this, SvtMatchContext_Impl, Select_Impl ) )
    , aBaseURL( pBoxP->aBaseURL )
    , aText( rText )
    , pBox( pBoxP )
    , bOnlyDirectories( pBoxP->bOnlyDirectories )
    , bNoSelection( pBoxP->bNoSelection )
    , stopped_(false)
    , commandId_(0)
{
    aLink.CreateMutex();

    FillPicklist( aPickList );
}

SvtMatchContext_Impl::~SvtMatchContext_Impl()
{
    aLink.ClearPendingCall();
}

void SvtMatchContext_Impl::FillPicklist(std::vector<OUString>& rPickList)
{
    // Einlesung der Historypickliste
    Sequence< Sequence< PropertyValue > > seqPicklist = SvtHistoryOptions().GetList( ePICKLIST );
    sal_uInt32 nCount = seqPicklist.getLength();

    for( sal_uInt32 nItem=0; nItem < nCount; nItem++ )
    {
        Sequence< PropertyValue > seqPropertySet = seqPicklist[ nItem ];

        OUString sTitle;
        INetURLObject aURL;

        sal_uInt32 nPropertyCount = seqPropertySet.getLength();

        for( sal_uInt32 nProperty=0; nProperty < nPropertyCount; nProperty++ )
        {
            if( seqPropertySet[nProperty].Name == HISTORY_PROPERTYNAME_TITLE )
            {
                seqPropertySet[nProperty].Value >>= sTitle;
                aURL.SetURL( sTitle );
                rPickList.insert(rPickList.begin() + nItem, aURL.GetMainURL(INetURLObject::DECODE_WITH_CHARSET));
                break;
            }
        }
    }
}

void SvtMatchContext_Impl::Stop()
{
    css::uno::Reference< css::ucb::XCommandProcessor > proc;
    sal_Int32 id(0);
    {
        osl::MutexGuard g(mutex_);
        if (!stopped_) {
            stopped_ = true;
            proc = processor_;
            id = commandId_;
        }
    }
    if (proc.is()) {
        proc->abort(id);
    }
    terminate();
}

void SvtMatchContext_Impl::execute( )
{
    doExecute();
    aLink.Call( this );
}


// This method is called via AsynchronLink, so it has the SolarMutex and
// calling solar code ( VCL ... ) is safe. It is called when the thread is
// terminated ( finished work or stopped ). Cancelling the thread via
// Cancellable does not discard the information gained so far, it
// inserts all collected completions into the listbox.

IMPL_STATIC_LINK( SvtMatchContext_Impl, Select_Impl, void*, )
{
    // avoid recursion through cancel button
    {
        osl::MutexGuard g(pThis->mutex_);
        if (pThis->stopped_) {
            // Completion was stopped, no display:
            return 0;
        }
    }

    SvtURLBox* pBox = pThis->pBox;
    pBox->bAutoCompleteMode = true;

    // did we filter completions which otherwise would have been valid?
    // (to be filled below)
    bool bValidCompletionsFiltered = false;

    // insert all completed strings into the listbox
    pBox->Clear();

    for(std::vector<OUString>::iterator i = pThis->aCompletions.begin(); i != pThis->aCompletions.end(); ++i)
    {
        OUString sCompletion(*i);

        // convert the file into an URL
        OUString sURL( sCompletion );
        ::utl::LocalFileHelper::ConvertPhysicalNameToURL( sCompletion, sURL );
            // note: if this doesn't work, we're not interested in: we're checking the
            // untouched sCompletion then

        if ( !sURL.isEmpty() && !sURL.endsWith("/") )
        {
            OUString sUpperURL( sURL.toAsciiUpperCase() );

            ::std::vector< WildCard >::const_iterator aMatchingFilter =
                ::std::find_if(
                    pBox->pImp->m_aFilters.begin(),
                    pBox->pImp->m_aFilters.end(),
                    FilterMatch( sUpperURL )
                );
            if ( aMatchingFilter == pBox->pImp->m_aFilters.end() )

            {   // this URL is not allowed
                bValidCompletionsFiltered = true;
                continue;
            }
        }

        pBox->InsertEntry( sCompletion );
    }

    if( !pThis->bNoSelection && !pThis->aCompletions.empty() && !bValidCompletionsFiltered )
    {
        // select the first one
        OUString aTmp( pBox->GetEntry(0) );
        pBox->SetText( aTmp );
        pBox->SetSelection( Selection( pThis->aText.getLength(), aTmp.getLength() ) );
    }

    // transfer string lists to listbox and forget them
    pBox->pImp->aURLs = pThis->aURLs;
    pBox->pImp->aCompletions = pThis->aCompletions;
    pThis->aURLs.clear();
    pThis->aCompletions.clear();

    // force listbox to resize ( it may be open )
    pBox->Resize();

    // the box has this control as a member so we have to set that member
    // to zero before deleting ourself.
    pBox->pCtx.clear();

    return 0;
}


void SvtMatchContext_Impl::Insert( const OUString& rCompletion,
                                   const OUString& rURL,
                                   bool bForce )
{
    if( !bForce )
    {
        // avoid doubles
        if(find(aCompletions.begin(), aCompletions.end(), OUString(rCompletion)) != aCompletions.end())
            return;
    }

    aCompletions.push_back(rCompletion);
    aURLs.push_back(rURL);
}


void SvtMatchContext_Impl::ReadFolder( const OUString& rURL,
                                       const OUString& rMatch,
                                       bool bSmart )
{
    // check folder to scan
#if defined USE_JAVA && defined MACOSX
    // Eliminate sandbox deny file-read-data messages by checking if the
    // URL is a directory. If the path is inaccessible or contains an alias,
    // skip display of the
    // native Open dialog and treat path as not a directory.
    OUString aSystemPath;
    struct stat aSystemPathStat;
    if ( osl::FileBase::getSystemPathFromFileURL( rURL, aSystemPath ) != osl::FileBase::E_None || stat( OUStringToOString( aSystemPath, osl_getThreadTextEncoding() ).getStr(), &aSystemPathStat ) || !S_ISDIR( aSystemPathStat.st_mode ) || access( OUStringToOString( aSystemPath, osl_getThreadTextEncoding() ).getStr(), R_OK ) )
#else	// USE_JAVA && MACOSX
    if( !UCBContentHelper::IsFolder( rURL ) )
#endif	// USE_JAVA && MACOSX
        return;

    bool bPureHomePath = false;
#ifdef UNX
    bPureHomePath = aText.startsWith( "~" ) && aText.indexOf( '/' ) == -1;
#endif

    bool bExectMatch = bPureHomePath
                || aText == "."
                || aText.endsWith("/.")
                || aText.endsWith("/..");

    // for pure home paths ( ~username ) the '.' at the end of rMatch
    // means that it poits to root catalog
    // this is done only for file contents since home paths parsing is useful only for them
    if ( bPureHomePath && rMatch == "file:///." )
    {
        // a home that refers to /

        OUString aNewText( aText );
        aNewText += "/";
        Insert( aNewText, rURL, true );

        return;
    }

    // string to match with
    INetURLObject aMatchObj( rMatch );
    OUString aMatchName;

    if ( rURL != aMatchObj.GetMainURL( INetURLObject::NO_DECODE ) )
    {
        aMatchName = aMatchObj.getName( INetURLObject::LAST_SEGMENT, true, INetURLObject::DECODE_WITH_CHARSET );

        // matching is always done case insensitive, but completion will be case sensitive and case preserving
        aMatchName = aMatchName.toAsciiLowerCase();

        // if the matchstring ends with a slash, we must search for this also
        if ( rMatch.endsWith("/") )
            aMatchName += "/";
    }

    sal_Int32 nMatchLen = aMatchName.getLength();

    INetURLObject aFolderObj( rURL );
    DBG_ASSERT( aFolderObj.GetProtocol() != INET_PROT_NOT_VALID, "Invalid URL!" );

    try
    {
        Content aCnt( aFolderObj.GetMainURL( INetURLObject::NO_DECODE ),
                      new ::ucbhelper::CommandEnvironment( uno::Reference< XInteractionHandler >(),
                                                     uno::Reference< XProgressHandler >() ),
                      comphelper::getProcessComponentContext() );
        uno::Reference< XResultSet > xResultSet;
        Sequence< OUString > aProps(2);
        OUString* pProps = aProps.getArray();
        pProps[0] = "Title";
        pProps[1] = "IsFolder";

        try
        {
            uno::Reference< XDynamicResultSet > xDynResultSet;
            ResultSetInclude eInclude = INCLUDE_FOLDERS_AND_DOCUMENTS;
            if ( bOnlyDirectories )
                eInclude = INCLUDE_FOLDERS_ONLY;

            xDynResultSet = aCnt.createDynamicCursor( aProps, eInclude );

            uno::Reference < XAnyCompareFactory > xCompare;
            uno::Reference < XSortedDynamicResultSetFactory > xSRSFac =
                SortedDynamicResultSetFactory::create( ::comphelper::getProcessComponentContext() );

            Sequence< NumberedSortingInfo > aSortInfo( 2 );
            NumberedSortingInfo* pInfo = aSortInfo.getArray();
            pInfo[ 0 ].ColumnIndex = 2;
            pInfo[ 0 ].Ascending   = false;
            pInfo[ 1 ].ColumnIndex = 1;
            pInfo[ 1 ].Ascending   = true;

            uno::Reference< XDynamicResultSet > xDynamicResultSet;
            xDynamicResultSet =
                xSRSFac->createSortedDynamicResultSet( xDynResultSet, aSortInfo, xCompare );

            if ( xDynamicResultSet.is() )
            {
                xResultSet = xDynamicResultSet->getStaticResultSet();
            }
        }
        catch( ::com::sun::star::uno::Exception& ) {}

        if ( xResultSet.is() )
        {
            uno::Reference< XRow > xRow( xResultSet, UNO_QUERY );
            uno::Reference< XContentAccess > xContentAccess( xResultSet, UNO_QUERY );

            try
            {
                while ( schedule() && xResultSet->next() )
                {
                    OUString   aURL      = xContentAccess->queryContentIdentifierString();
                    OUString   aTitle    = xRow->getString(1);
                    bool   bIsFolder = xRow->getBoolean(2);

                    // matching is always done case insensitive, but completion will be case sensitive and case preserving
                    aTitle = aTitle.toAsciiLowerCase();

                    if (
                        !nMatchLen ||
                        (bExectMatch && aMatchName == aTitle) ||
                        (!bExectMatch && aMatchName.compareTo(aTitle, nMatchLen) == 0)
                       )
                    {
                        // all names fit if matchstring is empty
                        INetURLObject aObj( aURL );
                        sal_Unicode aDelimiter = '/';
                        if ( bSmart )
                            // when parsing is done "smart", the delimiter must be "guessed"
                            aObj.getFSysPath( (INetURLObject::FSysStyle)(INetURLObject::FSYS_DETECT & ~INetURLObject::FSYS_VOS), &aDelimiter );

                        if ( bIsFolder )
                            aObj.setFinalSlash();

                        // get the last name of the URL
                        OUString aMatch = aObj.getName( INetURLObject::LAST_SEGMENT, true, INetURLObject::DECODE_WITH_CHARSET );
                        OUString aInput( aText );
                        if ( nMatchLen )
                        {
                            if (aText.endsWith(".") || bPureHomePath)
                            {
                                // if a "special folder" URL was typed, don't touch the user input
                                aMatch = aMatch.copy( nMatchLen );
                            }
                            else
                            {
                                // make the user input case preserving
                                DBG_ASSERT( aInput.getLength() >= nMatchLen, "Suspicious Matching!" );
                                aInput = aInput.copy( 0, aInput.getLength() - nMatchLen );
                            }
                        }

                        aInput += aMatch;

                        // folders should get a final slash automatically
                        if ( bIsFolder )
                            aInput += OUString(aDelimiter);

                        Insert( aInput, aObj.GetMainURL( INetURLObject::NO_DECODE ), true );
                    }
                }
            }
            catch( ::com::sun::star::uno::Exception& )
            {
            }
        }
    }
    catch( ::com::sun::star::uno::Exception& )
    {
    }
}


OUString SvtURLBox::ParseSmart( const OUString& _aText, const OUString& _aBaseURL, const OUString& aWorkDir )
{
    OUString aMatch;
    OUString aText = _aText;
    OUString aBaseURL = _aBaseURL;

    // parse ~ for Unix systems
    // does nothing for Windows
    if( !SvtURLBox_Impl::TildeParsing( aText, aBaseURL ) )
        return OUString();

    if( !aBaseURL.isEmpty() )
    {
        INetProtocol eBaseProt = INetURLObject::CompareProtocolScheme( aBaseURL );

        // if a base URL is set the string may be parsed relative
        if( aText.startsWith( "/" ) )
        {
            // text starting with slashes means absolute file URLs
            OUString aTemp = INetURLObject::GetScheme( eBaseProt );

            // file URL must be correctly encoded!
            OUString aTextURL = INetURLObject::encode( aText, INetURLObject::PART_FPATH,
                                                     '%', INetURLObject::ENCODE_ALL );
            aTemp += aTextURL;

            INetURLObject aTmp( aTemp );
            if ( !aTmp.HasError() && aTmp.GetProtocol() != INET_PROT_NOT_VALID )
                aMatch = aTmp.GetMainURL( INetURLObject::NO_DECODE );
        }
        else
        {
            OUString aSmart( aText );
            INetURLObject aObj( aBaseURL );

            // HRO: I suppose this hack should only be done for Windows !!!???
#ifdef WNT
            // HRO: INetURLObject::smatRel2Abs does not recognize '\\' as a relative path
            //      but in case of "\\\\" INetURLObject is right - this is an absolute path !

            if( aText.startsWith("\\") && (aText.getLength() < 2 || aText[ 1 ] != '\\') )
            {
                // cut to first segment
                OUString aTmp = INetURLObject::GetScheme( eBaseProt );
                aTmp += "/";
                aTmp += aObj.getName( 0, true, INetURLObject::DECODE_WITH_CHARSET );
                aObj.SetURL( aTmp );

                aSmart = aSmart.copy(1);
            }
#endif
            // base URL must be a directory !
            aObj.setFinalSlash();

            // take base URL and append current input
            bool bWasAbsolute = false;
#ifdef UNX
            INetURLObject::FSysStyle eStyle = static_cast< INetURLObject::FSysStyle >( INetURLObject::FSYS_VOS | INetURLObject::FSYS_UNX | INetURLObject::FSYS_DOS );
            // encode file URL correctly
            aSmart = INetURLObject::encode( aSmart, INetURLObject::PART_FPATH, '%', INetURLObject::ENCODE_ALL );
            INetURLObject aTmp( aObj.smartRel2Abs(
                aSmart, bWasAbsolute, false, INetURLObject::WAS_ENCODED, RTL_TEXTENCODING_UTF8, false, eStyle ) );
#else
            INetURLObject aTmp( aObj.smartRel2Abs( aSmart, bWasAbsolute ) );
#endif

            if ( aText.endsWith(".") )
                // INetURLObject appends a final slash for the directories "." and "..", this is a bug!
                // Remove it as a workaround
                aTmp.removeFinalSlash();
            if ( !aTmp.HasError() && aTmp.GetProtocol() != INET_PROT_NOT_VALID )
                aMatch = aTmp.GetMainURL( INetURLObject::NO_DECODE );
        }
    }
    else
    {
        OUString aTmpMatch;
        ::utl::LocalFileHelper::ConvertSystemPathToURL( aText, aWorkDir, aTmpMatch );
        aMatch = aTmpMatch;
    }

    return aMatch;
}


void SvtMatchContext_Impl::doExecute()
{
    ::osl::MutexGuard aGuard( theSvtMatchContextMutex::get() );
    {
        // have we been stopped while we were waiting for the mutex?
        osl::MutexGuard g(mutex_);
        if (stopped_) {
            return;
        }
    }

    // Reset match lists
    aCompletions.clear();
    aURLs.clear();

    // check for input
    sal_uInt16 nTextLen = aText.getLength();
    if ( !nTextLen )
        return;

    if( aText.indexOf( '*' ) != -1 || aText.indexOf( '?' ) != -1 )
        // no autocompletion for wildcards
        return;

    OUString aMatch;
    OUString aWorkDir( SvtPathOptions().GetWorkPath() );
    INetProtocol eProt = INetURLObject::CompareProtocolScheme( aText );
    INetProtocol eBaseProt = INetURLObject::CompareProtocolScheme( aBaseURL );
    if ( aBaseURL.isEmpty() )
        eBaseProt = INetURLObject::CompareProtocolScheme( aWorkDir );
    INetProtocol eSmartProt = pBox->GetSmartProtocol();

    // if the user input is a valid URL, go on with it
    // otherwise it could be parsed smart with a predefined smart protocol
    // ( or if this is not set with the protocol of a predefined base URL )
    if( eProt == INET_PROT_NOT_VALID || eProt == eSmartProt || (eSmartProt == INET_PROT_NOT_VALID && eProt == eBaseProt) )
    {
        // not stopped yet ?
        if( schedule() )
        {
            if ( eProt == INET_PROT_NOT_VALID )
                aMatch = SvtURLBox::ParseSmart( aText, aBaseURL, aWorkDir );
            else
                aMatch = aText;
            if ( !aMatch.isEmpty() )
            {
                INetURLObject aURLObject( aMatch );
                OUString aMainURL( aURLObject.GetMainURL( INetURLObject::NO_DECODE ) );
                // Disable autocompletion for anything but the (local) file
                // system (for which access is hopefully fast), as the logic of
                // how SvtMatchContext_Impl is used requires this code to run to
                // completion before further user input is processed, and even
                // SvtMatchContext_Impl::Stop does not guarantee a speedy
                // return:
                if ( !aMainURL.isEmpty()
                     && aURLObject.GetProtocol() == INET_PROT_FILE )
                {
                    // if text input is a directory, it must be part of the match list! Until then it is scanned
                    bool folder = false;
#if defined USE_JAVA && defined MACOSX
                    // Eliminate sandbox deny file-read-data messages by
                    // checking if the URL is a directory. If the path is
                    // inaccessible or contains an alias, skip display of the
                    // native Open dialog and treat path as not a directory.
                    OUString aSystemPath;
                    struct stat aSystemPathStat;
                    if (aURLObject.hasFinalSlash() && osl::FileBase::getSystemPathFromFileURL( aMainURL, aSystemPath ) == osl::FileBase::E_None && stat( OUStringToOString( aSystemPath, osl_getThreadTextEncoding() ).getStr(), &aSystemPathStat ) && S_ISDIR( aSystemPathStat.st_mode ) && !access( OUStringToOString( aSystemPath, osl_getThreadTextEncoding() ).getStr(), R_OK )) {
#else	// USE_JAVA && MACOSX
                    if (aURLObject.hasFinalSlash()) {
#endif	// USE_JAVA && MACOSX
                        try {
                            css::uno::Reference< css::uno::XComponentContext >
                                ctx(comphelper::getProcessComponentContext());
                            css::uno::Reference<
                                css::ucb::XUniversalContentBroker > ucb(
                                    css::ucb::UniversalContentBroker::create(
                                        ctx));
                            css::uno::Sequence< css::beans::Property > prop(1);
                            prop[0].Name = "IsFolder";
                            prop[0].Handle = -1;
                            prop[0].Type = cppu::UnoType< bool >::get();
                            css::uno::Any res;
                            css::uno::Reference< css::ucb::XCommandProcessor >
                                proc(
                                    ucb->queryContent(
                                        ucb->createContentIdentifier(aMainURL)),
                                    css::uno::UNO_QUERY_THROW);
                            css::uno::Reference< css::ucb::XCommandProcessor2 >
                                proc2(proc, css::uno::UNO_QUERY);
                            sal_Int32 id = proc->createCommandIdentifier();
                            try {
                                {
                                    osl::MutexGuard g(mutex_);
                                    processor_ = proc;
                                    commandId_ = id;
                                }
                                res = proc->execute(
                                    css::ucb::Command(
                                        "getPropertyValues", -1,
                                        css::uno::makeAny(prop)),
                                    id,
                                    css::uno::Reference<
                                        css::ucb::XCommandEnvironment >());
                            } catch (...) {
                                if (proc2.is()) {
                                    try {
                                        proc2->releaseCommandIdentifier(id);
                                    } catch (css::uno::RuntimeException & e) {
                                        SAL_WARN(
                                            "svtools.control",
                                            "ignoring UNO RuntimeException "
                                            << e.Message);
                                    }
                                }
                                throw;
                            }
                            if (proc2.is()) {
                                proc2->releaseCommandIdentifier(id);
                            }
                            {
                                osl::MutexGuard g(mutex_);
                                processor_.clear();
                                // At least the neon-based WebDAV UCP does not
                                // properly support aborting commands, so return
                                // anyway now if an abort request had been
                                // ignored and the command execution only
                                // returned "successfully" after some timeout:
                                if (stopped_) {
                                    return;
                                }
                            }
                            css::uno::Reference< css::sdbc::XRow > row(
                                res, css::uno::UNO_QUERY_THROW);
                            folder = row->getBoolean(1) && !row->wasNull();
                        } catch (css::uno::Exception & e) {
                            SAL_WARN(
                                "svtools.control",
                                "ignoring UNO Exception " << typeid(*&e).name()
                                << ": " << e.Message);
                            return;
                        }
                    }
                    if ( folder )
                            Insert( aText, aMatch );
                    else
                        // otherwise the parent folder will be taken
                        aURLObject.removeSegment();

                    // scan directory and insert all matches
                    ReadFolder( aURLObject.GetMainURL( INetURLObject::NO_DECODE ), aMatch, eProt == INET_PROT_NOT_VALID );
                }
            }
        }
    }

    if ( bOnlyDirectories )
        // don't scan history picklist if only directories are allowed, picklist contains only files
        return;

    bool bFull = false;

    INetURLObject aCurObj;
    OUString aEmpty, aCurString, aCurMainURL;
    INetURLObject aObj;
    aObj.SetSmartProtocol( eSmartProt == INET_PROT_NOT_VALID ? INET_PROT_HTTP : eSmartProt );
    for( ;; )
    {
        for(std::vector<OUString>::iterator i = aPickList.begin(); schedule() && i != aPickList.end(); ++i)
        {
            aCurObj.SetURL(*i);
            aCurObj.SetSmartURL( aCurObj.GetURLNoPass());
            aCurMainURL = aCurObj.GetMainURL( INetURLObject::NO_DECODE );

            if( eProt != INET_PROT_NOT_VALID && aCurObj.GetProtocol() != eProt )
                continue;

            if( eSmartProt != INET_PROT_NOT_VALID && aCurObj.GetProtocol() != eSmartProt )
                continue;

            switch( aCurObj.GetProtocol() )
            {
                case INET_PROT_HTTP:
                case INET_PROT_HTTPS:
                case INET_PROT_FTP:
                {
                    if( eProt == INET_PROT_NOT_VALID && !bFull )
                    {
                        aObj.SetSmartURL( aText );
                        if( aObj.GetURLPath().getLength() > 1 )
                            continue;
                    }

                    aCurString = aCurMainURL;
                    if( eProt == INET_PROT_NOT_VALID )
                    {
                        // try if text matches the scheme
                        OUString aScheme( INetURLObject::GetScheme( aCurObj.GetProtocol() ) );
                        if ( aScheme.startsWithIgnoreAsciiCase( aText ) && aText.getLength() < aScheme.getLength() )
                        {
                            if( bFull )
                                aMatch = aCurObj.GetMainURL( INetURLObject::NO_DECODE );
                            else
                            {
                                aCurObj.SetMark( aEmpty );
                                aCurObj.SetParam( aEmpty );
                                aCurObj.SetURLPath( aEmpty );
                                aMatch = aCurObj.GetMainURL( INetURLObject::NO_DECODE );
                            }

                            Insert( aMatch, aMatch );
                        }

                        // now try smart matching
                        aCurString = aCurString.copy( aScheme.getLength() );
                    }

                    if( aCurString.startsWithIgnoreAsciiCase( aText ) )
                    {
                        if( bFull )
                            aMatch = aCurObj.GetMainURL( INetURLObject::NO_DECODE );
                        else
                        {
                            aCurObj.SetMark( aEmpty );
                            aCurObj.SetParam( aEmpty );
                            aCurObj.SetURLPath( aEmpty );
                            aMatch = aCurObj.GetMainURL( INetURLObject::NO_DECODE );
                        }

                        OUString aURL( aMatch );
                        if( eProt == INET_PROT_NOT_VALID )
                            aMatch = aMatch.copy( INetURLObject::GetScheme( aCurObj.GetProtocol() ).getLength() );

                        if( aText.getLength() < aMatch.getLength() )
                            Insert( aMatch, aURL );

                        continue;
                    }
                    break;
                }
                default:
                {
                    if( bFull )
                        continue;

                    if( aCurMainURL.startsWith(aText) )
                    {
                        if( aText.getLength() < aCurMainURL.getLength() )
                            Insert( aCurMainURL, aCurMainURL );

                        continue;
                    }
                    break;
                }
            }
        }

        if( !bFull )
            bFull = true;
        else
            break;
    }

    return;
}

void SvtURLBox::TryAutoComplete()
{
    if( Application::AnyInput( VCL_INPUT_KEYBOARD ) ) return;

    OUString aCurText = GetText();
    Selection aSelection( GetSelection() );
    if( aSelection.Max() != aCurText.getLength() )
        return;
    sal_uInt16 nLen = (sal_uInt16)aSelection.Min();
    aCurText = aCurText.copy( 0, nLen );
    if( !aCurText.isEmpty() && bIsAutoCompleteEnabled )
    {
        if ( pCtx.is() )
        {
            pCtx->Stop();
            pCtx->join();
            pCtx.clear();
        }
        pCtx = new SvtMatchContext_Impl( this, aCurText );
        pCtx->launch();
    }
}


SvtURLBox::SvtURLBox( vcl::Window* pParent, INetProtocol eSmart, bool bSetDefaultHelpID )
    :   ComboBox( pParent , WB_DROPDOWN | WB_AUTOSIZE | WB_AUTOHSCROLL ),
        eSmartProtocol( eSmart ),
        bAutoCompleteMode( false ),
        bOnlyDirectories( false ),
        bCtrlClick( false ),
        bHistoryDisabled( false ),
        bNoSelection( false ),
        bIsAutoCompleteEnabled( true )
{
    Init(bSetDefaultHelpID);

    if ( GetDesktopRectPixel().GetWidth() > 800 )
        SetSizePixel( Size( 300, 240 ) );
    else
        SetSizePixel( Size( 225, 240 ) );
}


SvtURLBox::SvtURLBox( vcl::Window* pParent, WinBits _nStyle, INetProtocol eSmart,
    bool bSetDefaultHelpID )
    :   ComboBox( pParent, _nStyle ),
        eSmartProtocol( eSmart ),
        bAutoCompleteMode( false ),
        bOnlyDirectories( false ),
        bCtrlClick( false ),
        bHistoryDisabled( false ),
        bNoSelection( false ),
        bIsAutoCompleteEnabled( true )
{
    Init(bSetDefaultHelpID);
}

extern "C" SAL_DLLPUBLIC_EXPORT vcl::Window* SAL_CALL makeSvtURLBox(vcl::Window *pParent, VclBuilder::stringmap &)
{
    WinBits nWinBits = WB_LEFT|WB_VCENTER|WB_3DLOOK|WB_TABSTOP|
                       WB_DROPDOWN|WB_AUTOSIZE|WB_AUTOHSCROLL;
    SvtURLBox* pListBox = new SvtURLBox(pParent, nWinBits, INET_PROT_NOT_VALID, false);
    pListBox->EnableAutoSize(true);
    return pListBox;
}


SvtURLBox::SvtURLBox( vcl::Window* pParent, const ResId& _rResId, INetProtocol eSmart,
    bool bSetDefaultHelpID )
    :   ComboBox( pParent , _rResId ),
        eSmartProtocol( eSmart ),
        bAutoCompleteMode( false ),
        bOnlyDirectories( false ),
        bCtrlClick( false ),
        bHistoryDisabled( false ),
        bNoSelection( false ),
        bIsAutoCompleteEnabled( true )
{
    Init(bSetDefaultHelpID);
}

void SvtURLBox::Init(bool bSetDefaultHelpID)
{
    pImp = new SvtURLBox_Impl();

    if (bSetDefaultHelpID && GetHelpId().isEmpty())
        SetHelpId( ".uno:OpenURL" );
    EnableAutocomplete( false );

    SetText( OUString() );

    GetSubEdit()->autocompleteSignal.connect( boost::bind( &SvtURLBox::AutoCompleteHandler, this, _1 ) );
    UpdatePicklistForSmartProtocol_Impl();

    EnableAutoSize(GetStyle() & WB_AUTOSIZE);
}

SvtURLBox::~SvtURLBox()
{
    if( pCtx.is() )
    {
        pCtx->Stop();
        pCtx->join();
    }

    delete pImp;
}

void SvtURLBox::UpdatePickList( )
{
    if( pCtx.is() )
    {
        pCtx->Stop();
        pCtx->join();
        pCtx.clear();
    }

    OUString sText = GetText();
    if ( !sText.isEmpty() && bIsAutoCompleteEnabled )
    {
        pCtx = new SvtMatchContext_Impl( this, sText );
        pCtx->launch();
    }
}

void SvtURLBox::SetSmartProtocol( INetProtocol eProt )
{
    if ( eSmartProtocol != eProt )
    {
        eSmartProtocol = eProt;
        UpdatePicklistForSmartProtocol_Impl();
    }
}

void SvtURLBox::UpdatePicklistForSmartProtocol_Impl()
{
    Clear();
    if ( !bHistoryDisabled )
    {
        // read history pick list
        Sequence< Sequence< PropertyValue > > seqPicklist = SvtHistoryOptions().GetList( ePICKLIST );
        sal_uInt32 nCount = seqPicklist.getLength();
        INetURLObject aCurObj;

        for( sal_uInt32 nItem=0; nItem < nCount; nItem++ )
        {
            Sequence< PropertyValue > seqPropertySet = seqPicklist[ nItem ];

            OUString sURL;

            sal_uInt32 nPropertyCount = seqPropertySet.getLength();

            for( sal_uInt32 nProperty=0; nProperty < nPropertyCount; nProperty++ )
            {
                if( seqPropertySet[nProperty].Name == HISTORY_PROPERTYNAME_URL )
                {
                    seqPropertySet[nProperty].Value >>= sURL;
                    aCurObj.SetURL( sURL );

                    if ( !sURL.isEmpty() && ( eSmartProtocol != INET_PROT_NOT_VALID ) )
                    {
                        if( aCurObj.GetProtocol() != eSmartProtocol )
                            break;
                    }

                    OUString aURL( aCurObj.GetMainURL( INetURLObject::DECODE_WITH_CHARSET ) );

                    if ( !aURL.isEmpty() )
                    {
                        bool bFound = aURL.endsWith("/");
                        if ( !bFound )
                        {
                            OUString aUpperURL( aURL );
                            aUpperURL = aUpperURL.toAsciiUpperCase();

                            bFound
                                = (::std::find_if(
                                    pImp->m_aFilters.begin(),
                                    pImp->m_aFilters.end(),
                                    FilterMatch( aUpperURL ) )
                                        != pImp->m_aFilters.end());
                        }
                        if ( bFound )
                        {
                            OUString aFile;
                            if (::utl::LocalFileHelper::ConvertURLToSystemPath(aURL,aFile))
                                InsertEntry(aFile);
                            else
                                InsertEntry(aURL);
                        }
                    }
                    break;
                }
            }
        }
    }
}


bool SvtURLBox::ProcessKey( const vcl::KeyCode& rKey )
{
    // every key input stops the current matching thread
    if( pCtx.is() )
    {
        pCtx->Stop();
        pCtx->join();
        pCtx.clear();
    }

    vcl::KeyCode aCode( rKey.GetCode() );
    if ( aCode == KEY_RETURN && !GetText().isEmpty() )
    {
        // wait for completion of matching thread
        ::osl::MutexGuard aGuard( theSvtMatchContextMutex::get() );

        if ( bAutoCompleteMode )
        {
            // reset picklist
            bAutoCompleteMode = false;
            Selection aSelection( GetSelection() );
            SetSelection( Selection( aSelection.Min(), aSelection.Min() ) );
            if ( bOnlyDirectories )
                Clear();
            else
                UpdatePicklistForSmartProtocol_Impl();
            Resize();
        }

        bCtrlClick = rKey.IsMod1();
        bool bHandled = false;
        if ( GetOpenHdl().IsSet() )
        {
            bHandled = true;
            GetOpenHdl().Call(this);
        }
        else if ( GetSelectHdl().IsSet() )
        {
            bHandled = true;
            GetSelectHdl().Call(this);
        }

        bCtrlClick = false;

        ClearModifyFlag();
        return bHandled;
    }
    else if ( aCode == KEY_RETURN && GetText().isEmpty() && GetOpenHdl().IsSet() )
    {
        // for file dialog
        bAutoCompleteMode = false;
        GetOpenHdl().Call(this);
        return true;
    }
    else if( aCode == KEY_ESCAPE )
    {
        Selection aSelection( GetSelection() );
        if ( bAutoCompleteMode || aSelection.Min() != aSelection.Max() )
        {
            SetSelection( Selection( aSelection.Min(), aSelection.Min() ) );
            if ( bOnlyDirectories )
                Clear();
            else
                UpdatePicklistForSmartProtocol_Impl();
            Resize();
        }
        else
        {
           return false;
        }

        bAutoCompleteMode = false;
        return true;
    }
    else
    {
        return false;
    }
}


void SvtURLBox::Modify()
{
    ComboBox::Modify();
}


bool SvtURLBox::PreNotify( NotifyEvent& rNEvt )
{
    if( rNEvt.GetWindow() == GetSubEdit() && rNEvt.GetType() == EVENT_KEYINPUT )
    {

        const KeyEvent& rEvent = *rNEvt.GetKeyEvent();
        const vcl::KeyCode& rKey = rEvent.GetKeyCode();
        vcl::KeyCode aCode( rKey.GetCode() );
        if( ProcessKey( rKey ) )
        {
            return true;
        }
        else if( ( aCode == KEY_UP || aCode == KEY_DOWN ) && !rKey.IsMod2() )
        {
            Selection aSelection( GetSelection() );
            sal_uInt16 nLen = (sal_uInt16)aSelection.Min();
            GetSubEdit()->KeyInput( rEvent );
            SetSelection( Selection( nLen, GetText().getLength() ) );
            return true;
        }

        if ( MatchesPlaceHolder( GetText() ) )
        {
            // set the selection so a key stroke will overwrite
            // the placeholder rather than edit it
            SetSelection( Selection( 0, GetText().getLength() ) );
        }
    }

    return ComboBox::PreNotify( rNEvt );
}


void SvtURLBox::AutoCompleteHandler( Edit* )
{
    if ( GetSubEdit()->GetAutocompleteAction() == AUTOCOMPLETE_KEYINPUT )
        TryAutoComplete();
}


bool SvtURLBox::Notify( NotifyEvent &rEvt )
{
    if ( EVENT_GETFOCUS == rEvt.GetType() )
    {
#ifndef UNX
        // pb: don't select automatically on unix #93251#
        SetSelection( Selection( 0, GetText().getLength() ) );
#endif
    }
    else if ( EVENT_LOSEFOCUS == rEvt.GetType() )
    {
        if( GetText().isEmpty() )
            ClearModifyFlag();
        if ( pCtx.is() )
        {
            pCtx->Stop();
            pCtx->join();
            pCtx.clear();
        }
    }

    return ComboBox::Notify( rEvt );
}


void SvtURLBox::Select()
{
    ComboBox::Select();
    ClearModifyFlag();
}


void SvtURLBox::SetOnlyDirectories( bool bDir )
{
    bOnlyDirectories = bDir;
    if ( bOnlyDirectories )
        Clear();
}


void SvtURLBox::SetNoURLSelection( bool bSet )
{
    bNoSelection = bSet;
}


OUString SvtURLBox::GetURL()
{
    // wait for end of autocompletion
    ::osl::MutexGuard aGuard( theSvtMatchContextMutex::get() );

    OUString aText( GetText() );
    if ( MatchesPlaceHolder( aText ) )
        return aPlaceHolder;

    // try to get the right case preserving URL from the list of URLs
    for(std::vector<OUString>::iterator i = pImp->aCompletions.begin(), j = pImp->aURLs.begin(); i != pImp->aCompletions.end() && j != pImp->aURLs.end(); ++i, ++j)
    {
        if((*i).equals(aText))
            return *j;
    }

#ifdef WNT
    // erase trailing spaces on Windows since thay are invalid on this OS and
    // most of the time they are inserted by accident via copy / paste
    aText = comphelper::string::stripEnd(aText, ' ');
    if ( aText.isEmpty() )
        return aText;
    // #i9739#
#endif

    INetURLObject aObj( aText );
    if( aText.indexOf( '*' ) != -1 || aText.indexOf( '?' ) != -1 )
    {
        // no autocompletion for wildcards
        INetURLObject aTempObj;
        if ( eSmartProtocol != INET_PROT_NOT_VALID )
            aTempObj.SetSmartProtocol( eSmartProtocol );
        if ( aTempObj.SetSmartURL( aText ) )
            return aTempObj.GetMainURL( INetURLObject::NO_DECODE );
        else
            return aText;
    }

    if ( aObj.GetProtocol() == INET_PROT_NOT_VALID )
    {
        OUString aName = ParseSmart( aText, aBaseURL, SvtPathOptions().GetWorkPath() );
        aObj.SetURL(aName);
        OUString aURL( aObj.GetMainURL( INetURLObject::NO_DECODE ) );
        if ( aURL.isEmpty() )
            // aText itself is invalid, and even together with aBaseURL, it could not
            // made valid -> no chance
            return aText;

        bool bSlash = aObj.hasFinalSlash();
        {
            const OUString aPropName("CasePreservingURL");

            OUString aFileURL;

            Any aAny = UCBContentHelper::GetProperty(aURL, aPropName);
            bool success = (aAny >>= aFileURL);
            OUString aTitle;
            if(success)
                aTitle = INetURLObject(aFileURL).getName(
                             INetURLObject::LAST_SEGMENT,
                             true,
                             INetURLObject::DECODE_WITH_CHARSET );
            else
                success =
                    UCBContentHelper::GetTitle(aURL,&aTitle);

            if( success && aTitle != "/" && aTitle != "." )
            {
                    aObj.SetName( aTitle );
                    if ( bSlash )
                        aObj.setFinalSlash();
            }
        }
    }

    return aObj.GetMainURL( INetURLObject::NO_DECODE );
}

void SvtURLBox::DisableHistory()
{
    bHistoryDisabled = true;
    UpdatePicklistForSmartProtocol_Impl();
}

void SvtURLBox::SetBaseURL( const OUString& rURL )
{
    ::osl::MutexGuard aGuard( theSvtMatchContextMutex::get() );

    // Reset match lists
    pImp->aCompletions.clear();
    pImp->aURLs.clear();

    aBaseURL = rURL;
}

/** Parse leading ~ for Unix systems,
    does nothing for Windows
 */
bool SvtURLBox_Impl::TildeParsing(
    OUString&
#ifdef UNX
    aText
#endif
    , OUString&
#ifdef UNX
    aBaseURL
#endif
)
{
#ifdef UNX
    if( aText.startsWith( "~" ) )
    {
        OUString aParseTilde;
        bool bTrailingSlash = true; // use trailing slash

        if( aText.getLength() == 1 || aText[ 1 ] == '/' )
        {
            // covers "~" or "~/..." cases
            const char* aHomeLocation = getenv( "HOME" );
            if( !aHomeLocation )
                aHomeLocation = "";

            aParseTilde = OUString::createFromAscii(aHomeLocation);

            // in case the whole path is just "~" then there should
            // be no trailing slash at the end
            if( aText.getLength() == 1 )
                bTrailingSlash = false;
        }
        else
        {
            // covers "~username" and "~username/..." cases
            sal_Int32 nNameEnd = aText.indexOf( '/' );
            OUString aUserName = aText.copy( 1, ( nNameEnd != -1 ) ? nNameEnd : ( aText.getLength() - 1 ) );

            struct passwd* pPasswd = NULL;
#ifdef SOLARIS
            Sequence< sal_Int8 > sBuf( 1024 );
            struct passwd aTmp;
            sal_Int32 nRes = getpwnam_r( OUStringToOString( aUserName, RTL_TEXTENCODING_ASCII_US ).getStr(),
                                  &aTmp,
                                  (char*)sBuf.getArray(),
                                  1024,
                                  &pPasswd );
            if( !nRes && pPasswd )
                aParseTilde = OUString::createFromAscii(pPasswd->pw_dir);
            else
                return false; // no such user
#else
            pPasswd = getpwnam( OUStringToOString( aUserName, RTL_TEXTENCODING_ASCII_US ).getStr() );
            if( pPasswd )
                aParseTilde = OUString::createFromAscii(pPasswd->pw_dir);
            else
                return false; // no such user
#endif

            // in case the path is "~username" then there should
            // be no trailing slash at the end
            if( nNameEnd == -1 )
                bTrailingSlash = false;
        }

        if( !bTrailingSlash )
        {
            if( aParseTilde.isEmpty() || aParseTilde == "/" )
            {
                // "/" path should be converted to "/."
                aParseTilde = "/.";
            }
            else
            {
                // "blabla/" path should be converted to "blabla"
                aParseTilde = comphelper::string::stripEnd(aParseTilde, '/');
            }
        }
        else
        {
            if( !aParseTilde.endsWith("/") )
                aParseTilde += "/";
            if( aText.getLength() > 2 )
                aParseTilde += aText.copy( 2 );
        }

        aText = aParseTilde;
        aBaseURL = ""; // tilde provide absolute path
    }
#endif

    return true;
}

void SvtURLBox::SetFilter(const OUString& _sFilter)
{
    pImp->m_aFilters.clear();
    FilterMatch::createWildCardFilterList(_sFilter,pImp->m_aFilters);
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
