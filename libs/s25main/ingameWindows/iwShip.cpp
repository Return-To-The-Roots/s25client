// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
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
    : IngameWindow(CGI_SHIP, pos, Extent(252, 238), _("Ship register"), LOADER.getImageN("resource", 41)), gwv(gwv),
      gcFactory(gcFactory), player(ship ? ship->getPlayerId() : gwv.getViewer().getPlayerId()),
      ship_id(ship ? gwv.getWorld().getPlayer(player).getShipID(*ship) : 0)
{
    addImage(0, DrawPoint(126, 101), LOADER.getImageN("io", 228));
    addImageButton(2, DrawPoint(18, 192), Extent(30, 35), TextureColor::Grey,
                   LOADER.getImageN("io", 225)); // Viewer: 226 - Hilfe
    addImageButton(3, DrawPoint(51, 196), Extent(30, 26), TextureColor::Grey,
                   LOADER.getImageN("io", 102)); // Viewer: 103 - Schnell zurück
    addImageButton(4, DrawPoint(81, 196), Extent(30, 26), TextureColor::Grey,
                   LOADER.getImageN("io", 103)); // Viewer: 104 - Zurück
    addImageButton(5, DrawPoint(111, 196), Extent(30, 26), TextureColor::Grey,
                   LOADER.getImageN("io", 104)); // Viewer: 105 - Vor
    addImageButton(6, DrawPoint(141, 196), Extent(30, 26), TextureColor::Grey,
                   LOADER.getImageN("io", 105)); // Viewer: 106 - Schnell vor
    addImageButton(7, DrawPoint(181, 196), Extent(30, 26), TextureColor::Grey, LOADER.getImageN("io", 107),
                   _("Go to place")); // "Gehe Zu Ort"

    // Die Expeditionsweiterfahrbuttons
    addImageButton(10, DrawPoint(60, 81), Extent(18, 18), TextureColor::Grey, LOADER.getImageN("io", 187),
                   _("Found colony"))
      ->setVisible(false);

    constexpr helpers::EnumArray<DrawPoint, ShipDirection> BUTTON_POS = {
      {{60, 61}, {80, 70}, {80, 90}, {60, 101}, {40, 90}, {40, 70}}};
    constexpr helpers::EnumArray<unsigned, ShipDirection> BUTTON_IDs = {{185, 186, 181, 182, 183, 184}};

    // Expedition abbrechen
    addImageButton(11, DrawPoint(200, 143), Extent(18, 18), TextureColor::Red1, LOADER.getImageN("io", 40),
                   _("Return to harbor"))
      ->setVisible(false);

    // Die 6 Richtungen
    for(const auto dir : helpers::EnumRange<ShipDirection>{})
        addImageButton(12 + rttr::enumCast(dir), BUTTON_POS[dir], Extent(18, 18), TextureColor::Grey,
                       LOADER.getImageN("io", BUTTON_IDs[dir]))
          ->setVisible(false);
}

void iwShip::drawContent()
{
    static boost::format valByValFmt{"%1%/%2%"};

    const GamePlayer& owner = gwv.getWorld().getPlayer(player);
    // Schiff holen
    noShip* ship = (player == 0xff) ? nullptr : owner.getShipByID(ship_id);

    // Kein Schiff gefunden? Dann erstes Schiff holen
    if(!ship)
    {
        ship_id = 0;
        // Nochmal probieren
        if(player != 0xff)
            ship = owner.getShipByID(ship_id);
        // Immer noch nicht? Dann gibt es keine Schiffe mehr und wir zeigen eine entsprechende Meldung an
        if(!ship)
        {
            NormalFont->draw(getDrawPos() + DrawPoint(getSize().x / 2, 60), _("No ships available"),
                             FontStyle::CENTER | FontStyle::NO_OUTLINE, COLOR_WINDOWBROWN);
            return;
        }
    }

    // Schiffsname
    NormalFont->draw(getDrawPos() + DrawPoint(42, 42), ship->getName(), FontStyle::NO_OUTLINE, COLOR_WINDOWBROWN);
    // Schiffs-Nr.
    valByValFmt % (ship_id + 1) % owner.getNumShips();
    NormalFont->draw(getDrawPos() + DrawPoint(208, 42), valByValFmt.str(), FontStyle::RIGHT | FontStyle::NO_OUTLINE,
                     COLOR_WINDOWBROWN);
    // Das Schiffs-Bild
    LOADER.getImageN("boot_z", 12)->drawFull(getDrawPos() + DrawPoint(138, 117));

    // Expeditions-Buttons malen?
    if(ship->isWaitingForExpeditionInstructions())
    {
        getCtrl<Window>(10)->setVisible(ship->isAbleToFoundColony());
        getCtrl<Window>(11)->setVisible(true);

        for(const auto dir : helpers::EnumRange<ShipDirection>{})
        {
            getCtrl<Window>(12 + rttr::enumCast(dir))
              ->setVisible(gwv.getWorld()
                             .getNextFreeHarborPoint(ship->getPos(), ship->getCurrentHarbor(), dir, ship->getPlayerId())
                             .isValid());
        }
    } else
    {
        // Alle Buttons inklusive Anker in der Mitte ausblenden
        for(unsigned i = 0; i < 8; ++i)
            getCtrl<Window>(10 + i)->setVisible(false);
    }

    drawCargo();
}

void iwShip::msgButtonClick(const unsigned ctrl_id)
{
    if(ctrl_id == 2) // Hilfe
    {
        WINDOWMANAGER.replaceWindow(
          std::make_unique<iwHelp>(_("The ship register contains all the ships in your fleet. Here you can monitor "
                                     "the loading and destinations of individual ships. Ships on an expedition are "
                                     "controlled from here as well.")));
        return;
    }

    noShip* ship = gwv.getWorld().getPlayer(player).getShipByID(ship_id);

    if(!ship)
        return;

    // Expeditionskommando? (Schiff weiterfahren lassen, Kolonie gründen)
    if(ctrl_id >= 10 && ctrl_id <= 17)
    {
        bool success = false;
        if(ctrl_id == 10)
            success = gcFactory.foundColony(ship_id);
        else if(ctrl_id == 11)
            success = gcFactory.CancelExpedition(ship_id);
        else
            success = gcFactory.travelToNextSpot(ShipDirection(ctrl_id - 12), ship_id);

        if(success)
            close();
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
                ship_id = gwv.getWorld().getPlayer(ship->getPlayerId()).getNumShips() - 1;
            else
                --ship_id;
        }
        break;
        // Eins vor
        case 5:
        {
            ++ship_id;
            if(ship_id == gwv.getWorld().getPlayer(ship->getPlayerId()).getNumShips())
                ship_id = 0;
        }
        break;
        // Letztes Schiff
        case 6:
        {
            ship_id = gwv.getWorld().getPlayer(ship->getPlayerId()).getNumShips() - 1;
        }
        break;
        case 7: // "Gehe Zu Ort"
        {
            gwv.moveToMapPt(ship->getPos());
        }
        break;
    }
}

void iwShip::drawCargo()
{
    const GamePlayer& owner = gwv.getWorld().getPlayer(player);
    const noShip* ship = owner.getShipByID(ship_id);

    // Count figures by type
    helpers::EnumArray<unsigned short, Job> orderedFigures{};
    helpers::EnumArray<unsigned short, ArmoredSoldier> armoredFigures{};
    for(const noFigure& figure : ship->getFigures())
    {
        orderedFigures[figure.getJobType()]++;

        if(isSoldier(figure.getJobType()) && figure.hasArmor())
            armoredFigures[jobEnumToAmoredSoldierEnum(figure.getJobType())]++;
    }

    // Count wares by type
    helpers::EnumArray<unsigned short, GoodType> orderedWares{};
    for(const Ware& ware : ship->getWares())
        orderedWares[ware.type]++;

    // Special cases: expeditions
    if(ship->isOnExpedition())
    {
        orderedFigures[Job::Builder] = 1;
        orderedWares[GoodType::Boards] = BUILDING_COSTS[BuildingType::HarborBuilding].boards;
        orderedWares[GoodType::Stones] = BUILDING_COSTS[BuildingType::HarborBuilding].stones;
    } else if(ship->isOnExplorationExpedition())
    {
        orderedFigures[Job::Scout] = gwv.getWorld().getGGS().getNumScoutsExpedition();
    }

    // Start Offset zum malen
    const DrawPoint startPt = getDrawPos() + DrawPoint(40, 130);

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
                LOADER.getMapTexture(2016)->drawFull(drawPt);
            else if(job == Job::BoatCarrier)
                LOADER.getBob("carrier")->draw(rttr::enumCast(GoodType::Boat), libsiedler2::ImgDir::SW, false, 0,
                                               drawPt, owner.color);
            else if(wineaddon::isWineAddonJobType(job))
            {
                wineaddon::BobTypes type = wineaddon::BobTypes::VINTNER_WALKING;
                if(job == Job::Winegrower)
                    type = wineaddon::BobTypes::WINEGROWER_WALKING_WITH_SHOVEL;
                if(job == Job::TempleServant)
                    type = wineaddon::BobTypes::TEMPLESERVANT_WALKING;

                LOADER
                  .getPlayerImage("wine_bobs",
                                  wineaddon::bobIndex[type] + static_cast<unsigned>(libsiedler2::ImgDir::SW) * 8)
                  ->drawFull(drawPt, COLOR_WHITE, owner.color);
            } else if(leatheraddon::isLeatherAddonJobType(job))
            {
                leatheraddon::BobType type = leatheraddon::BobType::SkinnerWalking;
                if(job == Job::Tanner)
                    type = leatheraddon::BobType::TannerWalking;
                else if(job == Job::LeatherWorker)
                    type = leatheraddon::BobType::LeatherworkerWalking;

                LOADER
                  .getPlayerImage("leather_bobs",
                                  leatheraddon::bobIndex[type] + static_cast<unsigned>(libsiedler2::ImgDir::SW) * 8)
                  ->drawFull(drawPt, COLOR_WHITE, owner.color);
            } else
            {
                const auto& spriteData = JOB_SPRITE_CONSTS[job];
                LOADER.getBob("jobs")->draw(spriteData.getBobId(owner.nation), libsiedler2::ImgDir::SW,
                                            spriteData.isFat(), 0, drawPt, owner.color);
            }

            if(isSoldier(job) && armoredFigures[jobEnumToAmoredSoldierEnum(job)] > 0)
            {
                armoredFigures[jobEnumToAmoredSoldierEnum(job)]--;
                if(gwv.getWorld().getGGS().isEnabled(AddonId::MILITARY_HITPOINTS))
                {
                    SmallFont->draw(drawPt + DrawPoint(-2, -25), "+", FontStyle::CENTER, COLOR_RED);
                    SmallFont->draw(drawPt + DrawPoint(1, -25), "1", FontStyle::CENTER, COLOR_RED);
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

            LOADER.getWareStackTex(draw_id)->drawFull(drawPt);
            drawPt.x += xStep;
            lineCounter++;
        }
    }
}
