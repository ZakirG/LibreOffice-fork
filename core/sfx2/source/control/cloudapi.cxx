/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <sfx2/cloudapi.hxx>

#include <curl/curl.h>
#include <rtl/string.hxx>
#include <rtl/strbuf.hxx>
#include <sal/log.hxx>
#include <tools/json_writer.hxx>
#include <comphelper/string.hxx>
#include <iostream>
#include <cstdio>
#include <cstring>

namespace
{
    // Helper structure for curl response data
    struct CurlResponse
    {
        OStringBuffer data;
    };

    // Simple JSON field extraction (basic implementation)
    OUString extractJsonValue(const OUString& sJson, const OUString& sField)
    {
        OString sJsonUtf8 = OUStringToOString(sJson, RTL_TEXTENCODING_UTF8);
        OString sFieldUtf8 = OUStringToOString(sField, RTL_TEXTENCODING_UTF8);
        
        OString sSearch = "\"" + sFieldUtf8 + "\":\"";
        sal_Int32 nStart = sJsonUtf8.indexOf(sSearch);
        if (nStart == -1)
            return OUString();
            
        nStart += sSearch.getLength();
        sal_Int32 nEnd = sJsonUtf8.indexOf("\"", nStart);
        if (nEnd == -1)
            return OUString();
            
        OString sValue = sJsonUtf8.copy(nStart, nEnd - nStart);
        return OUString::fromUtf8(sValue);
    }
}

CloudApiClient::CloudApiClient()
    : m_pCurl(nullptr)
    , m_pHeaders(nullptr)
    , m_nLastResponseCode(0)
{
    // Initialize libcurl
    curl_global_init(CURL_GLOBAL_DEFAULT);
    m_pCurl = curl_easy_init();
    
    if (m_pCurl)
    {
        setupCommonOptions();
    }
}

CloudApiClient::~CloudApiClient()
{
    if (m_pHeaders)
    {
        curl_slist_free_all(m_pHeaders);
    }
    
    if (m_pCurl)
    {
        curl_easy_cleanup(m_pCurl);
    }
    
    curl_global_cleanup();
}

void CloudApiClient::setBaseUrl(const OUString& sBaseUrl)
{
    m_sBaseUrl = sBaseUrl;
}

void CloudApiClient::setJwtToken(const OUString& sToken)
{
    m_sJwtToken = sToken;
}

long CloudApiClient::getLastResponseCode() const
{
    return m_nLastResponseCode;
}

bool CloudApiClient::initDesktopAuth(OUString& rsNonce, OUString& rsLoginUrl)
{
    std::cerr << "*** DEBUG: CloudApiClient::initDesktopAuth() called ***" << std::endl;
    SAL_WARN("sfx.control", "=== CloudApiClient::initDesktopAuth() called ===");
    
    if (!m_pCurl)
    {
        std::cerr << "*** DEBUG: m_pCurl is NULL ***" << std::endl;
        SAL_WARN("sfx.control", "initDesktopAuth failed: m_pCurl is null");
        return false;
    }

    OUString sUrl = m_sBaseUrl + "/api/desktop-init";
    OUString sResponse;
    long nResponseCode = 0;

    std::cerr << "*** DEBUG: About to POST to: " << OUStringToOString(sUrl, RTL_TEXTENCODING_UTF8).getStr() << " ***" << std::endl;
    SAL_WARN("sfx.control", "Sending POST request to: " << sUrl);

    // Empty POST body
    if (!httpPost(sUrl, "", sResponse, &nResponseCode))
    {
        std::cerr << "*** DEBUG: httpPost FAILED ***" << std::endl;
        SAL_WARN("sfx.control", "httpPost failed for desktop-init");
        return false;
    }

    std::cerr << "*** DEBUG: HTTP response code: " << nResponseCode << " ***" << std::endl;
    std::cerr << "*** DEBUG: HTTP response body: " << OUStringToOString(sResponse, RTL_TEXTENCODING_UTF8).getStr() << " ***" << std::endl;
    SAL_WARN("sfx.control", "desktop-init response code: " << nResponseCode);
    SAL_WARN("sfx.control", "desktop-init response body: " << sResponse);

    if (nResponseCode != 200)
    {
        std::cerr << "*** DEBUG: Non-200 response code ***" << std::endl;
        SAL_WARN("sfx.control", "Desktop auth init failed with code: " << nResponseCode);
        return false;
    }

    // Extract nonce and loginUrl from JSON response
    rsNonce = extractJsonValue(sResponse, "nonce");
    rsLoginUrl = extractJsonValue(sResponse, "loginUrl");

    std::cerr << "*** DEBUG: Extracted nonce: " << OUStringToOString(rsNonce, RTL_TEXTENCODING_UTF8).getStr() << " ***" << std::endl;
    std::cerr << "*** DEBUG: Extracted loginUrl: " << OUStringToOString(rsLoginUrl, RTL_TEXTENCODING_UTF8).getStr() << " ***" << std::endl;
    SAL_WARN("sfx.control", "Extracted nonce: " << rsNonce);
    SAL_WARN("sfx.control", "Extracted loginUrl: " << rsLoginUrl);

    bool bSuccess = !rsNonce.isEmpty() && !rsLoginUrl.isEmpty();
    std::cerr << "*** DEBUG: initDesktopAuth result: " << (bSuccess ? "SUCCESS" : "FAILED") << " ***" << std::endl;
    SAL_WARN("sfx.control", "initDesktopAuth result: " << (bSuccess ? "SUCCESS" : "FAILED"));
    
    return bSuccess;
}

bool CloudApiClient::pollForToken(const OUString& sNonce, OUString& rsToken)
{
    if (!m_pCurl || sNonce.isEmpty())
    {
        SAL_WARN("sfx.control", "CloudApiClient::pollForToken - Invalid state: curl=" << (m_pCurl ? "OK" : "NULL") << ", nonce empty=" << sNonce.isEmpty());
        return false;
    }

    OUString sUrl = m_sBaseUrl + "/api/desktop-token?nonce=" + sNonce;
    OUString sResponse;
    long nResponseCode = 0;

    SAL_WARN("sfx.control", "CloudApiClient polling URL: " << sUrl);

    if (!httpGet(sUrl, sResponse, &nResponseCode))
    {
        SAL_WARN("sfx.control", "CloudApiClient::pollForToken - HTTP request failed");
        return false;
    }

    SAL_WARN("sfx.control", "CloudApiClient got response code: " << nResponseCode);
    SAL_WARN("sfx.control", "CloudApiClient response body: " << sResponse);

    if (nResponseCode == 200)
    {
        // Authentication completed, extract token
        rsToken = extractJsonValue(sResponse, "token");
        std::cerr << "*** DEBUG: Raw token extracted: " << OUStringToOString(rsToken, RTL_TEXTENCODING_UTF8).getStr() << " ***" << std::endl;
        SAL_WARN("sfx.control", "CloudApiClient extracted token, empty=" << rsToken.isEmpty());
        if (!rsToken.isEmpty())
        {
            SAL_WARN("sfx.control", "CloudApiClient token preview: " << rsToken.copy(0, std::min(50, rsToken.getLength())));
        }
        return !rsToken.isEmpty();
    }
    else if (nResponseCode == 202)
    {
        // Still pending
        SAL_WARN("sfx.control", "CloudApiClient - Authentication still pending");
        return false;
    }
    else
    {
        // Error or not found
        SAL_WARN("sfx.control", "CloudApiClient - Token poll failed with code: " << nResponseCode << ", response: " << sResponse);
        return false;
    }
}

bool CloudApiClient::requestPresignedUrl(const OUString& sMode, const OUString& sFileName,
                                        const OUString& sContentType, OUString& rsPresignedUrl,
                                        OUString& rsDocId)
{
    std::cerr << "*** DEBUG: requestPresignedUrl called ***" << std::endl;
    std::cerr << "*** DEBUG: Mode: " << OUStringToOString(sMode, RTL_TEXTENCODING_UTF8).getStr() << " ***" << std::endl;
    std::cerr << "*** DEBUG: FileName: " << OUStringToOString(sFileName, RTL_TEXTENCODING_UTF8).getStr() << " ***" << std::endl;
    
    if (!m_pCurl || m_sJwtToken.isEmpty())
    {
        std::cerr << "*** DEBUG: requestPresignedUrl FAILED - curl: " << (m_pCurl ? "OK" : "NULL") << ", JWT empty: " << (m_sJwtToken.isEmpty() ? "YES" : "NO") << " ***" << std::endl;
        return false;
    }

    OUString sUrl = m_sBaseUrl + "/api/presign";
    std::cerr << "*** DEBUG: Presign URL: " << OUStringToOString(sUrl, RTL_TEXTENCODING_UTF8).getStr() << " ***" << std::endl;
    
    // Build JSON request body
    tools::JsonWriter aJson;
    aJson.put("mode", sMode);
    aJson.put("fileName", sFileName);
    if (!sContentType.isEmpty())
        aJson.put("contentType", sContentType);

    OUString sRequestBody = OUString::fromUtf8(aJson.finishAndGetAsOString());
    OUString sResponse;
    long nResponseCode = 0;

    std::cerr << "*** DEBUG: Sending presign request ***" << std::endl;
    std::cerr << "*** DEBUG: Request body: " << OUStringToOString(sRequestBody, RTL_TEXTENCODING_UTF8).getStr() << " ***" << std::endl;
    
    if (!httpPost(sUrl, sRequestBody, sResponse, &nResponseCode))
    {
        std::cerr << "*** DEBUG: httpPost FAILED for presign request ***" << std::endl;
        return false;
    }

    std::cerr << "*** DEBUG: Presign response code: " << nResponseCode << " ***" << std::endl;
    std::cerr << "*** DEBUG: Presign response body: " << OUStringToOString(sResponse, RTL_TEXTENCODING_UTF8).getStr() << " ***" << std::endl;

    if (nResponseCode != 200)
    {
        std::cerr << "*** DEBUG: Presign request failed with non-200 code ***" << std::endl;
        SAL_WARN("sfx.control", "Presign request failed with code: " << nResponseCode);
        return false;
    }

    // Extract presigned URL and document ID
    rsPresignedUrl = extractJsonValue(sResponse, "presignedUrl");
    rsDocId = extractJsonValue(sResponse, "docId");

    std::cerr << "*** DEBUG: Extracted presignedUrl empty: " << (rsPresignedUrl.isEmpty() ? "YES" : "NO") << " ***" << std::endl;
    std::cerr << "*** DEBUG: Extracted docId empty: " << (rsDocId.isEmpty() ? "YES" : "NO") << " ***" << std::endl;

    return !rsPresignedUrl.isEmpty() && !rsDocId.isEmpty();
}

bool CloudApiClient::registerDocument(const OUString& sDocId, const OUString& sFileName, sal_Int64 nFileSize)
{
    std::cerr << "*** DEBUG: registerDocument() called ***" << std::endl;
    std::cerr << "*** DEBUG: DocId: " << OUStringToOString(sDocId, RTL_TEXTENCODING_UTF8).getStr() << " ***" << std::endl;
    std::cerr << "*** DEBUG: FileName: " << OUStringToOString(sFileName, RTL_TEXTENCODING_UTF8).getStr() << " ***" << std::endl;
    std::cerr << "*** DEBUG: FileSize: " << nFileSize << " bytes ***" << std::endl;
    
    if (!m_pCurl || m_sJwtToken.isEmpty())
    {
        std::cerr << "*** DEBUG: registerDocument failed - curl: " << (m_pCurl ? "OK" : "NULL") << ", JWT empty: " << (m_sJwtToken.isEmpty() ? "YES" : "NO") << " ***" << std::endl;
        return false;
    }

    OUString sUrl = m_sBaseUrl + "/api/documents";
    std::cerr << "*** DEBUG: Registration URL: " << OUStringToOString(sUrl, RTL_TEXTENCODING_UTF8).getStr() << " ***" << std::endl;
    
    // Build JSON request body
    tools::JsonWriter aJson;
    aJson.put("docId", sDocId);
    aJson.put("fileName", sFileName);
    aJson.put("fileSize", static_cast<sal_Int64>(nFileSize));

    OUString sRequestBody = OUString::fromUtf8(aJson.finishAndGetAsOString());
    std::cerr << "*** DEBUG: Request body: " << OUStringToOString(sRequestBody, RTL_TEXTENCODING_UTF8).getStr() << " ***" << std::endl;
    
    OUString sResponse;
    long nResponseCode = 0;

    std::cerr << "*** DEBUG: Making POST request to register document ***" << std::endl;
    if (!httpPost(sUrl, sRequestBody, sResponse, &nResponseCode))
    {
        std::cerr << "*** DEBUG: httpPost failed for document registration ***" << std::endl;
        return false;
    }

    std::cerr << "*** DEBUG: Registration response code: " << nResponseCode << " ***" << std::endl;
    std::cerr << "*** DEBUG: Registration response body: " << OUStringToOString(sResponse, RTL_TEXTENCODING_UTF8).getStr() << " ***" << std::endl;
    
    bool bSuccess = (nResponseCode == 200 || nResponseCode == 201);
    std::cerr << "*** DEBUG: Registration " << (bSuccess ? "SUCCESS" : "FAILED") << " ***" << std::endl;
    
    return bSuccess;
}

bool CloudApiClient::getDocuments(OUString& rsDocumentsJson)
{
    std::cerr << "*** DEBUG: CloudApiClient::getDocuments() called ***" << std::endl;
    
    if (!m_pCurl || m_sJwtToken.isEmpty())
    {
        std::cerr << "*** DEBUG: getDocuments failed - curl: " << (m_pCurl ? "OK" : "NULL") << ", JWT empty: " << (m_sJwtToken.isEmpty() ? "YES" : "NO") << " ***" << std::endl;
        return false;
    }

    OUString sUrl = m_sBaseUrl + "/api/documents";
    long nResponseCode = 0;

    std::cerr << "*** DEBUG: Making GET request to: " << OUStringToOString(sUrl, RTL_TEXTENCODING_UTF8).getStr() << " ***" << std::endl;
    std::cerr << "*** DEBUG: JWT token length: " << m_sJwtToken.getLength() << " ***" << std::endl;

    if (!httpGet(sUrl, rsDocumentsJson, &nResponseCode))
    {
        std::cerr << "*** DEBUG: httpGet failed ***" << std::endl;
        return false;
    }

    std::cerr << "*** DEBUG: getDocuments response code: " << nResponseCode << " ***" << std::endl;
    std::cerr << "*** DEBUG: getDocuments response: " << OUStringToOString(rsDocumentsJson, RTL_TEXTENCODING_UTF8).getStr() << " ***" << std::endl;

    return nResponseCode == 200;
}

bool CloudApiClient::deleteDocument(const OUString& sDocId)
{
    if (!m_pCurl || m_sJwtToken.isEmpty() || sDocId.isEmpty())
        return false;

    OUString sUrl = m_sBaseUrl + "/api/documents?docId=" + sDocId;
    OUString sResponse;
    long nResponseCode = 0;

    if (!httpDelete(sUrl, sResponse, &nResponseCode))
        return false;

    return nResponseCode == 200;
}

bool CloudApiClient::requestPresignedUrlForDocument(const OUString& sDocId, const OUString& sMode, OUString& rsPresignedUrl)
{
    if (!m_pCurl || m_sJwtToken.isEmpty() || sDocId.isEmpty())
    {
        SAL_WARN("sfx.control", "CloudApiClient::requestPresignedUrlForDocument - Invalid parameters");
        return false;
    }

    OUString sUrl = m_sBaseUrl + "/api/presign";
    
    // Create JSON request body with mode and docId
    OUString sRequestBody = "{\"mode\":\"" + sMode + "\",\"docId\":\"" + sDocId + "\"}";
    OUString sResponse;
    long nResponseCode = 0;

    SAL_WARN("sfx.control", "Requesting presigned URL for document: " << sDocId << " mode: " << sMode);

    if (!httpPost(sUrl, sRequestBody, sResponse, &nResponseCode))
    {
        SAL_WARN("sfx.control", "Failed to request presigned URL for document");
        return false;
    }

    if (nResponseCode != 200)
    {
        SAL_WARN("sfx.control", "Presigned URL request failed with code: " << nResponseCode);
        return false;
    }

    // Extract presigned URL from response
    rsPresignedUrl = extractJsonValue(sResponse, "presignedUrl");
    
    return !rsPresignedUrl.isEmpty();
}

bool CloudApiClient::downloadDocument(const OUString& sPresignedUrl, std::vector<char>& rDocumentData)
{
    std::cerr << "*** CLOUD DEBUG: downloadDocument() function entry" << std::endl;
    
    if (!m_pCurl || sPresignedUrl.isEmpty())
    {
        std::cerr << "*** CLOUD DEBUG: ERROR - Invalid parameters in downloadDocument()" << std::endl;
        std::cerr << "*** CLOUD DEBUG: m_pCurl is null: " << (m_pCurl == nullptr) << std::endl;
        std::cerr << "*** CLOUD DEBUG: sPresignedUrl is empty: " << sPresignedUrl.isEmpty() << std::endl;
        SAL_WARN("sfx.control", "CloudApiClient::downloadDocument - Invalid parameters");
        return false;
    }

    std::cerr << "*** CLOUD DEBUG: downloadDocument() called" << std::endl;
    std::cerr << "*** CLOUD DEBUG: Presigned URL: " << sPresignedUrl << std::endl;
    std::cerr << "*** CLOUD DEBUG: Parameters validated successfully" << std::endl;
    SAL_WARN("sfx.control", "*** CLOUD DEBUG: Downloading document from presigned URL");
    SAL_WARN("sfx.control", "*** CLOUD DEBUG: Presigned URL: " << sPresignedUrl);

    // Simple structure to hold document data
    struct DocumentDownload
    {
        std::vector<char> data;
    };

    DocumentDownload download;
    OString sUrlUtf8 = OUStringToOString(sPresignedUrl, RTL_TEXTENCODING_UTF8);
    std::cerr << "*** CLOUD DEBUG: URL converted to UTF8: " << sUrlUtf8.getStr() << std::endl;
    SAL_WARN("sfx.control", "*** CLOUD DEBUG: URL converted to UTF8: " << sUrlUtf8.getStr());

    std::cerr << "*** CLOUD DEBUG: About to reset curl options" << std::endl;
    // Reset curl state for clean GET request
    curl_easy_setopt(m_pCurl, CURLOPT_POST, 0L);
    curl_easy_setopt(m_pCurl, CURLOPT_UPLOAD, 0L);
    curl_easy_setopt(m_pCurl, CURLOPT_NOBODY, 0L);
    curl_easy_setopt(m_pCurl, CURLOPT_CUSTOMREQUEST, NULL);
    curl_easy_setopt(m_pCurl, CURLOPT_POSTFIELDS, NULL);
    curl_easy_setopt(m_pCurl, CURLOPT_POSTFIELDSIZE, 0L);
    curl_easy_setopt(m_pCurl, CURLOPT_HTTPHEADER, NULL);
    std::cerr << "*** CLOUD DEBUG: Curl options reset completed" << std::endl;

    // Set URL
    std::cerr << "*** CLOUD DEBUG: About to set CURLOPT_URL" << std::endl;
    curl_easy_setopt(m_pCurl, CURLOPT_URL, sUrlUtf8.getStr());
    std::cerr << "*** CLOUD DEBUG: CURLOPT_URL set successfully" << std::endl;
    
    std::cerr << "*** CLOUD DEBUG: About to set CURLOPT_HTTPGET" << std::endl;
    curl_easy_setopt(m_pCurl, CURLOPT_HTTPGET, 1L);
    std::cerr << "*** CLOUD DEBUG: CURLOPT_HTTPGET set successfully" << std::endl;

    // Set write callback for binary data - use plain C function to avoid lambda issues
    std::cerr << "*** CLOUD DEBUG: About to set up write callback" << std::endl;

    // Simple C-style callback function  
    static auto simpleWriteCallback = +[](void* contents, size_t size, size_t nmemb, void* userp) -> size_t {
        std::cerr << "*** CLOUD DEBUG: Write callback called with " << (size * nmemb) << " bytes" << std::endl;
        size_t realsize = size * nmemb;
        DocumentDownload* download = static_cast<DocumentDownload*>(userp);
        
        if (download) {
            try {
                size_t oldSize = download->data.size();
                download->data.resize(oldSize + realsize);
                std::memcpy(&download->data[oldSize], contents, realsize);
                std::cerr << "*** CLOUD DEBUG: Write callback completed, total size now: " << download->data.size() << std::endl;
            } catch (...) {
                std::cerr << "*** CLOUD DEBUG: Exception in write callback!" << std::endl;
                return 0; // Signal error to curl
            }
        } else {
            std::cerr << "*** CLOUD DEBUG: ERROR - userp is null in write callback!" << std::endl;
            return 0; // Signal error to curl
        }
        
        return realsize;
    };

    std::cerr << "*** CLOUD DEBUG: About to set CURLOPT_WRITEFUNCTION" << std::endl;
    curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, simpleWriteCallback);
    std::cerr << "*** CLOUD DEBUG: About to set CURLOPT_WRITEDATA" << std::endl;
    curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, &download);
    std::cerr << "*** CLOUD DEBUG: Write callback setup completed" << std::endl;

    // Perform request
    std::cerr << "*** CLOUD DEBUG: About to call curl_easy_perform()" << std::endl;
    SAL_WARN("sfx.control", "*** CLOUD DEBUG: Starting HTTP GET request");
    
    CURLcode res;
    try {
        std::cerr << "*** CLOUD DEBUG: Calling curl_easy_perform() now..." << std::endl;
        res = curl_easy_perform(m_pCurl);
        std::cerr << "*** CLOUD DEBUG: curl_easy_perform() completed with code: " << res << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "*** CLOUD DEBUG: std::exception caught during curl_easy_perform(): " << e.what() << std::endl;
        return false;
    } catch (...) {
        std::cerr << "*** CLOUD DEBUG: Unknown exception caught during curl_easy_perform()!" << std::endl;
        return false;
    }
    
    SAL_WARN("sfx.control", "*** CLOUD DEBUG: HTTP GET request completed with CURLcode: " << res);

    if (res == CURLE_OK)
    {
        long nResponseCode = 0;
        curl_easy_getinfo(m_pCurl, CURLINFO_RESPONSE_CODE, &nResponseCode);
        m_nLastResponseCode = nResponseCode;
        SAL_WARN("sfx.control", "*** CLOUD DEBUG: HTTP response code: " << nResponseCode);
        SAL_WARN("sfx.control", "*** CLOUD DEBUG: Downloaded data size: " << download.data.size() << " bytes");

        if (nResponseCode == 200)
        {
            rDocumentData = std::move(download.data);
            std::cerr << "*** CLOUD DEBUG: Document download successful!" << std::endl;
            std::cerr << "*** CLOUD DEBUG: Final document data size: " << rDocumentData.size() << " bytes" << std::endl;
            SAL_WARN("sfx.control", "*** CLOUD DEBUG: Document download successful!");
            SAL_WARN("sfx.control", "*** CLOUD DEBUG: Final document data size: " << rDocumentData.size() << " bytes");
            
            // Show first few bytes for debugging (if text content)
            if (rDocumentData.size() > 0) {
                std::string preview;
                size_t previewSize = std::min(static_cast<size_t>(100), rDocumentData.size());
                for (size_t i = 0; i < previewSize; ++i) {
                    char c = rDocumentData[i];
                    if (c >= 32 && c <= 126) { // Printable ASCII
                        preview += c;
                    } else {
                        preview += "?";
                    }
                }
                            std::cerr << "*** CLOUD DEBUG: Content preview (first " << previewSize << " bytes): " << preview << std::endl;
            SAL_WARN("sfx.control", "*** CLOUD DEBUG: Content preview (first " << previewSize << " bytes): " << preview.c_str());
            }
            
            std::cerr << "*** CLOUD DEBUG: downloadDocument() returning TRUE (success)" << std::endl;
            return true;
        }
        else
        {
            std::cerr << "*** CLOUD DEBUG: downloadDocument() returning FALSE - HTTP error: " << nResponseCode << std::endl;
            SAL_WARN("sfx.control", "*** CLOUD DEBUG: Document download failed with HTTP code: " << nResponseCode);
            return false;
        }
    }
    else
    {
        m_nLastResponseCode = 0;
        std::cerr << "*** CLOUD DEBUG: downloadDocument() returning FALSE - CURL error: " << curl_easy_strerror(res) << std::endl;
        SAL_WARN("sfx.control", "*** CLOUD DEBUG: Document download failed with curl error: " << curl_easy_strerror(res));
        return false;
    }
}

// Private helper methods

bool CloudApiClient::httpGet(const OUString& sUrl, OUString& rsResponse, long* pnResponseCode)
{
    if (!m_pCurl)
        return false;

    std::cerr << "*** DEBUG: httpGet() called ***" << std::endl;
    std::cerr << "*** DEBUG: URL: " << OUStringToOString(sUrl, RTL_TEXTENCODING_UTF8).getStr() << " ***" << std::endl;

    CurlResponse response;
    OString sUrlUtf8 = OUStringToOString(sUrl, RTL_TEXTENCODING_UTF8);

    // CRITICAL FIX: Complete curl reset to prevent corruption
    std::cerr << "*** DEBUG: Resetting curl options for clean GET request ***" << std::endl;
    curl_easy_setopt(m_pCurl, CURLOPT_POST, 0L);
    curl_easy_setopt(m_pCurl, CURLOPT_UPLOAD, 0L);
    curl_easy_setopt(m_pCurl, CURLOPT_NOBODY, 0L);
    curl_easy_setopt(m_pCurl, CURLOPT_CUSTOMREQUEST, NULL);
    curl_easy_setopt(m_pCurl, CURLOPT_POSTFIELDS, NULL);
    curl_easy_setopt(m_pCurl, CURLOPT_POSTFIELDSIZE, 0L);
    curl_easy_setopt(m_pCurl, CURLOPT_HTTPHEADER, NULL); // Clear any existing headers

    // Set URL
    curl_easy_setopt(m_pCurl, CURLOPT_URL, sUrlUtf8.getStr());
    
    // Set to GET request
    curl_easy_setopt(m_pCurl, CURLOPT_HTTPGET, 1L);
    
    // Set write callback
    curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, &response);

    // Create headers locally (not using shared m_pHeaders to avoid thread issues)
    struct curl_slist* headers = nullptr;
    if (!m_sJwtToken.isEmpty()) {
        std::cerr << "*** DEBUG: Adding authorization header to GET request ***" << std::endl;
        OString sAuthHeader = "Authorization: Bearer " + OUStringToOString(m_sJwtToken, RTL_TEXTENCODING_UTF8);
        headers = curl_slist_append(headers, sAuthHeader.getStr());
        curl_easy_setopt(m_pCurl, CURLOPT_HTTPHEADER, headers);
    }
    
    // Perform request
    std::cerr << "*** DEBUG: Performing HTTP GET request ***" << std::endl;
    CURLcode res = curl_easy_perform(m_pCurl);
    
    // Clean up headers
    if (headers) {
        curl_slist_free_all(headers);
    }
    
    if (res == CURLE_OK)
    {
        if (pnResponseCode)
        {
            curl_easy_getinfo(m_pCurl, CURLINFO_RESPONSE_CODE, pnResponseCode);
            m_nLastResponseCode = *pnResponseCode;
        }
        else
        {
            curl_easy_getinfo(m_pCurl, CURLINFO_RESPONSE_CODE, &m_nLastResponseCode);
        }
        
        std::cerr << "*** DEBUG: GET request completed with response code: " << m_nLastResponseCode << " ***" << std::endl;
        rsResponse = OUString::fromUtf8(response.data.toString());
        std::cerr << "*** DEBUG: GET response body: " << OUStringToOString(rsResponse, RTL_TEXTENCODING_UTF8).getStr() << " ***" << std::endl;
        return true;
    }
    else
    {
        m_nLastResponseCode = 0; // Network error
        std::cerr << "*** DEBUG: HTTP GET failed with curl error: " << curl_easy_strerror(res) << " ***" << std::endl;
        SAL_WARN("sfx.control", "HTTP GET failed: " << curl_easy_strerror(res));
        return false;
    }
}

bool CloudApiClient::httpPost(const OUString& sUrl, const OUString& sBody, OUString& rsResponse, long* pnResponseCode)
{
    if (!m_pCurl)
        return false;

    std::cerr << "*** DEBUG: httpPost() called ***" << std::endl;
    std::cerr << "*** DEBUG: URL: " << OUStringToOString(sUrl, RTL_TEXTENCODING_UTF8).getStr() << " ***" << std::endl;
    std::cerr << "*** DEBUG: Request body: " << OUStringToOString(sBody, RTL_TEXTENCODING_UTF8).getStr() << " ***" << std::endl;

    CurlResponse response;
    OString sUrlUtf8 = OUStringToOString(sUrl, RTL_TEXTENCODING_UTF8);
    OString sBodyUtf8 = OUStringToOString(sBody, RTL_TEXTENCODING_UTF8);

    // Reset curl options that might interfere from previous requests (especially S3 upload)
    std::cerr << "*** DEBUG: Resetting curl options for clean POST request ***" << std::endl;
    curl_easy_setopt(m_pCurl, CURLOPT_HTTPGET, 0L);
    curl_easy_setopt(m_pCurl, CURLOPT_UPLOAD, 0L);
    curl_easy_setopt(m_pCurl, CURLOPT_NOBODY, 0L);
    curl_easy_setopt(m_pCurl, CURLOPT_CUSTOMREQUEST, NULL);
    curl_easy_setopt(m_pCurl, CURLOPT_HTTPHEADER, NULL); // Clear any existing headers

    // Set URL
    curl_easy_setopt(m_pCurl, CURLOPT_URL, sUrlUtf8.getStr());
    
    // Set to POST request
    std::cerr << "*** DEBUG: Setting POST options ***" << std::endl;
    curl_easy_setopt(m_pCurl, CURLOPT_POST, 1L);
    curl_easy_setopt(m_pCurl, CURLOPT_POSTFIELDS, sBodyUtf8.getStr());
    curl_easy_setopt(m_pCurl, CURLOPT_POSTFIELDSIZE, sBodyUtf8.getLength());
    
    // Set write callback
    curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, &response);

    // Create headers locally (not using shared addAuthHeader to avoid corruption)
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    
    if (!m_sJwtToken.isEmpty()) {
        std::cerr << "*** DEBUG: Adding authentication header ***" << std::endl;
        OString sAuthHeader = "Authorization: Bearer " + OUStringToOString(m_sJwtToken, RTL_TEXTENCODING_UTF8);
        headers = curl_slist_append(headers, sAuthHeader.getStr());
    }
    
    curl_easy_setopt(m_pCurl, CURLOPT_HTTPHEADER, headers);
    std::cerr << "*** DEBUG: Added Content-Type: application/json header ***" << std::endl;
    
    // Perform request
    std::cerr << "*** DEBUG: Performing HTTP POST request ***" << std::endl;
    CURLcode res = curl_easy_perform(m_pCurl);
    
    // Clean up headers
    curl_slist_free_all(headers);
    
    if (res == CURLE_OK)
    {
        if (pnResponseCode)
        {
            curl_easy_getinfo(m_pCurl, CURLINFO_RESPONSE_CODE, pnResponseCode);
            m_nLastResponseCode = *pnResponseCode;
        }
        else
        {
            curl_easy_getinfo(m_pCurl, CURLINFO_RESPONSE_CODE, &m_nLastResponseCode);
        }
        
        std::cerr << "*** DEBUG: POST request completed with response code: " << m_nLastResponseCode << " ***" << std::endl;
        rsResponse = OUString::fromUtf8(response.data.toString());
        std::cerr << "*** DEBUG: POST response body: " << OUStringToOString(rsResponse, RTL_TEXTENCODING_UTF8).getStr() << " ***" << std::endl;
        return true;
    }
    else
    {
        m_nLastResponseCode = 0; // Network error
        std::cerr << "*** DEBUG: HTTP POST failed with curl error: " << curl_easy_strerror(res) << " ***" << std::endl;
        SAL_WARN("sfx.control", "HTTP POST failed: " << curl_easy_strerror(res));
        return false;
    }
}

bool CloudApiClient::httpDelete(const OUString& sUrl, OUString& rsResponse, long* pnResponseCode)
{
    if (!m_pCurl)
        return false;

    std::cerr << "*** DEBUG: httpDelete() called ***" << std::endl;
    std::cerr << "*** DEBUG: URL: " << OUStringToOString(sUrl, RTL_TEXTENCODING_UTF8).getStr() << " ***" << std::endl;

    CurlResponse response;
    OString sUrlUtf8 = OUStringToOString(sUrl, RTL_TEXTENCODING_UTF8);

    // Reset curl options for clean DELETE request
    std::cerr << "*** DEBUG: Resetting curl options for clean DELETE request ***" << std::endl;
    curl_easy_setopt(m_pCurl, CURLOPT_POST, 0L);
    curl_easy_setopt(m_pCurl, CURLOPT_HTTPGET, 0L);
    curl_easy_setopt(m_pCurl, CURLOPT_UPLOAD, 0L);
    curl_easy_setopt(m_pCurl, CURLOPT_NOBODY, 0L);
    curl_easy_setopt(m_pCurl, CURLOPT_POSTFIELDS, NULL);
    curl_easy_setopt(m_pCurl, CURLOPT_POSTFIELDSIZE, 0L);
    curl_easy_setopt(m_pCurl, CURLOPT_HTTPHEADER, NULL); // Clear any existing headers

    // Set URL
    curl_easy_setopt(m_pCurl, CURLOPT_URL, sUrlUtf8.getStr());
    
    // Set to DELETE request
    curl_easy_setopt(m_pCurl, CURLOPT_CUSTOMREQUEST, "DELETE");
    
    // Set write callback
    curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, &response);

    // Create headers locally (not using shared addAuthHeader to avoid corruption)
    struct curl_slist* headers = nullptr;
    if (!m_sJwtToken.isEmpty()) {
        std::cerr << "*** DEBUG: Adding authorization header to DELETE request ***" << std::endl;
        OString sAuthHeader = "Authorization: Bearer " + OUStringToOString(m_sJwtToken, RTL_TEXTENCODING_UTF8);
        headers = curl_slist_append(headers, sAuthHeader.getStr());
        curl_easy_setopt(m_pCurl, CURLOPT_HTTPHEADER, headers);
    }
    
    // Perform request
    std::cerr << "*** DEBUG: Performing HTTP DELETE request ***" << std::endl;
    CURLcode res = curl_easy_perform(m_pCurl);
    
    // Clean up headers
    if (headers) {
        curl_slist_free_all(headers);
    }
    
    if (res == CURLE_OK)
    {
        if (pnResponseCode)
        {
            curl_easy_getinfo(m_pCurl, CURLINFO_RESPONSE_CODE, pnResponseCode);
            m_nLastResponseCode = *pnResponseCode;
        }
        else
        {
            curl_easy_getinfo(m_pCurl, CURLINFO_RESPONSE_CODE, &m_nLastResponseCode);
        }
        
        std::cerr << "*** DEBUG: DELETE request completed with response code: " << m_nLastResponseCode << " ***" << std::endl;
        rsResponse = OUString::fromUtf8(response.data.toString());
        std::cerr << "*** DEBUG: DELETE response body: " << OUStringToOString(rsResponse, RTL_TEXTENCODING_UTF8).getStr() << " ***" << std::endl;
        return true;
    }
    else
    {
        m_nLastResponseCode = 0; // Network error
        std::cerr << "*** DEBUG: HTTP DELETE failed with curl error: " << curl_easy_strerror(res) << " ***" << std::endl;
        SAL_WARN("sfx.control", "HTTP DELETE failed: " << curl_easy_strerror(res));
        return false;
    }
}

bool CloudApiClient::uploadFile(const OUString& sUrl, const char* pData, size_t nDataSize, 
                               const OUString& sContentType, OUString& rsResponse, long* pnResponseCode)
{
    (void)sContentType; // Suppress unused parameter warning
    
    if (!m_pCurl || !pData || nDataSize == 0)
        return false;

    CurlResponse response;
    OString sUrlUtf8 = OUStringToOString(sUrl, RTL_TEXTENCODING_UTF8);

    // Reset curl options that might interfere from previous requests
    std::cerr << "*** DEBUG: Resetting curl options for clean S3 upload ***" << std::endl;
    curl_easy_setopt(m_pCurl, CURLOPT_POST, 0L);
    curl_easy_setopt(m_pCurl, CURLOPT_HTTPGET, 0L);
    curl_easy_setopt(m_pCurl, CURLOPT_UPLOAD, 0L);
    curl_easy_setopt(m_pCurl, CURLOPT_NOBODY, 0L);
    
    // Set URL
    curl_easy_setopt(m_pCurl, CURLOPT_URL, sUrlUtf8.getStr());
    
    // Configure for PUT request with data
    std::cerr << "*** DEBUG: Configuring curl for PUT request with " << nDataSize << " bytes ***" << std::endl;
    
    // Use CUSTOMREQUEST for PUT with POSTFIELDS (this is the correct way for S3)
    curl_easy_setopt(m_pCurl, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_easy_setopt(m_pCurl, CURLOPT_POSTFIELDS, pData);
    curl_easy_setopt(m_pCurl, CURLOPT_POSTFIELDSIZE, nDataSize);
    
    // Set write callback for response
    curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, &response);

    // Set up headers for file upload - minimize headers to match S3 signature
    struct curl_slist* uploadHeaders = nullptr;
    
    // For S3 presigned URLs, we should NOT add Content-Type header unless it was 
    // included in the signature. The SignedHeaders=host indicates only host was signed.
    // Adding extra headers can cause 403 errors.
    
    std::cerr << "*** DEBUG: Setting minimal headers for S3 upload ***" << std::endl;
    
    curl_easy_setopt(m_pCurl, CURLOPT_HTTPHEADER, uploadHeaders);
    
    // Perform request
    CURLcode res = curl_easy_perform(m_pCurl);
    
    // Clean up upload headers
    if (uploadHeaders)
    {
        curl_slist_free_all(uploadHeaders);
    }
    
    if (res == CURLE_OK)
    {
        if (pnResponseCode)
        {
            curl_easy_getinfo(m_pCurl, CURLINFO_RESPONSE_CODE, pnResponseCode);
            m_nLastResponseCode = *pnResponseCode;
        }
        else
        {
            curl_easy_getinfo(m_pCurl, CURLINFO_RESPONSE_CODE, &m_nLastResponseCode);
        }
        
        rsResponse = OUString::fromUtf8(response.data.toString());
        
        // Log the upload result
        std::cerr << "*** DEBUG: HTTP PUT completed with code: " << m_nLastResponseCode << " ***" << std::endl;
        SAL_WARN("sfx.control", "HTTP PUT completed with code: " << m_nLastResponseCode);
        
        return true;
    }
    else
    {
        m_nLastResponseCode = 0; // Network error
        std::cerr << "*** DEBUG: HTTP PUT failed: " << curl_easy_strerror(res) << " ***" << std::endl;
        SAL_WARN("sfx.control", "HTTP PUT failed: " << curl_easy_strerror(res));
        return false;
    }
}

void CloudApiClient::setupCommonOptions()
{
    if (!m_pCurl)
        return;

    // Set common options
    curl_easy_setopt(m_pCurl, CURLOPT_USERAGENT, "LibreOffice Cloud Client/1.0");
    curl_easy_setopt(m_pCurl, CURLOPT_TIMEOUT, 30L);
    curl_easy_setopt(m_pCurl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(m_pCurl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(m_pCurl, CURLOPT_MAXREDIRS, 3L);
    curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(m_pCurl, CURLOPT_SSL_VERIFYHOST, 2L);

    // Set up headers
    m_pHeaders = curl_slist_append(m_pHeaders, "Content-Type: application/json");
    m_pHeaders = curl_slist_append(m_pHeaders, "Accept: application/json");
    
    curl_easy_setopt(m_pCurl, CURLOPT_HTTPHEADER, m_pHeaders);
}

void CloudApiClient::addAuthHeader()
{
    std::cerr << "*** DEBUG: addAuthHeader() called ***" << std::endl;
    
    if (!m_pCurl || m_sJwtToken.isEmpty())
    {
        std::cerr << "*** DEBUG: addAuthHeader skipped - curl: " << (m_pCurl ? "OK" : "NULL") << ", token empty: " << (m_sJwtToken.isEmpty() ? "YES" : "NO") << " ***" << std::endl;
        return;
    }

    // Free existing auth header if any
    if (m_pHeaders)
    {
        curl_slist_free_all(m_pHeaders);
        m_pHeaders = nullptr;
    }

    // Rebuild headers with auth
    m_pHeaders = curl_slist_append(m_pHeaders, "Content-Type: application/json");
    m_pHeaders = curl_slist_append(m_pHeaders, "Accept: application/json");
    
    OString sAuthHeader = "Authorization: Bearer " + OUStringToOString(m_sJwtToken, RTL_TEXTENCODING_UTF8);
    std::cerr << "*** DEBUG: Setting Authorization header: " << sAuthHeader.getStr() << " ***" << std::endl;
    
    m_pHeaders = curl_slist_append(m_pHeaders, sAuthHeader.getStr());
    
    curl_easy_setopt(m_pCurl, CURLOPT_HTTPHEADER, m_pHeaders);
    
    std::cerr << "*** DEBUG: addAuthHeader completed successfully ***" << std::endl;
}

OUString CloudApiClient::extractJsonField(const OUString& sJson, const OUString& sField) const
{
    return extractJsonValue(sJson, sField);
}

size_t CloudApiClient::writeCallback(void* contents, size_t size, size_t nmemb, void* userp)
{
    size_t realsize = size * nmemb;
    CurlResponse* response = static_cast<CurlResponse*>(userp);
    
    if (response)
    {
        response->data.append(static_cast<const char*>(contents), realsize);
    }
    
    return realsize;
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */ 