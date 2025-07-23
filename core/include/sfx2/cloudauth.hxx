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
#include <osl/mutex.hxx>
#include <osl/thread.hxx>
#include <tools/link.hxx>
#include <memory>

class CloudApiClient;

/**
 * CloudAuthHandler manages authentication with the Libre Cloud service.
 * Implements singleton pattern and handles the complete authentication flow:
 * 1. Generate nonce and get login URL from /api/desktop-init
 * 2. Open browser to login URL
 * 3. Poll /api/desktop-token for JWT token
 * 4. Store JWT securely in LibreOffice configuration
 */
class SFX2_DLLPUBLIC CloudAuthHandler
{
private:
    static std::unique_ptr<CloudAuthHandler> s_pInstance;
    static osl::Mutex s_aMutex;

    mutable osl::Mutex m_aMutex;
    std::unique_ptr<CloudApiClient> m_pApiClient;
    OUString m_sJwtToken;
    OUString m_sCurrentNonce;
    bool m_bIsAuthenticated;
    bool m_bAuthInProgress;
    
    // Private constructor for singleton
    CloudAuthHandler();

public:
    ~CloudAuthHandler();

    // Singleton access
    static CloudAuthHandler& getInstance();

    /**
     * Start the cloud authentication process
     * - Generates nonce via /api/desktop-init
     * - Opens browser to login URL
     * - Starts background polling thread
     */
    void startAuthentication();

    /**
     * Check if user is currently authenticated
     * @return true if valid JWT token is stored
     */
    bool isAuthenticated() const;

    /**
     * Get the current JWT token
     * @return JWT token string, empty if not authenticated
     */
    const OUString& getJwtToken() const;

    /**
     * Check if authentication is currently in progress
     * @return true if authentication flow is active
     */
    bool isAuthInProgress() const;

    /**
     * Clear stored authentication data and logout
     */
    void logout();

    /**
     * Load stored JWT from LibreOffice configuration on startup
     */
    void loadStoredAuth();

private:
    /**
     * Store JWT token securely in LibreOffice configuration
     * @param sToken JWT token to store
     */
    void storeJwtToken(const OUString& sToken);

    /**
     * Background thread function for polling token endpoint
     * @param sNonce The nonce to poll for
     */
    void pollForToken(const OUString& sNonce);

    /**
     * Open browser to the given URL using platform-specific method
     * @param sUrl URL to open
     */
    void openBrowser(const OUString& sUrl);

    /**
     * Validate JWT token format and expiration
     * @param sToken JWT token to validate
     * @return true if token is valid
     */
    bool validateJwtToken(const OUString& sToken) const;

    // Static callback methods for UI messages
    DECL_STATIC_LINK(CloudAuthHandler, ShowSuccessMessage, void*, void);
    DECL_STATIC_LINK(CloudAuthHandler, ShowErrorMessage, void*, void);
    DECL_STATIC_LINK(CloudAuthHandler, ShowTimeoutMessage, void*, void);
};

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */ 