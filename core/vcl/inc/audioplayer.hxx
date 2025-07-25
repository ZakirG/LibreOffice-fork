/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include <vcl/ctrl.hxx>
#include <vcl/dllapi.h>
#include <com/sun/star/media/XPlayer.hpp>
#include <rtl/ustring.hxx>

namespace vcl {

class VCL_DLLPUBLIC AudioPlayerControl final : public Control
{
private:
    OUString m_sMediaURL;
    OUString m_sFilename;
    css::uno::Reference<css::media::XPlayer> m_xPlayer;
    bool m_bIsPlaying;
    
    void ImplInitAudioPlayerControlData();
    
public:
    explicit AudioPlayerControl(vcl::Window* pParent, WinBits nStyle = 0);
    virtual ~AudioPlayerControl() override;
    
    // Public interface
    void SetMediaURL(const OUString& rURL);
    const OUString& GetMediaURL() const { return m_sMediaURL; }
    
    void SetFilename(const OUString& rFilename);
    const OUString& GetFilename() const { return m_sFilename; }
    
    bool IsPlaying() const { return m_bIsPlaying; }
    
    // Player control methods
    void Play();
    void Stop();
    void TogglePlayStop();
    
protected:
    // Override virtual methods from Control/Window
    virtual void Paint(vcl::RenderContext& rRenderContext, const tools::Rectangle& rRect) override;
    virtual void MouseButtonDown(const MouseEvent& rMEvt) override;
    virtual void Resize() override;
    virtual Size GetOptimalSize() const override;
    
private:
    void DrawControl(vcl::RenderContext& rRenderContext);
    void DrawPlayButton(vcl::RenderContext& rRenderContext, const tools::Rectangle& rButtonRect);
    void DrawFilename(vcl::RenderContext& rRenderContext, const tools::Rectangle& rTextRect);
    tools::Rectangle GetPlayButtonRect() const;
    tools::Rectangle GetFilenameRect() const;
    
    AudioPlayerControl(const AudioPlayerControl&) = delete;
    AudioPlayerControl& operator=(const AudioPlayerControl&) = delete;
};

} // namespace vcl

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */ 