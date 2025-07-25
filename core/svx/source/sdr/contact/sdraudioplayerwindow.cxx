/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "sdraudioplayerwindow.hxx"
#include <sdr/contact/viewobjectcontactofsdrmediaobj.hxx>
#include <vcl/event.hxx>
#include <vcl/commandevent.hxx>
#include <vcl/settings.hxx>
#include <vcl/svapp.hxx>
#include <tools/urlobj.hxx>
#include <rtl/uri.hxx>

namespace sdr::contact {

SdrAudioPlayerWindow::SdrAudioPlayerWindow(vcl::Window* pParent, ViewObjectContactOfSdrMediaObj& rViewObjContact)
    : vcl::Window(pParent, WB_CLIPCHILDREN)
    , mrViewObjectContactOfSdrMediaObj(rViewObjContact)
{
    // Create the AudioPlayerControl as a child
    mpAudioPlayerControl = VclPtr<vcl::AudioPlayerControl>::Create(this);
    mpAudioPlayerControl->Show();
    
    // Set background
    SetBackground(Wallpaper(Application::GetSettings().GetStyleSettings().GetFaceColor()));
}

SdrAudioPlayerWindow::~SdrAudioPlayerWindow()
{
    disposeOnce();
}

void SdrAudioPlayerWindow::dispose()
{
    mpAudioPlayerControl.disposeAndClear();
    vcl::Window::dispose();
}

void SdrAudioPlayerWindow::setURL(const OUString& rURL, const OUString& rReferer)
{
    m_sURL = rURL;
    m_sReferer = rReferer;
    
    if (mpAudioPlayerControl)
    {
        mpAudioPlayerControl->SetMediaURL(rURL);
        
        // Extract filename from URL for display
        OUString sFilename;
        if (!rURL.isEmpty())
        {
            INetURLObject aURL(rURL);
            sFilename = aURL.getName();
            if (sFilename.isEmpty())
            {
                // Fallback: extract filename from path
                sal_Int32 nLastSlash = std::max(rURL.lastIndexOf('/'), rURL.lastIndexOf('\\'));
                if (nLastSlash != -1 && nLastSlash < rURL.getLength() - 1)
                {
                    sFilename = rURL.copy(nLastSlash + 1);
                }
                else
                {
                    sFilename = rURL;
                }
            }
            
            // Decode URL-encoded filename (e.g., %20 -> space)
            sFilename = rtl::Uri::decode(sFilename, rtl_UriDecodeWithCharset, RTL_TEXTENCODING_UTF8);
        }
        mpAudioPlayerControl->SetFilename(sFilename);
    }
}

const OUString& SdrAudioPlayerWindow::getURL() const
{
    return m_sURL;
}

bool SdrAudioPlayerWindow::isValid() const
{
    return mpAudioPlayerControl && !m_sURL.isEmpty();
}

Size SdrAudioPlayerWindow::getPreferredSize() const
{
    if (mpAudioPlayerControl)
        return mpAudioPlayerControl->GetOptimalSize();
    return Size(320, 80); // Default 4:1 aspect ratio
}

bool SdrAudioPlayerWindow::start()
{
    if (mpAudioPlayerControl && !m_sURL.isEmpty())
    {
        mpAudioPlayerControl->Play();
        return true;
    }
    return false;
}

void SdrAudioPlayerWindow::updateMediaItem(::avmedia::MediaItem& rItem) const
{
    if (mpAudioPlayerControl)
    {
        rItem.setURL(m_sURL, OUString(), m_sReferer);
        rItem.setState(mpAudioPlayerControl->IsPlaying() ? ::avmedia::MediaState::Play : ::avmedia::MediaState::Stop);
    }
}

void SdrAudioPlayerWindow::executeMediaItem(const ::avmedia::MediaItem& rItem)
{
    const OUString aURL(rItem.getURL());
    
    if (!aURL.isEmpty())
        setURL(aURL, rItem.getReferer());
    
    if (mpAudioPlayerControl)
    {
        const ::avmedia::MediaState eState = rItem.getState();
        
        switch (eState)
        {
            case ::avmedia::MediaState::Play:
                mpAudioPlayerControl->Play();
                break;
            case ::avmedia::MediaState::Pause:
            case ::avmedia::MediaState::Stop:
                mpAudioPlayerControl->Stop();
                break;
            default:
                break;
        }
    }
}

void SdrAudioPlayerWindow::setPosSize(const tools::Rectangle& rRect)
{
    SetPosSizePixel(rRect.TopLeft(), rRect.GetSize());
}

void SdrAudioPlayerWindow::show()
{
    Show();
}

void SdrAudioPlayerWindow::hide()
{
    Hide();
}

bool SdrAudioPlayerWindow::isVisible() const
{
    return IsVisible();
}

void SdrAudioPlayerWindow::Paint(vcl::RenderContext& rRenderContext, const tools::Rectangle& rRect)
{
    // Let the AudioPlayerControl handle its own painting
    vcl::Window::Paint(rRenderContext, rRect);
}

void SdrAudioPlayerWindow::Resize()
{
    vcl::Window::Resize();
    
    if (mpAudioPlayerControl)
    {
        // Make the AudioPlayerControl fill the entire window
        mpAudioPlayerControl->SetPosSizePixel(Point(0, 0), GetSizePixel());
    }
}

void SdrAudioPlayerWindow::MouseMove(const MouseEvent& rMEvt)
{
    // Forward to the parent window using the same coordinate transformation as SdrMediaWindow
    vcl::Window* pWindow = mrViewObjectContactOfSdrMediaObj.getWindow();
    
    if (pWindow)
    {
        const MouseEvent aTransformedEvent(
            pWindow->ScreenToOutputPixel(OutputToScreenPixel(rMEvt.GetPosPixel())),
            rMEvt.GetClicks(), rMEvt.GetMode(), rMEvt.GetButtons(), rMEvt.GetModifier());
        
        pWindow->MouseMove(aTransformedEvent);
        SetPointer(pWindow->GetPointer());
    }
}

void SdrAudioPlayerWindow::MouseButtonDown(const MouseEvent& rMEvt)
{
    // Forward to AudioPlayerControl first
    if (mpAudioPlayerControl)
    {
        mpAudioPlayerControl->HandleMouseClick(rMEvt);
    }
    
    // Then forward to parent for proper handling using same coordinate transformation as SdrMediaWindow
    vcl::Window* pWindow = mrViewObjectContactOfSdrMediaObj.getWindow();
    if (pWindow)
    {
        const MouseEvent aTransformedEvent(
            pWindow->ScreenToOutputPixel(OutputToScreenPixel(rMEvt.GetPosPixel())),
            rMEvt.GetClicks(), rMEvt.GetMode(), rMEvt.GetButtons(), rMEvt.GetModifier());
        
        pWindow->MouseButtonDown(aTransformedEvent);
    }
}

void SdrAudioPlayerWindow::MouseButtonUp(const MouseEvent& rMEvt)
{
    vcl::Window* pWindow = mrViewObjectContactOfSdrMediaObj.getWindow();
    
    if (pWindow)
    {
        const MouseEvent aTransformedEvent(
            pWindow->ScreenToOutputPixel(OutputToScreenPixel(rMEvt.GetPosPixel())),
            rMEvt.GetClicks(), rMEvt.GetMode(), rMEvt.GetButtons(), rMEvt.GetModifier());
        
        pWindow->MouseButtonUp(aTransformedEvent);
    }
}

void SdrAudioPlayerWindow::KeyInput(const KeyEvent& rKEvt)
{
    vcl::Window* pWindow = mrViewObjectContactOfSdrMediaObj.getWindow();
    
    if (pWindow)
        pWindow->KeyInput(rKEvt);
}

void SdrAudioPlayerWindow::KeyUp(const KeyEvent& rKEvt)
{
    vcl::Window* pWindow = mrViewObjectContactOfSdrMediaObj.getWindow();
    
    if (pWindow)
        pWindow->KeyUp(rKEvt);
}

void SdrAudioPlayerWindow::Command(const CommandEvent& rCEvt)
{
    vcl::Window* pWindow = mrViewObjectContactOfSdrMediaObj.getWindow();
    
    if (pWindow)
    {
        const CommandEvent aTransformedEvent(
            pWindow->ScreenToOutputPixel(OutputToScreenPixel(rCEvt.GetMousePosPixel())),
            rCEvt.GetCommand(), rCEvt.IsMouseEvent(), rCEvt.GetEventData());
        
        pWindow->Command(aTransformedEvent);
    }
}

} // namespace sdr::contact

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */ 