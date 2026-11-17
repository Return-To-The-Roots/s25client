// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwMapGenerator.h"
#include "Loader.h"
#include "controls/ctrlCheck.h"
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
    : IngameWindow(CGI_MAP_GENERATOR, IngameWindow::posLastOrCenter, Extent(270, 645), _("Map Generator"),
                   LOADER.getImageN("resource", 41), true, CloseBehavior::Custom),
      mapSettings(settings)
{
    WorldDescription desc;
    GameDataLoader gdLoader(desc);
    if(!gdLoader.load())
    {
        close();
        return;
    }

    DrawPoint curPos(20, 0);

    constexpr Extent comboSize(230, 20);
    constexpr Extent comboSizeSmall(130, 20);
    constexpr Extent progressSize(130, 20);
    constexpr Extent buttonSize(100, 20);

    curPos.y += 30;
    ctrlComboBox* combo = addComboBox(ID_cbNumPlayers, curPos, comboSize, TextureColor::Grey, NormalFont, 100);
    for(unsigned n = 2; n <= MAX_PLAYERS; n++)
        combo->addItem(helpers::format(_("%1% players"), n));

    curPos.y += 30;
    addText(ID_txtMapStyle, curPos, _("Style"), COLOR_YELLOW, FontStyle{}, NormalFont);
    combo =
      addComboBox(ID_cbMapStyle, curPos + DrawPoint(100, -5), comboSizeSmall, TextureColor::Grey, NormalFont, 100);
    combo->addItem(_("Water"));
    combo->addItem(_("Land"));
    combo->addItem(_("Mixed"));

    curPos.y += 30;
    addText(ID_txtMapSize, curPos, _("Size"), COLOR_YELLOW, FontStyle{}, NormalFont);
    auto* cbSizeX =
      addComboBox(ID_cbMapSizeX, curPos + DrawPoint(100, -5), Extent(50, 20), TextureColor::Grey, NormalFont, 200);
    addText(ID_txtMapSizeX, curPos + DrawPoint(160, 5), "x", COLOR_YELLOW, FontStyle::VCENTER, NormalFont);
    auto* cbSizeY =
      addComboBox(ID_cbMapSizeY, curPos + DrawPoint(180, -5), Extent(50, 20), TextureColor::Grey, NormalFont, 200);
    for(unsigned size = 32; size <= 320; size += 32)
    {
        const auto strSize = s25util::toStringClassic(size);
        cbSizeX->addItem(strSize);
        cbSizeY->addItem(strSize);
    }

    curPos.y += 30;
    addText(ID_txtLandscape, curPos, _("Landscape"), COLOR_YELLOW, FontStyle{}, NormalFont);
    curPos.y += 20;
    combo = addComboBox(ID_cbMapType, curPos, comboSize, TextureColor::Grey, NormalFont, 100);
    for(unsigned i = 0; i < desc.landscapes.size(); i++)
        combo->addItem(_(desc.get(DescIdx<LandscapeDesc>(i)).name));

    curPos.y += 30;
    addText(ID_txtMountainDist, curPos, _("HQ distance to mountain"), COLOR_YELLOW, FontStyle{}, NormalFont);
    curPos.y += 20;
    combo = addComboBox(ID_cbMountainDist, curPos, comboSize, TextureColor::Grey, NormalFont, 100);
    combo->addItem(_("Close"));
    combo->addItem(_("Normal"));
    combo->addItem(_("Far"));
    combo->addItem(_("Very far"));

    curPos.y += 30;
    addText(ID_txtIslands, curPos, _("Islands"), COLOR_YELLOW, FontStyle{}, NormalFont);
    curPos.y += 20;
    combo = addComboBox(ID_cbIslands, curPos, comboSize, TextureColor::Grey, NormalFont, 100);
    combo->addItem(_("Few"));
    combo->addItem(_("Medium"));
    combo->addItem(_("Many"));

    constexpr int pgrOffset = 120;
    curPos.y += 35;
    addText(ID_txtGold, curPos, _("Gold:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    addProgress(ID_pgGoldRatio, DrawPoint(pgrOffset, curPos.y - 5), progressSize, TextureColor::Grey, 139, 138, 100);
    curPos.y += 30;
    addText(ID_txtIron, curPos, _("Iron:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    addProgress(ID_pgIronRatio, DrawPoint(pgrOffset, curPos.y - 5), progressSize, TextureColor::Grey, 139, 138, 100);
    curPos.y += 30;
    addText(ID_txtCoal, curPos, _("Coal:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    addProgress(ID_pgCoalRatio, DrawPoint(pgrOffset, curPos.y - 5), progressSize, TextureColor::Grey, 139, 138, 100);
    curPos.y += 30;
    addText(ID_txtGranite, curPos, _("Granite:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    addProgress(ID_pgGraniteRatio, DrawPoint(pgrOffset, curPos.y - 5), progressSize, TextureColor::Grey, 139, 138, 100);
    curPos.y += 30;
    addText(ID_txtRivers, curPos, _("Rivers:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    addProgress(ID_pgRivers, DrawPoint(pgrOffset, curPos.y - 5), progressSize, TextureColor::Grey, 139, 138, 100);
    curPos.y += 30;
    addText(ID_txtTrees, curPos, _("Trees:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    addProgress(ID_pgTrees, DrawPoint(pgrOffset, curPos.y - 5), progressSize, TextureColor::Grey, 139, 138, 100);
    curPos.y += 30;
    addText(ID_txtMountains, curPos, _("Mountains:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    addProgress(ID_pgMountains, DrawPoint(pgrOffset, curPos.y - 5), progressSize, TextureColor::Grey, 139, 138, 100);
    curPos.y += 30;
    addText(ID_txtDesert, curPos, _("Desert:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    addProgress(ID_pgDesert, DrawPoint(pgrOffset, curPos.y - 5), progressSize, TextureColor::Grey, 139, 138, 100);
    curPos.y += 30;
    addText(ID_txtSwamp, curPos, _("Swamp:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    addProgress(ID_pgSwamp, DrawPoint(pgrOffset, curPos.y - 5), progressSize, TextureColor::Grey, 139, 138, 100);
    curPos.y += 30;
    addText(ID_txtStonePiles, curPos, _("Stone piles:"), COLOR_YELLOW, FontStyle{}, NormalFont);
    addProgress(ID_pgStonePiles, DrawPoint(pgrOffset, curPos.y - 5), progressSize, TextureColor::Grey, 139, 138, 100);

    curPos.y += 25;
    addCheckBox(ID_cbSpecialObjects, curPos, comboSize, TextureColor::Grey, _("Special Objects"), NormalFont, false);

    curPos.y += 35;
    addTextButton(ID_btBack, curPos, buttonSize, TextureColor::Red1, _("Back"), NormalFont);
    addTextButton(ID_btApply, DrawPoint(130, curPos.y), buttonSize, TextureColor::Green2, _("Apply"), NormalFont);

    reset();
}

void iwMapGenerator::msgButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case ID_btBack: close(); break;
        case ID_btApply:
            apply();
            close();
            break;
    }
}

void iwMapGenerator::apply()
{
    mapSettings.numPlayers = getCtrl<ctrlComboBox>(ID_cbNumPlayers)->getSelection().value() + 2;
    mapSettings.ratioGold = getCtrl<ctrlProgress>(ID_pgGoldRatio)->getPosition();
    mapSettings.ratioIron = getCtrl<ctrlProgress>(ID_pgIronRatio)->getPosition();
    mapSettings.ratioCoal = getCtrl<ctrlProgress>(ID_pgCoalRatio)->getPosition();
    mapSettings.ratioGranite = getCtrl<ctrlProgress>(ID_pgGraniteRatio)->getPosition();
    mapSettings.rivers = getCtrl<ctrlProgress>(ID_pgRivers)->getPosition();
    mapSettings.trees = getCtrl<ctrlProgress>(ID_pgTrees)->getPosition();
    mapSettings.mountains = getCtrl<ctrlProgress>(ID_pgMountains)->getPosition();
    mapSettings.desert = getCtrl<ctrlProgress>(ID_pgDesert)->getPosition();
    mapSettings.swamp = getCtrl<ctrlProgress>(ID_pgSwamp)->getPosition();
    mapSettings.stonePiles = getCtrl<ctrlProgress>(ID_pgStonePiles)->getPosition();
    mapSettings.specialObjects = getCtrl<ctrlCheck>(ID_cbSpecialObjects)->isChecked();

    switch(getCtrl<ctrlComboBox>(ID_cbMountainDist)->getSelection().value())
    {
        case 0: mapSettings.mountainDistance = MountainDistance::Close; break;
        case 1: mapSettings.mountainDistance = MountainDistance::Normal; break;
        case 2: mapSettings.mountainDistance = MountainDistance::Far; break;
        case 3: mapSettings.mountainDistance = MountainDistance::VeryFar; break;
    }
    switch(getCtrl<ctrlComboBox>(ID_cbMapStyle)->getSelection().value())
    {
        case 0: mapSettings.style = MapStyle::Water; break;
        case 1: mapSettings.style = MapStyle::Land; break;
        case 2: mapSettings.style = MapStyle::Mixed; break;
    }
    mapSettings.size.x =
      s25util::fromStringClassic<unsigned>(getCtrl<ctrlComboBox>(ID_cbMapSizeX)->getSelectedText().value_or("128"));
    mapSettings.size.y =
      s25util::fromStringClassic<unsigned>(getCtrl<ctrlComboBox>(ID_cbMapSizeY)->getSelectedText().value_or("128"));
    switch(getCtrl<ctrlComboBox>(ID_cbIslands)->getSelection().value())
    {
        case 0: mapSettings.islands = IslandAmount::Few; break;
        case 1: mapSettings.islands = IslandAmount::Normal; break;
        case 2: mapSettings.islands = IslandAmount::Many; break;
    }
    const auto& mapType = getCtrl<ctrlComboBox>(ID_cbMapType)->getSelection();
    if(mapType)
        mapSettings.type = DescIdx<LandscapeDesc>(*mapType);
}

void iwMapGenerator::reset()
{
    getCtrl<ctrlComboBox>(ID_cbNumPlayers)
      ->setSelection(std::min(mapSettings.numPlayers, MAX_PLAYERS) - 2); // List starts at 2 players

    getCtrl<ctrlProgress>(ID_pgGoldRatio)->setPosition(mapSettings.ratioGold);
    getCtrl<ctrlProgress>(ID_pgIronRatio)->setPosition(mapSettings.ratioIron);
    getCtrl<ctrlProgress>(ID_pgCoalRatio)->setPosition(mapSettings.ratioCoal);
    getCtrl<ctrlProgress>(ID_pgGraniteRatio)->setPosition(mapSettings.ratioGranite);
    getCtrl<ctrlProgress>(ID_pgRivers)->setPosition(mapSettings.rivers);
    getCtrl<ctrlProgress>(ID_pgTrees)->setPosition(mapSettings.trees);
    getCtrl<ctrlProgress>(ID_pgMountains)->setPosition(mapSettings.mountains);
    getCtrl<ctrlProgress>(ID_pgDesert)->setPosition(mapSettings.desert);
    getCtrl<ctrlProgress>(ID_pgSwamp)->setPosition(mapSettings.swamp);
    getCtrl<ctrlProgress>(ID_pgStonePiles)->setPosition(mapSettings.stonePiles);
    getCtrl<ctrlCheck>(ID_cbSpecialObjects)->setChecked(mapSettings.specialObjects);

    auto* combo = getCtrl<ctrlComboBox>(ID_cbMountainDist);
    switch(mapSettings.mountainDistance)
    {
        case MountainDistance::Close: combo->setSelection(0); break;
        case MountainDistance::Normal: combo->setSelection(1); break;
        case MountainDistance::Far: combo->setSelection(2); break;
        case MountainDistance::VeryFar: combo->setSelection(3); break;
    }

    combo = getCtrl<ctrlComboBox>(ID_cbMapStyle);
    switch(mapSettings.style)
    {
        case MapStyle::Water: combo->setSelection(0); break;
        case MapStyle::Land: combo->setSelection(1); break;
        case MapStyle::Mixed: combo->setSelection(2); break;
    }

    const auto strSizeX = s25util::toStringClassic(mapSettings.size.x);
    const auto strSizeY = s25util::toStringClassic(mapSettings.size.y);
    auto* cbSizeX = getCtrl<ctrlComboBox>(ID_cbMapSizeX);
    auto* cbSizeY = getCtrl<ctrlComboBox>(ID_cbMapSizeY);
    const auto numSizes = cbSizeX->getNumItems();
    RTTR_Assert(numSizes == cbSizeY->getNumItems());
    for(unsigned i = 0; i < numSizes; ++i)
    {
        if(cbSizeX->getText(i) == strSizeX)
            cbSizeX->setSelection(i);
        if(cbSizeY->getText(i) == strSizeY)
            cbSizeY->setSelection(i);
    }

    combo = getCtrl<ctrlComboBox>(ID_cbIslands);
    switch(mapSettings.islands)
    {
        case IslandAmount::Few: combo->setSelection(0); break;
        case IslandAmount::Normal: combo->setSelection(1); break;
        case IslandAmount::Many: combo->setSelection(2); break;
    }

    getCtrl<ctrlComboBox>(ID_cbMapType)->setSelection(mapSettings.type.value);
}
