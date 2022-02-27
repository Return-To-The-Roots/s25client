// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwMapGenerator.h"
#include "Loader.h"
#include "controls/ctrlComboBox.h"
#include "controls/ctrlProgress.h"
#include "helpers/containerUtils.h"
#include "helpers/format.hpp"
#include "helpers/make_array.h"
#include "lua/GameDataLoader.h"
#include "gameData/MaxPlayers.h"
#include "gameData/WorldDescription.h"
#include "gameData/const_gui_ids.h"
#include "s25util/colors.h"
#include <string>

using namespace rttr::mapGenerator;

namespace {
/// Selectable map sizes. Note that maps bigger than 256^2 often cause lags
constexpr auto mapSizes = helpers::make_array(MapExtent::all(64), MapExtent::all(128), MapExtent::all(256),
                                              MapExtent::all(320), MapExtent::all(384));
} // namespace

iwMapGenerator::iwMapGenerator(MapSettings& settings)
    : IngameWindow(CGI_MAP_GENERATOR, IngameWindow::posLastOrCenter, Extent(270, 520), _("Map Generator"),
                   LOADER.GetImageN("resource", 41), true, CloseBehavior::Custom),
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
    ctrlComboBox* combo = AddComboBox(ID_cbNumPlayers, curPos, comboSize, TextureColor::Grey, NormalFont, 100);
    for(unsigned n = 2; n <= MAX_PLAYERS; n++)
        combo->AddString(helpers::format(_("%1% players"), n));

    curPos.y += 30;
    combo = AddComboBox(ID_cbMapStyle, curPos, comboSize, TextureColor::Grey, NormalFont, 100);
    combo->AddString(_("Water"));
    combo->AddString(_("Land"));
    combo->AddString(_("Mixed"));

    curPos.y += 30;
    combo = AddComboBox(ID_cbMapSize, curPos, comboSize, TextureColor::Grey, NormalFont, 100);
    for(const auto& size : mapSizes)
        combo->AddString(helpers::format("%1% x %2%", size.x, size.y));

    curPos.y += 30;
    AddText(ID_txtLandscape, curPos, _("Landscape"), COLOR_YELLOW, FontStyle{}, NormalFont);
    curPos.y += 20;
    combo = AddComboBox(ID_cbMapType, curPos, comboSize, TextureColor::Grey, NormalFont, 100);
    for(unsigned i = 0; i < desc.landscapes.size(); i++)
        combo->AddString(_(desc.get(DescIdx<LandscapeDesc>(i)).name));

    curPos.y += 30;
    AddText(ID_txtMountainDist, curPos, _("HQ distance to mountain"), COLOR_YELLOW, FontStyle{}, NormalFont);
    curPos.y += 20;
    combo = AddComboBox(ID_cbMountainDist, curPos, comboSize, TextureColor::Grey, NormalFont, 100);
    combo->AddString(_("Close"));
    combo->AddString(_("Normal"));
    combo->AddString(_("Far"));
    combo->AddString(_("Very far"));

    curPos.y += 30;
    AddText(ID_txtIslands, curPos, _("Islands"), COLOR_YELLOW, FontStyle{}, NormalFont);
    curPos.y += 20;
    combo = AddComboBox(ID_cbIslands, curPos, comboSize, TextureColor::Grey, NormalFont, 100);
    combo->AddString(_("Few"));
    combo->AddString(_("Medium"));
    combo->AddString(_("Many"));

    const int pgrOffset = 120;
    curPos.y += 35;
    AddText(ID_txtGold, curPos, _("Gold:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    AddProgress(ID_pgGoldRatio, DrawPoint(pgrOffset, curPos.y - 5), progressSize, TextureColor::Grey, 139, 138, 100);
    curPos.y += 30;
    AddText(ID_txtIron, curPos, _("Iron:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    AddProgress(ID_pgIronRatio, DrawPoint(pgrOffset, curPos.y - 5), progressSize, TextureColor::Grey, 139, 138, 100);
    curPos.y += 30;
    AddText(ID_txtCoal, curPos, _("Coal:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    AddProgress(ID_pgCoalRatio, DrawPoint(pgrOffset, curPos.y - 5), progressSize, TextureColor::Grey, 139, 138, 100);
    curPos.y += 30;
    AddText(ID_txtGranite, curPos, _("Granite:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    AddProgress(ID_pgGraniteRatio, DrawPoint(pgrOffset, curPos.y - 5), progressSize, TextureColor::Grey, 139, 138, 100);
    curPos.y += 30;
    AddText(ID_txtRivers, curPos, _("Rivers:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    AddProgress(ID_pgRivers, DrawPoint(pgrOffset, curPos.y - 5), progressSize, TextureColor::Grey, 139, 138, 100);
    curPos.y += 30;
    AddText(ID_txtTrees, curPos, _("Trees:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    AddProgress(ID_pgTrees, DrawPoint(pgrOffset, curPos.y - 5), progressSize, TextureColor::Grey, 139, 138, 100);
    curPos.y += 30;
    AddText(ID_txtStonePiles, curPos, _("Stone piles:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    AddProgress(ID_pgStonePiles, DrawPoint(pgrOffset, curPos.y - 5), progressSize, TextureColor::Grey, 139, 138, 100);

    curPos.y += 25;
    AddTextButton(ID_btBack, curPos, buttonSize, TextureColor::Red1, _("Back"), NormalFont);
    AddTextButton(ID_btApply, DrawPoint(130, curPos.y), buttonSize, TextureColor::Green2, _("Apply"), NormalFont);

    Reset();
}

void iwMapGenerator::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case ID_btBack: Close(); break;
        case ID_btApply:
            Apply();
            Close();
            break;
    }
}

void iwMapGenerator::Apply()
{
    mapSettings.numPlayers = GetCtrl<ctrlComboBox>(ID_cbNumPlayers)->GetSelection().get() + 2;
    mapSettings.ratioGold = GetCtrl<ctrlProgress>(ID_pgGoldRatio)->GetPosition();
    mapSettings.ratioIron = GetCtrl<ctrlProgress>(ID_pgIronRatio)->GetPosition();
    mapSettings.ratioCoal = GetCtrl<ctrlProgress>(ID_pgCoalRatio)->GetPosition();
    mapSettings.ratioGranite = GetCtrl<ctrlProgress>(ID_pgGraniteRatio)->GetPosition();
    mapSettings.rivers = GetCtrl<ctrlProgress>(ID_pgRivers)->GetPosition();
    mapSettings.trees = GetCtrl<ctrlProgress>(ID_pgTrees)->GetPosition();
    mapSettings.stonePiles = GetCtrl<ctrlProgress>(ID_pgStonePiles)->GetPosition();

    switch(*GetCtrl<ctrlComboBox>(ID_cbMountainDist)->GetSelection())
    {
        case 0: mapSettings.mountainDistance = MountainDistance::Close; break;
        case 1: mapSettings.mountainDistance = MountainDistance::Normal; break;
        case 2: mapSettings.mountainDistance = MountainDistance::Far; break;
        case 3: mapSettings.mountainDistance = MountainDistance::VeryFar; break;
    }
    switch(*GetCtrl<ctrlComboBox>(ID_cbMapStyle)->GetSelection())
    {
        case 0: mapSettings.style = MapStyle::Water; break;
        case 1: mapSettings.style = MapStyle::Land; break;
        case 2: mapSettings.style = MapStyle::Mixed; break;
    }
    mapSettings.size = mapSizes[*GetCtrl<ctrlComboBox>(ID_cbMapSize)->GetSelection()];
    switch(*GetCtrl<ctrlComboBox>(ID_cbIslands)->GetSelection())
    {
        case 0: mapSettings.islands = IslandAmount::Few; break;
        case 1: mapSettings.islands = IslandAmount::Normal; break;
        case 2: mapSettings.islands = IslandAmount::Many; break;
    }
    const auto& mapType = GetCtrl<ctrlComboBox>(ID_cbMapType)->GetSelection();
    if(mapType)
        mapSettings.type = DescIdx<LandscapeDesc>(*mapType);
}

void iwMapGenerator::Reset()
{
    GetCtrl<ctrlComboBox>(ID_cbNumPlayers)
      ->SetSelection(std::min(mapSettings.numPlayers, MAX_PLAYERS) - 2); // List starts at 2 players

    GetCtrl<ctrlProgress>(ID_pgGoldRatio)->SetPosition(mapSettings.ratioGold);
    GetCtrl<ctrlProgress>(ID_pgIronRatio)->SetPosition(mapSettings.ratioIron);
    GetCtrl<ctrlProgress>(ID_pgCoalRatio)->SetPosition(mapSettings.ratioCoal);
    GetCtrl<ctrlProgress>(ID_pgGraniteRatio)->SetPosition(mapSettings.ratioGranite);
    GetCtrl<ctrlProgress>(ID_pgRivers)->SetPosition(mapSettings.rivers);
    GetCtrl<ctrlProgress>(ID_pgTrees)->SetPosition(mapSettings.trees);
    GetCtrl<ctrlProgress>(ID_pgStonePiles)->SetPosition(mapSettings.stonePiles);

    auto* combo = GetCtrl<ctrlComboBox>(ID_cbMountainDist);
    switch(mapSettings.mountainDistance)
    {
        case MountainDistance::Close: combo->SetSelection(0); break;
        case MountainDistance::Normal: combo->SetSelection(1); break;
        case MountainDistance::Far: combo->SetSelection(2); break;
        case MountainDistance::VeryFar: combo->SetSelection(3); break;
    }

    combo = GetCtrl<ctrlComboBox>(ID_cbMapStyle);
    switch(mapSettings.style)
    {
        case MapStyle::Water: combo->SetSelection(0); break;
        case MapStyle::Land: combo->SetSelection(1); break;
        case MapStyle::Mixed: combo->SetSelection(2); break;
    }

    const auto idxSize = helpers::indexOf(mapSizes, mapSettings.size);
    if(idxSize >= 0)
        GetCtrl<ctrlComboBox>(ID_cbMapSize)->SetSelection(idxSize);

    combo = GetCtrl<ctrlComboBox>(ID_cbIslands);
    switch(mapSettings.islands)
    {
        case IslandAmount::Few: combo->SetSelection(0); break;
        case IslandAmount::Normal: combo->SetSelection(1); break;
        case IslandAmount::Many: combo->SetSelection(2); break;
    }

    GetCtrl<ctrlComboBox>(ID_cbMapType)->SetSelection(mapSettings.type.value);
}
