//
// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwMilitaryBuilding.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
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
#include <set>

iwMilitaryBuilding::iwMilitaryBuilding(GameWorldView& gwv, GameCommandFactory& gcFactory, nobMilitary* const building)
    : IngameWindow(CGI_BUILDING + MapBase::CreateGUIID(building->GetPos()), IngameWindow::posAtMouse, Extent(226, 194),
                   _(BUILDING_NAMES[building->GetBuildingType()]), LOADER.GetImageN("resource", 41)),
      gwv(gwv), gcFactory(gcFactory), building(building)
{
    unsigned btOffset = 0;
    if(gwv.GetWorld().GetGGS().getSelection(AddonId::MILITARY_CONTROL) == 2)
    {
        btOffset = 154;
        Resize(Extent(226, 348));
    }

    // Schwert
    AddImage(0, DrawPoint(28, 39), LOADER.GetMapTexture(2298));
    AddImage(1, DrawPoint(28, 39), LOADER.GetWareTex(GoodType::Sword));

    // Schild
    AddImage(2, DrawPoint(196, 39), LOADER.GetMapTexture(2298));
    AddImage(3, DrawPoint(196, 39), LOADER.GetWareTex(GoodType::ShieldRomans));

    // Hilfe
    AddImageButton(4, DrawPoint(16, btOffset + 147), Extent(30, 32), TextureColor::Grey, LOADER.GetImageN("io", 225),
                   _("Help"));
    // Abreißen
    AddImageButton(5, DrawPoint(50, btOffset + 147), Extent(34, 32), TextureColor::Grey, LOADER.GetImageN("io", 23),
                   _("Demolish house"));
    // Gold an/aus (227,226)
    AddImageButton(6, DrawPoint(90, btOffset + 147), Extent(32, 32), TextureColor::Grey,
                   LOADER.GetImageN("io", ((building->IsGoldDisabledVirtual()) ? 226 : 227)), _("Gold delivery"));
    // "Gehe Zu Ort"
    AddImageButton(7, DrawPoint(179, btOffset + 147), Extent(30, 32), TextureColor::Grey, LOADER.GetImageN("io", 107),
                   _("Go to place"));

    // Gebäudebild
    AddImage(8, DrawPoint(117, 114), &building->GetBuildingImage());
    // "Go to next" (building of same type)
    AddImageButton(9, DrawPoint(179, btOffset + 115), Extent(30, 32), TextureColor::Grey,
                   LOADER.GetImageN("io_new", 11), _("Go to next military building"));

    if(gwv.GetWorld().GetGGS().getSelection(AddonId::MILITARY_CONTROL) == 1)
    {
        // Minimal troop controls
        AddImageButton(10, DrawPoint(126, btOffset + 147), Extent(32, 32), TextureColor::Grey,
                       LOADER.GetImageN("io_new", 12), _("Send max rank soldiers to a warehouse"));
    } else if(gwv.GetWorld().GetGGS().getSelection(AddonId::MILITARY_CONTROL) == 2)
    {
        // Full troop controls
        AddImageButton(10, DrawPoint(126, btOffset + 147), Extent(32, 32), TextureColor::Grey,
                       LOADER.GetImageN("io_new", 12), _("Send soldiers home"));

        const unsigned Y_SPACING = 30;
        for(unsigned i = 0; i < NUM_SOLDIER_RANKS; ++i)
        {
            // Minus
            AddImageButton(11 + (4 * i), DrawPoint(69, 136 + Y_SPACING * i), Extent(24, 24), TextureColor::Red1,
                           LOADER.GetImageN("io", 139), _("Fewer"));
            // Background
            AddImage(12 + (4 * i), DrawPoint(113, 148 + Y_SPACING * i), LOADER.GetMapTexture(2298));
            // Rank image
            AddImage(13 + (4 * i), DrawPoint(113, 148 + Y_SPACING * i), LOADER.GetMapTexture(2321 + i));
            // Plus
            AddImageButton(14 + (4 * i), DrawPoint(133, 136 + Y_SPACING * i), Extent(24, 24), TextureColor::Green2,
                           LOADER.GetImageN("io", 138), _("More"));
        }
    }
}

void iwMilitaryBuilding::Draw_()
{
    IngameWindow::Draw_();

    if(IsMinimized())
        return;

    // Schwarzer Untergrund für Goldanzeige
    const unsigned maxCoinCt = building->GetMaxCoinCt();
    DrawPoint goldPos = GetDrawPos() + DrawPoint((GetSize().x - 22 * maxCoinCt) / 2, 60);
    DrawRectangle(Rect(goldPos, Extent(22 * maxCoinCt, 24)), 0x96000000);
    // Gold
    goldPos += DrawPoint(12, 12);
    for(unsigned short i = 0; i < maxCoinCt; ++i)
    {
        LOADER.GetMapTexture(2278)->DrawFull(goldPos, (i >= building->GetNumCoins() ? 0xFFA0A0A0 : 0xFFFFFFFF));
        goldPos.x += 22;
    }

    // Sammeln aus der Rausgeh-Liste und denen, die wirklich noch drinne sind
    boost::container::flat_set<const nofSoldier*, ComparatorSoldiersByRank> soldiers;
    for(const auto& soldier : building->GetTroops())
        soldiers.insert(&soldier);
    for(const noFigure& fig : building->GetLeavingFigures())
    {
        const GO_Type figType = fig.GetGOT();
        if(figType == GO_Type::NofAttacker || figType == GO_Type::NofAggressivedefender
           || figType == GO_Type::NofDefender || figType == GO_Type::NofPassivesoldier)
        {
            soldiers.insert(static_cast<const nofSoldier*>(&fig));
        }
    }

    const unsigned maxSoldierCt = building->GetMaxTroopsCt();
    DrawPoint troopsPos = GetDrawPos() + DrawPoint((GetSize().x - 22 * maxSoldierCt) / 2, 98);
    // Schwarzer Untergrund für Soldatenanzeige
    DrawRectangle(Rect(troopsPos, Extent(22 * maxSoldierCt, 24)), 0x96000000);

    // Soldaten zeichnen
    DrawPoint curTroopsPos = troopsPos + DrawPoint(12, 12);
    for(const auto* soldier : soldiers)
    {
        LOADER.GetMapTexture(2321 + soldier->GetRank())->DrawFull(curTroopsPos);
        curTroopsPos.x += 22;
    }

    // Draw health above soldiers
    if(gwv.GetWorld().GetGGS().isEnabled(AddonId::MILITARY_HITPOINTS))
    {
        DrawPoint healthPos = troopsPos - DrawPoint(0, 14);

        // black background for hitpoints
        DrawRectangle(Rect(healthPos, Extent(22 * maxSoldierCt, 14)), 0x96000000);

        healthPos += DrawPoint(12, 2);
        for(const auto* soldier : soldiers)
        {
            auto hitpoints = static_cast<int>(soldier->GetHitpoints());
            auto maxHitpoints = static_cast<int>(HITPOINTS[soldier->GetRank()]);
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
            NormalFont->Draw(healthPos, std::to_string(hitpoints), FontStyle::CENTER, hitpointsColour);
            healthPos.x += 22;
        }
    }

    if(gwv.GetWorld().GetGGS().getSelection(AddonId::MILITARY_CONTROL) == 2)
    {
        const unsigned Y_SPACING = 30;
        for(unsigned i = 0; i < NUM_SOLDIER_RANKS; ++i)
            NormalFont->Draw(GetDrawPos() + DrawPoint(101, 136 + Y_SPACING * i),
                             std::to_string(building->GetTroopLimit(i)), FontStyle::LEFT, COLOR_YELLOW);
    }
}

void iwMilitaryBuilding::Msg_ButtonClick(const unsigned ctrl_id)
{
    switch(ctrl_id)
    {
        case 4: // Hilfe
        {
            WINDOWMANAGER.ReplaceWindow(
              std::make_unique<iwHelp>(_(BUILDING_HELP_STRINGS[building->GetBuildingType()])));
        }
        break;
        case 5: // Gebäude abbrennen
        {
            // Darf das Gebäude abgerissen werden?
            if(!building->IsDemolitionAllowed())
            {
                // Messagebox anzeigen
                DemolitionNotAllowed(gwv.GetWorld().GetGGS());
            } else
            {
                // Abreißen?
                Close();
                WINDOWMANAGER.Show(std::make_unique<iwDemolishBuilding>(gwv, building));
            }
        }
        break;
        case 6: // Gold einstellen/erlauben
        {
            if(!GAMECLIENT.IsReplayModeOn())
            {
                // NC senden
                if(gcFactory.SetCoinsAllowed(building->GetPos(), building->IsGoldDisabledVirtual()))
                {
                    // visuell anzeigen
                    building->ToggleCoinsVirtual();
                    // anderes Bild auf dem Button
                    if(building->IsGoldDisabledVirtual())
                        GetCtrl<ctrlImageButton>(6)->SetImage(LOADER.GetImageN("io", 226));
                    else
                        GetCtrl<ctrlImageButton>(6)->SetImage(LOADER.GetImageN("io", 227));
                }
            }
        }
        break;
        case 7: // "Gehe Zu Ort"
        {
            gwv.MoveToMapPt(building->GetPos());
        }
        break;
        case 9: // go to next of same type
        {
            const std::list<nobMilitary*>& militaryBuildings =
              gwv.GetWorld().GetPlayer(building->GetPlayer()).GetBuildingRegister().GetMilitaryBuildings();
            // go through list once we get to current building -> open window for the next one and go to next location
            auto it = helpers::find_if(
              militaryBuildings, [bldPos = building->GetPos()](const auto* it) { return it->GetPos() == bldPos; });
            if(it != militaryBuildings.end()) // got to current building in the list?
            {
                // close old window, open new window (todo: only open if it isnt already open), move to location of next
                // building
                Close();
                ++it;
                if(it == militaryBuildings.end()) // was last entry in list -> goto first
                    it = militaryBuildings.begin();
                gwv.MoveToMapPt((*it)->GetPos());
                WINDOWMANAGER.ReplaceWindow(std::make_unique<iwMilitaryBuilding>(gwv, gcFactory, *it)).SetPos(GetPos());
                break;
            }
        }
        break;
        case 10:
        {
            if(gwv.GetWorld().GetGGS().getSelection(AddonId::MILITARY_CONTROL) == 1)
            {
                // Send the highest rank soldiers in this building home and get new soldiers
                if(building->GetNumTroops() > 1)
                {
                    auto maxRank = building->GetTroops().back().GetRank();
                    gcFactory.SetTroopLimit(building->GetPos(), maxRank, 0);
                    gcFactory.SetTroopLimit(building->GetPos(), maxRank, building->GetMaxTroopsCt());
                }
            } else
            {
                // Send all soldiers home except one
                gcFactory.SetTroopLimit(building->GetPos(), 0, 1);
                for(unsigned rank = 1; rank < NUM_SOLDIER_RANKS; ++rank)
                    gcFactory.SetTroopLimit(building->GetPos(), rank, 0);
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
                    const unsigned count = building->GetTroopLimit(rank);
                    if(count < building->GetMaxTroopsCt())
                        gcFactory.SetTroopLimit(building->GetPos(), rank, count + 1);
                } else
                {
                    RTTR_Assert(id % 4 == 0);
                    const unsigned rank = id / 4;
                    const unsigned count = building->GetTroopLimit(rank);
                    if(count > 0)
                        gcFactory.SetTroopLimit(building->GetPos(), rank, count - 1);
                }
            }
        }
    }
}

void iwMilitaryBuilding::DemolitionNotAllowed(const GlobalGameSettings& ggs)
{
    // Meldung auswählen, je nach Einstellung
    std::string msg;
    switch(ggs.getSelection(AddonId::DEMOLITION_PROHIBITION))
    {
        default: RTTR_Assert(false); break;
        case 1: msg = _("Demolition ist not allowed because the building is under attack!"); break;
        case 2: msg = _("Demolition ist not allowed because the building is located in border area!"); break;
    }

    WINDOWMANAGER.Show(std::make_unique<iwMsgbox>(_("Demolition not possible"), msg, nullptr, MsgboxButton::Ok,
                                                  MsgboxIcon::ExclamationRed));
}
