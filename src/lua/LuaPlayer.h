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

#ifndef LuaPlayer_h__
#define LuaPlayer_h__

#include "LuaPlayerBase.h"
#include "gameTypes/BuildingTypes.h"
#include "gameTypes/GoodTypes.h"
#include "gameTypes/JobTypes.h"
#include <kaguya/kaguya.hpp>
#include <map>

class GameClientPlayer;

class LuaPlayer: public LuaPlayerBase
{
    GameClientPlayer& player;
protected:
    const GamePlayerInfo& GetPlayer() const override;
public:
    LuaPlayer(GameClientPlayer& player): player(player){}
    static void Register(kaguya::State& state);

    void EnableBuilding(BuildingType bld, bool notify);
    void DisableBuilding(BuildingType bld);
    void EnableAllBuildings();
    void DisableAllBuildings();
    void SetRestrictedArea(kaguya::VariadicArgType points);
    void ClearResources();
    bool AddWares(const std::map<GoodType, unsigned>& wares);
    bool AddPeople(const std::map<Job, unsigned>& people);
    unsigned GetBuildingCount(BuildingType bld);
    unsigned GetWareCount(GoodType ware);
    unsigned GetPeopleCount(Job job);
    bool AIConstructionOrder(unsigned x, unsigned y, BuildingType bld);
    void ModifyHQ(bool isTent);
    kaguya::standard::tuple<unsigned, unsigned> GetHQPos();
};

#endif // LuaPlayer_h__
