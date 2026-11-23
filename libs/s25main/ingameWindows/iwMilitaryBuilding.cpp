//
// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwMilitaryBuilding.h"
#include "ChurchLoader.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "LeatherLoader.h"
#include "Loader.h"
#include "WindowManager.h"
#include "addons/const_addons.h"
#include "buildings/nobMilitary.h"
#include "controls/ctrlImageButton.h"
#include "figures/nofPassiveSoldier.h"
#include "helpers/containerUtils.h"
#include "iwDemolishBuilding.h"
#include "iwHelp.h"
#include "iwMsgbox.h"
#include "network/GameClient.h"
#include "ogl/FontStyle.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glFont.h"
#include "world/GameWorldBase.h"
#include "world/GameWorldView.h"
#include "gameData/BuildingConsts.h"
#include "gameData/MilitaryConsts.h"
#include "gameData/const_gui_ids.h"

constexpr unsigned ARMOR_ENABLE_BUTTON_ID = 10 + NUM_SOLDIER_RANKS * 4 + 1;
constexpr unsigned HEIGHT_OF_ROW = 38;

iwMilitaryBuilding::iwMilitaryBuilding(GameWorldView& gwv, GameCommandFactory& gcFactory, nobMilitary* const building)
    : IngameWindow(CGI_BUILDING + MapBase::createGUIID(building->getPos()), IngameWindow::posAtMouse, Extent(226, 194),
                   _(BUILDING_NAMES[building->getBuildingType()]), LOADER.getImageN("resource", 41)),
      gwv(gwv), gcFactory(gcFactory), building(building)
{
    const auto addonStatusMilitaryControl = gwv.getWorld().getGGS().getSelection(AddonId::MILITARY_CONTROL);

    DrawPoint btOffset(0, 0);
    if(addonStatusMilitaryControl == 2)
        btOffset.y += 154;

    DrawPoint btOffsetLeatherAddon(0, 0);
    if(leatheraddon::isAddonActive(gwv.getWorld()))
    {
        btOffsetLeatherAddon = DrawPoint(addonStatusMilitaryControl == 0 ? 0 : 36, HEIGHT_OF_ROW);
        btOffset += btOffsetLeatherAddon;
    }

    resize(Extent(226 + btOffset.x, 194 + btOffset.y));

    // Schwert
    addImage(0, DrawPoint(28, 39), LOADER.getMapTexture(2298));
    addImage(1, DrawPoint(28, 39), LOADER.getWareTex(GoodType::Sword));

    // Schild
    addImage(2, DrawPoint(196 + btOffset.x, 39), LOADER.getMapTexture(2298));
    addImage(3, DrawPoint(196 + btOffset.x, 39), LOADER.getWareTex(GoodType::ShieldRomans));

    // Hilfe
    addImageButton(4, DrawPoint(16, btOffset.y + 147), Extent(30, 32), TextureColor::Grey, LOADER.getImageN("io", 225),
                   _("Help"));
    // Abreißen
    addImageButton(5, DrawPoint(50, btOffset.y + 147), Extent(34, 32), TextureColor::Grey, LOADER.getImageN("io", 23),
                   _("Demolish house"));
    // Gold an/aus (227,226)
    addImageButton(6, DrawPoint(90, btOffset.y + 147), Extent(32, 32), TextureColor::Grey,
                   LOADER.getImageN("io", ((building->isGoldDisabledVirtual()) ? 226 : 227)), _("Gold delivery"));
    // "Gehe Zu Ort"
    addImageButton(7, DrawPoint(179 + btOffset.x, btOffset.y + 147), Extent(30, 32), TextureColor::Grey,
                   LOADER.getImageN("io", 107), _("Go to place"));

    // Gebäudebild
    addImage(8, DrawPoint(117 + btOffset.x / 2, 114), &building->getBuildingImage());
    // "Go to next" (building of same type)
    addImageButton(9, DrawPoint(179 + btOffset.x, btOffset.y + 115), Extent(30, 32), TextureColor::Grey,
                   LOADER.getImageN("io_new", 11), _("Go to next military building"));

    if(leatheraddon::isAddonActive(gwv.getWorld()))
    {
        addImageButton(
          ARMOR_ENABLE_BUTTON_ID, DrawPoint(126 + btOffsetLeatherAddon.x, btOffset.y + 147), Extent(32, 32),
          TextureColor::Grey,
          LOADER.getImageN("leather_bobs", leatheraddon::bobIndex[building->isArmorAllowedVirtual() ?
                                                                    leatheraddon::BobType::ArmorDeliverIcon :
                                                                    leatheraddon::BobType::DisableDeliveryArmorIcon]),
          _("Armor delivery"));
    }

    if(addonStatusMilitaryControl == 1)
    {
        // Minimal troop controls
        addImageButton(10, DrawPoint(126, btOffset.y + 147), Extent(32, 32), TextureColor::Grey,
                       LOADER.getImageN("io_new", 12), _("Send max rank soldiers to a warehouse"));
    } else if(addonStatusMilitaryControl == 2)
    {
        // Full troop controls
        addImageButton(10, DrawPoint(126, btOffset.y + 147), Extent(32, 32), TextureColor::Grey,
                       LOADER.getImageN("io_new", 12), _("Send soldiers home"));

        const unsigned Y_SPACING = 30;
        for(unsigned i = 0; i < NUM_SOLDIER_RANKS; ++i)
        {
            // Minus
            addImageButton(11 + (4 * i), DrawPoint(69 + btOffset.x / 2, 136 + btOffsetLeatherAddon.y + Y_SPACING * i),
                           Extent(24, 24), TextureColor::Red1, LOADER.getImageN("io", 139), _("Fewer"));
            // Background
            addImage(12 + (4 * i), DrawPoint(113 + btOffset.x / 2, 148 + btOffsetLeatherAddon.y + Y_SPACING * i),
                     LOADER.getMapTexture(2298));
            // Rank image
            addImage(13 + (4 * i), DrawPoint(113 + btOffset.x / 2, 148 + btOffsetLeatherAddon.y + Y_SPACING * i),
                     LOADER.getMapTexture(2321 + i));
            // Plus
            addImageButton(14 + (4 * i), DrawPoint(133 + btOffset.x / 2, 136 + btOffsetLeatherAddon.y + Y_SPACING * i),
                           Extent(24, 24), TextureColor::Green2, LOADER.getImageN("io", 138), _("More"));
        }
    }
}

