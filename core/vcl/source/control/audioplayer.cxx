/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include <vcl/audioplayer.hxx>
#include <vcl/event.hxx>
#include <vcl/outdev.hxx>
#include <vcl/settings.hxx>
#include <vcl/svapp.hxx>
#include <avmedia/mediawindow.hxx>
#include <tools/gen.hxx>
#include <sal/log.hxx>

using namespace vcl;

void AudioPlayerControl::ImplInitAudioPlayerControlData()
{
    m_bIsPlaying = false;
}

AudioPlayerControl::AudioPlayerControl(vcl::Window* pParent, WinBits nStyle)
    : Control(pParent, nStyle)
{
    ImplInitAudioPlayerControlData();
    SetBackground(Wallpaper(Application::GetSettings().GetStyleSettings().GetFaceColor()));
    
    // Set a reasonable default size (wide rectangle as per requirement)
    SetSizePixel(Size(320, 80)); // 4:1 aspect ratio
}

AudioPlayerControl::~AudioPlayerControl()
{
    disposeOnce();
}

void AudioPlayerControl::SetMediaURL(const OUString& rURL)
{
    if (m_sMediaURL != rURL)
    {
        // Stop current playback if any
        if (m_bIsPlaying)
        {
            Stop();
        }
        
        m_sMediaURL = rURL;
        
        // Create placeholder player instance (as per prompt requirements)
        if (!rURL.isEmpty())
        {
            try
            {
                m_xPlayer = avmedia::MediaWindow::createPlayer(rURL, OUString());
            }
            catch (const css::uno::Exception&)
            {
                SAL_WARN("vcl", "Failed to create media player for URL: " << rURL);
            }
        }
        else
        {
            m_xPlayer.clear();
        }
        
        Invalidate(); // Trigger repaint
    }
}

void AudioPlayerControl::SetFilename(const OUString& rFilename)
{
    if (m_sFilename != rFilename)
    {
        m_sFilename = rFilename;
        Invalidate(); // Trigger repaint
    }
}

void AudioPlayerControl::Play()
{
    if (m_xPlayer.is() && !m_bIsPlaying)
    {
        try
        {
            SAL_INFO("vcl", "AudioPlayerControl::Play() - Starting playback");
            m_xPlayer->start();
            m_bIsPlaying = true;
            Invalidate(); // Update visual state to show pause button
        }
        catch (const css::uno::Exception& e)
        {
            SAL_WARN("vcl", "AudioPlayerControl::Play() - Failed to start playback: " << e.Message);
        }
    }
}

void AudioPlayerControl::Stop()
{
    if (m_xPlayer.is() && m_bIsPlaying)
    {
        try
        {
            SAL_INFO("vcl", "AudioPlayerControl::Stop() - Stopping playback");
            m_xPlayer->stop();
            m_bIsPlaying = false;
            Invalidate(); // Update visual state to show play button
        }
        catch (const css::uno::Exception& e)
        {
            SAL_WARN("vcl", "AudioPlayerControl::Stop() - Failed to stop playback: " << e.Message);
        }
    }
}

void AudioPlayerControl::TogglePlayStop()
{
    if (m_bIsPlaying)
        Stop();
    else
        Play();
}

void AudioPlayerControl::HandleMouseClick(const MouseEvent& rMEvt)
{
    MouseButtonDown(rMEvt);
}

void AudioPlayerControl::Paint(vcl::RenderContext& rRenderContext, const tools::Rectangle& /*rRect*/)
{
    DrawControl(rRenderContext);
}

void AudioPlayerControl::MouseButtonDown(const MouseEvent& rMEvt)
{
    // Log click event as per prompt requirement
    SAL_INFO("vcl", "AudioPlayerControl::MouseButtonDown() - Control clicked at (" 
             << rMEvt.GetPosPixel().X() << ", " << rMEvt.GetPosPixel().Y() << ")");
    
    // Check if click is on play button area
    tools::Rectangle aPlayButtonRect = GetPlayButtonRect();
    if (aPlayButtonRect.Contains(rMEvt.GetPosPixel()))
    {
        TogglePlayStop();
    }
    
    Control::MouseButtonDown(rMEvt);
}

void AudioPlayerControl::Resize()
{
    Control::Resize();
    Invalidate();
}

Size AudioPlayerControl::GetOptimalSize() const
{
    // Return a wide rectangle (4:1 aspect ratio) as per requirement
    return Size(320, 80);
}

void AudioPlayerControl::DrawControl(vcl::RenderContext& rRenderContext)
{
    const Size aOutputSize = GetOutputSizePixel();
    const tools::Rectangle aRect(Point(0, 0), aOutputSize);
    
    // Clear background
    rRenderContext.SetFillColor(Application::GetSettings().GetStyleSettings().GetFaceColor());
    rRenderContext.SetLineColor(Application::GetSettings().GetStyleSettings().GetShadowColor());
    rRenderContext.DrawRect(aRect);
    
    // Draw play button area
    tools::Rectangle aPlayButtonRect = GetPlayButtonRect();
    DrawPlayButton(rRenderContext, aPlayButtonRect);
    
    // Draw filename text area
    tools::Rectangle aFilenameRect = GetFilenameRect();
    DrawFilename(rRenderContext, aFilenameRect);
}

void AudioPlayerControl::DrawPlayButton(vcl::RenderContext& rRenderContext, const tools::Rectangle& rButtonRect)
{
    // Draw button background
    rRenderContext.SetFillColor(Application::GetSettings().GetStyleSettings().GetFaceColor());
    rRenderContext.SetLineColor(Application::GetSettings().GetStyleSettings().GetButtonTextColor());
    rRenderContext.DrawRect(rButtonRect);
    
    // Draw play/pause symbol
    const Point aCenter = rButtonRect.Center();
    const sal_Int32 nSymbolSize = std::min(rButtonRect.GetWidth(), rButtonRect.GetHeight()) / 3;
    
    rRenderContext.SetFillColor(Application::GetSettings().GetStyleSettings().GetButtonTextColor());
    rRenderContext.SetLineColor();
    
    if (m_bIsPlaying)
    {
        // Draw pause symbol (two vertical bars)
        const sal_Int32 nBarWidth = nSymbolSize / 4;
        const sal_Int32 nBarSpacing = nSymbolSize / 3;
        tools::Rectangle aBar1(aCenter.X() - nBarSpacing/2 - nBarWidth, aCenter.Y() - nSymbolSize/2,
                              aCenter.X() - nBarSpacing/2, aCenter.Y() + nSymbolSize/2);
        tools::Rectangle aBar2(aCenter.X() + nBarSpacing/2, aCenter.Y() - nSymbolSize/2,
                              aCenter.X() + nBarSpacing/2 + nBarWidth, aCenter.Y() + nSymbolSize/2);
        rRenderContext.DrawRect(aBar1);
        rRenderContext.DrawRect(aBar2);
    }
    else
    {
        // Draw play symbol (triangle pointing right)
        tools::Polygon aTriangle(3);
        aTriangle.SetPoint(Point(aCenter.X() - nSymbolSize/3, aCenter.Y() - nSymbolSize/2), 0);
        aTriangle.SetPoint(Point(aCenter.X() - nSymbolSize/3, aCenter.Y() + nSymbolSize/2), 1);
        aTriangle.SetPoint(Point(aCenter.X() + nSymbolSize/2, aCenter.Y()), 2);
        rRenderContext.DrawPolygon(aTriangle);
    }
}

void AudioPlayerControl::DrawFilename(vcl::RenderContext& rRenderContext, const tools::Rectangle& rTextRect)
{
    if (!m_sFilename.isEmpty())
    {
        rRenderContext.SetTextColor(Application::GetSettings().GetStyleSettings().GetButtonTextColor());
        rRenderContext.SetTextFillColor();
        
        // Draw filename text
        DrawTextFlags nTextStyle = DrawTextFlags::Left | DrawTextFlags::VCenter | DrawTextFlags::EndEllipsis;
        rRenderContext.DrawText(rTextRect, m_sFilename, nTextStyle);
    }
    else
    {
        // Draw placeholder text
        rRenderContext.SetTextColor(Application::GetSettings().GetStyleSettings().GetDeactiveTextColor());
        rRenderContext.SetTextFillColor();
        
        DrawTextFlags nTextStyle = DrawTextFlags::Left | DrawTextFlags::VCenter;
        rRenderContext.DrawText(rTextRect, "No audio file selected", nTextStyle);
    }
}

tools::Rectangle AudioPlayerControl::GetPlayButtonRect() const
{
    const Size aOutputSize = GetOutputSizePixel();
    const sal_Int32 nButtonSize = std::min(static_cast<sal_Int32>(aOutputSize.Height()) - 10, 60); // Leave some margin
    const sal_Int32 nMargin = 5;
    
    return tools::Rectangle(Point(nMargin, (aOutputSize.Height() - nButtonSize) / 2),
                          Size(nButtonSize, nButtonSize));
}

tools::Rectangle AudioPlayerControl::GetFilenameRect() const
{
    const Size aOutputSize = GetOutputSizePixel();
    const tools::Rectangle aPlayButtonRect = GetPlayButtonRect();
    const sal_Int32 nMargin = 10;
    
    const sal_Int32 nTextLeft = aPlayButtonRect.Right() + nMargin;
    const sal_Int32 nTextTop = 5;
    const sal_Int32 nTextWidth = aOutputSize.Width() - nTextLeft - 5; // Right margin
    const sal_Int32 nTextHeight = aOutputSize.Height() - 10;
    
    return tools::Rectangle(Point(nTextLeft, nTextTop), Size(nTextWidth, nTextHeight));
}

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */ 