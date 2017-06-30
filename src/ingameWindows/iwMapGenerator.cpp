// Copyright (c) 2005-2010 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Siedler II.5 RTTR.
//
// Siedler II.5 RTTR is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Siedler II.5 RTTR is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Siedler II.5 RTTR. If not, see <http://www.gnu.org/licenses/>.

#include "defines.h" // IWYU pragma: keep
#include "iwMapGenerator.h"

#include "Loader.h"
#include "controls/ctrlOptionGroup.h"
#include "controls/ctrlComboBox.h"
#include "controls/ctrlProgress.h"

#include "gameData/const_gui_ids.h"
#include "gameData/MaxPlayers.h"
#include "helpers/containerUtils.h"
#include "libutil/src/colors.h"

#include <boost/format.hpp>
#include <string>

enum
{
    CTRL_LAST_ID = 7, // last UI control ID used before enum controls
    CTRL_PLAYER_NUMBER,
    CTRL_MAP_STYLE,
    CTRL_MAP_SIZE,
    CTRL_PLAYER_RADIUS,
    CTRL_MAP_TYPE,
    CTRL_RATIO_GOLD,
    CTRL_RATIO_IRON,
    CTRL_RATIO_COAL,
    CTRL_RATIO_GRANITE
};

iwMapGenerator::iwMapGenerator(MapSettings& settings) : IngameWindow(CGI_MAP_GENERATOR,
                                                IngameWindow::posLastOrCenter,
                                                250, 400, _("Map Generator"),
                                                LOADER.GetImageN("resource", 41), true, false), mapSettings(settings)
{
    AddTextButton(0,  20, 360, 100, 20, TC_RED2, _("Back"), NormalFont);
    AddTextButton(1, 130, 360, 100, 20, TC_GREEN2, _("Apply"), NormalFont);

    ctrlComboBox* combo = AddComboBox(CTRL_PLAYER_NUMBER, 20, 30, 210, 20, TC_GREY, NormalFont, 100);
    for (unsigned n = 2; n < MAX_PLAYERS; n++)
    {
        combo->AddString(boost::str(boost::format(_("%1% players")) % n));
    }
    
    combo = AddComboBox(CTRL_MAP_STYLE, 20, 60, 210, 20, TC_GREY, NormalFont, 100);
    combo->AddString(_("Islands"));
    combo->AddString(_("Continent"));
    combo->AddString(_("Greenland"));
    combo->AddString(_("Migration"));
    combo->AddString(_("Riverland"));
    combo->AddString(_("Ringland"));
    combo->AddString(_("Random"));

    combo = AddComboBox(CTRL_MAP_SIZE, 20, 90, 210, 20, TC_GREY, NormalFont, 100);
    combo->AddString("64 x 64");
    combo->AddString("128 x 128");
    combo->AddString("256 x 256");
    combo->AddString("512 x 512");
    combo->AddString("1024 x 1024");

    AddText(2, 20, 120, _("Player Distribution"), COLOR_YELLOW, 0, NormalFont);
    combo = AddComboBox(CTRL_PLAYER_RADIUS, 20, 140, 210, 20, TC_GREY, NormalFont, 100);
    combo->AddString(_("Very Close"));
    combo->AddString(_("Close"));
    combo->AddString(_("Medium"));
    combo->AddString(_("Far"));
    combo->AddString(_("Very Far"));

    AddText(3, 20, 170, _("Landscape"), COLOR_YELLOW, 0, NormalFont);
    combo = AddComboBox(CTRL_MAP_TYPE, 20, 190, 210, 20, TC_GREY, NormalFont, 100);
    combo->AddString(_("Greenland"));
    combo->AddString(_("Winterworld"));
    combo->AddString(_("Wasteland"));
    
    AddText(4, 20, 225, _("Gold:"), COLOR_YELLOW, 0, NormalFont);
    AddProgress(CTRL_RATIO_GOLD, 100, 220, 130, 20, TC_GREY, 139, 138, 100);
    AddText(5, 20, 255, _("Iron:"), COLOR_YELLOW, 0, NormalFont);
    AddProgress(CTRL_RATIO_IRON, 100, 250, 130, 20, TC_GREY, 139, 138, 100);
    AddText(6, 20, 285, _("Coal:"), COLOR_YELLOW, 0, NormalFont);
    AddProgress(CTRL_RATIO_COAL, 100, 280, 130, 20, TC_GREY, 139, 138, 100);
    AddText(7, 20, 315, _("Granite:"), COLOR_YELLOW, 0, NormalFont);
    AddProgress(CTRL_RATIO_GRANITE, 100, 310, 130, 20, TC_GREY, 139, 138, 100);
    
    Reset();
}

iwMapGenerator::~iwMapGenerator()
{
}

void iwMapGenerator::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        default:
            break;

        case 0: // back
        {
            Close();
        } break;
            
        case 1: // apply
        {
            Apply();
            Close();
        } break;
    }
}

void iwMapGenerator::Apply()
{
    mapSettings.players = GetCtrl<ctrlComboBox>(CTRL_PLAYER_NUMBER)->GetSelection() + 2;
    mapSettings.ratioGold = GetCtrl<ctrlProgress>(CTRL_RATIO_GOLD)->GetPosition();
    mapSettings.ratioIron = GetCtrl<ctrlProgress>(CTRL_RATIO_IRON)->GetPosition();
    mapSettings.ratioCoal = GetCtrl<ctrlProgress>(CTRL_RATIO_COAL)->GetPosition();
    mapSettings.ratioGranite = GetCtrl<ctrlProgress>(CTRL_RATIO_GRANITE)->GetPosition();
    
    switch (GetCtrl<ctrlComboBox>(CTRL_MAP_STYLE)->GetSelection())
    {
        case 0:
            mapSettings.style = MS_Islands;
            break;
        case 1:
            mapSettings.style = MS_Continent;
            break;
        case 2:
            mapSettings.style = MS_Greenland;
            break;
        case 3:
            mapSettings.style = MS_Migration;
            break;
        case 4:
            mapSettings.style = MS_Riverland;
            break;
        case 5:
            mapSettings.style = MS_Ringland;
            break;
        case 6:
            mapSettings.style = MS_Random;
            break;
        default:
            break;
    }
    switch (GetCtrl<ctrlComboBox>(CTRL_MAP_SIZE)->GetSelection())
    {
        case 0:
            mapSettings.width = 64;
            mapSettings.height = 64;
            break;
        case 1:
            mapSettings.width = 128;
            mapSettings.height = 128;
            break;
        case 2:
            mapSettings.width = 256;
            mapSettings.height = 256;
            break;
        case 3:
            mapSettings.width = 512;
            mapSettings.height = 512;
            break;
        case 4:
            mapSettings.width = 1024;
            mapSettings.height = 1024;
            break;
        default:
            break;
    }
    switch (GetCtrl<ctrlComboBox>(CTRL_PLAYER_RADIUS)->GetSelection())
    {
        case 0:
            mapSettings.minPlayerRadius = 0.19;
            mapSettings.maxPlayerRadius = 0.3;
            break;
        case 1:
            mapSettings.minPlayerRadius = 0.29;
            mapSettings.maxPlayerRadius = 0.5;
            break;
        case 2:
            mapSettings.minPlayerRadius = 0.39;
            mapSettings.maxPlayerRadius = 0.59;
            break;
        case 3:
            mapSettings.minPlayerRadius = 0.49;
            mapSettings.maxPlayerRadius = 0.61;
            break;
        case 4:
            mapSettings.minPlayerRadius = 0.71;
            mapSettings.maxPlayerRadius = 0.72;
            break;
        default:
            break;
    }
    switch (GetCtrl<ctrlComboBox>(CTRL_MAP_TYPE)->GetSelection())
    {
        case 0:
            mapSettings.type = LT_GREENLAND;
            break;
        case 1:
            mapSettings.type = LT_WINTERWORLD;
            break;
        case 2:
            mapSettings.type = LT_WASTELAND;
            break;
        default:
            break;
    }
}

void iwMapGenerator::Reset()
{
    ctrlComboBox* combo = GetCtrl<ctrlComboBox>(CTRL_PLAYER_NUMBER);
    const int16_t playersSelection = mapSettings.players - 2;
    if (playersSelection >= 0 && playersSelection < MAX_PLAYERS - 2)
    {
        combo->SetSelection(playersSelection);
    }
    
    GetCtrl<ctrlProgress>(CTRL_RATIO_GOLD)->SetPosition(mapSettings.ratioGold);
    GetCtrl<ctrlProgress>(CTRL_RATIO_IRON)->SetPosition(mapSettings.ratioIron);
    GetCtrl<ctrlProgress>(CTRL_RATIO_COAL)->SetPosition(mapSettings.ratioCoal);
    GetCtrl<ctrlProgress>(CTRL_RATIO_GRANITE)->SetPosition(mapSettings.ratioGranite);
    
    combo = GetCtrl<ctrlComboBox>(CTRL_MAP_STYLE);
    switch (mapSettings.style)
    {
        case MS_Islands:
            combo->SetSelection(0);
            break;
        case MS_Continent:
            combo->SetSelection(1);
            break;
        case MS_Greenland:
            combo->SetSelection(2);
            break;
        case MS_Migration:
            combo->SetSelection(3);
            break;
        case MS_Riverland:
            combo->SetSelection(4);
            break;
        case MS_Ringland:
            combo->SetSelection(5);
            break;
        case MS_Random:
            combo->SetSelection(6);
            break;
        default:
            break;
    }
    
    
    combo = GetCtrl<ctrlComboBox>(CTRL_MAP_SIZE);
    switch (mapSettings.width)
    {
        case 64:
            combo->SetSelection(0);
            break;
        case 128:
            combo->SetSelection(1);
            break;
        case 256:
            combo->SetSelection(2);
            break;
        case 512:
            combo->SetSelection(3);
            break;
        case 1024:
            combo->SetSelection(4);
            break;
        default:
            break;
    }
    
    combo = GetCtrl<ctrlComboBox>(CTRL_PLAYER_RADIUS);
    if (mapSettings.minPlayerRadius <= 0.2)
        combo->SetSelection(0);
    else if (mapSettings.minPlayerRadius <= 0.3)
        combo->SetSelection(1);
    else if (mapSettings.minPlayerRadius <= 0.4)
        combo->SetSelection(2);
    else if (mapSettings.minPlayerRadius <= 0.5)
        combo->SetSelection(3);
    else
        combo->SetSelection(4);
    
    combo = GetCtrl<ctrlComboBox>(CTRL_MAP_TYPE);
    switch (mapSettings.type)
    {
        case LT_GREENLAND:
            combo->SetSelection(0);
            break;
        case LT_WINTERWORLD:
            combo->SetSelection(1);
            break;
        case LT_WASTELAND:
            combo->SetSelection(2);
            break;
        default:
            break;
    }
}

