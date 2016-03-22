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

#ifndef LuaPlayerBase_h__
#define LuaPlayerBase_h__

#include "gameData/NationConsts.h"
#include "gameData/PlayerConsts.h"
#include <kaguya/kaguya.hpp>

class GamePlayerInfo;

class LuaPlayerBase
{
protected:
    LuaPlayerBase(){}
    virtual ~LuaPlayerBase(){}

    virtual const GamePlayerInfo& GetPlayer() const = 0;
public:
    static void Register(kaguya::State& state);

    std::string GetName() const;
    Nation GetNation() const;
    Team GetTeam() const;
    unsigned GetColor() const;
    bool IsHuman() const;
    bool IsAI() const;
    bool IsClosed() const;
    bool IsFree() const;
    int GetAILevel() const;
};

#endif // LuaPlayerBase_h__