void iwMilitaryBuilding::drawContent()
{
    unsigned btOffsetY = 60;

    // Schwarzer Untergrund für Goldanzeige
    const unsigned maxCoinCt = building->getMaxCoinCt();
    DrawPoint goldPos = getDrawPos() + DrawPoint((getSize().x - 22 * maxCoinCt) / 2, btOffsetY);
    drawRectangle(Rect(goldPos, Extent(22 * maxCoinCt, 24)), 0x96000000);
    // Gold
    goldPos += DrawPoint(12, 12);
    for(unsigned short i = 0; i < maxCoinCt; ++i)
    {
        LOADER.getMapTexture(2278)->drawFull(goldPos, (i >= building->getNumCoins() ? 0xFFA0A0A0 : 0xFFFFFFFF));
        goldPos.x += 22;
    }
    btOffsetY += HEIGHT_OF_ROW;

    auto* armorWareIcon =
      LOADER.getImageN("leather_bobs", leatheraddon::bobIndex[leatheraddon::BobType::ArmorWareIcon]);

    if(leatheraddon::isAddonActive(gwv.getWorld()))
    {
        // Black background for armor display
        const unsigned maxArmorCt = building->getMaxArmorCt();
        DrawPoint armorPos = getDrawPos() + DrawPoint((getSize().x - 22 * maxArmorCt) / 2, btOffsetY);
        drawRectangle(Rect(armorPos, Extent(22 * maxArmorCt, 24)), 0x96000000);
        // Armor
        armorPos += DrawPoint(12, 12);
        for(unsigned short i = 0; i < maxArmorCt; ++i)
        {
            armorWareIcon->drawFull(armorPos, (i >= building->getNumArmor() ? 0xFFA0A0A0 : 0xFFFFFFFF));
            armorPos.x += 22;
        }
        btOffsetY += HEIGHT_OF_ROW;
    }

    // Sammeln aus der Rausgeh-Liste und denen, die wirklich noch drinne sind
    boost::container::flat_set<const nofSoldier*, ComparatorSoldiersByRank> soldiers;
    for(const auto& soldier : building->getTroops())
        soldiers.insert(&soldier);
    for(const noFigure& fig : building->getLeavingFigures())
    {
        const GO_Type figType = fig.getGOT();
        if(figType == GO_Type::NofAttacker || figType == GO_Type::NofAggressivedefender
           || figType == GO_Type::NofDefender || figType == GO_Type::NofPassivesoldier)
        {
            soldiers.insert(static_cast<const nofSoldier*>(&fig));
        }
    }

    const unsigned maxSoldierCt = building->getMaxTroopsCt();
    DrawPoint troopsPos = getDrawPos() + DrawPoint((getSize().x - 22 * maxSoldierCt) / 2, btOffsetY);
    // Schwarzer Untergrund für Soldatenanzeige
    drawRectangle(Rect(troopsPos, Extent(22 * maxSoldierCt, 24)), 0x96000000);

    // Soldaten zeichnen
    DrawPoint curTroopsPos = troopsPos + DrawPoint(12, 12);
    for(const auto* soldier : soldiers)
    {
        if(leatheraddon::isAddonActive(gwv.getWorld()) && soldier->hasArmor())
            armorWareIcon->drawFull(curTroopsPos, 0xFFA0A0A0);
        LOADER.getMapTexture(2321 + soldier->getRank())->drawFull(curTroopsPos);
        if(churchaddon::isAddonActive(gwv.getWorld()) && soldier->hasHealing())
        {
            if(ITexture* doveTex = churchaddon::getDoveTex())
                doveTex->drawFull(curTroopsPos + DrawPoint(6, -6));
        }
        curTroopsPos.x += 22;
    }

    // Draw health above soldiers
    if(gwv.getWorld().getGGS().isEnabled(AddonId::MILITARY_HITPOINTS))
    {
        DrawPoint healthPos = troopsPos - DrawPoint(0, 14);

        // black background for hitpoints
        drawRectangle(Rect(healthPos, Extent(22 * maxSoldierCt, 14)), 0x96000000);

        healthPos += DrawPoint(12, 2);
        for(const auto* soldier : soldiers)
        {
            auto hitpoints = static_cast<int>(soldier->getHitpoints());
            auto maxHitpoints = static_cast<int>(soldier->getMaxHitpoints());
            unsigned hitpointsColour;
            if(hitpoints <= maxHitpoints / 2)
                hitpointsColour = COLOR_RED;
            else
            {
                if(hitpoints == maxHitpoints)
                    hitpointsColour = COLOR_GREEN;
                else
                    hitpointsColour = COLOR_ORANGE;
            }
            NormalFont->draw(healthPos, std::to_string(hitpoints), FontStyle::CENTER, hitpointsColour);
            if(leatheraddon::isAddonActive(gwv.getWorld()) && soldier->hasArmor())
            {
                SmallFont->draw(healthPos + DrawPoint(7, 0), "+", FontStyle::CENTER, hitpointsColour);
                SmallFont->draw(healthPos + DrawPoint(10, 0), "1", FontStyle::CENTER, hitpointsColour);
            }
            healthPos.x += 22;
        }
    }
    btOffsetY += HEIGHT_OF_ROW;

    if(gwv.getWorld().getGGS().getSelection(AddonId::MILITARY_CONTROL) == 2)
    {
        const unsigned Y_SPACING = 30;
        for(unsigned i = 0; i < NUM_SOLDIER_RANKS; ++i)
            NormalFont->draw(getDrawPos() + DrawPoint(getSize().x / 2 - 12, btOffsetY + Y_SPACING * i),
                             std::to_string(building->getTroopLimit(i)), FontStyle::LEFT, COLOR_YELLOW);
    }
}

void iwMilitaryBuilding::msgButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 4: // Hilfe
        {
            WINDOWMANAGER.replaceWindow(
              std::make_unique<iwHelp>(_(BUILDING_HELP_STRINGS[building->getBuildingType()])));
        }
        break;
        case 5: // Gebäude abbrennen
        {
            // Darf das Gebäude abgerissen werden?
            if(!building->isDemolitionAllowed())
            {
                // Messagebox anzeigen
                demolitionNotAllowed(gwv.getWorld().getGGS());
            } else
            {
                // Abreißen?
                close();
                WINDOWMANAGER.show(std::make_unique<iwDemolishBuilding>(gwv, building));
            }
        }
        break;
        case 6: // Gold einstellen/erlauben
        {
            if(!GAMECLIENT.isReplayModeOn())
            {
                // NC senden
                if(gcFactory.SetCoinsAllowed(building->getPos(), building->isGoldDisabledVirtual()))
                {
                    // visuell anzeigen
                    building->toggleCoinsVirtual();
                    // anderes Bild auf dem Button
                    if(building->isGoldDisabledVirtual())
                        getCtrl<ctrlImageButton>(6)->setImage(LOADER.getImageN("io", 226));
                    else
                        getCtrl<ctrlImageButton>(6)->setImage(LOADER.getImageN("io", 227));
                }
            }
        }
        break;
        case 7: // "Gehe Zu Ort"
        {
            gwv.moveToMapPt(building->getPos());
        }
        break;
        case 9: // go to next of same type
        {
            const std::list<nobMilitary*>& militaryBuildings =
              gwv.getWorld().getPlayer(building->getPlayer()).getBuildingRegister().getMilitaryBuildings();
            // go through list once we get to current building -> open window for the next one and go to next location
            auto it = helpers::findIf(militaryBuildings,
                                      [bldPos = building->getPos()](const auto* it) { return it->getPos() == bldPos; });
            if(it != militaryBuildings.end()) // got to current building in the list?
            {
                // close old window, open new window (todo: only open if it isnt already open), move to location of next
                // building
                close();
                ++it;
                if(it == militaryBuildings.end()) // was last entry in list -> goto first
                    it = militaryBuildings.begin();
                gwv.moveToMapPt((*it)->getPos());
                WINDOWMANAGER.replaceWindow(std::make_unique<iwMilitaryBuilding>(gwv, gcFactory, *it)).setPos(getPos());
                break;
            }
        }
        break;
        case 10:
        {
            if(gwv.getWorld().getGGS().getSelection(AddonId::MILITARY_CONTROL) == 1)
            {
                // Send the highest rank soldiers in this building home and get new soldiers
                if(building->getNumTroops() > 1)
                {
                    auto maxRank = building->getTroops().back().getRank();
                    gcFactory.SetTroopLimit(building->getPos(), maxRank, 0);
                    gcFactory.SetTroopLimit(building->getPos(), maxRank, building->getMaxTroopsCt());
                }
            } else
            {
                // Send all soldiers home except one
                gcFactory.SetTroopLimit(building->getPos(), 0, 1);
                for(unsigned rank = 1; rank < NUM_SOLDIER_RANKS; ++rank)
                    gcFactory.SetTroopLimit(building->getPos(), rank, 0);
            }
        }
        break;
        case ARMOR_ENABLE_BUTTON_ID:
        {
            if(!GAMECLIENT.isReplayModeOn())
            {
                if(gcFactory.SetArmorAllowed(building->getPos(), !building->isArmorAllowedVirtual()))
                {
                    building->toggleArmorVirtual();
                    getCtrl<ctrlImageButton>(ctrl_id)->setImage(LOADER.getImageN(
                      "leather_bobs", leatheraddon::bobIndex[building->isArmorAllowedVirtual() ?
                                                               leatheraddon::BobType::ArmorDeliverIcon :
                                                               leatheraddon::BobType::DisableDeliveryArmorIcon]));
                }
            }
        }
        break;
        default:
        {
            if(ctrl_id > 10)
            {
                const unsigned id = ctrl_id - 11;
                if(id % 4 == 3)
                {
                    const unsigned rank = (id - 3) / 4;
                    const unsigned count = building->getTroopLimit(rank);
                    if(count < building->getMaxTroopsCt())
                        gcFactory.SetTroopLimit(building->getPos(), rank, count + 1);
                } else
                {
                    RTTR_Assert(id % 4 == 0);
                    const unsigned rank = id / 4;
                    const unsigned count = building->getTroopLimit(rank);
                    if(count > 0)
                        gcFactory.SetTroopLimit(building->getPos(), rank, count - 1);
                }
            }
        }
    }
}

void iwMilitaryBuilding::demolitionNotAllowed(const GlobalGameSettings& ggs)
{
    // Meldung auswählen, je nach Einstellung
    std::string msg;
    switch(ggs.getSelection(AddonId::DEMOLITION_PROHIBITION))
    {
        default: RTTR_Assert(false); break;
        case 1: msg = _("Demolition ist not allowed because the building is under attack!"); break;
        case 2: msg = _("Demolition ist not allowed because the building is located in border area!"); break;
    }

    WINDOWMANAGER.show(std::make_unique<iwMsgbox>(_("Demolition not possible"), msg, nullptr, MsgboxButton::Ok,
                                                  MsgboxIcon::ExclamationRed));
}
