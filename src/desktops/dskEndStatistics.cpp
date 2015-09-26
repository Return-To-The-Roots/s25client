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

dskEndStatistics::dskEndStatistics(EndStatisticData *data) : Desktop(LOADER.GetImageN("setup013", 0)), _info_shown(false), data(data)
{
    assert(data != 0 && "no valid EndStatisticData supplied");
    
    // Back
    AddTextButton(0, 300, 550, 200, 22,   TC_RED1, _("Back"), NormalFont);

    // Title
    AddText(1, 400, 10, _("Final Score"), COLOR_YELLOW, glArchivItem_Font::DF_CENTER, LargeFont);  

    data->RemoveUnusedPlayerSlotsFromData();

    ShowOverview();

    // Temporary info 

}

dskEndStatistics::~dskEndStatistics()
{
    if (data)
        delete data;
}

void dskEndStatistics::ShowOverview()
{
    _in_overview = true;
    unsigned num_players = data->GetPlayerInfos().size(); 

    std::vector<std::pair<std::string,bool> > main_categories;
    main_categories.push_back(std::make_pair(_("Player"), false));

    for (unsigned i = 0; i < data->GetCategories().size(); ++i)
        main_categories.push_back(std::make_pair(data->GetCategories()[i].title, true));

    main_categories.push_back(std::make_pair(_("Total"), false));

    ctrlStatisticTable *table = AddStatisticTable(2, 25, 50, 750, 500, main_categories.size(), num_players);

    table->AddPlayerInfos(data->GetPlayerInfos());

    for (unsigned cat = 0; cat <= EndStatisticData::MAX_CATEGORIES; ++cat)
    {
        std::vector<unsigned> points;
        for (unsigned i = 0; i < num_players; ++i)
            points.push_back(data->CalcPointsForCategory((EndStatisticData::CategoryIndex)cat, i));

        table->AddColumn(cat+1, data->GetCategories()[cat].title, true, "", points);
    }


    std::vector<unsigned> total_points;
    for (unsigned i = 0; i < num_players; ++i)
        total_points.push_back(data->CalcTotalPoints(i));

    table->AddColumn(EndStatisticData::MAX_CATEGORIES + 2, _("Total"), false, "", total_points);

}

void dskEndStatistics::ShowCategory(EndStatisticData::CategoryIndex cat)
{
    _in_overview = false;
    unsigned num_players = data->GetPlayerInfos().size(); 

    std::vector<std::pair<std::string,bool> > categories;
    categories.push_back(std::make_pair(_("Player"), false));

    std::vector<EndStatisticData::ValueIndex> value_indices = data->GetValuesOfCategory(cat);

    for (unsigned i = 0; i < value_indices.size(); ++i)
        categories.push_back(std::make_pair(data->GetValue(value_indices[i]).name, false));

    ctrlStatisticTable *table = AddStatisticTable(2, 25, 50, 750, 500, value_indices.size() + 1, num_players);

    table->AddPlayerInfos(data->GetPlayerInfos());

    for (unsigned i = 0; i < value_indices.size(); ++i)
    {
        table->AddColumn(i+1, 
            data->GetValue(value_indices[i]).name, 
            false, 
            data->GetValue(value_indices[i]).description, 
            data->GetValue(value_indices[i]).value_per_player);
    }
}


void dskEndStatistics::Msg_PaintAfter()
{
    if (!_info_shown)
    {
        WINDOWMANAGER.Show(new iwMsgbox(_("New Feature!"), _("Currently, the data is not saved in a savegame, so the results only valid since the last loading of the game. This will be fixed in the future. Feel free to give feedback in the RTTR forum or via GitHub."), 
                                                         this, MSB_OK, MSB_EXCLAMATIONGREEN, 1));
        _info_shown = true;
    }
}

void dskEndStatistics::Msg_StatisticGroupChange(const unsigned int ctrl_id, const unsigned short selection)
{


    DeleteCtrl(2);
    ShowCategory(EndStatisticData::CategoryIndex(selection - 1));
}

void dskEndStatistics::Msg_ButtonClick(const unsigned int ctrl_id)
{
    switch(ctrl_id)
    {
    case 0: // "Back"
        {
            if (_in_overview)
                WINDOWMANAGER.Switch(new dskMainMenu);  // TODO might want to switch to lobby
            else
            {
                DeleteCtrl(2);
                ShowOverview();
            }
        }
        break;
    }
}