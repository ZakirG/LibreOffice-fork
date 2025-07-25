/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <sal/config.h>
#include <sfx2/dllapi.h>
#include <rtl/ustring.hxx>
#include <memory>
#include <vector>

class SfxObjectShell;
class SfxRequest;
class CloudApiClient;
class CloudAuthHandler;

enum class CloudUploadResult
{
    SUCCESS,
    AUTH_EXPIRED,
    GENERAL_ERROR
};

/**
 * SaveToCloudHandler handles saving LibreOffice documents to cloud storage.
 * Integrates with CloudAuthHandler and CloudApiClient for authentication and upload.
 */
class SFX2_DLLPUBLIC SaveToCloudHandler
{
private:
    SfxObjectShell* m_pObjectShell;
    CloudAuthHandler* m_pAuthHandler;
    CloudApiClient* m_pApiClient;
    
    // Progress tracking
    bool m_bOperationInProgress;
    
    // Cloud document tracking
    OUString m_sCloudDocumentId;
    
public:
    /**
     * Constructor
     * @param pObjectShell The document to save
     */
    explicit SaveToCloudHandler(SfxObjectShell* pObjectShell);
    
    /**
     * Destructor
     */
    ~SaveToCloudHandler();

    /**
     * Execute save to cloud operation
     * @param rReq The SfxRequest containing the command
     * @return true if operation started successfully
     */
    bool Execute(SfxRequest& rReq);

private:
    /**
     * Check if user is authenticated and ready for cloud operations
     * @return true if authenticated
     */
    bool checkAuthentication();

    /**
     * Get the document data as byte stream
     * @param rDocumentData [out] Document data as binary vector
     * @param rsFileName [out] Generated filename with extension
     * @param rsContentType [out] MIME content type
     * @return true if successful
     */
    bool getDocumentData(std::vector<char>& rDocumentData, OUString& rsFileName, OUString& rsContentType);

    /**
     * Get the document data with specific format
     * @param rDocumentData [out] Document data as binary vector
     * @param rsFileName File name with extension
     * @param rsContentType MIME content type
     * @param rsExtension File extension (e.g., ".odt", ".pdf")
     * @return true if successful
     */
    bool getDocumentDataWithFormat(std::vector<char>& rDocumentData, const OUString& rsFileName, const OUString& rsContentType, const OUString& rsExtension);

    /**
     * Upload document to cloud storage
     * @param rDocumentData Document data as binary vector
     * @param sFileName File name with extension
     * @param sContentType MIME content type
     * @return true if successful
     */
    bool uploadToCloud(const std::vector<char>& rDocumentData, const OUString& sFileName, const OUString& sContentType);

    /**
     * Check if document has an existing cloud document ID (was opened from cloud)
     * @param rsCloudDocId [out] The existing cloud document ID if found
     * @param rsOriginalFileName [out] The original filename if found
     * @param rsOriginalExtension [out] The original file extension if found
     * @param rsOriginalContentType [out] The original content type if found
     * @return true if document was opened from cloud
     */
    bool getExistingCloudDocumentId(OUString& rsCloudDocId, OUString& rsOriginalFileName, 
                                   OUString& rsOriginalExtension, OUString& rsOriginalContentType);

    /**
     * Upload document to cloud storage with improved error handling
     * @param rDocumentData Document data as binary vector
     * @param sFileName File name with extension
     * @param sContentType MIME content type
     * @return CloudUploadResult indicating success or specific error type
     */
    CloudUploadResult uploadToCloudWithErrorHandling(const std::vector<char>& rDocumentData, const OUString& sFileName, const OUString& sContentType);

    /**
     * Handle expired authentication gracefully
     */
    void handleExpiredAuthentication();

    /**
     * Show upload progress dialog
     * @param sMessage Progress message
     */
    void showProgressDialog(const OUString& sMessage);

    /**
     * Hide progress dialog
     */
    void hideProgressDialog();

    /**
     * Show success message
     * @param sDocumentName Name of the uploaded document
     */
    void showSuccessMessage(const OUString& sDocumentName);

    /**
     * Show error message
     * @param sError Error description
     */
    void showErrorMessage(const OUString& sError);

    /**
     * Generate appropriate filename based on document type
     * @return filename with extension
     */
    OUString generateFileName();

    /**
     * Get content type based on document type
     * @return MIME content type
     */
    OUString getContentType();
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */ 