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
#include <map>
#include <vector>

struct curl_slist;
typedef void CURL;

/**
 * CloudApiClient handles HTTP communication with the Libre Cloud backend.
 * Uses libcurl for cross-platform HTTP requests.
 */
class SFX2_DLLPUBLIC CloudApiClient
{
private:
    CURL* m_pCurl;
    curl_slist* m_pHeaders;
    OUString m_sBaseUrl;
    OUString m_sJwtToken;
    long m_nLastResponseCode;

public:
    CloudApiClient();
    ~CloudApiClient();

    /**
     * Set the base URL for the cloud API
     * @param sBaseUrl Base URL (e.g., "http://localhost:3009")
     */
    void setBaseUrl(const OUString& sBaseUrl);

    /**
     * Set JWT token for authenticated requests
     * @param sToken JWT token
     */
    void setJwtToken(const OUString& sToken);

    /**
     * Get the HTTP response code from the last request
     * @return HTTP response code (e.g., 200, 401, 404)
     */
    long getLastResponseCode() const;

    /**
     * Initialize desktop authentication by requesting a nonce
     * @param rsNonce [out] Generated nonce
     * @param rsLoginUrl [out] URL to open in browser
     * @return true if successful
     */
    bool initDesktopAuth(OUString& rsNonce, OUString& rsLoginUrl);

    /**
     * Poll for authentication token using nonce
     * @param sNonce The nonce to check
     * @param rsToken [out] JWT token if authentication completed
     * @return true if token received, false if still pending
     */
    bool pollForToken(const OUString& sNonce, OUString& rsToken);

    /**
     * Request presigned URL for document operations
     * @param sMode "put" for upload, "get" for download
     * @param sFileName File name with extension
     * @param sContentType MIME type (for uploads)
     * @param rsPresignedUrl [out] Presigned URL
     * @param rsDocId [out] Document ID
     * @return true if successful
     */
    bool requestPresignedUrl(const OUString& sMode, const OUString& sFileName, 
                            const OUString& sContentType, OUString& rsPresignedUrl, 
                            OUString& rsDocId);

    /**
     * Register document metadata after successful upload
     * @param sDocId Document ID
     * @param sFileName File name
     * @param nFileSize File size in bytes
     * @return true if successful
     */
    bool registerDocument(const OUString& sDocId, const OUString& sFileName, sal_Int64 nFileSize);

    /**
     * Get list of user documents
     * @param rsDocumentsJson [out] JSON response with document list
     * @return true if successful
     */
    bool getDocuments(OUString& rsDocumentsJson);

    /**
     * Delete document
     * @param sDocId Document ID to delete
     * @return true if successful
     */
    bool deleteDocument(const OUString& sDocId);

    /**
     * Request presigned URL for an existing document (by document ID)
     * @param sDocId Document ID 
     * @param sMode "get" for download, "put" for upload
     * @param sFileName File name (required for PUT operations)
     * @param sContentType MIME type (required for PUT operations)
     * @param rsPresignedUrl [out] Presigned URL
     * @return true if successful
     */
    bool requestPresignedUrlForDocument(const OUString& sDocId, const OUString& sMode, 
                                       const OUString& sFileName, const OUString& sContentType,
                                       OUString& rsPresignedUrl);

    /**
     * Download document content from presigned URL
     * @param sPresignedUrl Presigned URL to download from
     * @param rDocumentData [out] Document binary data
     * @return true if successful
     */
    bool downloadDocument(const OUString& sPresignedUrl, std::vector<char>& rDocumentData);

    /**
     * Update document metadata (lastModified, fileSize, etc.)
     * @param sDocId Document ID to update
     * @param sFileName File name
     * @param nFileSize File size in bytes
     * @return true if successful
     */
    bool updateDocumentMetadata(const OUString& sDocId, const OUString& sFileName, sal_Int64 nFileSize);

    /**
     * Upload binary data to URL (for presigned URL uploads)
     * @param sUrl URL to upload to (presigned URL)
     * @param pData Binary data to upload
     * @param nDataSize Size of binary data
     * @param sContentType MIME type of the data
     * @param rsResponse [out] Response body
     * @param pnResponseCode [out] HTTP response code (optional)
     * @return true if request completed successfully
     */
    bool uploadFile(const OUString& sUrl, const char* pData, size_t nDataSize, 
                   const OUString& sContentType, OUString& rsResponse, long* pnResponseCode = nullptr);

private:
    /**
     * Perform HTTP GET request
     * @param sUrl URL to request
     * @param rsResponse [out] Response body
     * @param pnResponseCode [out] HTTP response code (optional)
     * @return true if request completed successfully
     */
    bool httpGet(const OUString& sUrl, OUString& rsResponse, long* pnResponseCode = nullptr);

    /**
     * Perform HTTP POST request
     * @param sUrl URL to request
     * @param sBody Request body (JSON)
     * @param rsResponse [out] Response body
     * @param pnResponseCode [out] HTTP response code (optional)
     * @return true if request completed successfully
     */
    bool httpPost(const OUString& sUrl, const OUString& sBody, OUString& rsResponse, long* pnResponseCode = nullptr);

    /**
     * Perform HTTP DELETE request
     * @param sUrl URL to request
     * @param rsResponse [out] Response body
     * @param pnResponseCode [out] HTTP response code (optional)
     * @return true if request completed successfully
     */
    bool httpDelete(const OUString& sUrl, OUString& rsResponse, long* pnResponseCode = nullptr);

    /**
     * Perform HTTP PATCH request
     * @param sUrl URL to request
     * @param sBody Request body (JSON)
     * @param rsResponse [out] Response body
     * @param pnResponseCode [out] HTTP response code (optional)
     * @return true if request completed successfully
     */
    bool httpPatch(const OUString& sUrl, const OUString& sBody, OUString& rsResponse, long* pnResponseCode = nullptr);

    /**
     * Get current timestamp in ISO 8601 format
     * @return Current timestamp as ISO string
     */
    OUString getCurrentISOTimestamp();

    /**
     * Set up common curl options
     */
    void setupCommonOptions();

    /**
     * Add authentication header if JWT token is set
     */
    void addAuthHeader();

    /**
     * Parse JSON response to extract specific fields
     * @param sJson JSON string
     * @param sField Field name to extract
     * @return Field value or empty string if not found
     */
    OUString extractJsonField(const OUString& sJson, const OUString& sField) const;

    /**
     * Callback function for curl to write response data
     */
    static size_t writeCallback(void* contents, size_t size, size_t nmemb, void* userp);
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */ 