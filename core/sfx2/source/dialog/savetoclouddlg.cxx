/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <sfx2/savetoclouddlg.hxx>
#include <vcl/weld.hxx>
#include <tools/urlobj.hxx>

using namespace sfx2;

SaveToCloudDialog::SaveToCloudDialog(weld::Window* pParent, const OUString& rDocumentTitle)
    : GenericDialogController(pParent, u"sfx/ui/savetocloud.ui"_ustr, u"SaveToCloudDialog"_ustr)
    , m_xFileNameEntry(m_xBuilder->weld_entry(u"filename_entry"_ustr))
    , m_xFormatCombo(m_xBuilder->weld_combo_box(u"format_combo"_ustr))
    , m_xOKButton(m_xBuilder->weld_button(u"ok"_ustr))
    , m_xCancelButton(m_xBuilder->weld_button(u"cancel"_ustr))
{
    // Set up event handlers
    m_xOKButton->connect_clicked(LINK(this, SaveToCloudDialog, OKHdl));
    m_xCancelButton->connect_clicked(LINK(this, SaveToCloudDialog, CancelHdl));
    m_xFileNameEntry->connect_changed(LINK(this, SaveToCloudDialog, FileNameModifyHdl));
    m_xFormatCombo->connect_changed(LINK(this, SaveToCloudDialog, FormatSelectHdl));

    // Set default values
    setDefaultFileName(rDocumentTitle);
    setDefaultFormat(u".odt"_ustr); // Default to ODT
    
    // Focus the filename entry
    m_xFileNameEntry->grab_focus();
    
    // Update OK button state
    updateOKButton();
}

SaveToCloudDialog::~SaveToCloudDialog()
{
}

void SaveToCloudDialog::setDefaultFileName(const OUString& rDocumentTitle)
{
    OUString sFileName = rDocumentTitle;
    
    if (sFileName.isEmpty())
    {
        sFileName = u"Document"_ustr;
    }
    else
    {
        // Remove any existing extension
        INetURLObject aURL;
        aURL.setName(sFileName);
        sFileName = aURL.getBase();
        
        // Clean up the filename - remove invalid characters
        sFileName = sFileName.replaceAll("/", "_");
        sFileName = sFileName.replaceAll("\\", "_");
        sFileName = sFileName.replaceAll(":", "_");
        sFileName = sFileName.replaceAll("*", "_");
        sFileName = sFileName.replaceAll("?", "_");
        sFileName = sFileName.replaceAll("\"", "_");
        sFileName = sFileName.replaceAll("<", "_");
        sFileName = sFileName.replaceAll(">", "_");
        sFileName = sFileName.replaceAll("|", "_");
    }
    
    m_xFileNameEntry->set_text(sFileName);
}

void SaveToCloudDialog::setDefaultFormat(const OUString& rCurrentExtension)
{
    // Map common extensions to combo box items
    OUString sExt = rCurrentExtension.toAsciiLowerCase();
    
    if (sExt == u".odt" || sExt == u".ott")
        m_xFormatCombo->set_active(0); // OpenDocument Text
    else if (sExt == u".ods" || sExt == u".ots")
        m_xFormatCombo->set_active(1); // OpenDocument Spreadsheet
    else if (sExt == u".odp" || sExt == u".otp")
        m_xFormatCombo->set_active(2); // OpenDocument Presentation
    else if (sExt == u".pdf")
        m_xFormatCombo->set_active(3); // PDF
    else if (sExt == u".docx" || sExt == u".doc")
        m_xFormatCombo->set_active(4); // Microsoft Word
    else if (sExt == u".xlsx" || sExt == u".xls")
        m_xFormatCombo->set_active(5); // Microsoft Excel
    else if (sExt == u".pptx" || sExt == u".ppt")
        m_xFormatCombo->set_active(6); // Microsoft PowerPoint
    else if (sExt == u".txt")
        m_xFormatCombo->set_active(7); // Text File
    else
        m_xFormatCombo->set_active(0); // Default to ODT
}

OUString SaveToCloudDialog::getFileExtensionFromFormat() const
{
    switch (m_xFormatCombo->get_active())
    {
        case 0: return u".odt"_ustr;
        case 1: return u".ods"_ustr;
        case 2: return u".odp"_ustr;
        case 3: return u".pdf"_ustr;
        case 4: return u".docx"_ustr;
        case 5: return u".xlsx"_ustr;
        case 6: return u".pptx"_ustr;
        case 7: return u".txt"_ustr;
        default: return u".odt"_ustr;
    }
}

OUString SaveToCloudDialog::getContentTypeFromFormat() const
{
    switch (m_xFormatCombo->get_active())
    {
        case 0: return u"application/vnd.oasis.opendocument.text"_ustr;
        case 1: return u"application/vnd.oasis.opendocument.spreadsheet"_ustr;
        case 2: return u"application/vnd.oasis.opendocument.presentation"_ustr;
        case 3: return u"application/pdf"_ustr;
        case 4: return u"application/vnd.openxmlformats-officedocument.wordprocessingml.document"_ustr;
        case 5: return u"application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"_ustr;
        case 6: return u"application/vnd.openxmlformats-officedocument.presentationml.presentation"_ustr;
        case 7: return u"text/plain"_ustr;
        default: return u"application/vnd.oasis.opendocument.text"_ustr;
    }
}

void SaveToCloudDialog::updateOKButton()
{
    // Enable OK button only if filename is not empty
    OUString sFileName = m_xFileNameEntry->get_text().trim();
    m_xOKButton->set_sensitive(!sFileName.isEmpty());
}

IMPL_LINK_NOARG(SaveToCloudDialog, OKHdl, weld::Button&, void)
{
    // Prepare result
    m_aResult.cancelled = false;
    m_aResult.fileName = m_xFileNameEntry->get_text().trim();
    m_aResult.fileExtension = getFileExtensionFromFormat();
    m_aResult.contentType = getContentTypeFromFormat();
    
    // Add extension to filename if not already present
    if (!m_aResult.fileName.endsWith(m_aResult.fileExtension))
    {
        m_aResult.fileName += m_aResult.fileExtension;
    }
    
    m_xDialog->response(RET_OK);
}

IMPL_LINK_NOARG(SaveToCloudDialog, CancelHdl, weld::Button&, void)
{
    m_aResult.cancelled = true;
    m_xDialog->response(RET_CANCEL);
}

IMPL_LINK_NOARG(SaveToCloudDialog, FileNameModifyHdl, weld::Entry&, void)
{
    updateOKButton();
}

IMPL_LINK_NOARG(SaveToCloudDialog, FormatSelectHdl, weld::ComboBox&, void)
{
    // Update the filename extension if needed
    OUString sCurrentName = m_xFileNameEntry->get_text();
    OUString sNewExtension = getFileExtensionFromFormat();
    
    // Remove existing extension and add new one
    INetURLObject aURL;
    aURL.setName(sCurrentName);
    OUString sBaseName = aURL.getBase();
    
    if (!sBaseName.isEmpty())
    {
        m_xFileNameEntry->set_text(sBaseName + sNewExtension);
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */ 