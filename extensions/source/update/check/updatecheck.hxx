/**************************************************************
 * 
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 * 
 * This file incorporates work covered by the following license notice:
 * 
 *   Modified March 2016 by Patrick Luby. NeoOffice is only distributed
 *   under the GNU General Public License, Version 3 as allowed by Section 4
 *   of the Apache License, Version 2.0.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 * 
 *************************************************************/



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
class UpdateCheckConfig;

class UpdateCheckInitData {
    
public:
    inline rtl::Reference< UpdateCheck > SAL_CALL operator() () const;
};

class WorkerThread : public osl::Thread
{
public:
    virtual void SAL_CALL cancel() = 0;
};

class UpdateCheck : 
    public UpdateCheckConfigListener,
    public IActionListener,
    public DownloadInteractionHandler,
    public salhelper::ReferenceObject,
    public rtl::StaticWithInit< rtl::Reference< UpdateCheck >, UpdateCheckInitData >
{
    UpdateCheck() : m_eState(NOT_INITIALIZED), m_eUpdateState(UPDATESTATES_COUNT), m_pThread(NULL) {};
    
public:
    inline SAL_CALL operator rtl::Reference< UpdateCheckConfigListener > ()
        { return static_cast< UpdateCheckConfigListener * > (this); }
       
    void initialize(const com::sun::star::uno::Sequence<com::sun::star::beans::NamedValue>& rValues,
                    const com::sun::star::uno::Reference<com::sun::star::uno::XComponentContext>& xContext);
        
    /* Returns an instance of the specified service obtained from the specified
     * component context
     */
    
    static com::sun::star::uno::Reference< com::sun::star::uno::XInterface > createService(
        const rtl::OUString& aServiceName, 
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
    virtual bool downloadTargetExists(const rtl::OUString& rFileName);
    virtual void downloadStalled(const rtl::OUString& rErrorMessage);
    virtual void downloadProgressAt(sal_Int8 nProcent);
    virtual void downloadStarted(const rtl::OUString& rLocalFileName, sal_Int64 nFileSize);
    virtual void downloadFinished(const rtl::OUString& rLocalFileName);
    // checks if the download target already exists and asks user what to do next
    virtual bool checkDownloadDestination( const rtl::OUString& rFile );

    // Cancels the download action (and resumes checking if enabled)
    void cancelDownload();
 
    // Returns the XInteractionHandler of the UpdateHandler instance if present (and visible)
    com::sun::star::uno::Reference< com::sun::star::task::XInteractionHandler > getInteractionHandler() const;

    // UpdateCheckConfigListener
    virtual void autoCheckStatusChanged(bool enabled); 
    virtual void autoCheckIntervalChanged();

    // IActionListener    
    void cancel();
    void download();
    void install();
    void pause();
    void resume();
    void closeAfterFailure();
                
    // rtl::IReference
    virtual oslInterlockedCount SAL_CALL acquire() SAL_THROW(());
    virtual oslInterlockedCount SAL_CALL release() SAL_THROW(());

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
    void showReleaseNote(const rtl::OUString& rURL) const;

    // stores the release note url on disk to be used by setup app
    static bool storeReleaseNote(sal_Int8 nNum, const rtl::OUString &rURL);

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
    rtl::OUString m_aImageName;
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
