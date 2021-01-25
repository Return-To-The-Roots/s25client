// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "iwMapGenerator.h"
#include "Loader.h"
#include "controls/ctrlComboBox.h"
#include "controls/ctrlProgress.h"
#include "lua/GameDataLoader.h"
#include "gameData/MaxPlayers.h"
#include "gameData/WorldDescription.h"
#include "gameData/const_gui_ids.h"
#include "s25util/colors.h"
#include <string>

using namespace rttr::mapGenerator;

enum
{
    CTRL_BTN_BACK = 0,
    CTRL_BTN_APPLY,
    CTRL_TXT_LANDSCAPE,
    CTRL_TXT_GOAL,
    CTRL_TXT_IRON,
    CTRL_TXT_COAL,
    CTRL_TXT_GRANITE,
    CTRL_TXT_RIVERS,
    CTRL_TXT_MOUNTAIN_DIST,
    CTRL_TXT_TREES,
    CTRL_TXT_STONE_PILES,
    CTRL_PLAYER_NUMBER,
    CTRL_MAP_STYLE,
    CTRL_MAP_SIZE,
    CTRL_MAP_TYPE,
    CTRL_RATIO_GOLD,
    CTRL_RATIO_IRON,
    CTRL_RATIO_COAL,
    CTRL_RATIO_GRANITE,
    CTRL_RIVERS,
    CTRL_MOUNTAIN_DIST,
    CTRL_TREES,
    CTRL_STONE_PILES
};

iwMapGenerator::iwMapGenerator(MapSettings& settings)
    : IngameWindow(CGI_MAP_GENERATOR, IngameWindow::posLastOrCenter, Extent(270, 470), _("Map Generator"),
                   LOADER.GetImageN("resource", 41), true),
      mapSettings(settings)
{
    WorldDescription desc;
    GameDataLoader gdLoader(desc);
    if(!gdLoader.Load())
    {
        Close();
        return;
    }

    DrawPoint curPos(20, 0);

    const Extent comboSize(230, 20);
    const Extent progressSize(130, 20);
    const Extent buttonSize(100, 20);

    curPos.y += 30;
    ctrlComboBox* combo = AddComboBox(CTRL_PLAYER_NUMBER, curPos, comboSize, TextureColor::Grey, NormalFont, 100);
    for(unsigned n = 2; n <= MAX_PLAYERS; n++)
        combo->AddString(boost::str(boost::format(_("%1% players")) % n));

    curPos.y += 30;
    combo = AddComboBox(CTRL_MAP_STYLE, curPos, comboSize, TextureColor::Grey, NormalFont, 100);
    combo->AddString(_("Water"));
    combo->AddString(_("Land"));
    combo->AddString(_("Mixed"));

    curPos.y += 30;
    combo = AddComboBox(CTRL_MAP_SIZE, curPos, comboSize, TextureColor::Grey, NormalFont, 100);
    combo->AddString("64 x 64");
    combo->AddString("128 x 128");
    combo->AddString("256 x 256");
    combo->AddString("512 x 512");
    combo->AddString("1024 x 1024");

    curPos.y += 30;
    AddText(CTRL_TXT_LANDSCAPE, curPos, _("Landscape"), COLOR_YELLOW, FontStyle{}, NormalFont);
    curPos.y += 20;
    combo = AddComboBox(CTRL_MAP_TYPE, curPos, comboSize, TextureColor::Grey, NormalFont, 100);
    for(unsigned i = 0; i < desc.landscapes.size(); i++)
        combo->AddString(_(desc.get(DescIdx<LandscapeDesc>(i)).name));

    curPos.y += 30;
    AddText(CTRL_TXT_MOUNTAIN_DIST, curPos, _("HQ distance to mountain"), COLOR_YELLOW, FontStyle{}, NormalFont);
    curPos.y += 20;
    combo = AddComboBox(CTRL_MOUNTAIN_DIST, curPos, comboSize, TextureColor::Grey, NormalFont, 100);
    combo->AddString(_("Close"));
    combo->AddString(_("Normal"));
    combo->AddString(_("Far"));
    combo->AddString(_("Very far"));

    const int pgrOffset = 120;
    curPos.y += 35;
    AddText(CTRL_TXT_GOAL, curPos, _("Gold:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    AddProgress(CTRL_RATIO_GOLD, DrawPoint(pgrOffset, curPos.y - 5), progressSize, TextureColor::Grey, 139, 138, 100);
    curPos.y += 30;
    AddText(CTRL_TXT_IRON, curPos, _("Iron:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    AddProgress(CTRL_RATIO_IRON, DrawPoint(pgrOffset, curPos.y - 5), progressSize, TextureColor::Grey, 139, 138, 100);
    curPos.y += 30;
    AddText(CTRL_TXT_COAL, curPos, _("Coal:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    AddProgress(CTRL_RATIO_COAL, DrawPoint(pgrOffset, curPos.y - 5), progressSize, TextureColor::Grey, 139, 138, 100);
    curPos.y += 30;
    AddText(CTRL_TXT_GRANITE, curPos, _("Granite:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    AddProgress(CTRL_RATIO_GRANITE, DrawPoint(pgrOffset, curPos.y - 5), progressSize, TextureColor::Grey, 139, 138, 100);
    curPos.y += 30;
    AddText(CTRL_TXT_RIVERS, curPos, _("Rivers:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    AddProgress(CTRL_RIVERS, DrawPoint(pgrOffset, curPos.y - 5), progressSize, TextureColor::Grey, 139, 138, 100);
    curPos.y += 30;
    AddText(CTRL_TXT_TREES, curPos, _("Trees:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    AddProgress(CTRL_TREES, DrawPoint(pgrOffset, curPos.y - 5), progressSize, TextureColor::Grey, 139, 138, 100);
    curPos.y += 30;
    AddText(CTRL_TXT_STONE_PILES, curPos, _("Stone piles:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    AddProgress(CTRL_STONE_PILES, DrawPoint(pgrOffset, curPos.y - 5), progressSize, TextureColor::Grey, 139, 138, 100);

    curPos.y += 25;
    AddTextButton(CTRL_BTN_BACK, curPos, buttonSize, TextureColor::Red1, _("Back"), NormalFont);
    AddTextButton(CTRL_BTN_APPLY, DrawPoint(130, curPos.y), buttonSize, TextureColor::Green2, _("Apply"), NormalFont);

    Reset();
}

iwMapGenerator::~iwMapGenerator() = default;

void iwMapGenerator::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        default: break;

        case CTRL_BTN_BACK: Close(); break;

        case CTRL_BTN_APPLY:
            Apply();
            Close();
            break;
    }
}

void iwMapGenerator::Apply()
{
    mapSettings.numPlayers = GetCtrl<ctrlComboBox>(CTRL_PLAYER_NUMBER)->GetSelection().get() + 2;
    mapSettings.ratioGold = GetCtrl<ctrlProgress>(CTRL_RATIO_GOLD)->GetPosition();
    mapSettings.ratioIron = GetCtrl<ctrlProgress>(CTRL_RATIO_IRON)->GetPosition();
    mapSettings.ratioCoal = GetCtrl<ctrlProgress>(CTRL_RATIO_COAL)->GetPosition();
    mapSettings.ratioGranite = GetCtrl<ctrlProgress>(CTRL_RATIO_GRANITE)->GetPosition();
    mapSettings.rivers = GetCtrl<ctrlProgress>(CTRL_RIVERS)->GetPosition();
    mapSettings.trees = GetCtrl<ctrlProgress>(CTRL_TREES)->GetPosition();
    mapSettings.stonePiles = GetCtrl<ctrlProgress>(CTRL_STONE_PILES)->GetPosition();

    switch(GetCtrl<ctrlComboBox>(CTRL_MOUNTAIN_DIST)->GetSelection().get())
    {
        case 0: mapSettings.mountainDistance = MountainDistance::Close; break;
        case 1: mapSettings.mountainDistance = MountainDistance::Normal; break;
        case 2: mapSettings.mountainDistance = MountainDistance::Far; break;
        case 3: mapSettings.mountainDistance = MountainDistance::VeryFar; break;
        default: break;
    }
    switch(GetCtrl<ctrlComboBox>(CTRL_MAP_STYLE)->GetSelection().get())
    {
        case 0: mapSettings.style = MapStyle::Water; break;
        case 1: mapSettings.style = MapStyle::Land; break;
        case 2: mapSettings.style = MapStyle::Mixed; break;
        default: break;
    }
    switch(GetCtrl<ctrlComboBox>(CTRL_MAP_SIZE)->GetSelection().get())
    {
        case 0: mapSettings.size = MapExtent::all(64); break;
        case 1: mapSettings.size = MapExtent::all(128); break;
        case 2: mapSettings.size = MapExtent::all(256); break;
        case 3: mapSettings.size = MapExtent::all(512); break;
        case 4: mapSettings.size = MapExtent::all(1024); break;
        default: break;
    }
    int mapType = GetCtrl<ctrlComboBox>(CTRL_MAP_TYPE)->GetSelection().get();
    if(mapType >= 0)
        mapSettings.type = DescIdx<LandscapeDesc>(mapType);
}

void iwMapGenerator::Reset()
{
    auto* combo = GetCtrl<ctrlComboBox>(CTRL_PLAYER_NUMBER);
    if(mapSettings.numPlayers <= MAX_PLAYERS)
    {
        combo->SetSelection(mapSettings.numPlayers - 2); // List starts at 2 players
    }

    GetCtrl<ctrlProgress>(CTRL_RATIO_GOLD)->SetPosition(mapSettings.ratioGold);
    GetCtrl<ctrlProgress>(CTRL_RATIO_IRON)->SetPosition(mapSettings.ratioIron);
    GetCtrl<ctrlProgress>(CTRL_RATIO_COAL)->SetPosition(mapSettings.ratioCoal);
    GetCtrl<ctrlProgress>(CTRL_RATIO_GRANITE)->SetPosition(mapSettings.ratioGranite);
    GetCtrl<ctrlProgress>(CTRL_RIVERS)->SetPosition(mapSettings.rivers);
    GetCtrl<ctrlProgress>(CTRL_TREES)->SetPosition(mapSettings.trees);
    GetCtrl<ctrlProgress>(CTRL_STONE_PILES)->SetPosition(mapSettings.stonePiles);

    combo = GetCtrl<ctrlComboBox>(CTRL_MOUNTAIN_DIST);
    switch(mapSettings.mountainDistance)
    {
        case MountainDistance::Close: combo->SetSelection(0); break;
        case MountainDistance::Normal: combo->SetSelection(1); break;
        case MountainDistance::Far: combo->SetSelection(2); break;
        case MountainDistance::VeryFar: combo->SetSelection(3); break;
        default: break;
    }

    combo = GetCtrl<ctrlComboBox>(CTRL_MAP_STYLE);
    switch(mapSettings.style)
    {
        case MapStyle::Water: combo->SetSelection(0); break;
        case MapStyle::Land: combo->SetSelection(1); break;
        case MapStyle::Mixed: combo->SetSelection(2); break;
        default: break;
    }

    combo = GetCtrl<ctrlComboBox>(CTRL_MAP_SIZE);
    switch(mapSettings.size.x)
    {
        case 64: combo->SetSelection(0); break;
        case 128: combo->SetSelection(1); break;
        case 256: combo->SetSelection(2); break;
        case 512: combo->SetSelection(3); break;
        case 1024: combo->SetSelection(4); break;
        default: break;
    }

    combo = GetCtrl<ctrlComboBox>(CTRL_MAP_TYPE);
    combo->SetSelection(mapSettings.type.value);
}
