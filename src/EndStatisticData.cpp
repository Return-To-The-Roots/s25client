// $Id: EndStatisticData.cpp
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

#include "EndStatisticData.h"
#include "defines.h"
#include "mygettext.h" //?? TODO ok?

#include "SerializedGameData.h"
#include "GamePlayerList.h"

EndStatisticData::EndStatisticData(const GameClientPlayerList& players)
{
    unsigned number_of_players = players.getCount();

    for (unsigned i = 0; i < number_of_players; ++i)
    {
        GameClientPlayer gcp = players[i];
        _player_infos.push_back(PlayerInfo(gcp.name, gcp.color, gcp.team, gcp.nation)); 
    }

    _main_categories.resize(MAX_CATEGORIES + 1);
    _values.resize(MAX_VALUES + 1);

    _main_categories[ECONOMY].title = _("Economy");
    
    _values[ECO_COINS] = Value (_("Coins"), _(""), number_of_players);
    _main_categories[ECONOMY].values.push_back(ECO_COINS);

    _values[ECO_TOOLS] = Value (_("Tools"), _(""), number_of_players);
    _main_categories[ECONOMY].values.push_back(ECO_TOOLS);

    _values[ECO_PRODUCED_WARES] = Value (_("Produced Wares"), _(""), number_of_players);
    _main_categories[ECONOMY].values.push_back(ECO_PRODUCED_WARES);

    _values[ECO_USED_WARES] = Value (_("Consumed Wares"), _(""), number_of_players);
    _main_categories[ECONOMY].values.push_back(ECO_USED_WARES);

    _values[ECO_RESOURCE_SHORTAGE] = Value (_("Resource Shortages"), _(""), number_of_players);
    _main_categories[ECONOMY].values.push_back(ECO_RESOURCE_SHORTAGE);

    _values[ECO_SHIPS] = Value (_("Ships"), _(""), number_of_players);
    _main_categories[ECONOMY].values.push_back(ECO_SHIPS);


    _main_categories[INFRASTRUCTURE].title = _("Infrastructure");

    _values[INF_BUILDINGS] = Value (_("Buildings"), _(""), number_of_players);
    _main_categories[INFRASTRUCTURE].values.push_back(INF_BUILDINGS);

    _values[INF_WAYLENGTH] = Value (_("Way length"), _(""), number_of_players);
    _main_categories[INFRASTRUCTURE].values.push_back(INF_WAYLENGTH);

    _values[INF_FLAGS] = Value (_("Flags"), _(""), number_of_players);
    _main_categories[INFRASTRUCTURE].values.push_back(INF_FLAGS);

    _values[INF_STOREHOUSES] = Value (_("Storehouses"), _(""), number_of_players);
    _main_categories[INFRASTRUCTURE].values.push_back(INF_STOREHOUSES);

    _values[INF_CATAPULTS] = Value (_("Catapults"), _(""), number_of_players);
    _main_categories[INFRASTRUCTURE].values.push_back(INF_CATAPULTS);


    _main_categories[PRODUCTION].title = _("Production");

    _values[PROD_SETTLERS] = Value (_("Settlers"), _(""), number_of_players);
    _main_categories[PRODUCTION].values.push_back(PROD_SETTLERS);

    _values[PROD_BUILING_MATERIALS] = Value (_("Building Materials"), _(""), number_of_players);
    _main_categories[PRODUCTION].values.push_back(PROD_BUILING_MATERIALS);

    _values[PROD_FOOD] = Value (_("Food"), _(""), number_of_players);
    _main_categories[PRODUCTION].values.push_back(PROD_FOOD);

    _values[PROD_HEAVY_INDUSTRY] = Value (_("Heavy Industry"), _(""), number_of_players);
    _main_categories[PRODUCTION].values.push_back(PROD_HEAVY_INDUSTRY);

    _values[PROD_HEAVY_INDUSTRY_PRODUCTIVITY] = Value (_("Heavy Industry Productivity"), _(""), number_of_players);
    _main_categories[PRODUCTION].values.push_back(PROD_HEAVY_INDUSTRY_PRODUCTIVITY);

    
    _main_categories[MILITARY].title = _("Military");
    
    _values[MIL_TRAINED_SOLDIERS] = Value (_("Trained Soldiers"), _(""), number_of_players);
    _main_categories[MILITARY].values.push_back(MIL_TRAINED_SOLDIERS);

    _values[MIL_TRAINED_GENERALS] = Value (_("Trained Generals"), _(""), number_of_players);
    _main_categories[MILITARY].values.push_back(MIL_TRAINED_GENERALS);

    _values[MIL_KILLED_ENEMIES] = Value (_("Killed Enemies"), _(""), number_of_players);
    _main_categories[MILITARY].values.push_back(MIL_KILLED_ENEMIES);

    _values[MIL_LOST_SOLDIERS] = Value (_("Lost Soldiers"), _(""), number_of_players);
    _main_categories[MILITARY].values.push_back(MIL_LOST_SOLDIERS);

    _values[MIL_DESTROYED_BUILDINGS] = Value (_("Destroyed Buildings"), _(""), number_of_players);
    _main_categories[MILITARY].values.push_back(MIL_DESTROYED_BUILDINGS);

    _values[MIL_LOST_MILBUILDINGS] = Value (_("Lost Military Buildings"), _(""), number_of_players);
    _main_categories[MILITARY].values.push_back(MIL_LOST_MILBUILDINGS);


    _main_categories[MISC].title = _("Miscellaneous");
    
    _values[MISC_EXPLORED_MAP] = Value (_("Explored"), _(""), number_of_players);
    _main_categories[MISC].values.push_back(MISC_EXPLORED_MAP);

    _values[MISC_SPEED] = Value (_("Player Speed"), _(""), number_of_players);
    _main_categories[MISC].values.push_back(MISC_SPEED);

    _values[MISC_TRADED_WARES] = Value (_("Traded Wares"), _(""), number_of_players);
    _main_categories[MISC].values.push_back(MISC_TRADED_WARES);

    _values[MISC_ATTACKS] = Value (_("Attacks"), _(""), number_of_players);
    _main_categories[MISC].values.push_back(MISC_ATTACKS);

    _values[MISC_SPYTOWERS] = Value (_("Spy Towers"), _(""), number_of_players);
    _main_categories[MISC].values.push_back(MISC_SPYTOWERS);

    _values[MISC_CATAPULT_SHOTS] = Value (_("Catapult Shots"), _(""), number_of_players);
    _main_categories[MISC].values.push_back(MISC_CATAPULT_SHOTS);
}


EndStatisticData::~EndStatisticData()
{

}

void EndStatisticData::SetValue(ValueIndex value, unsigned char player_id, unsigned new_value)
{
    _values[value].value_per_player[player_id] = new_value;
}

void EndStatisticData::IncreaseValue(ValueIndex value, unsigned char player_id, unsigned increment)
{
    _values[value].value_per_player[player_id] += increment;
}

const EndStatisticData::Value& EndStatisticData::GetValue(ValueIndex value) const
{
    return _values[value];
}

const std::vector<EndStatisticData::MainCategory>& EndStatisticData::GetCategories() const
{
    return _main_categories;
}

const std::vector<EndStatisticData::ValueIndex>& EndStatisticData::GetValuesOfCategory(CategoryIndex cat) const
{
    return _main_categories[cat].values;
}

void EndStatisticData::CalculateDependantValues()
{
}

unsigned EndStatisticData::CalcPointsForCategory(CategoryIndex cat, unsigned char player_id) const
{
    unsigned points = 0;

    for (unsigned i = 0; i < _main_categories[cat].values.size(); ++i)
    {
        ValueIndex vi = _main_categories[cat].values[i];
        points += _values[vi].value_per_player[player_id];
    }
    //switch(cat)
    //{
    //case MILITARY:
    //    points +=   _values[MIL_PRODUCED_SOLDIERS].value_per_player[player_id]      * 10;
    //    points +=   _values[MIL_PRODUCED_GENERALS].value_per_player[player_id]      * 20;
    //    points +=   _values[MIL_ATTACKS].value_per_player[player_id]                * 5;
    //    points +=   _values[MIL_CONQUERED_BUILDINGS].value_per_player[player_id]    * 5;
    //    points +=   _values[MIL_KILLED_SOLDIERS].value_per_player[player_id]        * 5;
    //    return points;

    //case ECONOMY:
    //    points +=   _values[ECO_LAND_SIZE].value_per_player[player_id]              * 10;
    //    points +=   _values[ECO_WAY_LENGTH].value_per_player[player_id]             * 2;
    //    points +=   _values[ECO_PRODUCED_WARES].value_per_player[player_id]         * 1;
    //    points +=   _values[ECO_BUILT_SHIPS].value_per_player[player_id]            * 5;
    //    return points;

    //case BUILDINGS:
    //    points +=   _values[BLD_MILITARY].value_per_player[player_id]               * 5;
    //    points +=   _values[BLD_CATAPULTS].value_per_player[player_id]              * 5;
    //    points +=   _values[BLD_MINES].value_per_player[player_id]                  * 10;
    //    points +=   _values[BLD_SMITHS_AND_MELTERS].value_per_player[player_id]     * 20;
    //    points +=   _values[BLD_HARBORS].value_per_player[player_id]                * 10;
    //    return points;
    //}
    return points;
}

unsigned EndStatisticData::CalcTotalPoints(unsigned char player_id) const
{
    unsigned points = 0;
    for (unsigned i = 0; i < MAX_CATEGORIES; ++i)
        points += CalcPointsForCategory(CategoryIndex(i), player_id);

    return points;
}

const std::vector<EndStatisticData::PlayerInfo>& EndStatisticData::GetPlayerInfos() const
{
    return _player_infos;
}

void EndStatisticData::Serialize(SerializedGameData* sgd) const
{
    // Not yet done, to avoid multiple savegame format changes
}

void EndStatisticData::Deserialize(SerializedGameData* sgd)
{
    // Not yet done, to avoid multiple savegame format changes
}


