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


#include <list>

#include <cppuhelper/implbase3.hxx>
#include <cppuhelper/implementationentry.hxx>
#include <cppuhelper/supportsservice.hxx>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/lang/XComponent.hpp>
#include <com/sun/star/document/XDocumentEventListener.hpp>
#include <com/sun/star/document/XDocumentEventBroadcaster.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/frame/theGlobalEventBroadcaster.hpp>
#include <com/sun/star/graphic/GraphicProvider.hpp>
#include <com/sun/star/graphic/XGraphicProvider.hpp>
#include <com/sun/star/task/XJob.hpp>

#include <comphelper/processfactory.hxx>

#include <osl/mutex.hxx>

#include <vcl/window.hxx>
#include <vcl/floatwin.hxx>
#include <vcl/timer.hxx>
#include <vcl/menu.hxx>
#include <vcl/outdev.hxx>
#include <vcl/layout.hxx>
#include <vcl/msgbox.hxx>
#include <vcl/lineinfo.hxx>
#include <vcl/button.hxx>
#include <vcl/settings.hxx>
#include <vcl/svapp.hxx>
#include <sfx2/sfx.hrc>
#include "rtl/ustrbuf.hxx"

#include "updatecheckui.hrc"

#ifdef USE_JAVA
#include <com/sun/star/configuration/theDefaultProvider.hpp>
#include <com/sun/star/container/XNameAccess.hpp>
#include <com/sun/star/frame/XDesktop.hpp>
#include <com/sun/star/frame/XDispatchProvider.hpp>
#include <com/sun/star/util/XURLTransformer.hpp>
#endif	// USE_JAVA

#define STR_NO_WEBBROWSER_FOUND  (RID_SFX_APP_START + 7)

#define PROPERTY_TITLE          "BubbleHeading"
#define PROPERTY_TEXT           "BubbleText"
#define PROPERTY_IMAGE          "BubbleImageURL"
#define PROPERTY_SHOW_BUBBLE    "BubbleVisible"
#define PROPERTY_CLICK_HDL      "MenuClickHDL"
#define PROPERTY_SHOW_MENUICON  "MenuIconVisible"

using namespace ::com::sun::star;



static uno::Sequence< OUString > getServiceNames()
{
    uno::Sequence< OUString > aServiceList(1);
    aServiceList[0] = "com.sun.star.setup.UpdateCheckUI";
    return aServiceList;
}



static OUString getImplementationName()
{
    return OUString("vnd.sun.UpdateCheckUI");
}



namespace
{


class BubbleWindow : public FloatingWindow
{
    Point           maTipPos;
    vcl::Region          maBounds;
    Polygon         maRectPoly;
    Polygon         maTriPoly;
    OUString        maBubbleTitle;
    OUString        maBubbleText;
    Image           maBubbleImage;
    Size            maMaxTextSize;
    Rectangle       maTitleRect;
    Rectangle       maTextRect;
    long            mnTipOffset;

private:
    void            RecalcTextRects();

public:
                    BubbleWindow( vcl::Window* pParent, const OUString& rTitle,
                                  const OUString& rText, const Image& rImage );
                   virtual ~BubbleWindow();

    virtual void    MouseButtonDown( const MouseEvent& rMEvt ) SAL_OVERRIDE;
    virtual void    Paint( const Rectangle& rRect ) SAL_OVERRIDE;
    void            Resize() SAL_OVERRIDE;
    void            Show( bool bVisible = true, sal_uInt16 nFlags = SHOW_NOACTIVATE );
    void            SetTipPosPixel( const Point& rTipPos ) { maTipPos = rTipPos; }
    void            SetTitleAndText( const OUString& rTitle, const OUString& rText,
                                     const Image& rImage );
};


class UpdateCheckUI : public ::cppu::WeakImplHelper3
                        < lang::XServiceInfo, document::XDocumentEventListener, beans::XPropertySet >
{
    uno::Reference< uno::XComponentContext > m_xContext;
    uno::Reference< task::XJob > mrJob;
    OUString       maBubbleTitle;
    OUString       maBubbleText;
    OUString       maBubbleImageURL;
    Image               maBubbleImage;
    BubbleWindow*       mpBubbleWin;
    SystemWindow*       mpIconSysWin;
    MenuBar*            mpIconMBar;
    ResMgr*             mpUpdResMgr;
    ResMgr*             mpSfxResMgr;
    Timer               maWaitTimer;
    Timer               maTimeoutTimer;
    Link                maWindowEventHdl;
    Link                maApplicationEventHdl;
    bool                mbShowBubble;
    bool                mbShowMenuIcon;
    bool                mbBubbleChanged;
    sal_uInt16              mnIconID;

private:
                    DECL_LINK(ClickHdl, void *);
                    DECL_LINK( HighlightHdl, MenuBar::MenuBarButtonCallbackArg* );
                    DECL_LINK(WaitTimeOutHdl, void *);
                    DECL_LINK(TimeOutHdl, void *);
                    DECL_LINK(UserEventHdl, void *);
                    DECL_LINK( WindowEventHdl, VclWindowEvent* );
                    DECL_LINK( ApplicationEventHdl, VclSimpleEvent* );

    BubbleWindow*   GetBubbleWindow();
    void            RemoveBubbleWindow( bool bRemoveIcon );
    Image           GetMenuBarIcon( MenuBar* pMBar );
    void            AddMenuBarIcon( SystemWindow* pSysWin, bool bAddEventHdl );
    Image           GetBubbleImage( OUString &rURL );

public:
                    UpdateCheckUI(const uno::Reference<uno::XComponentContext>&);
    virtual        ~UpdateCheckUI();

    // XServiceInfo
    virtual OUString SAL_CALL getImplementationName()
        throw (uno::RuntimeException, std::exception) SAL_OVERRIDE;
    virtual sal_Bool SAL_CALL supportsService(OUString const & serviceName)
        throw (uno::RuntimeException, std::exception) SAL_OVERRIDE;
    virtual uno::Sequence< OUString > SAL_CALL getSupportedServiceNames()
        throw (uno::RuntimeException, std::exception) SAL_OVERRIDE;

    // XDocumentEventListener
    virtual void SAL_CALL documentEventOccured(const document::DocumentEvent& Event)
        throw (uno::RuntimeException, std::exception) SAL_OVERRIDE;
    virtual void SAL_CALL disposing(const lang::EventObject& Event)
        throw (uno::RuntimeException, std::exception) SAL_OVERRIDE;

    //XPropertySet
    virtual uno::Reference< beans::XPropertySetInfo > SAL_CALL getPropertySetInfo(void)
        throw ( uno::RuntimeException, std::exception ) SAL_OVERRIDE;
    virtual void SAL_CALL setPropertyValue(const OUString& PropertyName, const uno::Any& aValue)
        throw( beans::UnknownPropertyException, beans::PropertyVetoException,
               lang::IllegalArgumentException, lang::WrappedTargetException, uno::RuntimeException, std::exception ) SAL_OVERRIDE;
    virtual uno::Any SAL_CALL getPropertyValue(const OUString& PropertyName)
        throw ( beans::UnknownPropertyException, lang::WrappedTargetException, uno::RuntimeException, std::exception ) SAL_OVERRIDE;
    virtual void SAL_CALL addPropertyChangeListener(const OUString& PropertyName,
                                                    const uno::Reference< beans::XPropertyChangeListener > & aListener)
        throw( beans::UnknownPropertyException, lang::WrappedTargetException, uno::RuntimeException, std::exception ) SAL_OVERRIDE;
    virtual void SAL_CALL removePropertyChangeListener(const OUString& PropertyName,
                                                       const uno::Reference< beans::XPropertyChangeListener > & aListener)
        throw( beans::UnknownPropertyException, lang::WrappedTargetException, uno::RuntimeException, std::exception ) SAL_OVERRIDE;
    virtual void SAL_CALL addVetoableChangeListener(const OUString& PropertyName,
                                                    const uno::Reference< beans::XVetoableChangeListener > & aListener)
        throw( beans::UnknownPropertyException, lang::WrappedTargetException, uno::RuntimeException, std::exception ) SAL_OVERRIDE;
    virtual void SAL_CALL removeVetoableChangeListener(const OUString& PropertyName,
                                                       const uno::Reference< beans::XVetoableChangeListener > & aListener)
        throw( beans::UnknownPropertyException, lang::WrappedTargetException, uno::RuntimeException, std::exception ) SAL_OVERRIDE;
};


UpdateCheckUI::UpdateCheckUI(const uno::Reference<uno::XComponentContext>& xContext) :
      m_xContext(xContext)
    , mpBubbleWin( NULL )
    , mpIconSysWin( NULL )
    , mpIconMBar( NULL )
    , mbShowBubble( false )
    , mbShowMenuIcon( false )
    , mbBubbleChanged( false )
    , mnIconID( 0 )
{
    mpUpdResMgr = ResMgr::CreateResMgr( "updchk" );
    mpSfxResMgr = ResMgr::CreateResMgr( "sfx" );

    maBubbleImage = GetBubbleImage( maBubbleImageURL );

    maWaitTimer.SetTimeout( 400 );
    maWaitTimer.SetTimeoutHdl( LINK( this, UpdateCheckUI, WaitTimeOutHdl ) );

    maTimeoutTimer.SetTimeout( 10000 );
    maTimeoutTimer.SetTimeoutHdl( LINK( this, UpdateCheckUI, TimeOutHdl ) );

    uno::Reference< document::XDocumentEventBroadcaster > xBroadcaster( frame::theGlobalEventBroadcaster::get(m_xContext) );
    xBroadcaster->addDocumentEventListener( this );

    maWindowEventHdl = LINK( this, UpdateCheckUI, WindowEventHdl );
    maApplicationEventHdl = LINK( this, UpdateCheckUI, ApplicationEventHdl );
    Application::AddEventListener( maApplicationEventHdl );
}


UpdateCheckUI::~UpdateCheckUI()
{
    Application::RemoveEventListener( maApplicationEventHdl );
    RemoveBubbleWindow( true );
    delete mpUpdResMgr;
    delete mpSfxResMgr;
}


OUString SAL_CALL
UpdateCheckUI::getImplementationName() throw (uno::RuntimeException, std::exception)
{
    return ::getImplementationName();
}


uno::Sequence< OUString > SAL_CALL
UpdateCheckUI::getSupportedServiceNames() throw (uno::RuntimeException, std::exception)
{
    return ::getServiceNames();
}

sal_Bool SAL_CALL
UpdateCheckUI::supportsService( OUString const & serviceName ) throw (uno::RuntimeException, std::exception)
{
    return cppu::supportsService(this, serviceName);
}


Image UpdateCheckUI::GetMenuBarIcon( MenuBar* pMBar )
{
    sal_uInt32 nResID;
    vcl::Window *pMBarWin = pMBar->GetWindow();
    sal_uInt32 nMBarHeight = 20;

    if ( pMBarWin )
        nMBarHeight = pMBarWin->GetOutputSizePixel().getHeight();

    if ( nMBarHeight >= 35 )
        nResID = RID_UPDATE_AVAILABLE_26;
    else
        nResID = RID_UPDATE_AVAILABLE_16;

    return Image( ResId( nResID, *mpUpdResMgr ) );
}


Image UpdateCheckUI::GetBubbleImage( OUString &rURL )
{
    Image aImage;

#ifndef USE_JAVA
    if ( !maBubbleImageURL.isEmpty() )
    {
        uno::Reference< uno::XComponentContext > xContext = ::comphelper::getProcessComponentContext();

        if( !xContext.is() )
            throw uno::RuntimeException(
                "UpdateCheckUI: unable to obtain service manager from component context" );

        try
        {
            uno::Reference< graphic::XGraphicProvider > xGraphProvider(graphic::GraphicProvider::create(xContext));
            uno::Sequence< beans::PropertyValue > aMediaProps( 1 );
            aMediaProps[0].Name = "URL";
            aMediaProps[0].Value <<= rURL;

            uno::Reference< graphic::XGraphic > xGraphic = xGraphProvider->queryGraphic( aMediaProps );
            if ( xGraphic.is() )
            {
                aImage = Image( xGraphic );
            }
        }
        catch( const uno::Exception& )
        {
        }
    }

    if ( aImage.GetSizePixel().Width() == 0 )
        aImage = InfoBox::GetStandardImage();
#endif	// !USE_JAVA

    return aImage;
}


void UpdateCheckUI::AddMenuBarIcon( SystemWindow *pSysWin, bool bAddEventHdl )
{
#ifndef USE_JAVA
    if ( ! mbShowMenuIcon )
        return;

    SolarMutexGuard aGuard;

    MenuBar *pActiveMBar = pSysWin->GetMenuBar();
    if ( ( pSysWin != mpIconSysWin ) || ( pActiveMBar != mpIconMBar ) )
    {
        if ( bAddEventHdl && mpIconSysWin )
            mpIconSysWin->RemoveEventListener( maWindowEventHdl );

        RemoveBubbleWindow( true );

        if ( pActiveMBar )
        {
            OUStringBuffer aBuf;
            if( !maBubbleTitle.isEmpty() )
                aBuf.append( maBubbleTitle );
            if( !maBubbleText.isEmpty() )
            {
                if( !maBubbleTitle.isEmpty() )
                    aBuf.appendAscii( "\n\n" );
                aBuf.append( maBubbleText );
            }

            Image aImage = GetMenuBarIcon( pActiveMBar );
            mnIconID = pActiveMBar->AddMenuBarButton( aImage,
                                    LINK( this, UpdateCheckUI, ClickHdl ),
                                    aBuf.makeStringAndClear()
                                    );
            pActiveMBar->SetMenuBarButtonHighlightHdl( mnIconID,
                                    LINK( this, UpdateCheckUI, HighlightHdl ) );
        }
        mpIconMBar = pActiveMBar;
        mpIconSysWin = pSysWin;
        if ( bAddEventHdl && mpIconSysWin )
            mpIconSysWin->AddEventListener( maWindowEventHdl );
    }

    if ( mbShowBubble && pActiveMBar )
    {
        mpBubbleWin = GetBubbleWindow();
        if ( mpBubbleWin )
        {
            mpBubbleWin->Show( true );
            maTimeoutTimer.Start();
        }
        mbShowBubble = false;
    }
#endif	// !USE_JAVA
}


void SAL_CALL UpdateCheckUI::documentEventOccured(const document::DocumentEvent& rEvent)
    throw (uno::RuntimeException, std::exception)
{
    SolarMutexGuard aGuard;

    if( rEvent.EventName == "OnPrepareViewClosing" )
    {
        RemoveBubbleWindow( true );
    }
}


void SAL_CALL UpdateCheckUI::disposing(const lang::EventObject&)
    throw (uno::RuntimeException, std::exception)
{
}


uno::Reference< beans::XPropertySetInfo > UpdateCheckUI::getPropertySetInfo(void)
    throw ( uno::RuntimeException, std::exception )
{
    return NULL;
}


void UpdateCheckUI::setPropertyValue(const OUString& rPropertyName,
                                     const uno::Any& rValue)
    throw( beans::UnknownPropertyException, beans::PropertyVetoException,
           lang::IllegalArgumentException, lang::WrappedTargetException, uno::RuntimeException, std::exception)
{
    SolarMutexGuard aGuard;

    OUString aString;

    if( rPropertyName == PROPERTY_TITLE ) {
        rValue >>= aString;
        if ( aString != maBubbleTitle ) {
            maBubbleTitle = aString;
            mbBubbleChanged = true;
        }
    }
    else if( rPropertyName == PROPERTY_TEXT ) {
        rValue >>= aString;
        if ( aString != maBubbleText ) {
            maBubbleText = aString;
            mbBubbleChanged = true;
        }
    }
    else if( rPropertyName == PROPERTY_IMAGE ) {
        rValue >>= aString;
        if ( aString != maBubbleImageURL ) {
            maBubbleImageURL = aString;
            maBubbleImage = GetBubbleImage( maBubbleImageURL );
            mbBubbleChanged = true;
        }
    }
    else if( rPropertyName == PROPERTY_SHOW_BUBBLE ) {
        rValue >>= mbShowBubble;
        if ( mbShowBubble )
            Application::PostUserEvent( LINK( this, UpdateCheckUI, UserEventHdl ) );
        else if ( mpBubbleWin )
            mpBubbleWin->Show( false );
    }
    else if( rPropertyName == PROPERTY_CLICK_HDL ) {
        uno::Reference< task::XJob > aJob;
        rValue >>= aJob;
        if ( aJob.is() )
            mrJob = aJob;
        else
            throw lang::IllegalArgumentException();
    }
    else if (rPropertyName == PROPERTY_SHOW_MENUICON ) {
        bool bShowMenuIcon = false;
        rValue >>= bShowMenuIcon;
        if ( bShowMenuIcon != mbShowMenuIcon )
        {
            mbShowMenuIcon = bShowMenuIcon;
            if ( bShowMenuIcon )
                Application::PostUserEvent( LINK( this, UpdateCheckUI, UserEventHdl ) );
            else
                RemoveBubbleWindow( true );
        }
    }
    else
        throw beans::UnknownPropertyException();

    if ( mbBubbleChanged && mpBubbleWin )
        mpBubbleWin->Show( false );
}


uno::Any UpdateCheckUI::getPropertyValue(const OUString& rPropertyName)
    throw( beans::UnknownPropertyException, lang::WrappedTargetException, uno::RuntimeException, std::exception )
{
    SolarMutexGuard aGuard;

    uno::Any aRet;

    if( rPropertyName == PROPERTY_TITLE )
        aRet = uno::makeAny( maBubbleTitle );
    else if( rPropertyName == PROPERTY_TEXT )
        aRet = uno::makeAny( maBubbleText );
    else if( rPropertyName == PROPERTY_SHOW_BUBBLE )
        aRet = uno::makeAny( mbShowBubble );
    else if( rPropertyName == PROPERTY_IMAGE )
        aRet = uno::makeAny( maBubbleImageURL );
    else if( rPropertyName == PROPERTY_CLICK_HDL )
        aRet = uno::makeAny( mrJob );
    else if( rPropertyName == PROPERTY_SHOW_MENUICON )
        aRet = uno::makeAny( mbShowMenuIcon );
    else
        throw beans::UnknownPropertyException();

    return aRet;
}


void UpdateCheckUI::addPropertyChangeListener( const OUString& /*aPropertyName*/,
                                               const uno::Reference< beans::XPropertyChangeListener > & /*aListener*/)
    throw( beans::UnknownPropertyException, lang::WrappedTargetException, uno::RuntimeException, std::exception )
{
    //no bound properties
}


void UpdateCheckUI::removePropertyChangeListener( const OUString& /*aPropertyName*/,
                                                  const uno::Reference< beans::XPropertyChangeListener > & /*aListener*/)
    throw( beans::UnknownPropertyException, lang::WrappedTargetException, uno::RuntimeException, std::exception )
{
    //no bound properties
}


void UpdateCheckUI::addVetoableChangeListener( const OUString& /*aPropertyName*/,
                                               const uno::Reference< beans::XVetoableChangeListener > & /*aListener*/)
    throw( beans::UnknownPropertyException, lang::WrappedTargetException, uno::RuntimeException, std::exception )
{
    //no vetoable properties
}


void UpdateCheckUI::removeVetoableChangeListener( const OUString& /*aPropertyName*/,
                                                  const uno::Reference< beans::XVetoableChangeListener > & /*aListener*/)
    throw( beans::UnknownPropertyException, lang::WrappedTargetException, uno::RuntimeException, std::exception )
{
    //no vetoable properties
}





BubbleWindow * UpdateCheckUI::GetBubbleWindow()
{
#ifdef USE_JAVA
    return NULL;
#else	// USE_JAVA
    if ( !mpIconSysWin )
        return NULL;

    Rectangle aIconRect = mpIconMBar->GetMenuBarButtonRectPixel( mnIconID );
    if( aIconRect.IsEmpty() )
        return NULL;

    BubbleWindow* pBubbleWin = mpBubbleWin;

    if ( !pBubbleWin ) {
        pBubbleWin = new BubbleWindow( mpIconSysWin, maBubbleTitle,
                                       maBubbleText, maBubbleImage );
        mbBubbleChanged = false;
    }
    else if ( mbBubbleChanged ) {
        pBubbleWin->SetTitleAndText( maBubbleTitle, maBubbleText,
                                     maBubbleImage );
        mbBubbleChanged = false;
    }

    Point aWinPos = aIconRect.BottomCenter();

    pBubbleWin->SetTipPosPixel( aWinPos );

    return pBubbleWin;
#endif	// USE_JAVA
}


void UpdateCheckUI::RemoveBubbleWindow( bool bRemoveIcon )
{
    SolarMutexGuard aGuard;

    maWaitTimer.Stop();
    maTimeoutTimer.Stop();

    if ( mpBubbleWin )
    {
        delete mpBubbleWin;
        mpBubbleWin = NULL;
    }

    if ( bRemoveIcon )
    {
        try {
            if ( mpIconMBar && ( mnIconID != 0 ) )
            {
                mpIconMBar->RemoveMenuBarButton( mnIconID );
                mpIconMBar = NULL;
                mnIconID = 0;
            }
        }
        catch ( ... ) {
            mpIconMBar = NULL;
            mnIconID = 0;
        }

        mpIconSysWin = NULL;
    }
}


IMPL_LINK_NOARG(UpdateCheckUI, ClickHdl)
{
    SolarMutexGuard aGuard;

    maWaitTimer.Stop();
    if ( mpBubbleWin )
        mpBubbleWin->Show( false );

    if ( mrJob.is() )
    {
        try {
            uno::Sequence<beans::NamedValue> aEmpty;
            mrJob->execute( aEmpty );
        }
        catch(const uno::Exception&) {
            MessageDialog( NULL, ResId( STR_NO_WEBBROWSER_FOUND, *mpSfxResMgr )).Execute();
        }
    }

    return 0;
}


IMPL_LINK( UpdateCheckUI, HighlightHdl, MenuBar::MenuBarButtonCallbackArg*, pData )
{
    if ( pData->bHighlight )
        maWaitTimer.Start();
    else
        RemoveBubbleWindow( false );

    return 0;
}


IMPL_LINK_NOARG(UpdateCheckUI, WaitTimeOutHdl)
{
    SolarMutexGuard aGuard;

    mpBubbleWin = GetBubbleWindow();

    if ( mpBubbleWin )
    {
        mpBubbleWin->Show();
    }

    return 0;
}


IMPL_LINK_NOARG(UpdateCheckUI, TimeOutHdl)
{
    RemoveBubbleWindow( false );

    return 0;
}


IMPL_LINK_NOARG(UpdateCheckUI, UserEventHdl)
{
    SolarMutexGuard aGuard;

#ifdef USE_JAVA
    // Note: the following code should be an exact copy of the code in the
    // CheckNowHdl_Impl callback in the cui/source/options/optupdt.cxx file
    uno::Reference < uno::XComponentContext> xContext( ::comphelper::getProcessComponentContext() );

    try
    {
        uno::Reference< lang::XMultiServiceFactory > xConfigProvider( com::sun::star::configuration::theDefaultProvider::get( xContext ) );

        beans::PropertyValue aProperty;
        aProperty.Name = "nodepath";
        aProperty.Value = uno::makeAny( OUString("org.openoffice.Office.Addons/AddonUI/OfficeHelp/UpdateCheckJob") );

        uno::Sequence< uno::Any > aArgumentList( 1 );
        aArgumentList[0] = uno::makeAny( aProperty );

        uno::Reference< container::XNameAccess > xNameAccess( xConfigProvider->createInstanceWithArguments( "com.sun.star.configuration.ConfigurationAccess", aArgumentList ), uno::UNO_QUERY_THROW );

        util::URL aURL;
        xNameAccess->getByName( "URL" ) >>= aURL.Complete;
        uno::Reference< util::XURLTransformer > xTransformer( xConfigProvider->createInstance( "com.sun.star.util.URLTransformer" ), uno::UNO_QUERY_THROW );
        xTransformer->parseStrict( aURL );
        uno::Reference< frame::XDesktop > xDesktop( xConfigProvider->createInstance( "com.sun.star.frame.Desktop" ), uno::UNO_QUERY_THROW );
        uno::Reference< frame::XDispatchProvider > xDispatchProvider( xDesktop->getCurrentFrame(), uno::UNO_QUERY );
        if( xDispatchProvider.is() )
        {
            uno::Reference< frame::XDispatch > xDispatch = xDispatchProvider->queryDispatch( aURL, "", 0 );

            // Prevent deadlocking when the dispatch method tries to grab the
            // application mutex on a separate thread
            sal_uLong nCount = Application::ReleaseSolarMutex();
            try
            {
                if( xDispatch.is() )
                   xDispatch->dispatch( aURL, uno::Sequence< beans::PropertyValue >() );
            }
            catch( ... )
            {
            }
            Application::AcquireSolarMutex( nCount );
        }
    }
    catch( ... )
    {
    }
#else	// USE_JAVA
    vcl::Window *pTopWin = Application::GetFirstTopLevelWindow();
    vcl::Window *pActiveWin = Application::GetActiveTopWindow();
    SystemWindow *pActiveSysWin = NULL;

    vcl::Window *pBubbleWin = NULL;
    if ( mpBubbleWin )
        pBubbleWin = mpBubbleWin;

    if ( pActiveWin && ( pActiveWin != pBubbleWin ) && pActiveWin->IsTopWindow() )
        pActiveSysWin = pActiveWin->GetSystemWindow();

    if ( pActiveWin == pBubbleWin )
        pActiveSysWin = NULL;

    while ( !pActiveSysWin && pTopWin )
    {
        if ( ( pTopWin != pBubbleWin ) && pTopWin->IsTopWindow() )
            pActiveSysWin = pTopWin->GetSystemWindow();
        if ( !pActiveSysWin )
            pTopWin = Application::GetNextTopLevelWindow( pTopWin );
    }

    if ( pActiveSysWin )
        AddMenuBarIcon( pActiveSysWin, true );
#endif	// USE_JAVA

    return 0;
}


IMPL_LINK( UpdateCheckUI, WindowEventHdl, VclWindowEvent*, pEvent )
{
    sal_uLong nEventID = pEvent->GetId();

    if ( VCLEVENT_OBJECT_DYING == nEventID )
    {
        SolarMutexGuard aGuard;
        if ( mpIconSysWin == pEvent->GetWindow() )
        {
            mpIconSysWin->RemoveEventListener( maWindowEventHdl );
            RemoveBubbleWindow( true );
        }
    }
    else if ( VCLEVENT_WINDOW_MENUBARADDED == nEventID )
    {
        SolarMutexGuard aGuard;
        vcl::Window *pWindow = pEvent->GetWindow();
        if ( pWindow )
        {
            SystemWindow *pSysWin = pWindow->GetSystemWindow();
            if ( pSysWin )
            {
                AddMenuBarIcon( pSysWin, false );
            }
        }
    }
    else if ( VCLEVENT_WINDOW_MENUBARREMOVED == nEventID )
    {
        SolarMutexGuard aGuard;
        MenuBar *pMBar = (MenuBar*) pEvent->GetData();
        if ( pMBar && ( pMBar == mpIconMBar ) )
            RemoveBubbleWindow( true );
    }
    else if ( ( nEventID == VCLEVENT_WINDOW_MOVE ) ||
              ( nEventID == VCLEVENT_WINDOW_RESIZE ) )
    {
        SolarMutexGuard aGuard;
        if ( ( mpIconSysWin == pEvent->GetWindow() ) &&
             ( mpBubbleWin != NULL ) && ( mpIconMBar != NULL ) )
        {
            Rectangle aIconRect = mpIconMBar->GetMenuBarButtonRectPixel( mnIconID );
            Point aWinPos = aIconRect.BottomCenter();
            mpBubbleWin->SetTipPosPixel( aWinPos );
            if ( mpBubbleWin->IsVisible() )
                mpBubbleWin->Show();    // This will recalc the screen position of the bubble
        }
    }

    return 0;
}


IMPL_LINK( UpdateCheckUI, ApplicationEventHdl, VclSimpleEvent *, pEvent)
{
    switch (pEvent->GetId())
    {
        case VCLEVENT_WINDOW_SHOW:
        case VCLEVENT_WINDOW_ACTIVATE:
        case VCLEVENT_WINDOW_GETFOCUS: {
            SolarMutexGuard aGuard;

            vcl::Window *pWindow = static_cast< VclWindowEvent * >(pEvent)->GetWindow();
            if ( pWindow && pWindow->IsTopWindow() )
            {
                SystemWindow *pSysWin = pWindow->GetSystemWindow();
                MenuBar      *pMBar   = pSysWin->GetMenuBar();
                if ( pSysWin && pMBar )
                {
                    AddMenuBarIcon( pSysWin, true );
                }
            }
            break;
        }
    }
    return 0;
}




#define TIP_HEIGHT             15
#define TIP_WIDTH               7
#define TIP_RIGHT_OFFSET       18
#define BUBBLE_BORDER          10
#define TEXT_MAX_WIDTH        300
#define TEXT_MAX_HEIGHT       200


BubbleWindow::BubbleWindow( vcl::Window* pParent, const OUString& rTitle,
                            const OUString& rText, const Image& rImage )
    : FloatingWindow( pParent, WB_SYSTEMWINDOW
                               | WB_OWNERDRAWDECORATION
                               | WB_NOBORDER
                    )
    , maBubbleTitle( rTitle )
    , maBubbleText( rText )
    , maBubbleImage( rImage )
    , maMaxTextSize( TEXT_MAX_WIDTH, TEXT_MAX_HEIGHT )
    , mnTipOffset( 0 )
{
    SetBackground( Wallpaper( GetSettings().GetStyleSettings().GetHelpColor() ) );
}


BubbleWindow::~BubbleWindow()
{
}


void BubbleWindow::Resize()
{
    SolarMutexGuard aGuard;

    FloatingWindow::Resize();

    Size aSize = GetSizePixel();

    if ( ( aSize.Height() < 20 ) || ( aSize.Width() < 60 ) )
        return;

    Rectangle aRect( 0, TIP_HEIGHT, aSize.Width(), aSize.Height() - TIP_HEIGHT );
    maRectPoly = Polygon( aRect, 6, 6 );
    vcl::Region aRegion( maRectPoly );
    long nTipOffset = aSize.Width() - TIP_RIGHT_OFFSET + mnTipOffset;

    Point aPointArr[4];
    aPointArr[0] = Point( nTipOffset, TIP_HEIGHT );
    aPointArr[1] = Point( nTipOffset, 0 );
    aPointArr[2] = Point( nTipOffset + TIP_WIDTH , TIP_HEIGHT );
    aPointArr[3] = Point( nTipOffset, TIP_HEIGHT );
     maTriPoly = Polygon( 4, aPointArr );
    vcl::Region aTriRegion( maTriPoly );

    aRegion.Union( aTriRegion);
    maBounds = aRegion;

    SetWindowRegionPixel( maBounds );
}


void BubbleWindow::SetTitleAndText( const OUString& rTitle,
                                    const OUString& rText,
                                    const Image& rImage )
{
    maBubbleTitle = rTitle;
    maBubbleText = rText;
    maBubbleImage = rImage;

    Resize();
}


void BubbleWindow::Paint( const Rectangle& )
{
    SolarMutexGuard aGuard;

    LineInfo aThickLine( LINE_SOLID, 2 );

    DrawPolyLine( maRectPoly, aThickLine );
    DrawPolyLine( maTriPoly );

    Color aOldLine = GetLineColor();
    Size aSize = GetSizePixel();
    long nTipOffset = aSize.Width() - TIP_RIGHT_OFFSET + mnTipOffset;

    SetLineColor( GetSettings().GetStyleSettings().GetHelpColor() );
    DrawLine( Point( nTipOffset+2, TIP_HEIGHT ),
              Point( nTipOffset + TIP_WIDTH -1 , TIP_HEIGHT ),
              aThickLine );
    SetLineColor( aOldLine );

    //Image aImage = InfoBox::GetStandardImage();
    Size aImgSize = maBubbleImage.GetSizePixel();

    DrawImage( Point( BUBBLE_BORDER, BUBBLE_BORDER + TIP_HEIGHT ), maBubbleImage );

    vcl::Font aOldFont = GetFont();
    vcl::Font aBoldFont = aOldFont;
    aBoldFont.SetWeight( WEIGHT_BOLD );

    SetFont( aBoldFont );
    Rectangle aTitleRect = maTitleRect;
    aTitleRect.Move( aImgSize.Width(), 0 );
    DrawText( aTitleRect, maBubbleTitle, TEXT_DRAW_MULTILINE | TEXT_DRAW_WORDBREAK );

    SetFont( aOldFont );
    Rectangle aTextRect = maTextRect;
    aTextRect.Move( aImgSize.Width(), 0 );
    DrawText( aTextRect, maBubbleText, TEXT_DRAW_MULTILINE | TEXT_DRAW_WORDBREAK );
}


void BubbleWindow::MouseButtonDown( const MouseEvent& )
{
    Show( false );
}


void BubbleWindow::Show( bool bVisible, sal_uInt16 nFlags )
{
    SolarMutexGuard aGuard;

    if ( !bVisible )
    {
        FloatingWindow::Show( bVisible );
        return;
    }

    // don't show bubbles without a text
    if ( ( maBubbleTitle.isEmpty() ) && ( maBubbleText.isEmpty() ) )
        return;

    Size aWindowSize = GetSizePixel();

    // Image aImage = InfoBox::GetStandardImage();
    Size aImgSize = maBubbleImage.GetSizePixel();

    RecalcTextRects();

    aWindowSize.setHeight( maTitleRect.GetHeight() * 7 / 4+ maTextRect.GetHeight() +
                           3 * BUBBLE_BORDER + TIP_HEIGHT );

    if ( maTitleRect.GetWidth() > maTextRect.GetWidth() )
        aWindowSize.setWidth( maTitleRect.GetWidth() );
    else
        aWindowSize.setWidth( maTextRect.GetWidth() );

    aWindowSize.setWidth( aWindowSize.Width() + 3 * BUBBLE_BORDER + aImgSize.Width() );

    if ( aWindowSize.Height() < aImgSize.Height() + TIP_HEIGHT + 2 * BUBBLE_BORDER )
        aWindowSize.setHeight( aImgSize.Height() + TIP_HEIGHT + 2 * BUBBLE_BORDER );

    Point aPos;
    aPos.X() = maTipPos.X() - aWindowSize.Width() + TIP_RIGHT_OFFSET;
    aPos.Y() = maTipPos.Y();
    Point aScreenPos = GetParent()->OutputToAbsoluteScreenPixel( aPos );
    if ( aScreenPos.X() < 0 )
    {
        mnTipOffset = aScreenPos.X();
        aPos.X() -= mnTipOffset;
    }
    SetPosSizePixel( aPos, aWindowSize );

    FloatingWindow::Show( bVisible, nFlags );
}


void BubbleWindow::RecalcTextRects()
{
    Size aTotalSize;
    bool bFinished = false;
    vcl::Font aOldFont = GetFont();
    vcl::Font aBoldFont = aOldFont;

    aBoldFont.SetWeight( WEIGHT_BOLD );

    while ( !bFinished )
    {
        SetFont( aBoldFont );

        maTitleRect = GetTextRect( Rectangle( Point( 0, 0 ), maMaxTextSize ),
                                   maBubbleTitle,
                                   TEXT_DRAW_MULTILINE | TEXT_DRAW_WORDBREAK );

        SetFont( aOldFont );
        maTextRect = GetTextRect( Rectangle( Point( 0, 0 ), maMaxTextSize ),
                                  maBubbleText,
                                  TEXT_DRAW_MULTILINE | TEXT_DRAW_WORDBREAK );

        if ( maTextRect.GetHeight() < 10 )
            maTextRect.setHeight( 10 );

        aTotalSize.setHeight( maTitleRect.GetHeight() +
                              aBoldFont.GetHeight() * 3 / 4 +
                              maTextRect.GetHeight() +
                              3 * BUBBLE_BORDER + TIP_HEIGHT );
        if ( aTotalSize.Height() > maMaxTextSize.Height() )
        {
            maMaxTextSize.Width() = maMaxTextSize.Width() * 3 / 2;
            maMaxTextSize.Height() = maMaxTextSize.Height() * 3 / 2;
        }
        else
            bFinished = true;
    }
    maTitleRect.Move( 2*BUBBLE_BORDER, BUBBLE_BORDER + TIP_HEIGHT );
    maTextRect.Move( 2*BUBBLE_BORDER, BUBBLE_BORDER + TIP_HEIGHT + maTitleRect.GetHeight() + aBoldFont.GetHeight() * 3 / 4 );
}




} // anonymous namespace



static uno::Reference<uno::XInterface> SAL_CALL
createInstance(const uno::Reference<uno::XComponentContext>& xContext)
{
    return  *new UpdateCheckUI(xContext);
}



static const cppu::ImplementationEntry kImplementations_entries[] =
{
    {
        createInstance,
        getImplementationName,
        getServiceNames,
        cppu::createSingleComponentFactory,
        NULL,
        0
    },
    { NULL, NULL, NULL, NULL, NULL, 0 }
} ;



extern "C" SAL_DLLPUBLIC_EXPORT void * SAL_CALL updchkui_component_getFactory(const sal_Char *pszImplementationName, void *pServiceManager, void *pRegistryKey)
{
    return cppu::component_getFactoryHelper(
        pszImplementationName,
        pServiceManager,
        pRegistryKey,
        kImplementations_entries) ;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
