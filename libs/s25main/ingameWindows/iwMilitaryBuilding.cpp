//
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
    // Schwert
    AddImage(0, DrawPoint(28, 39), LOADER.GetMapImageN(2298));
    AddImage(1, DrawPoint(28, 39), LOADER.GetWareTex(GoodType::Sword));

    // Schild
    AddImage(2, DrawPoint(196, 39), LOADER.GetMapImageN(2298));
    AddImage(3, DrawPoint(196, 39), LOADER.GetWareTex(GoodType::ShieldRomans));

    // Hilfe
    AddImageButton(4, DrawPoint(16, 147), Extent(30, 32), TC_GREY, LOADER.GetImageN("io", 225), _("Help"));
    // Abreißen
    AddImageButton(5, DrawPoint(50, 147), Extent(34, 32), TC_GREY, LOADER.GetImageN("io", 23), _("Demolish house"));
    // Gold an/aus (227,226)
    AddImageButton(6, DrawPoint(90, 147), Extent(32, 32), TC_GREY,
                   LOADER.GetImageN("io", ((building->IsGoldDisabledVirtual()) ? 226 : 227)), _("Gold delivery"));
    // "Gehe Zu Ort"
    AddImageButton(7, DrawPoint(179, 147), Extent(30, 32), TC_GREY, LOADER.GetImageN("io", 107), _("Go to place"));

    // Gebäudebild
    AddImage(8, DrawPoint(117, 114), &building->GetBuildingImage());
    // "Go to next" (building of same type)
    AddImageButton(9, DrawPoint(179, 115), Extent(30, 32), TC_GREY, LOADER.GetImageN("io_new", 11),
                   _("Go to next military building"));
    // addon military control active? -> show button
    if(gwv.GetWorld().GetGGS().isEnabled(AddonId::MILITARY_CONTROL))
        AddImageButton(10, DrawPoint(124, 147), Extent(30, 32), TC_GREY, LOADER.GetImageN("io_new", 12),
                       _("Send max rank soldiers to a warehouse"));
}

void iwMilitaryBuilding::Draw_()
{
    IngameWindow::Draw_();

    // Schwarzer Untergrund für Goldanzeige
    const unsigned maxCoinCt = building->GetMaxCoinCt();
    DrawPoint goldPos = GetDrawPos() + DrawPoint((GetSize().x - 22 * maxCoinCt) / 2, 60);
    DrawRectangle(Rect(goldPos, Extent(22 * maxCoinCt, 24)), 0x96000000);
    // Gold
    goldPos += DrawPoint(12, 12);
    for(unsigned short i = 0; i < maxCoinCt; ++i)
    {
        LOADER.GetMapImageN(2278)->DrawFull(goldPos, (i >= building->GetNumCoins() ? 0xFFA0A0A0 : 0xFFFFFFFF));
        goldPos.x += 22;
    }

    // Sammeln aus der Rausgeh-Liste und denen, die wirklich noch drinne sind
    std::multiset<const nofSoldier*, ComparatorSoldiersByRank<true>> soldiers(building->GetTroops().begin(),
                                                                              building->GetTroops().end());
    for(const noFigure* fig : building->GetLeavingFigures())
    {
        const GO_Type figType = fig->GetGOT();
        if(figType == GOT_NOF_ATTACKER || figType == GOT_NOF_AGGRESSIVEDEFENDER || figType == GOT_NOF_DEFENDER
           || figType == GOT_NOF_PASSIVESOLDIER)
        {
            soldiers.insert(static_cast<const nofSoldier*>(fig));
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
        LOADER.GetMapImageN(2321 + soldier->GetRank())->DrawFull(curTroopsPos);
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
        case 10: // send home button (addon)
        {
            gcFactory.SendSoldiersHome(building->GetPos());
        }
        break;
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

    WINDOWMANAGER.Show(
      std::make_unique<iwMsgbox>(_("Demolition not possible"), msg, nullptr, MSB_OK, MSB_EXCLAMATIONRED));
}
