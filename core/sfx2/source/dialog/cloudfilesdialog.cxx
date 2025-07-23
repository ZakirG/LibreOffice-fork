/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <sfx2/cloudfilesdialog.hxx>
#include <sfx2/cloudauth.hxx>
#include <sfx2/cloudapi.hxx>

#include <comphelper/string.hxx>
#include <sal/log.hxx>
#include <tools/json_writer.hxx>
#include <tools/stream.hxx>
#include <svtools/miscopt.hxx>
#include <unotools/syslocale.hxx>
#include <unotools/localedatawrapper.hxx>
#include <rtl/math.hxx>

using namespace css;

CloudFilesDialog::CloudFilesDialog(weld::Window* pParent)
    : GenericDialogController(pParent, u"sfx/ui/cloudfilesdialog.ui"_ustr, u"CloudFilesDialog"_ustr)
    , mxStatusLabel(m_xBuilder->weld_label(u"status_label"_ustr))
    , mxLoginButton(m_xBuilder->weld_button(u"login_button"_ustr))
    , mxRefreshButton(m_xBuilder->weld_button(u"refresh_button"_ustr))
    , mxDocumentsList(m_xBuilder->weld_tree_view(u"documents_list"_ustr))
    , mxOpenButton(m_xBuilder->weld_button(u"open"_ustr))
    , mxCancelButton(m_xBuilder->weld_button(u"cancel"_ustr))
    , mpAuthHandler(nullptr)
    , mpApiClient(nullptr)
    , mbInitialized(false)
{
    // Connect event handlers
    mxLoginButton->connect_clicked(LINK(this, CloudFilesDialog, LoginClickHdl));
    mxRefreshButton->connect_clicked(LINK(this, CloudFilesDialog, RefreshClickHdl));
    mxOpenButton->connect_clicked(LINK(this, CloudFilesDialog, OpenClickHdl));
    mxCancelButton->connect_clicked(LINK(this, CloudFilesDialog, CancelClickHdl));
    mxDocumentsList->connect_selection_changed(LINK(this, CloudFilesDialog, DocumentSelectHdl));
    mxDocumentsList->connect_row_activated(LINK(this, CloudFilesDialog, DocumentActivateHdl));

    // Set up tree view with simple text display
    // TreeView in LibreOffice uses tab-separated values for multiple columns
    
    // Initially disable open button
    mxOpenButton->set_sensitive(false);

    // Initialize dialog
    initializeDialog();
}

CloudFilesDialog::~CloudFilesDialog() = default;

void CloudFilesDialog::initializeDialog()
{
    try
    {
        mpAuthHandler = &CloudAuthHandler::getInstance();
        
        if (mpAuthHandler->isAuthenticated())
        {
            mpApiClient = mpAuthHandler->getApiClient();
        }
        
        updateAuthenticationStatus();
        mbInitialized = true;
    }
    catch (const std::exception& e)
    {
        SAL_WARN("sfx.control", "CloudFilesDialog initialization failed: " << e.what());
        mxStatusLabel->set_label("Failed to initialize cloud files dialog.");
        mxLoginButton->set_visible(false);
        mxRefreshButton->set_visible(false);
    }
}

void CloudFilesDialog::updateAuthenticationStatus()
{
    if (!mpAuthHandler)
    {
        mxStatusLabel->set_label("Authentication system not available.");
        mxLoginButton->set_visible(false);
        mxRefreshButton->set_visible(false);
        return;
    }

    if (mpAuthHandler->isAuthInProgress())
    {
        mxStatusLabel->set_label("Authentication in progress... Please complete login in your browser.");
        mxLoginButton->set_visible(false);
        mxRefreshButton->set_visible(true);
    }
    else if (mpAuthHandler->isAuthenticated())
    {
        mxStatusLabel->set_label("Connected to LibreCloud. Loading your documents...");
        mxLoginButton->set_visible(false);
        mxRefreshButton->set_visible(true);
        loadCloudDocuments();
    }
    else
    {
        mxStatusLabel->set_label("You are not logged in to LibreCloud.");
        mxLoginButton->set_visible(true);
        mxRefreshButton->set_visible(false);
        mDocuments.clear();
        updateDocumentsList();
    }
}

void CloudFilesDialog::loadCloudDocuments()
{
    if (!mpApiClient)
    {
        mxStatusLabel->set_label("API client not available.");
        return;
    }

    mxStatusLabel->set_label("Loading cloud documents...");
    mxRefreshButton->set_sensitive(false);

    OUString sDocumentsJson;
    if (mpApiClient->getDocuments(sDocumentsJson))
    {
        parseDocumentsJson(sDocumentsJson);
        updateDocumentsList();
        
        if (mDocuments.empty())
        {
            mxStatusLabel->set_label("No documents found in your LibreCloud storage.");
        }
        else
        {
            OUString sMessage = "Found " + OUString::number(mDocuments.size()) + " document(s) in your LibreCloud storage.";
            mxStatusLabel->set_label(sMessage);
        }
    }
    else
    {
        mxStatusLabel->set_label("Failed to load cloud documents. Please try again.");
        mDocuments.clear();
        updateDocumentsList();
    }

    mxRefreshButton->set_sensitive(true);
}

void CloudFilesDialog::parseDocumentsJson(const OUString& sJson)
{
    mDocuments.clear();
    
    // Simple JSON parsing for documents array
    // Expected format: {"documents": [{"docId": "...", "fileName": "...", "uploadedAt": "...", "fileSize": ...}]}
    
    OString sJsonUtf8 = OUStringToOString(sJson, RTL_TEXTENCODING_UTF8);
    
    // Find the documents array
    sal_Int32 nDocsStart = sJsonUtf8.indexOf("\"documents\"");
    if (nDocsStart == -1)
        return;
        
    nDocsStart = sJsonUtf8.indexOf('[', nDocsStart);
    if (nDocsStart == -1)
        return;
        
    sal_Int32 nDocsEnd = sJsonUtf8.lastIndexOf(']');
    if (nDocsEnd == -1 || nDocsEnd <= nDocsStart)
        return;

    // Extract documents array content
    OString sDocsArray = sJsonUtf8.copy(nDocsStart + 1, nDocsEnd - nDocsStart - 1);
    
    // Parse individual documents (simplified parsing)
    sal_Int32 nPos = 0;
    while (nPos < sDocsArray.getLength())
    {
        sal_Int32 nObjStart = sDocsArray.indexOf('{', nPos);
        if (nObjStart == -1)
            break;
            
        sal_Int32 nObjEnd = sDocsArray.indexOf('}', nObjStart);
        if (nObjEnd == -1)
            break;
            
        OString sDocObj = sDocsArray.copy(nObjStart, nObjEnd - nObjStart + 1);
        
        // Extract fields
        auto extractField = [&sDocObj](const char* field) -> OUString {
            OString sSearch = OString("\"") + field + "\":\"";
            sal_Int32 nStart = sDocObj.indexOf(sSearch);
            if (nStart == -1)
                return OUString();
            nStart += sSearch.getLength();
            sal_Int32 nEnd = sDocObj.indexOf("\"", nStart);
            if (nEnd == -1)
                return OUString();
            return OUString::fromUtf8(sDocObj.copy(nStart, nEnd - nStart));
        };
        
        auto extractNumberField = [&sDocObj](const char* field) -> sal_Int64 {
            OString sSearch = OString("\"") + field + "\":";
            sal_Int32 nStart = sDocObj.indexOf(sSearch);
            if (nStart == -1)
                return 0;
            nStart += sSearch.getLength();
            sal_Int32 nEnd = sDocObj.indexOf(',', nStart);
            sal_Int32 nEndBrace = sDocObj.indexOf('}', nStart);
            if (nEnd == -1 || (nEndBrace != -1 && nEndBrace < nEnd))
                nEnd = nEndBrace;
            if (nEnd == -1)
                nEnd = sDocObj.getLength();
            OString sValue = sDocObj.copy(nStart, nEnd - nStart).trim();
            return sValue.toInt64();
        };
        
        OUString sDocId = extractField("docId");
        OUString sFileName = extractField("fileName");
        OUString sUploadedAt = extractField("uploadedAt");
        sal_Int64 nFileSize = extractNumberField("fileSize");
        
        if (!sDocId.isEmpty() && !sFileName.isEmpty())
        {
            mDocuments.emplace_back(sDocId, sFileName, sUploadedAt, nFileSize);
        }
        
        nPos = nObjEnd + 1;
    }
}

void CloudFilesDialog::updateDocumentsList()
{
    mxDocumentsList->clear();
    
    for (const auto& doc : mDocuments)
    {
        OUString sRow = doc.fileName + "\t" + formatDate(doc.uploadedAt) + "\t" + formatFileSize(doc.fileSize);
        mxDocumentsList->append_text(sRow);
    }
    
    mxOpenButton->set_sensitive(false);
}

OUString CloudFilesDialog::formatFileSize(sal_Int64 nSize) const
{
    if (nSize <= 0)
        return "0 B";
        
    double fSize = static_cast<double>(nSize);
    const char* units[] = {"B", "KB", "MB", "GB"};
    int unitIndex = 0;
    
    while (fSize >= 1024.0 && unitIndex < 3)
    {
        fSize /= 1024.0;
        unitIndex++;
    }
    
    OUString sFormatted;
    if (unitIndex == 0)
    {
        sFormatted = OUString::number(static_cast<sal_Int64>(fSize));
    }
    else
    {
        sFormatted = rtl::math::doubleToUString(fSize, rtl_math_StringFormat_F, 1, '.', 0, 0);
    }
    
    return sFormatted + " " + OUString::fromUtf8(units[unitIndex]);
}

OUString CloudFilesDialog::formatDate(const OUString& sIsoDate) const
{
    if (sIsoDate.isEmpty())
        return "Unknown";
        
    // Simple date formatting - just return the date part for now
    sal_Int32 nTPos = sIsoDate.indexOf('T');
    if (nTPos != -1)
    {
        return sIsoDate.copy(0, nTPos);
    }
    
    return sIsoDate;
}

OUString CloudFilesDialog::getSelectedDocumentUrl() const
{
    int nSelected = mxDocumentsList->get_selected_index();
    if (nSelected < 0 || nSelected >= static_cast<int>(mDocuments.size()))
        return OUString();
        
    // Return a cloud:// URL with the document ID
    return "cloud://" + mDocuments[nSelected].docId;
}

// Event handlers

IMPL_LINK_NOARG(CloudFilesDialog, LoginClickHdl, weld::Button&, void)
{
    if (mpAuthHandler)
    {
        mpAuthHandler->startAuthentication();
        updateAuthenticationStatus();
    }
}

IMPL_LINK_NOARG(CloudFilesDialog, RefreshClickHdl, weld::Button&, void)
{
    updateAuthenticationStatus();
}

IMPL_LINK_NOARG(CloudFilesDialog, OpenClickHdl, weld::Button&, void)
{
    m_xDialog->response(RET_OK);
}

IMPL_LINK_NOARG(CloudFilesDialog, CancelClickHdl, weld::Button&, void)
{
    m_xDialog->response(RET_CANCEL);
}

IMPL_LINK_NOARG(CloudFilesDialog, DocumentSelectHdl, weld::TreeView&, void)
{
    int nSelected = mxDocumentsList->get_selected_index();
    mxOpenButton->set_sensitive(nSelected >= 0);
}

IMPL_LINK_NOARG(CloudFilesDialog, DocumentActivateHdl, weld::TreeView&, bool)
{
    // Double-click on document = open it
    int nSelected = mxDocumentsList->get_selected_index();
    if (nSelected >= 0)
    {
        m_xDialog->response(RET_OK);
    }
    return true;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */ 