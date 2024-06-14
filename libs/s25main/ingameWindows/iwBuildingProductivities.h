// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "IngameWindow.h"
#include "helpers/EnumArray.h"
#include "gameTypes/BuildingType.h"
#include <vector>

class GamePlayer;

class iwBuildingProductivities : public IngameWindow
{
    const GamePlayer& player;
    helpers::EnumArray<uint16_t, BuildingType> percents;

public:
    iwBuildingProductivities(const GamePlayer& player);

private:
    void UpdatePercents();
    void Msg_PaintAfter() override;

    void removeUnusedBuildings();
    void setBuildingOrder();
    /// Icons shown (in this order)
    std::vector<BuildingType> icons;
};
