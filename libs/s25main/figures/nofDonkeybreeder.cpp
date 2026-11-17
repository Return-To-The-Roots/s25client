// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofDonkeybreeder.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "buildings/nobUsual.h"
#include "network/GameClient.h"
#include "nofCarrier.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "ogl/glSmartBitmap.h"
#include "world/GameWorld.h"
#include "s25util/colors.h"

nofDonkeybreeder::nofDonkeybreeder(const MapPoint pos, unsigned char player, nobUsual* workplace)
    : nofWorkman(Job::DonkeyBreeder, pos, player, workplace)
{}

nofDonkeybreeder::nofDonkeybreeder(SerializedGameData& sgd, unsigned obj_id) : nofWorkman(sgd, obj_id) {}

/**
 *  Zeichnet ihn beim Arbeiten.
 */
void nofDonkeybreeder::drawWorking(DrawPoint drawPt)
{
    const Nation nation = workplace->getNation();
    constexpr helpers::EnumArray<DrawPoint, Nation> walk_start = {{{2, 2}, {-6, -6}, {-7, -7}, {-7, -7}, {-6, -6}}};
    const helpers::EnumArray<int8_t, Nation> walk_length = {22, 19, 19, 23, 19};
    const unsigned color = world->getPlayer(player).color;

    unsigned nowId = GAMECLIENT.interpolate(9600, current_ev);
    DrawPoint walkBasePos = drawPt + walk_start[nation];

    if(nowId < 400)
    {
        LOADER.building_cache[nation][BuildingType::DonkeyBreeder].door.drawFull(drawPt);
        LOADER.getBobSprite(nation, Job::DonkeyBreeder, Direction::SouthEast, (nowId / 70) % 8)
          .draw(walkBasePos + DrawPoint(nowId / 100, nowId / 100), COLOR_WHITE, color);
    } else if(nowId < 1200)
        LOADER.getBobSprite(nation, Job::DonkeyBreeder, Direction::East, ((nowId - 400) / 70) % 8)
          .draw(walkBasePos + DrawPoint((nowId - 400) / 800, 4), COLOR_WHITE, color);
    else if(nowId < 2000)
        LOADER.getPlayerImage("rom_bobs", 291 + (nowId - 1200) / 100)
          ->drawFull(walkBasePos + DrawPoint(walk_length[nation] + 4, 4), COLOR_WHITE, color);
    else if(nowId < 2800)
        LOADER.getBobSprite(nation, Job::DonkeyBreeder, Direction::West, ((nowId - 2000) / 70) % 8)
          .draw(walkBasePos + DrawPoint(4 + walk_length[nation] * (2800 - nowId) / 800, 4), COLOR_WHITE, color);
    else if(nowId < 3200)
    {
        LOADER.building_cache[nation][BuildingType::DonkeyBreeder].door.drawFull(drawPt);
        LOADER.getBobSprite(nation, Job::DonkeyBreeder, Direction::NorthWest, ((nowId - 2800) / 70) % 8)
          .draw(walkBasePos + DrawPoint((3200 - nowId) / 100, (3200 - nowId) / 100), COLOR_WHITE, color);
    }
}

/**
 *  Der Arbeiter erzeugt eine Ware.
 */
helpers::OptionalEnum<GoodType> nofDonkeybreeder::produceWare()
{
    /// @todo Wie kann ich hier eine Person erzeugen?
    return std::nullopt;
}

void nofDonkeybreeder::workFinished()
{
    // Straße und Zielflagge für Esel suchen
    noRoadNode* flag_goal;
    RoadSegment* road = world->getPlayer(player).findRoadForDonkey(*workplace, &flag_goal);

    // Esel erzeugen und zum Ziel beordern
    auto donkey = std::make_unique<nofCarrier>(CarrierType::Donkey, pos, player, road, flag_goal);
    world->getPlayer(player).increaseInventoryJob(Job::PackDonkey, 1);
    donkey->initializeRoadWalking(world->getSpecObj<noRoadNode>(pos)->getRoute(Direction::SouthEast), 0, true);

    // Wenn keine Straße gefunden wurde, muss er nach Hause gehen
    if(!road)
        donkey->goHome();
    else
        // ansonsten Arbeitsplatz Bescheid sagen
        road->gotDonkey(donkey.get());

    // Esel absetzen
    world->addFigure(pos, std::move(donkey)).actAtFirst();
}
