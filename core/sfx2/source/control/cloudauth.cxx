/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <sfx2/cloudauth.hxx>
#include <sfx2/cloudapi.hxx>

#include <comphelper/processfactory.hxx>
#include <officecfg/Office/Common.hxx>
#include <unotools/configmgr.hxx>
#include <com/sun/star/uno/Exception.hpp>
#include <comphelper/configuration.hxx>
#include <osl/process.h>

using namespace css;
#include <osl/file.hxx>
#include <rtl/process.h>
#include <vcl/svapp.hxx>
#include <vcl/weld.hxx>
#include <tools/json_writer.hxx>
#include <tools/stream.hxx>
#include <thread>
#include <chrono>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#elif defined MACOSX
#include <cstdlib>
#include <string>
#elif defined UNX
#include <cstdlib>
#include <unistd.h>
#endif

namespace
{
    const char* CLOUD_API_BASE_URL = "http://localhost:3009";
    const int TOKEN_POLL_INTERVAL_MS = 2000; // 2 seconds
    const int MAX_POLL_ATTEMPTS = 150; // 5 minutes total (150 * 2 seconds)
}

// Static member definitions
std::unique_ptr<CloudAuthHandler> CloudAuthHandler::s_pInstance;
osl::Mutex CloudAuthHandler::s_aMutex;

CloudAuthHandler::CloudAuthHandler()
    : m_pApiClient(std::make_unique<CloudApiClient>())
    , m_bIsAuthenticated(false)
    , m_bAuthInProgress(false)
{
    m_pApiClient->setBaseUrl(OUString::fromUtf8(CLOUD_API_BASE_URL));
    loadStoredAuth();
}

CloudAuthHandler::~CloudAuthHandler() = default;

CloudAuthHandler& CloudAuthHandler::getInstance()
{
    osl::MutexGuard aGuard(s_aMutex);
    if (!s_pInstance)
    {
        s_pInstance = std::unique_ptr<CloudAuthHandler>(new CloudAuthHandler());
    }
    return *s_pInstance;
}

void CloudAuthHandler::startAuthentication()
{
    osl::MutexGuard aGuard(m_aMutex);
    
    if (m_bAuthInProgress)
    {
        // Authentication already in progress
        return;
    }

    if (m_bIsAuthenticated)
    {
        // Already authenticated, offer logout option
        std::unique_ptr<weld::MessageDialog> xBox(Application::CreateMessageDialog(nullptr,
            VclMessageType::Question, VclButtonsType::YesNo,
            "You are already logged in to LibreCloud. Do you want to logout?"));
        xBox->set_title("LibreCloud Authentication");
        
        if (xBox->run() == RET_YES)
        {
            logout();
        }
        return;
    }

    m_bAuthInProgress = true;

    // Start authentication process
    OUString sNonce, sLoginUrl;
    if (!m_pApiClient->initDesktopAuth(sNonce, sLoginUrl))
    {
        m_bAuthInProgress = false;
        
        std::unique_ptr<weld::MessageDialog> xBox(Application::CreateMessageDialog(nullptr,
            VclMessageType::Error, VclButtonsType::Ok,
            "Failed to initialize cloud authentication. Please check your internet connection."));
        xBox->set_title("LibreCloud Authentication Error");
        xBox->run();
        return;
    }

    m_sCurrentNonce = sNonce;

    // Show dialog with instructions
    std::unique_ptr<weld::MessageDialog> xBox(Application::CreateMessageDialog(nullptr,
        VclMessageType::Info, VclButtonsType::OkCancel,
        "Your browser will open to complete LibreCloud authentication.\n\n"
        "Please sign in with your Google account and return to LibreOffice."));
    xBox->set_title("LibreCloud Authentication");
    
    int nResult = xBox->run();
    if (nResult != RET_OK)
    {
        m_bAuthInProgress = false;
        return;
    }

    // Open browser
    openBrowser(sLoginUrl);

    // Start polling in background thread
    std::thread([this, sNonce]() {
        pollForToken(sNonce);
    }).detach();
}

bool CloudAuthHandler::isAuthenticated() const
{
    osl::MutexGuard aGuard(m_aMutex);
    return m_bIsAuthenticated && !m_sJwtToken.isEmpty() && validateJwtToken(m_sJwtToken);
}

const OUString& CloudAuthHandler::getJwtToken() const
{
    osl::MutexGuard aGuard(m_aMutex);
    return m_sJwtToken;
}

bool CloudAuthHandler::isAuthInProgress() const
{
    osl::MutexGuard aGuard(m_aMutex);
    return m_bAuthInProgress;
}

void CloudAuthHandler::logout()
{
    osl::MutexGuard aGuard(m_aMutex);
    
    m_sJwtToken.clear();
    m_sCurrentNonce.clear();
    m_bIsAuthenticated = false;
    m_bAuthInProgress = false;

    // Clear stored token
    storeJwtToken("");

    // Show logout confirmation
    std::unique_ptr<weld::MessageDialog> xBox(Application::CreateMessageDialog(nullptr,
        VclMessageType::Info, VclButtonsType::Ok,
        "You have been logged out from LibreCloud."));
    xBox->set_title("LibreCloud Authentication");
    xBox->run();
}

void CloudAuthHandler::loadStoredAuth()
{
    try
    {
        // Load JWT token from LibreOffice configuration
        OUString sStoredToken = officecfg::Office::Common::Cloud::JwtToken::get();
        
        if (!sStoredToken.isEmpty() && validateJwtToken(sStoredToken))
        {
            m_sJwtToken = sStoredToken;
            m_bIsAuthenticated = true;
            m_pApiClient->setJwtToken(sStoredToken);
        }
    }
    catch (const uno::Exception&)
    {
        // Failed to load configuration, start with clean state
        m_sJwtToken.clear();
        m_bIsAuthenticated = false;
    }
}

void CloudAuthHandler::storeJwtToken(const OUString& sToken)
{
    try
    {
        // Store JWT token in LibreOffice configuration
        std::shared_ptr<comphelper::ConfigurationChanges> batch(
            comphelper::ConfigurationChanges::create());
        officecfg::Office::Common::Cloud::JwtToken::set(sToken, batch);
        batch->commit();
    }
    catch (const uno::Exception&)
    {
        // Failed to store configuration
        SAL_WARN("sfx.control", "Failed to store JWT token in configuration");
    }
}

void CloudAuthHandler::pollForToken(const OUString& sNonce)
{
    int nAttempts = 0;
    
    while (nAttempts < MAX_POLL_ATTEMPTS)
    {
        // Check if authentication was cancelled
        {
            osl::MutexGuard aGuard(m_aMutex);
            if (!m_bAuthInProgress || m_sCurrentNonce != sNonce)
            {
                return;
            }
        }

        OUString sToken;
        if (m_pApiClient->pollForToken(sNonce, sToken))
        {
            // Authentication successful
            osl::MutexGuard aGuard(m_aMutex);
            
            if (validateJwtToken(sToken))
            {
                m_sJwtToken = sToken;
                m_bIsAuthenticated = true;
                m_bAuthInProgress = false;
                m_pApiClient->setJwtToken(sToken);
                
                // Store token securely
                storeJwtToken(sToken);

                // Show success message on main thread
                Application::PostUserEvent(LINK(nullptr, CloudAuthHandler, ShowSuccessMessage));
            }
            else
            {
                // Invalid token received
                m_bAuthInProgress = false;
                Application::PostUserEvent(LINK(nullptr, CloudAuthHandler, ShowErrorMessage));
            }
            return;
        }

        // Wait before next poll
        std::this_thread::sleep_for(std::chrono::milliseconds(TOKEN_POLL_INTERVAL_MS));
        nAttempts++;
    }

    // Timeout reached
    {
        osl::MutexGuard aGuard(m_aMutex);
        m_bAuthInProgress = false;
    }
    
    Application::PostUserEvent(LINK(nullptr, CloudAuthHandler, ShowTimeoutMessage));
}

void CloudAuthHandler::openBrowser(const OUString& sUrl)
{
    OUString sUrlEncoded = sUrl;
    
#ifdef _WIN32
    // Windows: Use ShellExecute
    OString sUrlUtf8 = OUStringToOString(sUrlEncoded, RTL_TEXTENCODING_UTF8);
    ShellExecuteA(nullptr, "open", sUrlUtf8.getStr(), nullptr, nullptr, SW_SHOWNORMAL);
    
#elif defined MACOSX
    // macOS: Use 'open' command
    OString sUrlUtf8 = OUStringToOString(sUrlEncoded, RTL_TEXTENCODING_UTF8);
    std::string sCommand = "open \"" + std::string(sUrlUtf8.getStr()) + "\"";
    system(sCommand.c_str());
    
#elif defined UNX
    // Linux: Try xdg-open, then common browsers
    OString sUrlUtf8 = OUStringToOString(sUrlEncoded, RTL_TEXTENCODING_UTF8);
    
    // Try xdg-open first
    std::string sCommand = "xdg-open \"" + std::string(sUrlUtf8.getStr()) + "\" 2>/dev/null";
    if (system(sCommand.c_str()) != 0)
    {
        // Fallback to common browsers
        const char* browsers[] = {"firefox", "chromium", "google-chrome", "opera"};
        for (const char* browser : browsers)
        {
            sCommand = std::string(browser) + " \"" + std::string(sUrlUtf8.getStr()) + "\" 2>/dev/null &";
            if (system(sCommand.c_str()) == 0)
                break;
        }
    }
#endif
}

bool CloudAuthHandler::validateJwtToken(const OUString& sToken) const
{
    if (sToken.isEmpty())
        return false;

    // Basic JWT format validation (header.payload.signature)
    sal_Int32 nFirstDot = sToken.indexOf('.');
    if (nFirstDot == -1)
        return false;
        
    sal_Int32 nSecondDot = sToken.indexOf('.', nFirstDot + 1);
    if (nSecondDot == -1)
        return false;

    // For now, accept any properly formatted JWT
    // In production, we would validate signature and expiration
    return true;
}

// Static callback methods for UI messages
IMPL_STATIC_LINK_NOARG(CloudAuthHandler, ShowSuccessMessage, void*, void)
{
    std::unique_ptr<weld::MessageDialog> xBox(Application::CreateMessageDialog(nullptr,
        VclMessageType::Info, VclButtonsType::Ok,
        "Successfully authenticated with LibreCloud!\n\n"
        "You can now save documents to the cloud using File > Save to Cloud."));
    xBox->set_title("LibreCloud Authentication Success");
    xBox->run();
}

IMPL_STATIC_LINK_NOARG(CloudAuthHandler, ShowErrorMessage, void*, void)
{
    std::unique_ptr<weld::MessageDialog> xBox(Application::CreateMessageDialog(nullptr,
        VclMessageType::Error, VclButtonsType::Ok,
        "Authentication failed. Please try again."));
    xBox->set_title("LibreCloud Authentication Error");
    xBox->run();
}

IMPL_STATIC_LINK_NOARG(CloudAuthHandler, ShowTimeoutMessage, void*, void)
{
    std::unique_ptr<weld::MessageDialog> xBox(Application::CreateMessageDialog(nullptr,
        VclMessageType::Warning, VclButtonsType::Ok,
        "Authentication timed out. Please try again.\n\n"
        "Make sure you complete the sign-in process in your browser within 5 minutes."));
    xBox->set_title("LibreCloud Authentication Timeout");
    xBox->run();
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */ 