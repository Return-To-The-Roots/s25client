// $Id: dskEndStatistics.cpp 
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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "defines.h"
#include "dskEndStatistics.h"
#include "Loader.h"

#include "WindowManager.h"

#include "dskMainMenu.h"

#include "controls/ctrlStatisticTable.h"

#include "ingameWindows/iwAddons.h"
#include "ingameWindows/iwTextfile.h"
#include "ingameWindows/iwMsgbox.h"



#include <iostream>

// TOOD: remove
#include "Random.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


namespace {
    void addDummyNames(ctrlStatisticTable *table)
    {
        unsigned num_players = 8;
        std::string names_arr[8] = {"Player 1", "Player 2", "Player 3", "Player 4", "Player 5", "Player 6", "Player 7", "Player 8"};
        std::vector<EndStatisticData::StatisticPlayerInfo> player_infos;
        for (unsigned i = 0; i < num_players; ++i)
            player_infos.push_back(EndStatisticData::StatisticPlayerInfo(names_arr[i], COLORS[i], Team(0), Nation(0)));

        table->AddPlayerInfos(player_infos);
    }

    void createDummyData(EndStatisticData *data)
    {
        Random rand;
        rand.Init(0);

        for (unsigned i = 0; i < 8; ++i)
        {
            data->SetValue(EndStatisticData::MIL_PRODUCED_SOLDIERS, i, rand.Rand("", 0, 0, 200));
            data->SetValue(EndStatisticData::MIL_PRODUCED_GENERALS, i, rand.Rand("", 0, 0, 50));
            data->SetValue(EndStatisticData::MIL_ATTACKS, i, rand.Rand("", 0, 0, 200));
            data->SetValue(EndStatisticData::MIL_CONQUERED_BUILDINGS, i, rand.Rand("", 0, 0, 50));
            data->SetValue(EndStatisticData::MIL_KILLED_SOLDIERS, i, rand.Rand("", 0, 0, 100));
            data->SetValue(EndStatisticData::MIL_DUMMY, i, rand.Rand("", 0, 0, 42));

            data->SetValue(EndStatisticData::ECO_LAND_SIZE, i, rand.Rand("", 0, 0, 1000));
            data->SetValue(EndStatisticData::ECO_WAY_LENGTH, i, rand.Rand("", 0, 0, 500));
            data->SetValue(EndStatisticData::ECO_PRODUCED_WARES, i, rand.Rand("", 0, 0, 1000));
            data->SetValue(EndStatisticData::ECO_BUILT_SHIPS, i, rand.Rand("", 0, 0, 8));

            data->SetValue(EndStatisticData::BLD_MILITARY, i, rand.Rand("", 0, 0, 25));
            data->SetValue(EndStatisticData::BLD_CATAPULTS, i, rand.Rand("", 0, 0, 5));
            data->SetValue(EndStatisticData::BLD_MINES, i, rand.Rand("", 0, 0, 15));
            data->SetValue(EndStatisticData::BLD_SMITHS_AND_MELTERS, i, rand.Rand("", 0, 0, 8));
            data->SetValue(EndStatisticData::BLD_HARBORS, i, rand.Rand("", 0, 0, 3));
        }
    }
}

dskEndStatistics::dskEndStatistics(void) : Desktop(LOADER.GetImageN("setup013", 0)), data(8)
{
    // Back
    AddTextButton(0, 300, 550, 200, 22,   TC_RED1, _("Back"), NormalFont);

    // Title
    AddText(1, 400, 10, _("Final Score"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER, LargeFont);  

    // TODO get EndStatisticData from somewhere...
    createDummyData(&data);
    
    ShowOverview();
}

dskEndStatistics::~dskEndStatistics()
{

}



void dskEndStatistics::ShowOverview()
{
    _in_overview = true;
    unsigned num_players = 8;  // TODO

    std::vector<std::pair<std::string,bool> > main_categories;
    main_categories.push_back(std::make_pair(_("Player"), false));

    for (unsigned i = 0; i < data.GetCategories().size(); ++i)
        main_categories.push_back(std::make_pair(data.GetCategories()[i].title, true));

    main_categories.push_back(std::make_pair(_("Total"), false));

    ctrlStatisticTable *table = AddStatisticTable(2, 25, 50, 750, 500, main_categories, num_players);

    // TODO fix next two lines
    addDummyNames(table);


    for (unsigned cat = 0; cat <= EndStatisticData::MAX_CATEGORIES; ++cat)
    {
        std::vector<unsigned> points;
        for (unsigned i = 0; i < num_players; ++i)
            points.push_back(data.CalcPointsForCategory((EndStatisticData::CategoryIndex)cat, i));

        table->AddColumn(cat+1, points);
    }


    std::vector<unsigned> total_points;
    for (unsigned i = 0; i < num_players; ++i)
        total_points.push_back(data.CalcTotalPoints(i));

    table->AddColumn(EndStatisticData::MAX_CATEGORIES + 2, total_points);

}

void dskEndStatistics::ShowCategory(EndStatisticData::CategoryIndex cat)
{
    _in_overview = false;
    unsigned num_players = 8; // TODO

    std::vector<std::pair<std::string,bool> > categories;
    categories.push_back(std::make_pair(_("Player"), false));

    std::vector<EndStatisticData::ValueIndex> value_indices = data.GetValuesOfCategory(cat);

    for (unsigned i = 0; i < value_indices.size(); ++i)
        categories.push_back(std::make_pair(data.GetValue(value_indices[i]).name, false));

    ctrlStatisticTable *table = AddStatisticTable(2, 25, 50, 750, 500, categories, num_players);

    // TODO fix next two lines
    addDummyNames(table);

    for (unsigned i = 0; i < value_indices.size(); ++i)
    {
        table->AddColumn(i+1, data.GetValue(value_indices[i]).value_per_player);
    }
}


void dskEndStatistics::Msg_StatisticGroupChange(const unsigned int ctrl_id, const unsigned short selection)
{
    switch(selection)
    {
    case 1: // Military
        {
            DeleteCtrl(2);
            ShowCategory(EndStatisticData::MILITARY);
        }
        break;
    case 2: // Economy
        {
            DeleteCtrl(2);
            ShowCategory(EndStatisticData::ECONOMY);
        }
        break;
    case 3: // Buildings
        {
            DeleteCtrl(2);
            ShowCategory(EndStatisticData::BUILDINGS);
        }
        break;
    }
}

void dskEndStatistics::Msg_ButtonClick(const unsigned int ctrl_id)
{
    switch(ctrl_id)
    {
    case 0: // "Back"
        {
            if (_in_overview)
                WINDOWMANAGER.Switch(new dskMainMenu);
            else
            {
                DeleteCtrl(2);
                ShowOverview();
            }
        }
        break;
    }
}