// $Id: EndStatisticData.h
//
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
#ifndef ENDSTATISTICDATA_H_INCLUDED
#define ENDSTATISTICDATA_H_INCLUDED

#pragma once

#include <string>
#include <vector>

#include "gameData/GameConsts.h"
#include "gameData/NationConsts.h"


class SerializedGameData;
class GameClientPlayerList;

class EndStatisticData
{
public:

    /// A Value contains the name of a statistic value, a description and the data for this value for each player
    struct Value
    {
        std::string name;
        std::string description;
        std::vector<unsigned> value_per_player;

        Value () 
            : name(""), description(""), value_per_player(0) { }
        Value (const std::string &name, const std::string &description, unsigned num_players)
            : name(name), description(description), value_per_player(num_players, 0) { }
    };

    /// Contains an index for each value, for setter & getter methods
    enum ValueIndex
    {
        ECO_COINS,
        ECO_TOOLS,
        ECO_PRODUCED_WARES,
        ECO_USED_WARES,
        ECO_RESOURCE_SHORTAGE,
        ECO_SHIPS,

        INF_BUILDINGS,
        INF_WAYLENGTH,
        INF_FLAGS,
        INF_STOREHOUSES,
        INF_CATAPULTS,

        PROD_SETTLERS,
        PROD_BUILING_MATERIALS,
        PROD_FOOD,
        PROD_HEAVY_INDUSTRY,
        PROD_HEAVY_INDUSTRY_PRODUCTIVITY,

        MIL_TRAINED_SOLDIERS,
        MIL_TRAINED_GENERALS,
        MIL_KILLED_ENEMIES,
        MIL_LOST_SOLDIERS,
        MIL_DESTROYED_BUILDINGS,
        MIL_LOST_MILBUILDINGS,

        MISC_EXPLORED_MAP,
        MISC_ACTIONS,
        MISC_TRADED_WARES,
        MISC_ATTACKS,
        MISC_SPYTOWERS,
        MISC_CATAPULT_SHOTS,

        // ...
        MAX_VALUES = MISC_CATAPULT_SHOTS // always set to the last item when changing this enum
    };


    /// A MainCategory is shown in the overview of the statistic screen and has a clickable button
    struct MainCategory
    {
        std::string title;
        std::vector<ValueIndex> values;
    };

    enum CategoryIndex
    {
        ECONOMY,
        INFRASTRUCTURE,
        PRODUCTION,
        MILITARY,
        MISC,
        MAX_CATEGORIES = MISC // always set to the last item when changing this enum
    };


    /// Playerinfos, copied from GameClientPlayer before it is destroyed at the end of the game
    struct PlayerInfo
    {
        std::string name;
        unsigned color;
        Team team;
        Nation nation;

        PlayerInfo(const std::string& name, unsigned color, Team team, Nation nation) 
            : name(name), color(color), team(team), nation(nation) { }
    };

public:
    EndStatisticData(const GameClientPlayerList& players);
    ~EndStatisticData();

    void SetValue(ValueIndex value, unsigned char player_id, unsigned new_value);
    void IncreaseValue(ValueIndex value, unsigned char player_id, unsigned increment = 1);

    /// Some values are created from multiple other values. The calculation is done here, once
    void CalculateDependantValues();

    /// Calculates the points for a category/player
    unsigned CalcPointsForCategory(CategoryIndex cat, unsigned char player_id) const;

    /// Sum of the points of all categories for one player
    unsigned CalcTotalPoints(unsigned char player_id) const;


    const Value& GetValue(ValueIndex value) const;
    const std::vector<MainCategory>& GetCategories() const;

    const std::vector<ValueIndex>& GetValuesOfCategory(CategoryIndex cat) const;

    const std::vector<PlayerInfo>& GetPlayerInfos() const;

    /// Serialisieren
    void Serialize(SerializedGameData* sgd) const;
    /// Deserialisieren
    void Deserialize(SerializedGameData* sgd);

private:
    std::vector<Value> _values;
    std::vector<MainCategory> _main_categories;
    std::vector<PlayerInfo> _player_infos;

};

#endif ENDSTATISTICDATA_H_INCLUDED
