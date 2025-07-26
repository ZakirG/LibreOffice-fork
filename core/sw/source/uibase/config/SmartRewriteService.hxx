/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_SW_SOURCE_UI_SMARTREWRITE_SMARTREWRITESERVICE_HXX
#define INCLUDED_SW_SOURCE_UI_SMARTREWRITE_SMARTREWRITESERVICE_HXX

#include <rtl/ustring.hxx>

class SmartRewriteService
{
public:
    /**
     * Get the API key for the external AI service.
     * @return The API key if configured, empty string otherwise.
     */
    static OUString GetApiKey();

    /**
     * Get the endpoint URL for the external AI service.
     * @return The endpoint URL if configured, default OpenAI URL otherwise.
     */
    static OUString GetEndpointUrl();

    /**
     * Get the AI service mode/model to use.
     * @return The mode/model name if configured, default "gpt-4" otherwise.
     */
    static OUString GetMode();

    /**
     * Check if the Smart Rewrite feature is enabled.
     * @return true if enabled, false otherwise.
     */
    static bool IsFeatureEnabled();

    /**
     * Check if the Smart Rewrite feature is properly configured.
     * @return true if API key is set and feature is enabled, false otherwise.
     */
    static bool IsConfigured();

private:
    SmartRewriteService() = delete;
    ~SmartRewriteService() = delete;
    SmartRewriteService(const SmartRewriteService&) = delete;
    SmartRewriteService& operator=(const SmartRewriteService&) = delete;
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */ 