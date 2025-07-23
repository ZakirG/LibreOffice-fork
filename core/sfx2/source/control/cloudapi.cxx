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
    if (!m_pCurl || m_sJwtToken.isEmpty())
        return false;

    OUString sUrl = m_sBaseUrl + "/api/presign";
    
    // Build JSON request body
    tools::JsonWriter aJson;
    aJson.put("mode", sMode);
    aJson.put("fileName", sFileName);
    if (!sContentType.isEmpty())
        aJson.put("contentType", sContentType);

    OUString sRequestBody = OUString::fromUtf8(aJson.finishAndGetAsOString());
    OUString sResponse;
    long nResponseCode = 0;

    if (!httpPost(sUrl, sRequestBody, sResponse, &nResponseCode))
        return false;

    if (nResponseCode != 200)
    {
        SAL_WARN("sfx.control", "Presign request failed with code: " << nResponseCode);
        return false;
    }

    // Extract presigned URL and document ID
    rsPresignedUrl = extractJsonValue(sResponse, "presignedUrl");
    rsDocId = extractJsonValue(sResponse, "docId");

    return !rsPresignedUrl.isEmpty() && !rsDocId.isEmpty();
}

bool CloudApiClient::registerDocument(const OUString& sDocId, const OUString& sFileName, sal_Int64 nFileSize)
{
    if (!m_pCurl || m_sJwtToken.isEmpty())
        return false;

    OUString sUrl = m_sBaseUrl + "/api/documents";
    
    // Build JSON request body
    tools::JsonWriter aJson;
    aJson.put("docId", sDocId);
    aJson.put("fileName", sFileName);
    aJson.put("fileSize", static_cast<sal_Int64>(nFileSize));

    OUString sRequestBody = OUString::fromUtf8(aJson.finishAndGetAsOString());
    OUString sResponse;
    long nResponseCode = 0;

    if (!httpPost(sUrl, sRequestBody, sResponse, &nResponseCode))
        return false;

    return nResponseCode == 200 || nResponseCode == 201;
}

bool CloudApiClient::getDocuments(OUString& rsDocumentsJson)
{
    if (!m_pCurl || m_sJwtToken.isEmpty())
        return false;

    OUString sUrl = m_sBaseUrl + "/api/documents";
    long nResponseCode = 0;

    if (!httpGet(sUrl, rsDocumentsJson, &nResponseCode))
        return false;

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

// Private helper methods

bool CloudApiClient::httpGet(const OUString& sUrl, OUString& rsResponse, long* pnResponseCode)
{
    if (!m_pCurl)
        return false;

    CurlResponse response;
    OString sUrlUtf8 = OUStringToOString(sUrl, RTL_TEXTENCODING_UTF8);

    // Set URL
    curl_easy_setopt(m_pCurl, CURLOPT_URL, sUrlUtf8.getStr());
    
    // Set to GET request
    curl_easy_setopt(m_pCurl, CURLOPT_HTTPGET, 1L);
    
    // Set write callback
    curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, &response);

    // Add authentication header
    addAuthHeader();
    
    // Perform request
    CURLcode res = curl_easy_perform(m_pCurl);
    
    if (res == CURLE_OK)
    {
        if (pnResponseCode)
        {
            curl_easy_getinfo(m_pCurl, CURLINFO_RESPONSE_CODE, pnResponseCode);
        }
        
        rsResponse = OUString::fromUtf8(response.data.toString());
        return true;
    }
    else
    {
        SAL_WARN("sfx.control", "HTTP GET failed: " << curl_easy_strerror(res));
        return false;
    }
}

bool CloudApiClient::httpPost(const OUString& sUrl, const OUString& sBody, OUString& rsResponse, long* pnResponseCode)
{
    if (!m_pCurl)
        return false;

    CurlResponse response;
    OString sUrlUtf8 = OUStringToOString(sUrl, RTL_TEXTENCODING_UTF8);
    OString sBodyUtf8 = OUStringToOString(sBody, RTL_TEXTENCODING_UTF8);

    // Set URL
    curl_easy_setopt(m_pCurl, CURLOPT_URL, sUrlUtf8.getStr());
    
    // Set to POST request
    curl_easy_setopt(m_pCurl, CURLOPT_POST, 1L);
    curl_easy_setopt(m_pCurl, CURLOPT_POSTFIELDS, sBodyUtf8.getStr());
    curl_easy_setopt(m_pCurl, CURLOPT_POSTFIELDSIZE, sBodyUtf8.getLength());
    
    // Set write callback
    curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, &response);

    // Add authentication header
    addAuthHeader();
    
    // Perform request
    CURLcode res = curl_easy_perform(m_pCurl);
    
    if (res == CURLE_OK)
    {
        if (pnResponseCode)
        {
            curl_easy_getinfo(m_pCurl, CURLINFO_RESPONSE_CODE, pnResponseCode);
        }
        
        rsResponse = OUString::fromUtf8(response.data.toString());
        return true;
    }
    else
    {
        SAL_WARN("sfx.control", "HTTP POST failed: " << curl_easy_strerror(res));
        return false;
    }
}

bool CloudApiClient::httpDelete(const OUString& sUrl, OUString& rsResponse, long* pnResponseCode)
{
    if (!m_pCurl)
        return false;

    CurlResponse response;
    OString sUrlUtf8 = OUStringToOString(sUrl, RTL_TEXTENCODING_UTF8);

    // Set URL
    curl_easy_setopt(m_pCurl, CURLOPT_URL, sUrlUtf8.getStr());
    
    // Set to DELETE request
    curl_easy_setopt(m_pCurl, CURLOPT_CUSTOMREQUEST, "DELETE");
    
    // Set write callback
    curl_easy_setopt(m_pCurl, CURLOPT_WRITEFUNCTION, writeCallback);
    curl_easy_setopt(m_pCurl, CURLOPT_WRITEDATA, &response);

    // Add authentication header
    addAuthHeader();
    
    // Perform request
    CURLcode res = curl_easy_perform(m_pCurl);
    
    if (res == CURLE_OK)
    {
        if (pnResponseCode)
        {
            curl_easy_getinfo(m_pCurl, CURLINFO_RESPONSE_CODE, pnResponseCode);
        }
        
        rsResponse = OUString::fromUtf8(response.data.toString());
        return true;
    }
    else
    {
        SAL_WARN("sfx.control", "HTTP DELETE failed: " << curl_easy_strerror(res));
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
    if (!m_pCurl || m_sJwtToken.isEmpty())
        return;

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
    m_pHeaders = curl_slist_append(m_pHeaders, sAuthHeader.getStr());
    
    curl_easy_setopt(m_pCurl, CURLOPT_HTTPHEADER, m_pHeaders);
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