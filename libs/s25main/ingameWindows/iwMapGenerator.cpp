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
    CTRL_LAST_ID = 6, // last UI control ID used before enum controls
    CTRL_PLAYER_NUMBER,
    CTRL_MAP_STYLE,
    CTRL_MAP_SIZE,
    CTRL_MAP_TYPE,
    CTRL_RATIO_GOLD,
    CTRL_RATIO_IRON,
    CTRL_RATIO_COAL,
    CTRL_RATIO_GRANITE
};

iwMapGenerator::iwMapGenerator(MapSettings& settings)
    : IngameWindow(CGI_MAP_GENERATOR, IngameWindow::posLastOrCenter, Extent(250, 350), _("Map Generator"), LOADER.GetImageN("resource", 41),
                   true),
      mapSettings(settings)
{
    WorldDescription desc;
    GameDataLoader gdLoader(desc);
    if(!gdLoader.Load())
    {
        Close();
        return;
    }

    AddTextButton(0, DrawPoint(20, 310), Extent(100, 20), TC_RED1, _("Back"), NormalFont);
    AddTextButton(1, DrawPoint(130, 310), Extent(100, 20), TC_GREEN2, _("Apply"), NormalFont);

    ctrlComboBox* combo = AddComboBox(CTRL_PLAYER_NUMBER, DrawPoint(20, 30), Extent(210, 20), TC_GREY, NormalFont, 100);
    for(unsigned n = 2; n < MAX_PLAYERS; n++)
        combo->AddString(boost::str(boost::format(_("%1% players")) % n));

    combo = AddComboBox(CTRL_MAP_STYLE, DrawPoint(20, 60), Extent(210, 20), TC_GREY, NormalFont, 100);
    combo->AddString(_("Continent"));
    combo->AddString(_("Edgecase"));
    combo->AddString(_("Migration"));
    combo->AddString(_("Rivers"));
    combo->AddString(_("Random"));

    combo = AddComboBox(CTRL_MAP_SIZE, DrawPoint(20, 90), Extent(210, 20), TC_GREY, NormalFont, 100);
    combo->AddString("64 x 64");
    combo->AddString("128 x 128");
    combo->AddString("256 x 256");
    combo->AddString("512 x 512");
    combo->AddString("1024 x 1024");

    AddText(2, DrawPoint(20, 120), _("Landscape"), COLOR_YELLOW, FontStyle{}, NormalFont);
    combo = AddComboBox(CTRL_MAP_TYPE, DrawPoint(20, 140), Extent(210, 20), TC_GREY, NormalFont, 100);
    for(unsigned i = 0; i < desc.landscapes.size(); i++)
        combo->AddString(_(desc.get(DescIdx<LandscapeDesc>(i)).name));

    AddText(3, DrawPoint(20, 175), _("Gold:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    AddProgress(CTRL_RATIO_GOLD, DrawPoint(100, 170), Extent(130, 20), TC_GREY, 139, 138, 100);
    AddText(4, DrawPoint(20, 205), _("Iron:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    AddProgress(CTRL_RATIO_IRON, DrawPoint(100, 200), Extent(130, 20), TC_GREY, 139, 138, 100);
    AddText(5, DrawPoint(20, 235), _("Coal:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    AddProgress(CTRL_RATIO_COAL, DrawPoint(100, 230), Extent(130, 20), TC_GREY, 139, 138, 100);
    AddText(6, DrawPoint(20, 265), _("Granite:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    AddProgress(CTRL_RATIO_GRANITE, DrawPoint(100, 260), Extent(130, 20), TC_GREY, 139, 138, 100);

    Reset();
}

iwMapGenerator::~iwMapGenerator() = default;

void iwMapGenerator::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        default: break;

        case 0: // back
            Close();
            break;

        case 1: // apply
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

    switch(GetCtrl<ctrlComboBox>(CTRL_MAP_STYLE)->GetSelection().get())
    {
        case 0: mapSettings.style = MapStyle::Continent; break;
        case 1: mapSettings.style = MapStyle::Edgecase; break;
        case 2: mapSettings.style = MapStyle::Migration; break;
        case 3: mapSettings.style = MapStyle::Rivers; break;
        case 4: mapSettings.style = MapStyle::Random; break;
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
    int mapType = GetCtrl<ctrlComboBox>(CTRL_MAP_TYPE)->GetSelection();
    if(mapType >= 0)
        mapSettings.type = DescIdx<LandscapeDesc>(mapType);
}

void iwMapGenerator::Reset()
{
    auto* combo = GetCtrl<ctrlComboBox>(CTRL_PLAYER_NUMBER);
    const uint16_t playersSelection = mapSettings.numPlayers - 2;
    if(playersSelection < MAX_PLAYERS - 2)
    {
        combo->SetSelection(playersSelection);
    }

    GetCtrl<ctrlProgress>(CTRL_RATIO_GOLD)->SetPosition(mapSettings.ratioGold);
    GetCtrl<ctrlProgress>(CTRL_RATIO_IRON)->SetPosition(mapSettings.ratioIron);
    GetCtrl<ctrlProgress>(CTRL_RATIO_COAL)->SetPosition(mapSettings.ratioCoal);
    GetCtrl<ctrlProgress>(CTRL_RATIO_GRANITE)->SetPosition(mapSettings.ratioGranite);

    combo = GetCtrl<ctrlComboBox>(CTRL_MAP_STYLE);
    switch(mapSettings.style)
    {
        case MapStyle::Continent: combo->SetSelection(0); break;
        case MapStyle::Edgecase: combo->SetSelection(1); break;
        case MapStyle::Migration: combo->SetSelection(2); break;
        case MapStyle::Rivers: combo->SetSelection(3); break;
        case MapStyle::Random: combo->SetSelection(4); break;
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
