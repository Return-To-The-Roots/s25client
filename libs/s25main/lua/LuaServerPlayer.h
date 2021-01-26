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

#include "LuaPlayerBase.h"
#include "SafeEnum.h"
#include "gameTypes/Nation.h"

struct JoinPlayerInfo;
class IGameLobbyController;

class LuaServerPlayer : public LuaPlayerBase
{
    IGameLobbyController& lobbyServerController_;
    const unsigned playerId;
    JoinPlayerInfo& player;

protected:
    const BasePlayerInfo& GetPlayer() const override;

public:
    LuaServerPlayer(IGameLobbyController& lobbyServerController, unsigned playerId);
    static void Register(kaguya::State& state);

    void SetNation(lua::SafeEnum<Nation> nat);
    void SetTeam(lua::SafeEnum<Team> team);
    void SetColor(unsigned colorOrIdx);
    void Close();
    void SetAI(unsigned level);
    void SetName(const std::string& name);
};
