/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */
/*
 * This file is part of the LibreOffice project.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#ifndef INCLUDED_SVX_SOURCE_SDR_CONTACT_SDRMEDIAWINDOWINTERFACE_HXX
#define INCLUDED_SVX_SOURCE_SDR_CONTACT_SDRMEDIAWINDOWINTERFACE_HXX

#include <tools/gen.hxx>
#include <avmedia/mediaitem.hxx>

namespace sdr::contact {

/**
 * Common interface for media windows used by ViewObjectContactOfSdrMediaObj.
 * This allows both SdrMediaWindow and SdrAudioPlayerWindow to be used interchangeably.
 */
class SdrMediaWindowInterface
{
public:
    virtual ~SdrMediaWindowInterface() = default;

    // Core media functionality
    virtual Size getPreferredSize() const = 0;
    virtual void updateMediaItem(::avmedia::MediaItem& rItem) const = 0;
    virtual void executeMediaItem(const ::avmedia::MediaItem& rItem) = 0;
    virtual void setPosSize(const tools::Rectangle& rRect) = 0;
    
    // Visibility control
    virtual void show() = 0;
    virtual void hide() = 0;
    virtual bool isVisible() const = 0;
};

} // namespace sdr::contact

#endif // INCLUDED_SVX_SOURCE_SDR_CONTACT_SDRMEDIAWINDOWINTERFACE_HXX

/* vim:set shiftwidth=4 softtabstop=4 expandtab: */ 