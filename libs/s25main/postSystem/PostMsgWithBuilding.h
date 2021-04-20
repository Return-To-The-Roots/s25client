// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "postSystem/PostMsg.h"
#include "gameTypes/BuildingType.h"
#include "gameTypes/Nation.h"

class ITexture;
class noBaseBuilding;

/// Message that is related to a building, showing the buildings image and a GoTo button
class PostMsgWithBuilding : public PostMsg
{
public:
    /// Creates the message.
    /// NOTE: Building is only valid during the call (do not store)
    PostMsgWithBuilding(unsigned sendFrame, const std::string& text, PostCategory cat, const noBaseBuilding& bld,
                        SoundEffect soundEffect = SoundEffect::Pidgeon);
    PostMsgWithBuilding(unsigned sendFrame, const std::string& text, PostCategory cat, BuildingType bld, Nation nation,
                        const MapPoint& pos = MapPoint::Invalid());

    ITexture* GetImage_() const override;

private:
    const BuildingType bldType;
    const Nation nation;
};
