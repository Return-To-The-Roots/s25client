// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef GameLobby_h__
#define GameLobby_h__

#include "GlobalGameSettings.h"
#include <vector>

struct JoinPlayerInfo;

/// Model of the game lobby (preparation when starting a map)
/// Not to be confused with the lobby (List of online games)
class GameLobby
{
public:
    GameLobby(unsigned numPlayers);
    ~GameLobby();

    JoinPlayerInfo& GetPlayer(unsigned playerId);
    const JoinPlayerInfo& GetPlayer(unsigned playerId) const;
    const std::vector<JoinPlayerInfo>& GetPlayers() const { return players; }
    unsigned GetPlayerCount() const;

    GlobalGameSettings& GetSettings() { return ggs; }
    const GlobalGameSettings& GetSettings() const { return ggs; }
private:
    std::vector<JoinPlayerInfo> players;
    GlobalGameSettings ggs;
};

#endif // GameLobby_h__
