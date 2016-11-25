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

#include "gameData/const_gui_ids.h"
#include "helpers/containerUtils.h"
#include "libutil/src/colors.h"

#include <string>

iwMapGenerator::iwMapGenerator(MapSettings& settings) : IngameWindow(CGI_MAP_GENERATOR,
                                                IngameWindow::posLastOrCenter,
                                                300, 400, _("Map Generator"),
                                                LOADER.GetImageN("resource", 41), true, false), mapSettings(settings), tmpSettings(settings)
{
    AddTextButton(0,  40, 360, 100, 20, TC_RED2, _("Back"), NormalFont);
    AddTextButton(1, 160, 360, 100, 20, TC_GREEN2, _("Apply"), NormalFont);
    AddTextButton(2,  50,  50, 200, 20, TC_GREY, _("2 Players"), NormalFont);

    ctrlComboBox* combo = AddComboBox(3, 50, 80, 200, 20, TC_GREY, NormalFont, 100);
    combo->AddString("Islands");
    combo->AddString("Continent");
    combo->AddString("Greenland");
    combo->AddString("Migration");
    combo->AddString("Riverland");
    combo->AddString("Ringland");
    combo->AddString("Random");

    combo = AddComboBox(4, 50, 110, 200, 20, TC_GREY, NormalFont, 100);
    combo->AddString("64 x 64");
    combo->AddString("128 x 128");
    combo->AddString("256 x 256");
    combo->AddString("512 x 512");
    combo->AddString("1024 x 1024");

    AddText(5, 50, 140, _("Player Distribution"), COLOR_YELLOW, 0, NormalFont);
    combo = AddComboBox(6, 50, 170, 200, 20, TC_GREY, NormalFont, 100);
    combo->AddString("Very Close");
    combo->AddString("Close");
    combo->AddString("Medium");
    combo->AddString("Far");
    combo->AddString("Very Far");

    SetPlayers(settings.players);
    SetStyle(settings.style);
    SetSize(settings.width);
    SetPlayerDistribution(settings.minPlayerRadius, settings.maxPlayerRadius);
}

iwMapGenerator::~iwMapGenerator()
{
}

void iwMapGenerator::Msg_ButtonClick(const unsigned int ctrl_id)
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
            mapSettings = tmpSettings;
            Close();
        } break;
            
        case 2: // number of players
        {
            SetPlayers((tmpSettings.players + 1) % 8);

        } break;
    }
}

void iwMapGenerator::Msg_ComboSelectItem(const unsigned int ctrl_id, const int selection)
{
    if (ctrl_id == 3) // map style
    {
        switch (selection)
        {
            case 0: tmpSettings.style = MS_Islands;     break;
            case 1: tmpSettings.style = MS_Continent;   break;
            case 2: tmpSettings.style = MS_Greenland;   break;
            case 3: tmpSettings.style = MS_Migration;   break;
            case 4: tmpSettings.style = MS_Riverland;   break;
            case 5: tmpSettings.style = MS_Ringland;    break;
            case 6: tmpSettings.style = MS_Random;      break;
            default: break;
        }
    }
    else if (ctrl_id == 4) // map size
    {
        switch (selection)
        {
            case 0: tmpSettings.width = 64;   tmpSettings.height = 64;   break;
            case 1: tmpSettings.width = 128;  tmpSettings.height = 128;  break;
            case 2: tmpSettings.width = 256;  tmpSettings.height = 256;  break;
            case 3: tmpSettings.width = 512;  tmpSettings.height = 512;  break;
            case 4: tmpSettings.width = 1024; tmpSettings.height = 1024; break;
            default: break;
        }
    }
    else if (ctrl_id == 6) // player distribution
    {
        switch (selection)
        {
            case 0:
                tmpSettings.minPlayerRadius = 0.19;
                tmpSettings.maxPlayerRadius = 0.3;  break;
            case 1:
                tmpSettings.minPlayerRadius = 0.29;
                tmpSettings.maxPlayerRadius = 0.5;  break;
            case 2:
                tmpSettings.minPlayerRadius = 0.39;
                tmpSettings.maxPlayerRadius = 0.59;  break;
            case 3:
                tmpSettings.minPlayerRadius = 0.49;
                tmpSettings.maxPlayerRadius = 0.61;  break;
            case 4:
                tmpSettings.minPlayerRadius = 0.71;
                tmpSettings.maxPlayerRadius = 0.72; break;
            default: break;
        }
    }
}

void iwMapGenerator::SetPlayers(const int numberPlayers)
{
    tmpSettings.players = numberPlayers >= 2 ? numberPlayers : 2;
    ctrlTextButton* button = GetCtrl<ctrlTextButton>(2);
    switch (tmpSettings.players)
    {
        case 2: button->SetText("2 Players"); break;
        case 3: button->SetText("3 Players"); break;
        case 4: button->SetText("4 Players"); break;
        case 5: button->SetText("5 Players"); break;
        case 6: button->SetText("6 Players"); break;
        case 7: button->SetText("7 Players"); break;
        default: break;
    }
}

void iwMapGenerator::SetStyle(const MapStyle& style)
{
    tmpSettings.style = style;
    ctrlComboBox* combo = GetCtrl<ctrlComboBox>(3);
    switch (style)
    {
        case MS_Islands:    combo->SetSelection(0); break;
        case MS_Continent:  combo->SetSelection(1); break;
        case MS_Greenland:  combo->SetSelection(2); break;
        case MS_Migration:  combo->SetSelection(3); break;
        case MS_Riverland:  combo->SetSelection(4); break;
        case MS_Ringland:   combo->SetSelection(5); break;
        case MS_Random:     combo->SetSelection(6); break;
        default: break;
    }
}

void iwMapGenerator::SetSize(const int size)
{
    tmpSettings.width = size;
    tmpSettings.height = size;
    ctrlComboBox* combo = GetCtrl<ctrlComboBox>(4);
    switch (size)
    {
        case 64:    combo->SetSelection(0); break;
        case 128:   combo->SetSelection(1); break;
        case 256:   combo->SetSelection(2); break;
        case 512:   combo->SetSelection(3); break;
        case 1024:  combo->SetSelection(4); break;
        default: break;
    }
}

void iwMapGenerator::SetPlayerDistribution(const double min, const double max)
{
    tmpSettings.minPlayerRadius = min;
    tmpSettings.maxPlayerRadius = max;
    ctrlComboBox* combo = GetCtrl<ctrlComboBox>(6);
    if (min <= 0.2)
    {
        combo->SetSelection(0);
    }
    else if (min <= 0.3)
    {
        combo->SetSelection(1);
    }
    else if (min <= 0.4)
    {
        combo->SetSelection(2);
    }
    else if (min <= 0.5)
    {
        combo->SetSelection(3);
    }
    else
    {
        combo->SetSelection(4);
    }
}

