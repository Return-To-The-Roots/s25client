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

#ifndef LuaPlayer_h__
#define LuaPlayer_h__

#include "LuaPlayerBase.h"
#include "gameTypes/BuildingType.h"
#include "gameTypes/GoodTypes.h"
#include "gameTypes/JobTypes.h"
#include "gameTypes/PactTypes.h"
#include <kaguya/kaguya.hpp>
#include <map>
#include <memory>

class GamePlayer;
class Game;

class LuaPlayer : public LuaPlayerBase
{
    std::weak_ptr<Game> game;
    GamePlayer& player;

protected:
    const BasePlayerInfo& GetPlayer() const override;

public:
    LuaPlayer(std::weak_ptr<Game> game, GamePlayer& player) : game(game), player(player) {}
    static void Register(kaguya::State& state);

    void EnableBuilding(BuildingType bld, bool notify);
    void DisableBuilding(BuildingType bld);
    void EnableAllBuildings();
    void DisableAllBuildings();
    void SetRestrictedArea(kaguya::VariadicArgType inPoints);
    bool IsInRestrictedArea(unsigned x, unsigned y) const;
    void ClearResources();
    bool AddWares(const std::map<GoodType, unsigned>& wares);
    bool AddPeople(const std::map<Job, unsigned>& people);
    unsigned GetNumBuildings(BuildingType bld) const;
    unsigned GetNumBuildingSites(BuildingType bld) const;
    unsigned GetNumWares(GoodType ware) const;
    unsigned GetNumPeople(Job job) const;
    bool AIConstructionOrder(unsigned x, unsigned y, BuildingType bld);
    void ModifyHQ(bool isTent);
    bool IsDefeated() const;
    void Surrender(bool destroyBlds);
    std::tuple<unsigned, unsigned> GetHQPos() const;
    bool IsAlly(unsigned char otherPlayerId);
    bool IsAttackable(unsigned char otherPlayerId);
    void SuggestPact(unsigned char otherPlayerId, PactType pt, unsigned duration);
    void CancelPact(PactType pt, unsigned char otherPlayerId);
};

#endif // LuaPlayer_h__
