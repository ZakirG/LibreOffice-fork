/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_SVX_SOURCE_SDR_CONTACT_SDRAUDIOPLAYERWINDOW_HXX
#define INCLUDED_SVX_SOURCE_SDR_CONTACT_SDRAUDIOPLAYERWINDOW_HXX

#include <vcl/window.hxx>
#include <vcl/vclptr.hxx>
#include <vcl/audioplayer.hxx>
#include <avmedia/mediaitem.hxx>
#include "sdrmediawindowinterface.hxx"

namespace sdr::contact {

class ViewObjectContactOfSdrMediaObj;

class SdrAudioPlayerWindow : public vcl::Window, public SdrMediaWindowInterface
{
public:
    SdrAudioPlayerWindow(vcl::Window* pParent, ViewObjectContactOfSdrMediaObj& rViewObjContact);
    virtual ~SdrAudioPlayerWindow() override;
    virtual void dispose() override;

    // MediaWindow-like interface
    void setURL(const OUString& rURL, const OUString& rReferer = OUString());
    const OUString& getURL() const;
    bool isValid() const;
    bool start();
    
    // SdrMediaWindowInterface implementation
    virtual Size getPreferredSize() const override;
    virtual void updateMediaItem(::avmedia::MediaItem& rItem) const override;
    virtual void executeMediaItem(const ::avmedia::MediaItem& rItem) override;
    virtual void setPosSize(const tools::Rectangle& rRect) override;
    virtual void show() override;
    virtual void hide() override;
    virtual bool isVisible() const override;

    // Window overrides
    virtual void Paint(vcl::RenderContext& rRenderContext, const tools::Rectangle& rRect) override;
    virtual void Resize() override;
    virtual void MouseMove(const MouseEvent& rMEvt) override;
    virtual void MouseButtonDown(const MouseEvent& rMEvt) override;
    virtual void MouseButtonUp(const MouseEvent& rMEvt) override;
    virtual void KeyInput(const KeyEvent& rKEvt) override;
    virtual void KeyUp(const KeyEvent& rKEvt) override;
    virtual void Command(const CommandEvent& rCEvt) override;

private:
    ViewObjectContactOfSdrMediaObj& mrViewObjectContactOfSdrMediaObj;
    VclPtr<vcl::AudioPlayerControl> mpAudioPlayerControl;
    OUString m_sURL;
    OUString m_sReferer;
};

} // namespace sdr::contact

#endif // INCLUDED_SVX_SOURCE_SDR_CONTACT_SDRAUDIOPLAYERWINDOW_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */ 