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
#include <vector>
#include <memory>

class CloudAuthHandler;
class CloudApiClient;

struct CloudDocument
{
    OUString docId;
    OUString fileName;
    OUString uploadedAt;
    sal_Int64 fileSize;
    
    CloudDocument(const OUString& id, const OUString& name, const OUString& date, sal_Int64 size)
        : docId(id), fileName(name), uploadedAt(date), fileSize(size) {}
};

/**
 * CloudFilesDialog shows cloud documents and handles authentication flow.
 * If user is not authenticated, shows login prompt.
 * If authenticated, shows list of cloud documents that can be opened.
 */
class SFX2_DLLPUBLIC CloudFilesDialog : public weld::GenericDialogController
{
private:
    std::unique_ptr<weld::Label> mxStatusLabel;
    std::unique_ptr<weld::Button> mxLoginButton;
    std::unique_ptr<weld::Button> mxRefreshButton;
    std::unique_ptr<weld::TreeView> mxDocumentsList;
    std::unique_ptr<weld::Button> mxOpenButton;
    std::unique_ptr<weld::Button> mxCancelButton;
    
    CloudAuthHandler* mpAuthHandler;
    CloudApiClient* mpApiClient;
    std::vector<CloudDocument> mDocuments;
    bool mbInitialized;

public:
    explicit CloudFilesDialog(weld::Window* pParent);
    virtual ~CloudFilesDialog() override;

    /**
     * Get the selected document URL to open
     * @return URL of selected document, empty if none or cancelled
     */
    OUString getSelectedDocumentUrl() const;

private:
    /**
     * Initialize the dialog - check authentication status and load documents
     */
    void initializeDialog();
    
    /**
     * Update UI based on authentication status
     */
    void updateAuthenticationStatus();
    
    /**
     * Load and display cloud documents
     */
    void loadCloudDocuments();
    
    /**
     * Parse JSON response and populate documents list
     */
    void parseDocumentsJson(const OUString& sJson);
    
    /**
     * Update the documents tree view
     */
    void updateDocumentsList();
    
    /**
     * Format file size for display
     */
    OUString formatFileSize(sal_Int64 nSize) const;
    
    /**
     * Format date string for display  
     */
    OUString formatDate(const OUString& sIsoDate) const;

    // Event handlers
    DECL_LINK(LoginClickHdl, weld::Button&, void);
    DECL_LINK(RefreshClickHdl, weld::Button&, void);
    DECL_LINK(OpenClickHdl, weld::Button&, void);
    DECL_LINK(CancelClickHdl, weld::Button&, void);
    DECL_LINK(DocumentSelectHdl, weld::TreeView&, void);
    DECL_LINK(DocumentActivateHdl, weld::TreeView&, bool);
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */ 