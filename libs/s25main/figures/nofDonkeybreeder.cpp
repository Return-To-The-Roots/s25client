// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
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
void nofDonkeybreeder::DrawWorking(DrawPoint drawPt)
{
    const Nation nation = workplace->GetNation();
    constexpr helpers::EnumArray<DrawPoint, Nation> walk_start = {{{2, 2}, {-6, -6}, {-7, -7}, {-7, -7}, {-6, -6}}};
    const helpers::EnumArray<int8_t, Nation> walk_length = {22, 19, 19, 23, 19};
    const unsigned color = world->GetPlayer(player).color;

    unsigned now_id = GAMECLIENT.Interpolate(9600, current_ev);
    DrawPoint walkBasePos = drawPt + walk_start[nation];

    if(now_id < 400)
    {
        LOADER.building_cache[nation][BuildingType::DonkeyBreeder].door.DrawFull(drawPt);
        LOADER.getBobSprite(nation, Job::DonkeyBreeder, Direction::SouthEast, (now_id / 70) % 8)
          .draw(walkBasePos + DrawPoint(now_id / 100, now_id / 100), COLOR_WHITE, color);
    } else if(now_id < 1200)
        LOADER.getBobSprite(nation, Job::DonkeyBreeder, Direction::East, ((now_id - 400) / 70) % 8)
          .draw(walkBasePos + DrawPoint((now_id - 400) / 800, 4), COLOR_WHITE, color);
    else if(now_id < 2000)
        LOADER.GetPlayerImage("rom_bobs", 291 + (now_id - 1200) / 100)
          ->DrawFull(walkBasePos + DrawPoint(walk_length[nation] + 4, 4), COLOR_WHITE, color);
    else if(now_id < 2800)
        LOADER.getBobSprite(nation, Job::DonkeyBreeder, Direction::West, ((now_id - 2000) / 70) % 8)
          .draw(walkBasePos + DrawPoint(4 + walk_length[nation] * (2800 - now_id) / 800, 4), COLOR_WHITE, color);
    else if(now_id < 3200)
    {
        LOADER.building_cache[nation][BuildingType::DonkeyBreeder].door.DrawFull(drawPt);
        LOADER.getBobSprite(nation, Job::DonkeyBreeder, Direction::NorthWest, ((now_id - 2800) / 70) % 8)
          .draw(walkBasePos + DrawPoint((3200 - now_id) / 100, (3200 - now_id) / 100), COLOR_WHITE, color);
    }
}

/**
 *  Der Arbeiter erzeugt eine Ware.
 */
helpers::OptionalEnum<GoodType> nofDonkeybreeder::ProduceWare()
{
    /// @todo Wie kann ich hier eine Person erzeugen?
    return boost::none;
}

void nofDonkeybreeder::WorkFinished()
{
    // Straße und Zielflagge für Esel suchen
    noRoadNode* flag_goal;
    RoadSegment* road = world->GetPlayer(player).FindRoadForDonkey(workplace, &flag_goal);

    // Esel erzeugen und zum Ziel beordern
    auto donkey = std::make_unique<nofCarrier>(CarrierType::Donkey, pos, player, road, flag_goal);
    world->GetPlayer(player).IncreaseInventoryJob(Job::PackDonkey, 1);
    donkey->InitializeRoadWalking(world->GetSpecObj<noRoadNode>(pos)->GetRoute(Direction::SouthEast), 0, true);

    // Wenn keine Straße gefunden wurde, muss er nach Hause gehen
    if(!road)
        donkey->GoHome();
    else
        // ansonsten Arbeitsplatz Bescheid sagen
        road->GotDonkey(donkey.get());

    // Esel absetzen
    world->AddFigure(pos, std::move(donkey)).ActAtFirst();
}
