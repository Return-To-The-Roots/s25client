// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameTypes/MapCoordinates.h"
#include <memory>
#include <string>
class GamePlayer;
class GameWorldBase;

class Cheats
{
public:
    Cheats(GameWorldBase& world);

    bool areCheatsAllowed() const;

    void toggleCheatMode();
    bool isCheatModeOn() const { return isCheatModeOn_; }

    // Classic S2 cheats
    void toggleAllVisible();
    bool isAllVisible() const { return isAllVisible_; }

    void toggleAllBuildingsEnabled();
    bool areAllBuildingsEnabled() const { return areAllBuildingsEnabled_; }

    void toggleShowEnemyProductivityOverlay();
    bool shouldShowEnemyProductivityOverlay() const { return shouldShowEnemyProductivityOverlay_; }

    bool canPlaceCheatBuilding(const MapPoint& mp) const;
    void placeCheatBuilding(const MapPoint& mp, const GamePlayer& player);

    // RTTR cheats
    void toggleHumanAIPlayer();
    void armageddon() const;

private:
    void turnAllCheatsOff();

    bool isCheatModeOn_ = false;
    bool isAllVisible_ = false;
    bool areAllBuildingsEnabled_ = false;
    bool shouldShowEnemyProductivityOverlay_ = false;
    bool isHumanAIPlayer_ = false;
    GameWorldBase& world_;
};
