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
#include <typeinfo>

namespace sdr::contact {

SdrAudioPlayerWindow::SdrAudioPlayerWindow(vcl::Window* pParent, ViewObjectContactOfSdrMediaObj& rViewObjContact)
    : vcl::Window(pParent, WB_CLIPCHILDREN)
    , mrViewObjectContactOfSdrMediaObj(rViewObjContact)
{
    SAL_WARN("vcl.audio", "=== SdrAudioPlayerWindow CONSTRUCTOR CALLED ===");
    fprintf(stderr, "*** DIRECT LOG: SdrAudioPlayerWindow constructor - audio player created! ***\n");
    SAL_WARN("vcl.audio", "Parent window: " << pParent);
    
    // Create the AudioPlayerControl as a child
    mpAudioPlayerControl = VclPtr<vcl::AudioPlayerControl>::Create(this);
    mpAudioPlayerControl->Show();
    SAL_WARN("vcl.audio", "AudioPlayerControl created and shown: " << mpAudioPlayerControl.get());
    
    // Log the window we'll forward events to
    vcl::Window* pTargetWindow = mrViewObjectContactOfSdrMediaObj.getWindow();
    SAL_WARN("vcl.audio", "Target window for event forwarding: " << pTargetWindow);
    
    // Set background
    SetBackground(Wallpaper(Application::GetSettings().GetStyleSettings().GetFaceColor()));
    
    // Enable input and make sure window is properly set up for events
    EnableInput();
    Show();
    fprintf(stderr, "*** DIRECT LOG: Audio player window shown and input enabled ***\n");
    
    SAL_WARN("vcl.audio", "=== SdrAudioPlayerWindow CONSTRUCTOR COMPLETE ===");
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
    fprintf(stderr, "*** DIRECT LOG: Setting audio window pos/size to %d,%d %dx%d ***\n",
            rRect.Left(), rRect.Top(), rRect.GetWidth(), rRect.GetHeight());
    SetPosSizePixel(rRect.TopLeft(), rRect.GetSize());
    
    // Log actual position after setting
    Point aActualPos = GetPosPixel();
    Size aActualSize = GetSizePixel();
    fprintf(stderr, "*** DIRECT LOG: Actual audio window pos/size: %d,%d %dx%d ***\n",
            aActualPos.X(), aActualPos.Y(), aActualSize.Width(), aActualSize.Height());
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
    static int mouseMoveCount = 0;
    if (++mouseMoveCount % 10 == 1) { // Log every 10th move to avoid spam
        fprintf(stderr, "*** DIRECT LOG: Mouse move #%d on audio player ***\n", mouseMoveCount);
    }
    
    // Let VCL handle the event normally first
    vcl::Window::MouseMove(rMEvt);
    
    // Forward to the parent window using the same coordinate transformation as SdrMediaWindow
    vcl::Window* pWindow = mrViewObjectContactOfSdrMediaObj.getWindow();
    
    if (pWindow && mpAudioPlayerControl)
    {
        Point aScreenPos = mpAudioPlayerControl->OutputToScreenPixel(rMEvt.GetPosPixel());
        Point aTransformedPos = pWindow->ScreenToOutputPixel(aScreenPos);
        
        const MouseEvent aTransformedEvent(aTransformedPos, rMEvt.GetClicks(), rMEvt.GetMode(), rMEvt.GetButtons(), rMEvt.GetModifier());
        
        pWindow->MouseMove(aTransformedEvent);
        SetPointer(pWindow->GetPointer());
        
        // Log occasionally to see if mouse moves are working
        static int moveCount = 0;
        if (++moveCount % 50 == 0) // Log every 50th move event
        {
            SAL_INFO("vcl.audio", "MouseMove #" << moveCount << " - pos=" << rMEvt.GetPosPixel().X() << "," << rMEvt.GetPosPixel().Y() 
                     << " pointer=" << static_cast<int>(GetPointer()));
        }
    }
}

void SdrAudioPlayerWindow::MouseButtonDown(const MouseEvent& rMEvt)
{
    SAL_WARN("vcl.audio", "*** MOUSE CLICK on SdrAudioPlayerWindow ***");
    fprintf(stderr, "*** DIRECT LOG: Mouse click on audio player window ***\n");
    SAL_WARN("vcl.audio", "Click position: " << rMEvt.GetPosPixel().X() << "," << rMEvt.GetPosPixel().Y());
    
    // Let VCL handle the event normally first (for AudioPlayerControl)
    vcl::Window::MouseButtonDown(rMEvt);
    
    // Then forward to parent for dragging (same as SdrMediaWindow)
    vcl::Window* pWindow = mrViewObjectContactOfSdrMediaObj.getWindow();
    if (pWindow && mpAudioPlayerControl)
    {
        Point aScreenPos = mpAudioPlayerControl->OutputToScreenPixel(rMEvt.GetPosPixel());
        Point aTransformedPos = pWindow->ScreenToOutputPixel(aScreenPos);
        
        SAL_WARN("vcl.audio", "Forwarding event to parent window: " << pWindow);
        
        const MouseEvent aTransformedEvent(aTransformedPos, rMEvt.GetClicks(), rMEvt.GetMode(), rMEvt.GetButtons(), rMEvt.GetModifier());
        
        pWindow->MouseButtonDown(aTransformedEvent);
        SAL_WARN("vcl.audio", "Event forwarded successfully!");
    }
    else
    {
        SAL_WARN("vcl.audio", "FAILED to forward event - no parent window or audio control");
    }
}

void SdrAudioPlayerWindow::MouseButtonUp(const MouseEvent& rMEvt)
{
    // Let VCL handle the event normally first
    vcl::Window::MouseButtonUp(rMEvt);
    
    vcl::Window* pWindow = mrViewObjectContactOfSdrMediaObj.getWindow();
    
    if (pWindow && mpAudioPlayerControl)
    {
        const MouseEvent aTransformedEvent(
            pWindow->ScreenToOutputPixel(mpAudioPlayerControl->OutputToScreenPixel(rMEvt.GetPosPixel())),
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
    
    if (pWindow && mpAudioPlayerControl)
    {
        const CommandEvent aTransformedEvent(
            pWindow->ScreenToOutputPixel(mpAudioPlayerControl->OutputToScreenPixel(rCEvt.GetMousePosPixel())),
            rCEvt.GetCommand(), rCEvt.IsMouseEvent(), rCEvt.GetEventData());
        
        pWindow->Command(aTransformedEvent);
    }
}

} // namespace sdr::contact

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */ 