/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "SmartRewriteDialogController.hxx"
#include <wrtsh.hxx>
#include <sal/log.hxx>
#include "../../uibase/config/SmartRewriteService.hxx"

using namespace ::com::sun::star;

namespace sw
{

SmartRewriteDialogController::SmartRewriteDialogController(weld::Widget* pParent, SwWrtShell& rSh, const OUString& sSelectedText)
    : GenericDialogController(pParent, u"modules/swriter/ui/smartrewritedialog.ui"_ustr, u"SmartRewriteDialog"_ustr)
    , m_rSh(rSh)
    , m_sSelectedText(sSelectedText)
{
    SAL_INFO("sw.smartrewrite", "GenericDialogController constructor completed successfully");
    // Try to get UI elements with error checking
    SAL_INFO("sw.smartrewrite", "Getting style-combo...");
    m_xStyleCombo = m_xBuilder->weld_combo_box(u"style-combo"_ustr);
    if (!m_xStyleCombo) {
        SAL_WARN("sw.smartrewrite", "Failed to find style-combo widget - skipping for now");
        // Don't throw, just continue without combo box for debugging
    } else {
        SAL_INFO("sw.smartrewrite", "style-combo found successfully");
    }
    
    m_xPromptText = m_xBuilder->weld_text_view(u"prompt-text"_ustr);
    if (!m_xPromptText) {
        SAL_WARN("sw.smartrewrite", "Failed to find prompt-text widget");
        throw css::uno::RuntimeException(u"Failed to find prompt-text widget"_ustr);
    }
    
    m_xOKButton = m_xBuilder->weld_button(u"ok"_ustr);
    if (!m_xOKButton) {
        SAL_WARN("sw.smartrewrite", "Failed to find ok button");
        throw css::uno::RuntimeException(u"Failed to find ok button"_ustr);
    }
    
    m_xCancelButton = m_xBuilder->weld_button(u"cancel"_ustr);
    if (!m_xCancelButton) {
        SAL_WARN("sw.smartrewrite", "Failed to find cancel button");
        throw css::uno::RuntimeException(u"Failed to find cancel button"_ustr);
    }
    
    m_xHelpButton = m_xBuilder->weld_button(u"help"_ustr);
    if (!m_xHelpButton) {
        SAL_WARN("sw.smartrewrite", "Failed to find help button");
        throw css::uno::RuntimeException(u"Failed to find help button"_ustr);
    }
    
    // Set up event handlers
    if (m_xStyleCombo) {
        m_xStyleCombo->connect_changed(LINK(this, SmartRewriteDialogController, StyleComboChangedHdl));
        // Set focus to the style combo by default
        m_xStyleCombo->grab_focus();
    }
    m_xOKButton->connect_clicked(LINK(this, SmartRewriteDialogController, OKClickHdl));
    
    // For now, always enable OK button - configuration checking will be added in later prompts
    m_xOKButton->set_sensitive(true);
    
    SAL_INFO("sw.smartrewrite", "SmartRewriteDialogController initialized with text length: " << sSelectedText.getLength());
}

SmartRewriteDialogController::~SmartRewriteDialogController()
{
    SAL_INFO("sw.smartrewrite", "SmartRewriteDialogController destroyed");
}

OUString SmartRewriteDialogController::GetSelectedStyle() const
{
    if (m_xStyleCombo)
    {
        int nActive = m_xStyleCombo->get_active();
        if (nActive >= 0)
        {
            return m_xStyleCombo->get_text(nActive);
        }
    }
    return OUString();
}

OUString SmartRewriteDialogController::GetCustomPrompt() const
{
    if (m_xPromptText)
    {
        return m_xPromptText->get_text();
    }
    return OUString();
}

bool SmartRewriteDialogController::IsConfigurationValid() const
{
    // Check if Smart Rewrite is properly configured
    return SmartRewriteService::IsConfigured();
}

IMPL_LINK_NOARG(SmartRewriteDialogController, StyleComboChangedHdl, weld::ComboBox&, void)
{
    SAL_INFO("sw.smartrewrite", "Style changed to: " << GetSelectedStyle());
    
    // Enable custom prompt text when "Custom" style is selected
    if (m_xPromptText)
    {
        OUString sSelectedStyle = GetSelectedStyle();
        bool bCustomSelected = sSelectedStyle == "Custom";
        
        // You could enhance this to show/hide or enable/disable the prompt text
        // based on the selected style, but for now we'll always keep it available
        if (bCustomSelected)
        {
            m_xPromptText->grab_focus();
        }
    }
}

IMPL_LINK_NOARG(SmartRewriteDialogController, OKClickHdl, weld::Button&, void)
{
    SAL_INFO("sw.smartrewrite", "OK clicked - Style: " << GetSelectedStyle() 
             << ", Custom prompt length: " << GetCustomPrompt().getLength());
    
    // For now, just log the values. In later prompts, this will trigger the AI API call
    // The dialog will be closed by the standard response handling
    m_xDialog->response(RET_OK);
}

} // namespace sw

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */ 