/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <sfx2/dllapi.h>
#include <vcl/weld.hxx>
#include <rtl/ustring.hxx>

namespace sfx2
{

struct SaveToCloudResult
{
    OUString fileName;
    OUString fileExtension;
    OUString contentType;
    bool cancelled;

    SaveToCloudResult() : cancelled(true) {}
};

class SFX2_DLLPUBLIC SaveToCloudDialog final : public weld::GenericDialogController
{
private:
    std::unique_ptr<weld::Entry> m_xFileNameEntry;
    std::unique_ptr<weld::ComboBox> m_xFormatCombo;
    std::unique_ptr<weld::Button> m_xOKButton;
    std::unique_ptr<weld::Button> m_xCancelButton;

    SaveToCloudResult m_aResult;

    DECL_LINK(OKHdl, weld::Button&, void);
    DECL_LINK(CancelHdl, weld::Button&, void);
    DECL_LINK(FileNameModifyHdl, weld::Entry&, void);
    DECL_LINK(FormatSelectHdl, weld::ComboBox&, void);

    void updateOKButton();
    void setDefaultFileName(const OUString& rDocumentTitle);
    void setDefaultFormat(const OUString& rCurrentExtension);
    OUString getFileExtensionFromFormat() const;
    OUString getContentTypeFromFormat() const;

public:
    SaveToCloudDialog(weld::Window* pParent, const OUString& rDocumentTitle = OUString());
    virtual ~SaveToCloudDialog() override;

    const SaveToCloudResult& getResult() const { return m_aResult; }
};

} // namespace sfx2

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */ 