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

#include "GlobalGameSettings.h"
#include <vector>

struct JoinPlayerInfo;

/// Model of the game lobby (preparation when starting a map)
/// Not to be confused with the lobby (List of online games)
class GameLobby
{
public:
    GameLobby(bool isSavegame, bool isHost, unsigned numPlayers);
    ~GameLobby();

    JoinPlayerInfo& getPlayer(unsigned playerId);
    const JoinPlayerInfo& getPlayer(unsigned playerId) const;
    const std::vector<JoinPlayerInfo>& getPlayers() const { return players_; }
    unsigned getNumPlayers() const;

    GlobalGameSettings& getSettings() { return ggs_; }
    const GlobalGameSettings& getSettings() const { return ggs_; }

    bool isSavegame() const { return isSavegame_; }
    bool isHost() const { return isHost_; }

private:
    /// Is this a savegame or a loaded game?
    bool isSavegame_;
    /// Is this for the host/game creator
    bool isHost_;
    std::vector<JoinPlayerInfo> players_;
    GlobalGameSettings ggs_;
};
