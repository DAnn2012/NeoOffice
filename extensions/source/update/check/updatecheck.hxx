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

#ifndef INCLUDED_EXTENSIONS_SOURCE_UPDATE_CHECK_UPDATECHECK_HXX
#define INCLUDED_EXTENSIONS_SOURCE_UPDATE_CHECK_UPDATECHECK_HXX

#include <com/sun/star/beans/NamedValue.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/task/XInteractionHandler.hpp>
#include <com/sun/star/uno/XComponentContext.hpp>

#include <osl/conditn.hxx>
#include <osl/thread.hxx>
#include <rtl/instance.hxx>
#include <salhelper/refobj.hxx>

#include "updateinfo.hxx"
#include "updatecheckconfiglistener.hxx"
#include "actionlistener.hxx"
#include "updatehdl.hxx"
#include "download.hxx"


class UpdateCheck;

class UpdateCheckInitData {

public:
    inline rtl::Reference< UpdateCheck > SAL_CALL operator() () const;
};

class WorkerThread : public osl::Thread
{
public:
    virtual void cancel() = 0;
};

class UpdateCheck :
    public UpdateCheckConfigListener,
    public IActionListener,
    public DownloadInteractionHandler,
    public rtl::StaticWithInit< rtl::Reference< UpdateCheck >, UpdateCheckInitData >
{
    UpdateCheck();

    virtual ~UpdateCheck();

public:
    inline SAL_CALL operator rtl::Reference< UpdateCheckConfigListener > ()
        { return static_cast< UpdateCheckConfigListener * > (this); }

    void initialize(const com::sun::star::uno::Sequence<com::sun::star::beans::NamedValue>& rValues,
                    const com::sun::star::uno::Reference<com::sun::star::uno::XComponentContext>& xContext);

    // Update internal update info member
    void setUpdateInfo(const UpdateInfo& aInfo);

    /* This method turns on the menubar icon, triggers the bubble window or
     * updates the dialog text when appropriate
     */
    void setUIState(UpdateState eState, bool suppressBubble = false);

    // Returns the UI state that matches rInfo best
    static UpdateState getUIState(const UpdateInfo& rInfo);

    // Check for updates failed
    void setCheckFailedState();

    // Executes the update check dialog for manual checks and downloads interaction
    void showDialog(bool forceCheck = false);
#ifdef USE_JAVA
    void onCloseApp();
    void shutdownApp();
#endif	// USE_JAVA

    // Returns true if the update dialog is currently showing
    bool isDialogShowing() const;
    bool shouldShowExtUpdDlg() const { return ( m_bShowExtUpdDlg && m_bHasExtensionUpdate ); }
    void showExtensionDialog();
    void setHasExtensionUpdates( bool bHasUpdates ) { m_bHasExtensionUpdate = bHasUpdates; }
    bool hasOfficeUpdate() const { return (m_aUpdateInfo.BuildId.getLength() > 0); }

    // DownloadInteractionHandler
    virtual bool downloadTargetExists(const OUString& rFileName) SAL_OVERRIDE;
    virtual void downloadStalled(const OUString& rErrorMessage) SAL_OVERRIDE;
    virtual void downloadProgressAt(sal_Int8 nProcent) SAL_OVERRIDE;
    virtual void downloadStarted(const OUString& rLocalFileName, sal_Int64 nFileSize) SAL_OVERRIDE;
    virtual void downloadFinished(const OUString& rLocalFileName) SAL_OVERRIDE;
    // checks if the download target already exists and asks user what to do next
    virtual bool checkDownloadDestination( const OUString& rFile ) SAL_OVERRIDE;

    // Cancels the download action (and resumes checking if enabled)
    void cancelDownload();

    // Returns the XInteractionHandler of the UpdateHandler instance if present (and visible)
    com::sun::star::uno::Reference< com::sun::star::task::XInteractionHandler > getInteractionHandler() const;

    // UpdateCheckConfigListener
    virtual void autoCheckStatusChanged(bool enabled) SAL_OVERRIDE;
    virtual void autoCheckIntervalChanged() SAL_OVERRIDE;

    // IActionListener
    void cancel() SAL_OVERRIDE;
    void download() SAL_OVERRIDE;
    void install() SAL_OVERRIDE;
    void pause() SAL_OVERRIDE;
    void resume() SAL_OVERRIDE;
    void closeAfterFailure() SAL_OVERRIDE;

private:

    // Schedules or cancels next automatic check for updates
    void enableAutoCheck(bool enable);

    // Starts/resumes or stops a download
    void enableDownload(bool enable, bool paused=false);

    // Shuts down the currently running thread
    void shutdownThread(bool join);

    // Returns the update handler instance
    rtl::Reference<UpdateHandler> getUpdateHandler();

    // Open the given URL in a browser
    void showReleaseNote(const OUString& rURL) const;

    // stores the release note url on disk to be used by setup app
    static bool storeReleaseNote(sal_Int8 nNum, const OUString &rURL);

    /* This method turns on the menubar icon and triggers the bubble window
     */
    void handleMenuBarUI( rtl::Reference< UpdateHandler > rUpdateHandler,
                          UpdateState& eState, bool suppressBubble );
    enum State {
        NOT_INITIALIZED,
        DISABLED,
        CHECK_SCHEDULED,
        DOWNLOADING,
        DOWNLOAD_PAUSED
    };

    State m_eState;
    UpdateState m_eUpdateState;

    mutable osl::Mutex m_aMutex;
    WorkerThread *m_pThread;
    osl::Condition m_aCondition;

    UpdateInfo m_aUpdateInfo;
    OUString m_aImageName;
    bool m_bHasExtensionUpdate;
    bool m_bShowExtUpdDlg;

    rtl::Reference<UpdateHandler> m_aUpdateHandler;
    com::sun::star::uno::Reference<com::sun::star::beans::XPropertySet> m_xMenuBarUI;
    com::sun::star::uno::Reference<com::sun::star::uno::XComponentContext> m_xContext;

    friend class UpdateCheckInitData;
};

inline rtl::Reference< UpdateCheck > SAL_CALL
UpdateCheckInitData::operator() () const
{
    return rtl::Reference< UpdateCheck > (new UpdateCheck());
}

#endif // INCLUDED_EXTENSIONS_SOURCE_UPDATE_CHECK_UPDATECHECK_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
