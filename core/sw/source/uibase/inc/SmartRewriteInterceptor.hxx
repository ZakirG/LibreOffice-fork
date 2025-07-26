/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_SW_SOURCE_UIBASE_INC_SMARTREWRITEINTERCEPTOR_HXX
#define INCLUDED_SW_SOURCE_UIBASE_INC_SMARTREWRITEINTERCEPTOR_HXX

#include <com/sun/star/frame/XDispatchProviderInterception.hpp>
#include <com/sun/star/frame/XDispatchProviderInterceptor.hpp>
#include <com/sun/star/frame/XDispatch.hpp>
#include <com/sun/star/frame/XInterceptorInfo.hpp>
#include <com/sun/star/frame/XStatusListener.hpp>
#include <cppuhelper/implbase.hxx>
#include <vcl/svapp.hxx>

class SwView;

class SmartRewriteDispatch : public cppu::WeakImplHelper<css::frame::XDispatch>
{
private:
    SwView* m_pView;

public:
    explicit SmartRewriteDispatch(SwView& rView);
    virtual ~SmartRewriteDispatch() override;

    // XDispatch
    virtual void SAL_CALL dispatch(const css::util::URL& aURL,
                                 const css::uno::Sequence<css::beans::PropertyValue>& aArgs) override;
    virtual void SAL_CALL addStatusListener(const css::uno::Reference<css::frame::XStatusListener>& xControl,
                                          const css::util::URL& aURL) override;
    virtual void SAL_CALL removeStatusListener(const css::uno::Reference<css::frame::XStatusListener>& xControl,
                                             const css::util::URL& aURL) override;

    void Invalidate();
};

class SmartRewriteInterceptor final : public cppu::WeakImplHelper
<
    css::frame::XDispatchProviderInterceptor,
    css::lang::XEventListener,
    css::frame::XInterceptorInfo
>
{
    class DispatchMutexLock_Impl
    {
        SolarMutexGuard aGuard;
    public:
        DispatchMutexLock_Impl();
        ~DispatchMutexLock_Impl();
    };
    friend class DispatchMutexLock_Impl;

    // the component which's dispatches we're intercepting
    css::uno::Reference<css::frame::XDispatchProviderInterception> m_xIntercepted;

    // chaining
    css::uno::Reference<css::frame::XDispatchProvider> m_xSlaveDispatcher;
    css::uno::Reference<css::frame::XDispatchProvider> m_xMasterDispatcher;

    rtl::Reference<SmartRewriteDispatch> m_xDispatch;

    SwView* m_pView;

public:
    explicit SmartRewriteInterceptor(SwView& rView);
    virtual ~SmartRewriteInterceptor() override;

    // XDispatchProvider
    virtual css::uno::Reference<css::frame::XDispatch> SAL_CALL queryDispatch(
        const css::util::URL& aURL, const OUString& aTargetFrameName, sal_Int32 nSearchFlags) override;
    virtual css::uno::Sequence<css::uno::Reference<css::frame::XDispatch>> SAL_CALL queryDispatches(
        const css::uno::Sequence<css::frame::DispatchDescriptor>& aDescripts) override;

    // XDispatchProviderInterceptor
    virtual css::uno::Reference<css::frame::XDispatchProvider> SAL_CALL getSlaveDispatchProvider() override;
    virtual void SAL_CALL setSlaveDispatchProvider(
        const css::uno::Reference<css::frame::XDispatchProvider>& xNewDispatchProvider) override;
    virtual css::uno::Reference<css::frame::XDispatchProvider> SAL_CALL getMasterDispatchProvider() override;
    virtual void SAL_CALL setMasterDispatchProvider(
        const css::uno::Reference<css::frame::XDispatchProvider>& xNewSupplier) override;

    // XInterceptorInfo
    virtual css::uno::Sequence<OUString> SAL_CALL getInterceptedURLs() override;

    // XEventListener
    virtual void SAL_CALL disposing(const css::lang::EventObject& Source) override;

    void Invalidate();

private:
    bool IsTextSelected() const;
};

#endif

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */
