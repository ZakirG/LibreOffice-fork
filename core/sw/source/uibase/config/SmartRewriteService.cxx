/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "SmartRewriteService.hxx"
#include <officecfg/Office/Writer.hxx>
#include <comphelper/configuration.hxx>
#include <sal/log.hxx>

OUString SmartRewriteService::GetApiKey()
{
    try
    {
        auto apiKeyOpt = officecfg::Office::Writer::SmartRewrite::ApiKey::get();
        OUString sApiKey = apiKeyOpt.value_or(OUString());
        SAL_INFO("sw.smartrewrite", "SmartRewriteService::GetApiKey() - API key " 
                 << (sApiKey.isEmpty() ? "not configured" : "configured"));
        return sApiKey;
    }
    catch (const css::uno::Exception& e)
    {
        SAL_WARN("sw.smartrewrite", "SmartRewriteService::GetApiKey() - Exception: " << e.Message);
        return OUString();
    }
}

OUString SmartRewriteService::GetEndpointUrl()
{
    try
    {
        auto endpointOpt = officecfg::Office::Writer::SmartRewrite::EndpointUrl::get();
        OUString sEndpointUrl = endpointOpt.value_or(u"https://api.openai.com/v1/chat/completions"_ustr);
        SAL_INFO("sw.smartrewrite", "SmartRewriteService::GetEndpointUrl() - Endpoint URL: " << sEndpointUrl);
        return sEndpointUrl;
    }
    catch (const css::uno::Exception& e)
    {
        SAL_WARN("sw.smartrewrite", "SmartRewriteService::GetEndpointUrl() - Exception: " << e.Message);
        return u"https://api.openai.com/v1/chat/completions"_ustr; // fallback default
    }
}

OUString SmartRewriteService::GetMode()
{
    try
    {
        auto modeOpt = officecfg::Office::Writer::SmartRewrite::Mode::get();
        OUString sMode = modeOpt.value_or(u"gpt-4"_ustr);
        SAL_INFO("sw.smartrewrite", "SmartRewriteService::GetMode() - Mode: " << sMode);
        return sMode;
    }
    catch (const css::uno::Exception& e)
    {
        SAL_WARN("sw.smartrewrite", "SmartRewriteService::GetMode() - Exception: " << e.Message);
        return u"gpt-4"_ustr; // fallback default
    }
}

bool SmartRewriteService::IsFeatureEnabled()
{
    try
    {
        auto enabledOpt = officecfg::Office::Writer::SmartRewrite::EnableFeature::get();
        bool bEnabled = enabledOpt.value_or(false);
        SAL_INFO("sw.smartrewrite", "SmartRewriteService::IsFeatureEnabled() - Feature enabled: " << bEnabled);
        return bEnabled;
    }
    catch (const css::uno::Exception& e)
    {
        SAL_WARN("sw.smartrewrite", "SmartRewriteService::IsFeatureEnabled() - Exception: " << e.Message);
        return false; // fallback disabled
    }
}

bool SmartRewriteService::IsConfigured()
{
    try
    {
        bool bEnabled = IsFeatureEnabled();
        OUString sApiKey = GetApiKey();
        bool bConfigured = bEnabled && !sApiKey.isEmpty();
        
        SAL_INFO("sw.smartrewrite", "SmartRewriteService::IsConfigured() - Configured: " << bConfigured 
                 << " (enabled: " << bEnabled << ", API key: " 
                 << (sApiKey.isEmpty() ? "empty" : "set") << ")");
        
        return bConfigured;
    }
    catch (const css::uno::Exception& e)
    {
        SAL_WARN("sw.smartrewrite", "SmartRewriteService::IsConfigured() - Exception: " << e.Message);
        return false;
    }
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */ 