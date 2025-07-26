/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <SmartRewriteInterceptor.hxx>
#include <view.hxx>
#include <wrtsh.hxx>
#include <sal/log.hxx>
#include <sfx2/viewfrm.hxx>

#include <com/sun/star/frame/XFrame.hpp>
#include <comphelper/servicehelper.hxx>

using namespace ::com::sun::star;

const char cURLSmartRewrite[] = ".uno:SmartRewrite";

// SmartRewriteDispatch implementation

SmartRewriteDispatch::SmartRewriteDispatch(SwView& rView)
    : m_pView(&rView)
{
    SAL_INFO("sw.smartrewrite", "SmartRewriteDispatch constructed");
}

SmartRewriteDispatch::~SmartRewriteDispatch()
{
    SAL_INFO("sw.smartrewrite", "SmartRewriteDispatch destroyed");
}

void SmartRewriteDispatch::dispatch(const util::URL& aURL, const uno::Sequence<beans::PropertyValue>& /*aArgs*/)
{
    if (!m_pView)
    {
        SAL_WARN("sw.smartrewrite", "SmartRewriteDispatch::dispatch - no view available");
        return;
    }

    if (aURL.Complete != cURLSmartRewrite)
    {
        SAL_WARN("sw.smartrewrite", "SmartRewriteDispatch::dispatch - unexpected URL: " << aURL.Complete);
        return;
    }

    SAL_INFO("sw.smartrewrite", "SmartRewriteDispatch::dispatch - handling SmartRewrite command");

    // For now, just log that we received the command
    // In later prompts (4-6), this will actually launch the SmartRewrite dialog
    SwWrtShell* pWrtShell = m_pView->GetWrtShellPtr();
    if (pWrtShell && pWrtShell->HasSelection())
    {
        OUString sSelectedText = pWrtShell->GetSelText();
        SAL_INFO("sw.smartrewrite", "SmartRewriteDispatch would process text: " << sSelectedText);
        
        // TODO: In Prompt 4, this will launch the SmartRewriteDialogController
        // For now, just show a basic info box to demonstrate the integration works
        // This will be replaced with actual dialog launching in Prompt 4
    }
    else
    {
        SAL_WARN("sw.smartrewrite", "SmartRewriteDispatch::dispatch - no text selection found");
    }
}

void SmartRewriteDispatch::addStatusListener(const uno::Reference<frame::XStatusListener>& /*xControl*/,
                                            const util::URL& /*aURL*/)
{
    // Not implemented for this use case
}

void SmartRewriteDispatch::removeStatusListener(const uno::Reference<frame::XStatusListener>& /*xControl*/,
                                               const util::URL& /*aURL*/)
{
    // Not implemented for this use case  
}

void SmartRewriteDispatch::Invalidate()
{
    m_pView = nullptr;
}

// SmartRewriteInterceptor implementation

SmartRewriteInterceptor::DispatchMutexLock_Impl::DispatchMutexLock_Impl()
{
}

SmartRewriteInterceptor::DispatchMutexLock_Impl::~DispatchMutexLock_Impl()
{
}

SmartRewriteInterceptor::SmartRewriteInterceptor(SwView& rView)
    : m_pView(&rView)
{
    uno::Reference<frame::XFrame> xUnoFrame = m_pView->GetViewFrame().GetFrame().GetFrameInterface();
    m_xIntercepted.set(xUnoFrame, uno::UNO_QUERY);
    if (m_xIntercepted.is())
    {
        osl_atomic_increment(&m_refCount);
        m_xIntercepted->registerDispatchProviderInterceptor(static_cast<frame::XDispatchProviderInterceptor*>(this));
        // this should make us the top-level dispatch-provider for the component, via a call to our
        // setDispatchProvider we should have got a fallback for requests we (i.e. our master) cannot fulfill
        uno::Reference<lang::XComponent> xInterceptedComponent(m_xIntercepted, uno::UNO_QUERY);
        if (xInterceptedComponent.is())
            xInterceptedComponent->addEventListener(static_cast<lang::XEventListener*>(this));
        osl_atomic_decrement(&m_refCount);
    }
    
    SAL_INFO("sw.smartrewrite", "SmartRewriteInterceptor constructed and registered");
}

SmartRewriteInterceptor::~SmartRewriteInterceptor()
{
    SAL_INFO("sw.smartrewrite", "SmartRewriteInterceptor destroyed");
}

uno::Reference<frame::XDispatch> SmartRewriteInterceptor::queryDispatch(
    const util::URL& aURL, const OUString& aTargetFrameName, sal_Int32 nSearchFlags)
{
    DispatchMutexLock_Impl aLock;
    uno::Reference<frame::XDispatch> xResult;

    // Check if this is our SmartRewrite command and if text is selected
    if (m_pView && aURL.Complete == cURLSmartRewrite)
    {
        if (IsTextSelected())
        {
            SAL_INFO("sw.smartrewrite", "SmartRewriteInterceptor::queryDispatch - providing SmartRewrite dispatcher");
            if (!m_xDispatch.is())
                m_xDispatch = new SmartRewriteDispatch(*m_pView);
            xResult = m_xDispatch;
        }
        else
        {
            SAL_INFO("sw.smartrewrite", "SmartRewriteInterceptor::queryDispatch - no text selected, not providing dispatcher");
        }
    }

    // ask our slave provider for other requests
    if (!xResult.is() && m_xSlaveDispatcher.is())
        xResult = m_xSlaveDispatcher->queryDispatch(aURL, aTargetFrameName, nSearchFlags);

    return xResult;
}

uno::Sequence<OUString> SmartRewriteInterceptor::getInterceptedURLs()
{
    uno::Sequence<OUString> aRet = {
        u".uno:SmartRewrite"_ustr
    };
    return aRet;
}

uno::Sequence<uno::Reference<frame::XDispatch>> SmartRewriteInterceptor::queryDispatches(
    const uno::Sequence<frame::DispatchDescriptor>& aDescripts)
{
    DispatchMutexLock_Impl aLock;
    uno::Sequence<uno::Reference<frame::XDispatch>> aReturn(aDescripts.getLength());
    std::transform(aDescripts.begin(), aDescripts.end(), aReturn.getArray(),
        [this](const frame::DispatchDescriptor& rDescr) -> uno::Reference<frame::XDispatch> {
            return queryDispatch(rDescr.FeatureURL, rDescr.FrameName, rDescr.SearchFlags); });
    return aReturn;
}

uno::Reference<frame::XDispatchProvider> SmartRewriteInterceptor::getSlaveDispatchProvider()
{
    DispatchMutexLock_Impl aLock;
    return m_xSlaveDispatcher;
}

void SmartRewriteInterceptor::setSlaveDispatchProvider(
    const uno::Reference<frame::XDispatchProvider>& xNewDispatchProvider)
{
    DispatchMutexLock_Impl aLock;
    m_xSlaveDispatcher = xNewDispatchProvider;
}

uno::Reference<frame::XDispatchProvider> SmartRewriteInterceptor::getMasterDispatchProvider()
{
    DispatchMutexLock_Impl aLock;
    return m_xMasterDispatcher;
}

void SmartRewriteInterceptor::setMasterDispatchProvider(
    const uno::Reference<frame::XDispatchProvider>& xNewSupplier)
{
    DispatchMutexLock_Impl aLock;
    m_xMasterDispatcher = xNewSupplier;
}

void SmartRewriteInterceptor::disposing(const lang::EventObject& /*Source*/)
{
    DispatchMutexLock_Impl aLock;
    if (m_xIntercepted.is())
    {
        m_xIntercepted->releaseDispatchProviderInterceptor(static_cast<frame::XDispatchProviderInterceptor*>(this));
        uno::Reference<lang::XComponent> xInterceptedComponent(m_xIntercepted, uno::UNO_QUERY);
        if (xInterceptedComponent.is())
            xInterceptedComponent->removeEventListener(static_cast<lang::XEventListener*>(this));
        m_xDispatch = nullptr;
    }
    m_xIntercepted = nullptr;
}

void SmartRewriteInterceptor::Invalidate()
{
    DispatchMutexLock_Impl aLock;
    if (m_xIntercepted.is())
    {
        m_xIntercepted->releaseDispatchProviderInterceptor(static_cast<frame::XDispatchProviderInterceptor*>(this));
        uno::Reference<lang::XComponent> xInterceptedComponent(m_xIntercepted, uno::UNO_QUERY);
        if (xInterceptedComponent.is())
            xInterceptedComponent->removeEventListener(static_cast<lang::XEventListener*>(this));
        m_xDispatch = nullptr;
    }
    m_xIntercepted = nullptr;
    m_pView = nullptr;
}

bool SmartRewriteInterceptor::IsTextSelected() const
{
    if (!m_pView)
        return false;
    
    SwWrtShell* pWrtShell = m_pView->GetWrtShellPtr();
    if (!pWrtShell)
        return false;
    
    return pWrtShell->HasSelection();
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
