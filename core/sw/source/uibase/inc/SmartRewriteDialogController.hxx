/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_SW_SOURCE_UIBASE_INC_SMARTREWRITEDIALOGCONTROLLER_HXX
#define INCLUDED_SW_SOURCE_UIBASE_INC_SMARTREWRITEDIALOGCONTROLLER_HXX

#include <vcl/weld.hxx>
#include <rtl/ustring.hxx>

class SwWrtShell;

namespace sw
{
/**
 * Dialog controller for the Smart Rewrite with AI feature.
 * Allows users to select rewrite styles and enter custom prompts.
 */
class SmartRewriteDialogController final : public weld::GenericDialogController
{
private:
    SwWrtShell& m_rSh;
    OUString m_sSelectedText;
    
    // UI Controls
    std::unique_ptr<weld::ComboBox> m_xStyleCombo;
    std::unique_ptr<weld::TextView> m_xPromptText;
    std::unique_ptr<weld::Button> m_xOKButton;
    std::unique_ptr<weld::Button> m_xCancelButton;
    std::unique_ptr<weld::Button> m_xHelpButton;
    
    // Event handlers
    DECL_LINK(StyleComboChangedHdl, weld::ComboBox&, void);
    DECL_LINK(OKClickHdl, weld::Button&, void);
    
public:
    /**
     * Constructor for SmartRewriteDialogController.
     * @param pParent Parent widget
     * @param rSh Writer shell reference
     * @param sSelectedText The currently selected text to be rewritten
     */
    SmartRewriteDialogController(weld::Widget* pParent, SwWrtShell& rSh, const OUString& sSelectedText);
    
    virtual ~SmartRewriteDialogController() override;
    
    /**
     * Get the selected rewrite style.
     * @return The selected style as string
     */
    OUString GetSelectedStyle() const;
    
    /**
     * Get the custom prompt text entered by the user.
     * @return The custom prompt text
     */
    OUString GetCustomPrompt() const;
    
    /**
     * Get the currently selected text that will be rewritten.
     * @return The selected text
     */
    const OUString& GetSelectedText() const { return m_sSelectedText; }
    
    /**
     * Check if the feature is properly configured and can be used.
     * @return true if configuration is valid
     */
    bool IsConfigurationValid() const;
};

} // namespace sw

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */ 