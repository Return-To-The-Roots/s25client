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
#include "helpers/containerUtils.h"
#include "libutil/src/colors.h"

#include <boost/format.hpp>
#include <string>

iwMapGenerator::iwMapGenerator(MapSettings& settings) : IngameWindow(CGI_MAP_GENERATOR,
                                                IngameWindow::posLastOrCenter,
                                                250, 400, _("Map Generator"),
                                                LOADER.GetImageN("resource", 41), true, false), mapSettings(settings), tmpSettings(settings)
{
    AddTextButton(0,  20, 360, 100, 20, TC_RED2, _("Back"), NormalFont);
    AddTextButton(1, 130, 360, 100, 20, TC_GREEN2, _("Apply"), NormalFont);

    ctrlComboBox* combo = AddComboBox(2, 20, 30, 210, 20, TC_GREY, NormalFont, 100);
    for (int n = 2; n < 8; n++)
    {
        combo->AddString(boost::str(boost::format(_("%s players")) % n));
    }
    
    combo = AddComboBox(3, 20, 60, 210, 20, TC_GREY, NormalFont, 100);
    combo->AddString("Islands");
    combo->AddString("Continent");
    combo->AddString("Greenland");
    combo->AddString("Migration");
    combo->AddString("Riverland");
    combo->AddString("Ringland");
    combo->AddString("Random");

    combo = AddComboBox(4, 20, 90, 210, 20, TC_GREY, NormalFont, 100);
    combo->AddString("64 x 64");
    combo->AddString("128 x 128");
    combo->AddString("256 x 256");
    combo->AddString("512 x 512");
    combo->AddString("1024 x 1024");

    AddText(5, 20, 120, _("Player Distribution"), COLOR_YELLOW, 0, NormalFont);
    combo = AddComboBox(6, 20, 140, 210, 20, TC_GREY, NormalFont, 100);
    combo->AddString("Very Close");
    combo->AddString("Close");
    combo->AddString("Medium");
    combo->AddString("Far");
    combo->AddString("Very Far");

    AddText(7, 20, 170, _("Landscape"), COLOR_YELLOW, 0, NormalFont);
    combo = AddComboBox(8, 20, 190, 210, 20, TC_GREY, NormalFont, 100);
    combo->AddString("Greenland");
    combo->AddString("Winterworld");
    combo->AddString("Wasteland");
    
    AddText(9, 20, 225, _("Gold:"), COLOR_YELLOW, 0, NormalFont);
    ctrlProgress* progressBar = AddProgress(10, 100, 220, 130, 20,
                                            TC_GREY, 139, 138, 100);
    progressBar->SetPosition(settings.ratioGold);

    AddText(11, 20, 255, _("Iron:"), COLOR_YELLOW, 0, NormalFont);
    progressBar = AddProgress(12, 100, 250, 130, 20,
                              TC_GREY, 139, 138, 100);
    progressBar->SetPosition(settings.ratioIron);

    AddText(13, 20, 285, _("Coal:"), COLOR_YELLOW, 0, NormalFont);
    progressBar = AddProgress(14, 100, 280, 130, 20,
                              TC_GREY, 139, 138, 100);
    progressBar->SetPosition(settings.ratioCoal);

    AddText(15, 20, 315, _("Granite:"), COLOR_YELLOW, 0, NormalFont);
    progressBar = AddProgress(16, 100, 310, 130, 20,
                              TC_GREY, 139, 138, 100);
    progressBar->SetPosition(settings.ratioGranite);
    
    Reset();
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
            tmpSettings.ratioGold = GetCtrl<ctrlProgress>(10)->GetPosition();
            tmpSettings.ratioIron = GetCtrl<ctrlProgress>(12)->GetPosition();
            tmpSettings.ratioCoal = GetCtrl<ctrlProgress>(14)->GetPosition();
            tmpSettings.ratioGranite = GetCtrl<ctrlProgress>(16)->GetPosition();
            mapSettings = tmpSettings;
            Close();
        } break;
    }
}

void iwMapGenerator::Msg_ComboSelectItem(const unsigned int ctrl_id, const int selection)
{
    // combo-box: number of players
    if (ctrl_id == 2)
    {
        switch (selection)
        {
            case 0: tmpSettings.players = 2; break;
            case 1: tmpSettings.players = 3; break;
            case 2: tmpSettings.players = 4; break;
            case 3: tmpSettings.players = 5; break;
            case 4: tmpSettings.players = 6; break;
            case 5: tmpSettings.players = 7; break;
            default: break;
        }
    }
    
    // combo-box: random map style
    else if (ctrl_id == 3)
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
    
    // combo-box: map size
    else if (ctrl_id == 4)
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
    
    // combo-box: player distribution
    else if (ctrl_id == 6)
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
    
    // combo-box: landscape
    else if (ctrl_id == 8)
    {
        switch (selection)
        {
            case 0:
                tmpSettings.type = LT_GREENLAND;
                break;
            case 1:
                tmpSettings.type = LT_WINTERWORLD;
                break;
            case 2:
                tmpSettings.type = LT_WASTELAND;
                break;
            default: break;
        }
    }
}

void iwMapGenerator::Reset()
{
    // reset number of players to original value
    tmpSettings.players = mapSettings.players;
    
    // update number of player in UI
    ctrlComboBox* combo = GetCtrl<ctrlComboBox>(2);
    switch (tmpSettings.players)
    {
        case 2: combo->SetSelection(0); break;
        case 3: combo->SetSelection(1); break;
        case 4: combo->SetSelection(2); break;
        case 5: combo->SetSelection(3); break;
        case 6: combo->SetSelection(4); break;
        case 7: combo->SetSelection(5); break;
        default: break;
    }
    
    // reset map style to original value
    tmpSettings.style = mapSettings.style;

    // update map style in UI
    combo = GetCtrl<ctrlComboBox>(3);
    switch (tmpSettings.style)
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
    
    // reset map size to original values
    tmpSettings.width = mapSettings.width;
    tmpSettings.height = mapSettings.height;
    
    // update map size in UI
    combo = GetCtrl<ctrlComboBox>(4);
    switch (mapSettings.width)
    {
        case 64:    combo->SetSelection(0); break;
        case 128:   combo->SetSelection(1); break;
        case 256:   combo->SetSelection(2); break;
        case 512:   combo->SetSelection(3); break;
        case 1024:  combo->SetSelection(4); break;
        default: break;
    }
    
    // reset map size to original values
    tmpSettings.minPlayerRadius = mapSettings.minPlayerRadius;
    tmpSettings.maxPlayerRadius = mapSettings.maxPlayerRadius;
    
    // update player distance in UI
    combo = GetCtrl<ctrlComboBox>(6);
    if (tmpSettings.minPlayerRadius <= 0.2)
    {
        combo->SetSelection(0);
    }
    else
    if (tmpSettings.minPlayerRadius <= 0.3)
    {
        combo->SetSelection(1);
    }
    else
    if (tmpSettings.minPlayerRadius <= 0.4)
    {
        combo->SetSelection(2);
    }
    else
    if (tmpSettings.minPlayerRadius <= 0.5)
    {
        combo->SetSelection(3);
    }
    else
    {
        combo->SetSelection(4);
    }
    
    // reset landscape type
    tmpSettings.type = mapSettings.type;
    
    // update landscape type in UI
    combo = GetCtrl<ctrlComboBox>(8);
    switch (mapSettings.type)
    {
        case LT_GREENLAND:   combo->SetSelection(0); break;
        case LT_WINTERWORLD: combo->SetSelection(1); break;
        case LT_WASTELAND:   combo->SetSelection(2); break;
        default: break;
    }
}

