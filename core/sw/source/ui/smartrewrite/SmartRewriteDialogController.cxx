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

using namespace ::com::sun::star;

namespace sw
{

SmartRewriteDialogController::SmartRewriteDialogController(weld::Widget* pParent, SwWrtShell& rSh, const OUString& sSelectedText)
    : GenericDialogController(pParent, u"modules/swriter/ui/smartrewritedialog.ui"_ustr, u"SmartRewriteDialog"_ustr)
    , m_rSh(rSh)
    , m_sSelectedText(sSelectedText)
    , m_xStyleCombo(m_xBuilder->weld_combo_box(u"style-combo"_ustr))
    , m_xPromptText(m_xBuilder->weld_text_view(u"prompt-text"_ustr))
    , m_xOKButton(m_xBuilder->weld_button(u"ok"_ustr))
    , m_xCancelButton(m_xBuilder->weld_button(u"cancel"_ustr))
    , m_xHelpButton(m_xBuilder->weld_button(u"help"_ustr))
{
    // Set up event handlers
    m_xStyleCombo->connect_changed(LINK(this, SmartRewriteDialogController, StyleComboChangedHdl));
    m_xOKButton->connect_clicked(LINK(this, SmartRewriteDialogController, OKClickHdl));
    
    // Set focus to the style combo by default
    m_xStyleCombo->grab_focus();
    
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
    // For now, return true - configuration checking will be implemented in later prompts
    // when the SmartRewriteService dependency is properly resolved
    return true;
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