// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "PostCategory.h"
#include "SoundEffect.h"
#include "gameTypes/MapCoordinates.h"
#include "gameTypes/PactTypes.h"
#include <string>

struct BasePlayerInfo;
class ITexture;

class PostMsg
{
public:
    /// Create a simple message
    PostMsg(unsigned sendFrame, std::string text, PostCategory cat, const MapPoint& pt,
            SoundEffect soundEffect = SoundEffect::Pidgeon);
    PostMsg(unsigned sendFrame, std::string text, PostCategory cat, SoundEffect soundEffect = SoundEffect::Pidgeon);
    /// Response to a diplomacy question. Last parameter states if the pact was accepted(true) or canceled(false)
    PostMsg(unsigned sendFrame, PactType pt, const BasePlayerInfo& otherPlayer, bool acceptedOrCanceled,
            SoundEffect soundEffect = SoundEffect::Pidgeon);
    virtual ~PostMsg() = default;

    unsigned getSendFrame() const { return sendFrame_; }
    const std::string& getText() const { return text_; }
    PostCategory getCategory() const { return cat_; }
    /// Get position related to this message (optional, defaults to invalid)
    MapPoint getPos() const { return pt_; }
    SoundEffect getSoundEffect() const { return soundEffect_; }
    /// Get Associated image or nullptr if none exists
    virtual ITexture* getImage() const { return nullptr; }

protected:
    void setText(const std::string& text) { text_ = text; }

private:
    unsigned sendFrame_;
    std::string text_;
    PostCategory cat_;
    MapPoint pt_;
    SoundEffect soundEffect_;
};
