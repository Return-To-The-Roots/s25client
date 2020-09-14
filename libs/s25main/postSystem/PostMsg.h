// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

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
    PostMsg(unsigned sendFrame, std::string text, PostCategory cat, const MapPoint& pt, SoundEffect soundEffect = SoundEffect::Pidgeon);
    PostMsg(unsigned sendFrame, std::string text, PostCategory cat, SoundEffect soundEffect = SoundEffect::Pidgeon);
    /// Response to a diplomacy question. Last parameter states if the pact was accepted(true) or canceled(false)
    PostMsg(unsigned sendFrame, PactType pt, const BasePlayerInfo& otherPlayer, bool acceptedOrCanceled,
            SoundEffect soundEffect = SoundEffect::Pidgeon);
    virtual ~PostMsg() = default;

    unsigned GetSendFrame() const { return sendFrame_; }
    const std::string& GetText() const { return text_; }
    PostCategory GetCategory() const { return cat_; }
    /// Get position related to this message (optional, defaults to invalid)
    MapPoint GetPos() const { return pt_; }
    SoundEffect GetSoundEffect() const { return soundEffect_; }
    /// Get Associated image or nullptr if none exists
    virtual ITexture* GetImage_() const { return nullptr; }

protected:
    void SetText(const std::string& text) { text_ = text; }

private:
    unsigned sendFrame_;
    std::string text_;
    PostCategory cat_;
    MapPoint pt_;
    SoundEffect soundEffect_;
};
