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
#include "gameTypes/StatisticTypes.h"
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
    const BasePlayerInfo& getPlayer() const override;

public:
    LuaPlayer(Game& game, GamePlayer& player) : game(game), player(player) {}
    static void registerClasses(kaguya::State& state);

    void enableBuilding(lua::SafeEnum<BuildingType> bld, bool notify);
    void disableBuilding(lua::SafeEnum<BuildingType> bld);
    void enableAllBuildings();
    void disableAllBuildings();
    void setRestrictedArea(kaguya::VariadicArgType inPoints);
    bool isInRestrictedArea(unsigned x, unsigned y) const;
    void clearResources();
    bool addWares(const std::map<lua::SafeEnum<GoodType>, unsigned>& wares);
    bool addPeople(const std::map<lua::SafeEnum<Job>, unsigned>& people);
    unsigned getNumBuildings(lua::SafeEnum<BuildingType> bld) const;
    unsigned getNumBuildingSites(lua::SafeEnum<BuildingType> bld) const;
    unsigned getNumWares(lua::SafeEnum<GoodType> ware) const;
    unsigned getNumPeople(lua::SafeEnum<Job> job) const;
    unsigned getStatisticsValue(lua::SafeEnum<StatisticType> stat) const;
    bool aIConstructionOrder(unsigned x, unsigned y, lua::SafeEnum<BuildingType> bld);
    void modifyHQ(bool isTent);
    bool IsDefeated() const;
    void Surrender(bool destroyBlds);
    std::pair<unsigned, unsigned> getHQPos() const;
    bool isAlly(unsigned char otherPlayerId);
    bool isAttackable(unsigned char otherPlayerId);
    void SuggestPact(unsigned char otherPlayerId, lua::SafeEnum<PactType> pt, unsigned duration);
    void CancelPact(lua::SafeEnum<PactType> pt, unsigned char otherPlayerId);
};
