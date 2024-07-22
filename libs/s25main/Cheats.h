// Copyright (C) 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameTypes/MapCoordinates.h"
#include <memory>
#include <string>
#include <unordered_set>

class CheatCommandTracker;
class GamePlayer;
class GameWorldBase;
struct KeyEvent;

class Cheats
{
public:
    Cheats(GameWorldBase& world);
    ~Cheats(); // = default - for unique_ptr

    void trackKeyEvent(const KeyEvent& ke);
    void trackChatCommand(const std::string& cmd);

    void toggleCheatMode();
    bool isCheatModeOn() const { return isCheatModeOn_; }

    // Classic S2 cheats
    void toggleAllVisible();
    bool isAllVisible() const { return isAllVisible_; }

    bool canPlaceCheatBuilding(const MapPoint& mp) const;
    void placeCheatBuilding(const MapPoint& mp, const GamePlayer& player);

    // RTTR cheats
    void toggleHumanAIPlayer();

    void armageddon();

    enum class ResourceRevealMode
    {
        Nothing,
        Ores,
        Fish,
        Water
    };
    ResourceRevealMode getResourceRevealMode() const;
    void toggleResourceRevealMode();

    using PlayerIDSet = std::unordered_set<unsigned>;
    void destroyBuildings(const PlayerIDSet& playerIds);
    void destroyAllAIBuildings();

private:
    bool canCheatModeBeOn() const;

    std::unique_ptr<CheatCommandTracker> cheatCmdTracker_;
    bool isCheatModeOn_ = false;
    bool isAllVisible_ = false;
    GameWorldBase& world_;
    ResourceRevealMode resourceRevealMode_ = ResourceRevealMode::Nothing;
};
