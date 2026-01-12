// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "iwShip.h"
#include "DrawPoint.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "LeatherLoader.h"
#include "Loader.h"
#include "Ware.h"
#include "WindowManager.h"
#include "addons/const_addons.h"
#include "controls/ctrlButton.h"
#include "factories/GameCommandFactory.h"
#include "figures/noFigure.h"
#include "figures/nofSoldier.h"
#include "helpers/EnumArray.h"
#include "helpers/EnumRange.h"
#include "iwHelp.h"
#include "ogl/FontStyle.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "ogl/glArchivItem_Bob.h"
#include "ogl/glFont.h"
#include "world/GameWorldBase.h"
#include "world/GameWorldView.h"
#include "world/GameWorldViewer.h"
#include "nodeObjs/noShip.h"
#include "gameData/BuildingConsts.h"
#include "gameData/JobConsts.h"
#include "gameData/NationConsts.h"
#include "gameData/ShieldConsts.h"
#include "gameData/const_gui_ids.h"

iwShip::iwShip(GameWorldView& gwv, GameCommandFactory& gcFactory, const noShip* const ship, const DrawPoint& pos)
    : IngameWindow(CGI_SHIP, pos, Extent(252, 238), _("Ship register"), LOADER.GetImageN("resource", 41)), gwv(gwv),
      gcFactory(gcFactory), player(ship ? ship->GetPlayerId() : gwv.GetViewer().GetPlayerId()),
      ship_id(ship ? gwv.GetWorld().GetPlayer(player).GetShipID(ship) : 0)
{
    AddImage(0, DrawPoint(126, 101), LOADER.GetImageN("io", 228));
    AddImageButton(2, DrawPoint(18, 192), Extent(30, 35), TextureColor::Grey,
                   LOADER.GetImageN("io", 225)); // Viewer: 226 - Hilfe
    AddImageButton(3, DrawPoint(51, 196), Extent(30, 26), TextureColor::Grey,
                   LOADER.GetImageN("io", 102)); // Viewer: 103 - Schnell zurück
    AddImageButton(4, DrawPoint(81, 196), Extent(30, 26), TextureColor::Grey,
                   LOADER.GetImageN("io", 103)); // Viewer: 104 - Zurück
    AddImageButton(5, DrawPoint(111, 196), Extent(30, 26), TextureColor::Grey,
                   LOADER.GetImageN("io", 104)); // Viewer: 105 - Vor
    AddImageButton(6, DrawPoint(141, 196), Extent(30, 26), TextureColor::Grey,
                   LOADER.GetImageN("io", 105)); // Viewer: 106 - Schnell vor
    AddImageButton(7, DrawPoint(181, 196), Extent(30, 26), TextureColor::Grey, LOADER.GetImageN("io", 107),
                   _("Go to place")); // "Gehe Zu Ort"

    // Die Expeditionsweiterfahrbuttons
    AddImageButton(10, DrawPoint(60, 81), Extent(18, 18), TextureColor::Grey, LOADER.GetImageN("io", 187),
                   _("Found colony"))
      ->SetVisible(false);

    constexpr helpers::EnumArray<DrawPoint, ShipDirection> BUTTON_POS = {
      {{60, 61}, {80, 70}, {80, 90}, {60, 101}, {40, 90}, {40, 70}}};
    constexpr helpers::EnumArray<unsigned, ShipDirection> BUTTON_IDs = {{185, 186, 181, 182, 183, 184}};

    // Expedition abbrechen
    AddImageButton(11, DrawPoint(200, 143), Extent(18, 18), TextureColor::Red1, LOADER.GetImageN("io", 40),
                   _("Return to harbor"))
      ->SetVisible(false);

    // Die 6 Richtungen
    for(const auto dir : helpers::EnumRange<ShipDirection>{})
        AddImageButton(12 + rttr::enum_cast(dir), BUTTON_POS[dir], Extent(18, 18), TextureColor::Grey,
                       LOADER.GetImageN("io", BUTTON_IDs[dir]))
          ->SetVisible(false);
}

void iwShip::DrawContent()
{
    static boost::format valByValFmt{"%1%/%2%"};

    const GamePlayer& owner = gwv.GetWorld().GetPlayer(player);
    // Schiff holen
    noShip* ship = (player == 0xff) ? nullptr : owner.GetShipByID(ship_id);

    // Kein Schiff gefunden? Dann erstes Schiff holen
    if(!ship)
    {
        ship_id = 0;
        // Nochmal probieren
        if(player != 0xff)
            ship = owner.GetShipByID(ship_id);
        // Immer noch nicht? Dann gibt es keine Schiffe mehr und wir zeigen eine entsprechende Meldung an
        if(!ship)
        {
            NormalFont->Draw(GetDrawPos() + DrawPoint(GetSize().x / 2, 60), _("No ships available"),
                             FontStyle::CENTER | FontStyle::NO_OUTLINE, COLOR_WINDOWBROWN);
            return;
        }
    }

    // Schiffsname
    NormalFont->Draw(GetDrawPos() + DrawPoint(42, 42), ship->GetName(), FontStyle::NO_OUTLINE, COLOR_WINDOWBROWN);
    // Schiffs-Nr.
    valByValFmt % (ship_id + 1) % owner.GetNumShips();
    NormalFont->Draw(GetDrawPos() + DrawPoint(208, 42), valByValFmt.str(), FontStyle::RIGHT | FontStyle::NO_OUTLINE,
                     COLOR_WINDOWBROWN);
    // Das Schiffs-Bild
    LOADER.GetImageN("boot_z", 12)->DrawFull(GetDrawPos() + DrawPoint(138, 117));

    // Expeditions-Buttons malen?
    if(ship->IsWaitingForExpeditionInstructions())
    {
        GetCtrl<Window>(10)->SetVisible(ship->IsAbleToFoundColony());
        GetCtrl<Window>(11)->SetVisible(true);

        for(const auto dir : helpers::EnumRange<ShipDirection>{})
            GetCtrl<Window>(12 + rttr::enum_cast(dir))
              ->SetVisible(gwv.GetWorld().GetNextFreeHarborPoint(ship->GetPos(), ship->GetCurrentHarbor(), dir,
                                                                 ship->GetPlayerId())
                           > 0);
    } else
    {
        // Alle Buttons inklusive Anker in der Mitte ausblenden
        for(unsigned i = 0; i < 8; ++i)
            GetCtrl<Window>(10 + i)->SetVisible(false);
    }

    DrawCargo();
}

void iwShip::Msg_ButtonClick(const unsigned ctrl_id)
{
    if(ctrl_id == 2) // Hilfe
    {
        WINDOWMANAGER.ReplaceWindow(
          std::make_unique<iwHelp>(_("The ship register contains all the ships in your fleet. Here you can monitor "
                                     "the loading and destinations of individual ships. Ships on an expedition are "
                                     "controlled from here as well.")));
        return;
    }

    noShip* ship = gwv.GetWorld().GetPlayer(player).GetShipByID(ship_id);

    if(!ship)
        return;

    // Expeditionskommando? (Schiff weiterfahren lassen, Kolonie gründen)
    if(ctrl_id >= 10 && ctrl_id <= 17)
    {
        bool success = false;
        if(ctrl_id == 10)
            success = gcFactory.FoundColony(ship_id);
        else if(ctrl_id == 11)
            success = gcFactory.CancelExpedition(ship_id);
        else
            success = gcFactory.TravelToNextSpot(ShipDirection(ctrl_id - 12), ship_id);

        if(success)
            Close();
    }

    switch(ctrl_id)
    {
        default: break;
        // Erstes Schiff
        case 3:
        {
            ship_id = 0;
        }
        break;
        // Eins zurück
        case 4:
        {
            if(ship_id == 0)
                ship_id = gwv.GetWorld().GetPlayer(ship->GetPlayerId()).GetNumShips() - 1;
            else
                --ship_id;
        }
        break;
        // Eins vor
        case 5:
        {
            ++ship_id;
            if(ship_id == gwv.GetWorld().GetPlayer(ship->GetPlayerId()).GetNumShips())
                ship_id = 0;
        }
        break;
        // Letztes Schiff
        case 6:
        {
            ship_id = gwv.GetWorld().GetPlayer(ship->GetPlayerId()).GetNumShips() - 1;
        }
        break;
        case 7: // "Gehe Zu Ort"
        {
            gwv.MoveToMapPt(ship->GetPos());
        }
        break;
    }
}

void iwShip::DrawCargo()
{
    const GamePlayer& owner = gwv.GetWorld().GetPlayer(player);
    const noShip* ship = owner.GetShipByID(ship_id);

    // Count figures by type
    helpers::EnumArray<unsigned short, Job> orderedFigures{};
    helpers::EnumArray<unsigned short, ArmoredSoldier> armoredFigures{};
    for(const noFigure& figure : ship->GetFigures())
    {
        orderedFigures[figure.GetJobType()]++;

        if(isSoldier(figure.GetJobType()))
        {
            const auto* worker = dynamic_cast<nofSoldier const*>(&figure);
            if(worker->HasArmor())
                armoredFigures[figureToAmoredSoldierEnum(worker)]++;
        }
    }

    // Count wares by type
    helpers::EnumArray<unsigned short, GoodType> orderedWares{};
    for(const Ware& ware : ship->GetWares())
        orderedWares[ware.type]++;

    // Special cases: expeditions
    if(ship->IsOnExpedition())
    {
        orderedFigures[Job::Builder] = 1;
        orderedWares[GoodType::Boards] = BUILDING_COSTS[BuildingType::HarborBuilding].boards;
        orderedWares[GoodType::Stones] = BUILDING_COSTS[BuildingType::HarborBuilding].stones;
    } else if(ship->IsOnExplorationExpedition())
    {
        orderedFigures[Job::Scout] = gwv.GetWorld().GetGGS().GetNumScoutsExpedition();
    }

    // Start Offset zum malen
    const DrawPoint startPt = GetDrawPos() + DrawPoint(40, 130);

    // Step pro Ware/Figur
    const int xStep = 10;
    // Step pro Zeile
    const int yStep = 15;

    // Elemente pro Zeile
    const unsigned elementsPerLine = 17;

    DrawPoint drawPt = startPt;

    unsigned lineCounter = 0;

    // Leute zeichnen
    for(const auto job : helpers::EnumRange<Job>{})
    {
        while(orderedFigures[job] > 0)
        {
            if(lineCounter > elementsPerLine)
            {
                drawPt.x = startPt.x;
                drawPt.y += yStep;
                lineCounter = 0;
            }
            orderedFigures[job]--;

            if(job == Job::PackDonkey)
                LOADER.GetMapTexture(2016)->DrawFull(drawPt);
            else if(job == Job::BoatCarrier)
                LOADER.GetBob("carrier")->Draw(rttr::enum_cast(GoodType::Boat), libsiedler2::ImgDir::SW, false, 0,
                                               drawPt, owner.color);
            else if(wineaddon::isWineAddonJobType(job))
            {
                wineaddon::BobTypes type = wineaddon::BobTypes::VINTNER_WALKING;
                if(job == Job::Winegrower)
                    type = wineaddon::BobTypes::WINEGROWER_WALKING_WITH_SHOVEL;
                if(job == Job::TempleServant)
                    type = wineaddon::BobTypes::TEMPLESERVANT_WALKING;

                LOADER
                  .GetPlayerImage("wine_bobs",
                                  wineaddon::bobIndex[type] + static_cast<unsigned>(libsiedler2::ImgDir::SW) * 8)
                  ->DrawFull(drawPt, COLOR_WHITE, owner.color);
            } else if(leatheraddon::isLeatherAddonJobType(job))
            {
                leatheraddon::BobType type = leatheraddon::BobType::SkinnerWalking;
                if(job == Job::Tanner)
                    type = leatheraddon::BobType::TannerWalking;
                else if(job == Job::LeatherWorker)
                    type = leatheraddon::BobType::LeatherworkerWalking;

                LOADER
                  .GetPlayerImage("leather_bobs",
                                  leatheraddon::bobIndex[type] + static_cast<unsigned>(libsiedler2::ImgDir::SW) * 8)
                  ->DrawFull(drawPt, COLOR_WHITE, owner.color);
            } else
            {
                const auto& spriteData = JOB_SPRITE_CONSTS[job];
                LOADER.GetBob("jobs")->Draw(spriteData.getBobId(owner.nation), libsiedler2::ImgDir::SW,
                                            spriteData.isFat(), 0, drawPt, owner.color);
            }

            if(isSoldier(job) && armoredFigures[jobEnumToAmoredSoldierEnum(job)] > 0)
            {
                armoredFigures[jobEnumToAmoredSoldierEnum(job)]--;
                if(gwv.GetWorld().GetGGS().isEnabled(AddonId::MILITARY_HITPOINTS))
                {
                    SmallFont->Draw(drawPt + DrawPoint(-2, -25), "+", FontStyle::CENTER, COLOR_RED);
                    SmallFont->Draw(drawPt + DrawPoint(1, -25), "1", FontStyle::CENTER, COLOR_RED);
                }
            }

            drawPt.x += xStep;
            lineCounter++;
        }
    }

    // Waren zeichnen
    for(const auto ware : helpers::EnumRange<GoodType>{})
    {
        while(orderedWares[ware] > 0)
        {
            if(lineCounter > elementsPerLine)
            {
                drawPt.x = startPt.x;
                drawPt.y += yStep;
                lineCounter = 0;
            }
            orderedWares[ware]--;

            const auto draw_id = convertShieldToNation(ware, owner.nation);

            LOADER.GetWareStackTex(draw_id)->DrawFull(drawPt);
            drawPt.x += xStep;
            lineCounter++;
        }
    }
}
