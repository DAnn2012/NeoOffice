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
 *   Modified August 2017 by Patrick Luby. NeoOffice is only distributed
 *   under the GNU General Public License, Version 3 as allowed by Section 3.3
 *   of the Mozilla Public License, v. 2.0.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <unotools/securityoptions.hxx>
#include <svtools/stdctrl.hxx>
#include <dialmgr.hxx>
#include <cuires.hrc>
#include "securityoptions.hxx"

namespace
{
    bool enableAndSet( const SvtSecurityOptions& rOptions,
                       SvtSecurityOptions::EOption eOption,
                       CheckBox& rCheckBox, FixedImage& rFixedImage )
    {
        bool bEnable = rOptions.IsOptionEnabled( eOption );
        rCheckBox.Enable( bEnable );
        rFixedImage.Show( !bEnable );
        rCheckBox.Check( rOptions.IsOptionSet( eOption ) );
        return bEnable;
    }
}


namespace svx
{


SecurityOptionsDialog::SecurityOptionsDialog(vcl::Window* pParent, SvtSecurityOptions* pOptions)
    : ModalDialog(pParent, "SecurityOptionsDialog", "cui/ui/securityoptionsdialog.ui")
{
    DBG_ASSERT( pOptions, "SecurityOptionsDialog::SecurityOptionsDialog(): invalid SvtSecurityOptions" );
    get(m_pSaveOrSendDocsCB, "savesenddocs");
    enableAndSet(*pOptions, SvtSecurityOptions::E_DOCWARN_SAVEORSEND, *m_pSaveOrSendDocsCB,
        *get<FixedImage>("locksavesenddocs"));
    get(m_pSignDocsCB, "whensigning");
    enableAndSet(*pOptions, SvtSecurityOptions::E_DOCWARN_SIGNING, *m_pSignDocsCB,
        *get<FixedImage>("lockwhensigning"));
    get(m_pPrintDocsCB, "whenprinting");
    enableAndSet(*pOptions, SvtSecurityOptions::E_DOCWARN_PRINT, *m_pPrintDocsCB,
        *get<FixedImage>("lockwhenprinting"));
    get(m_pCreatePdfCB, "whenpdf");
    enableAndSet(*pOptions, SvtSecurityOptions::E_DOCWARN_CREATEPDF, *m_pCreatePdfCB,
        *get<FixedImage>("lockwhenpdf"));
    get(m_pRemovePersInfoCB, "removepersonal");
    enableAndSet(*pOptions, SvtSecurityOptions::E_DOCWARN_REMOVEPERSONALINFO, *m_pRemovePersInfoCB,
        *get<FixedImage>("lockremovepersonal"));
    get(m_pRecommPasswdCB, "password");
    enableAndSet(*pOptions, SvtSecurityOptions::E_DOCWARN_RECOMMENDPASSWORD, *m_pRecommPasswdCB,
        *get<FixedImage>("lockpassword"));
    get(m_pCtrlHyperlinkCB, "ctrlclick");
    enableAndSet(*pOptions, SvtSecurityOptions::E_CTRLCLICK_HYPERLINK, *m_pCtrlHyperlinkCB,
        *get<FixedImage>("lockctrlclick"));
    get(m_pBlockUntrustedRefererLinksCB, "blockuntrusted");
    enableAndSet(*pOptions, SvtSecurityOptions::E_BLOCKUNTRUSTEDREFERERLINKS, *m_pBlockUntrustedRefererLinksCB,
        *get<FixedImage>("lockblockuntrusted"));

#if defined USE_JAVA && defined MACOSX
    OUString aCtrlClickStr = m_pCtrlHyperlinkCB->GetText();
    aCtrlClickStr = aCtrlClickStr.replaceAll( "Ctrl", "Command" );
    aCtrlClickStr = aCtrlClickStr.replaceAll( "ctrl", "Command" );
    m_pCtrlHyperlinkCB->SetText(aCtrlClickStr);
#endif  // USE_JAVA && MACOSX
}

SecurityOptionsDialog::~SecurityOptionsDialog()
{
}


}   // namespace svx


/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
