// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "LuaPlayerBase.h"
#include "lua/LuaTraits.h"
#include "lua/SafeEnum.h"
#include "gameTypes/BuildingType.h"
#include "gameTypes/GoodTypes.h"
#include "gameTypes/JobTypes.h"
#include "gameTypes/PactTypes.h"
#include <map>
#include <memory>
#include <utility>

namespace kaguya {
class State;
class VariadicArgType;
} // namespace kaguya
class GamePlayer;
class Game;

class LuaPlayer : public LuaPlayerBase
{
    Game& game;
    GamePlayer& player;

protected:
    const BasePlayerInfo& GetPlayer() const override;

public:
    LuaPlayer(Game& game, GamePlayer& player) : game(game), player(player) {}
    static void Register(kaguya::State& state);

    void EnableBuilding(lua::SafeEnum<BuildingType> bld, bool notify);
    void DisableBuilding(lua::SafeEnum<BuildingType> bld);
    void EnableAllBuildings();
    void DisableAllBuildings();
    void SetRestrictedArea(kaguya::VariadicArgType inPoints);
    bool IsInRestrictedArea(unsigned x, unsigned y) const;
    void ClearResources();
    bool AddWares(const std::map<lua::SafeEnum<GoodType>, unsigned>& wares);
    bool AddPeople(const std::map<lua::SafeEnum<Job>, unsigned>& people);
    unsigned GetNumBuildings(lua::SafeEnum<BuildingType> bld) const;
    unsigned GetNumBuildingSites(lua::SafeEnum<BuildingType> bld) const;
    unsigned GetNumWares(lua::SafeEnum<GoodType> ware) const;
    unsigned GetNumPeople(lua::SafeEnum<Job> job) const;
    bool AIConstructionOrder(unsigned x, unsigned y, lua::SafeEnum<BuildingType> bld);
    void ModifyHQ(bool isTent);
    bool IsDefeated() const;
    void Surrender(bool destroyBlds);
    std::pair<unsigned, unsigned> GetHQPos() const;
    bool IsAlly(unsigned char otherPlayerId);
    bool IsAttackable(unsigned char otherPlayerId);
    void SuggestPact(unsigned char otherPlayerId, lua::SafeEnum<PactType> pt, unsigned duration);
    void CancelPact(lua::SafeEnum<PactType> pt, unsigned char otherPlayerId);
};
