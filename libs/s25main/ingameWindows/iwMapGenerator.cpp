// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwMapGenerator.h"
#include "Loader.h"
#include "controls/ctrlComboBox.h"
#include "controls/ctrlProgress.h"
#include "helpers/containerUtils.h"
#include "helpers/format.hpp"
#include "lua/GameDataLoader.h"
#include "gameData/MaxPlayers.h"
#include "gameData/WorldDescription.h"
#include "gameData/const_gui_ids.h"
#include "s25util/StringConversion.h"
#include "s25util/colors.h"
#include <string>

using namespace rttr::mapGenerator;

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

    constexpr Extent comboSize(230, 20);
    constexpr Extent comboSizeSmall(130, 20);
    constexpr Extent progressSize(130, 20);
    constexpr Extent buttonSize(100, 20);

    curPos.y += 30;
    ctrlComboBox* combo = AddComboBox(ID_cbNumPlayers, curPos, comboSize, TextureColor::Grey, NormalFont, 100);
    for(unsigned n = 2; n <= MAX_PLAYERS; n++)
        combo->AddString(helpers::format(_("%1% players"), n));

    curPos.y += 30;
    AddText(ID_txtMapStyle, curPos, _("Style"), COLOR_YELLOW, FontStyle{}, NormalFont);
    combo =
      AddComboBox(ID_cbMapStyle, curPos + DrawPoint(100, -5), comboSizeSmall, TextureColor::Grey, NormalFont, 100);
    combo->AddString(_("Water"));
    combo->AddString(_("Land"));
    combo->AddString(_("Mixed"));

    curPos.y += 30;
    AddText(ID_txtMapSize, curPos, _("Size"), COLOR_YELLOW, FontStyle{}, NormalFont);
    auto* cbSizeX =
      AddComboBox(ID_cbMapSizeX, curPos + DrawPoint(100, -5), Extent(50, 20), TextureColor::Grey, NormalFont, 200);
    AddText(ID_txtMapSizeX, curPos + DrawPoint(160, 5), "x", COLOR_YELLOW, FontStyle::VCENTER, NormalFont);
    auto* cbSizeY =
      AddComboBox(ID_cbMapSizeY, curPos + DrawPoint(180, -5), Extent(50, 20), TextureColor::Grey, NormalFont, 200);
    for(unsigned size = 32; size <= 320; size += 32)
    {
        const auto strSize = s25util::toStringClassic(size);
        cbSizeX->AddString(strSize);
        cbSizeY->AddString(strSize);
    }

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

    constexpr int pgrOffset = 120;
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
    mapSettings.size.x =
      s25util::fromStringClassic<unsigned>(GetCtrl<ctrlComboBox>(ID_cbMapSizeX)->GetSelectedText().get_value_or("128"));
    mapSettings.size.y =
      s25util::fromStringClassic<unsigned>(GetCtrl<ctrlComboBox>(ID_cbMapSizeY)->GetSelectedText().get_value_or("128"));
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

    const auto strSizeX = s25util::toStringClassic(mapSettings.size.x);
    const auto strSizeY = s25util::toStringClassic(mapSettings.size.y);
    auto* cbSizeX = GetCtrl<ctrlComboBox>(ID_cbMapSizeX);
    auto* cbSizeY = GetCtrl<ctrlComboBox>(ID_cbMapSizeY);
    const auto numSizes = cbSizeX->GetNumItems();
    RTTR_Assert(numSizes == cbSizeY->GetNumItems());
    for(unsigned i = 0; i < numSizes; ++i)
    {
        if(cbSizeX->GetText(i) == strSizeX)
            cbSizeX->SetSelection(i);
        if(cbSizeY->GetText(i) == strSizeY)
            cbSizeY->SetSelection(i);
    }

    combo = GetCtrl<ctrlComboBox>(ID_cbIslands);
    switch(mapSettings.islands)
    {
        case IslandAmount::Few: combo->SetSelection(0); break;
        case IslandAmount::Normal: combo->SetSelection(1); break;
        case IslandAmount::Many: combo->SetSelection(2); break;
    }

    GetCtrl<ctrlComboBox>(ID_cbMapType)->SetSelection(mapSettings.type.value);
}
