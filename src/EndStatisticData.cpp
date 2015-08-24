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

EndStatisticData::EndStatisticData(unsigned number_of_players)
{
    _main_categories.resize(MAX_CATEGORIES + 1);
    _values.resize(MAX_VALUES + 1);

    _main_categories[MILITARY].title = _("Military");
    
    _values[MIL_PRODUCED_SOLDIERS] = Value (_("Trained soldiers"), _("Total number of trained soldiers."), number_of_players);
    _main_categories[MILITARY].values.push_back(MIL_PRODUCED_SOLDIERS);

    _values[MIL_PRODUCED_GENERALS] = Value (_("Trained generals"), _("Total number of trained generals."), number_of_players);
    _main_categories[MILITARY].values.push_back(MIL_PRODUCED_GENERALS);

    _values[MIL_ATTACKS] = Value (_("Attacks"), _("Total number of attacks ordered."), number_of_players);
    _main_categories[MILITARY].values.push_back(MIL_ATTACKS);

    _values[MIL_CONQUERED_BUILDINGS] = Value (_("Conquered buildings"), _("Total number of conquered buildings."), number_of_players);
    _main_categories[MILITARY].values.push_back(MIL_CONQUERED_BUILDINGS);

    _values[MIL_KILLED_SOLDIERS] = Value (_("Killed soldiers"), _("Total number of killed soldiers."), number_of_players);
    _main_categories[MILITARY].values.push_back(MIL_KILLED_SOLDIERS);

    _values[MIL_DUMMY] = Value (_("Dummy"), _("Total number of killed soldiers."), number_of_players);
    _main_categories[MILITARY].values.push_back(MIL_DUMMY);


    _main_categories[ECONOMY].title = _("Economy");

    _values[ECO_LAND_SIZE] = Value (_("Land size"), _("Maximal size of land."), number_of_players);
    _main_categories[ECONOMY].values.push_back(ECO_LAND_SIZE);

    _values[ECO_WAY_LENGTH] = Value (_("Way length"), _("Maximal sum of all way lengths."), number_of_players);
    _main_categories[ECONOMY].values.push_back(ECO_WAY_LENGTH);

    _values[ECO_PRODUCED_WARES] = Value (_("Produced wares"), _("Total number of produced wares."), number_of_players);
    _main_categories[ECONOMY].values.push_back(ECO_PRODUCED_WARES);

    _values[ECO_BUILT_SHIPS] = Value (_("Ships"), _("Total number of built ships."), number_of_players);
    _main_categories[ECONOMY].values.push_back(ECO_BUILT_SHIPS);


    _main_categories[BUILDINGS].title = _("Buildings");

    _values[BLD_MILITARY] = Value (_("Military"), _("Total constructed number of military buildings."), number_of_players);
    _main_categories[BUILDINGS].values.push_back(BLD_MILITARY);

    _values[BLD_CATAPULTS] = Value (_("Catapults"), _("Number of catapults at the end of the game."), number_of_players);
    _main_categories[BUILDINGS].values.push_back(BLD_CATAPULTS);

    _values[BLD_MINES] = Value (_("Mines"), _("Number of mines at the end of the game."), number_of_players);
    _main_categories[BUILDINGS].values.push_back(BLD_MINES);

    _values[BLD_SMITHS_AND_MELTERS] = Value (_("Smiths & Melters"), _("Number of smiths and melters at the end of the game."), number_of_players);
    _main_categories[BUILDINGS].values.push_back(BLD_SMITHS_AND_MELTERS);

    _values[BLD_HARBORS] = Value (_("Habors"), _("Number of harbors at the end of the game."), number_of_players);
    _main_categories[BUILDINGS].values.push_back(BLD_HARBORS);


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
    switch(cat)
    {
    case MILITARY:
        points +=   _values[MIL_PRODUCED_SOLDIERS].value_per_player[player_id]      * 10;
        points +=   _values[MIL_PRODUCED_GENERALS].value_per_player[player_id]      * 20;
        points +=   _values[MIL_ATTACKS].value_per_player[player_id]                * 5;
        points +=   _values[MIL_CONQUERED_BUILDINGS].value_per_player[player_id]    * 5;
        points +=   _values[MIL_KILLED_SOLDIERS].value_per_player[player_id]        * 5;
        return points;

    case ECONOMY:
        points +=   _values[ECO_LAND_SIZE].value_per_player[player_id]              * 10;
        points +=   _values[ECO_WAY_LENGTH].value_per_player[player_id]             * 2;
        points +=   _values[ECO_PRODUCED_WARES].value_per_player[player_id]         * 1;
        points +=   _values[ECO_BUILT_SHIPS].value_per_player[player_id]            * 5;
        return points;

    case BUILDINGS:
        points +=   _values[BLD_MILITARY].value_per_player[player_id]               * 5;
        points +=   _values[BLD_CATAPULTS].value_per_player[player_id]              * 5;
        points +=   _values[BLD_MINES].value_per_player[player_id]                  * 10;
        points +=   _values[BLD_SMITHS_AND_MELTERS].value_per_player[player_id]     * 20;
        points +=   _values[BLD_HARBORS].value_per_player[player_id]                * 10;
        return points;
    }
    return 0;
}

unsigned EndStatisticData::CalcTotalPoints(unsigned char player_id) const
{
    unsigned points = 0;
    for (unsigned i = 0; i < MAX_CATEGORIES; ++i)
        points += CalcPointsForCategory(CategoryIndex(i), player_id);

    return points;
}


void EndStatisticData::Serialize(SerializedGameData* sgd) const
{
    // Not yet done, to avoid multiple savegame format changes
}

void EndStatisticData::Deserialize(SerializedGameData* sgd)
{
    // Not yet done, to avoid multiple savegame format changes
}


