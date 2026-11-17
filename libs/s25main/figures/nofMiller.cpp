// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofMiller.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "SoundManager.h"
#include "buildings/nobUsual.h"
#include "drivers/VideoDriverWrapper.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "ogl/glSmartBitmap.h"
#include "world/GameWorld.h"

nofMiller::nofMiller(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofWorkman(Job::Miller, pos, player, workplace), last_sound(0), next_interval(0)
{}

nofMiller::nofMiller(SerializedGameData& sgd, const unsigned obj_id)
    : nofWorkman(sgd, obj_id), last_sound(0), next_interval(0)
{}

void nofMiller::drawWorking(DrawPoint drawPt)
{
    constexpr helpers::EnumArray<DrawPoint, Nation> offsets = {{{20, 8}, {20, 8}, {20, 8}, {20, 8}, {20, 8}}};
    constexpr helpers::EnumArray<DrawPoint, Nation> offsets_sitdown = {{{23, 8}, {23, 8}, {23, 8}, {23, 8}, {23, 8}}};
    const std::array<DrawPoint, 8> walkoffsets = {
      {{8, 8}, {10, 9}, {12, 10}, {14, 11}, {16, 10}, {18, 9}, {20, 8}, {22, 8}}};

    unsigned maxId = 120;
    unsigned nowId = GAMECLIENT.interpolate(maxId, current_ev);
    bool rotateSails = true;
    const Nation nation = workplace->getNation();

    if(nowId < 4) // hinauslaufen teil 1
    {
        LOADER.building_cache[nation][BuildingType::Mill].door.drawFull(drawPt);
        LOADER.getBobSprite(nation, Job::Miller, Direction::SouthEast, nowId % 8)
          .draw(drawPt + walkoffsets[nowId], COLOR_WHITE, world->getPlayer(player).color);
        rotateSails = false;
    }
    if((nowId >= 4) && (nowId < 8)) // hinauslaufen teil 2
    {
        LOADER.getBobSprite(nation, Job::Miller, Direction::East, nowId % 8)
          .draw(drawPt + walkoffsets[nowId], COLOR_WHITE, world->getPlayer(player).color);
    }
    if((nowId >= 8) && (nowId < 16)) // hinsetzen
    {
        LOADER.getPlayerImage("rom_bobs", 166 + (nowId % 8))
          ->drawFull(drawPt + offsets_sitdown[nation], COLOR_WHITE, world->getPlayer(workplace->getPlayer()).color);
    }
    if((nowId >= 16) && (nowId < maxId - 16)) // schlafen
    {
        LOADER.getPlayerImage("rom_bobs", 174 + (nowId % 8))
          ->drawFull(drawPt + offsets[nation], COLOR_WHITE, world->getPlayer(workplace->getPlayer()).color);
    }
    if((nowId >= maxId - 16) && (nowId < maxId - 8)) // aufstehn
    {
        LOADER.getPlayerImage("rom_bobs", 166 + 7 - (nowId % 8))
          ->drawFull(drawPt + offsets_sitdown[nation], COLOR_WHITE, world->getPlayer(workplace->getPlayer()).color);
    }
    if((nowId >= maxId - 8) && (nowId < maxId - 4)) // zurücklaufen teil 1
    {
        LOADER.getBobSprite(nation, Job::Miller, Direction::West, nowId % 8)
          .draw(drawPt + walkoffsets[7 - (nowId % 8)], COLOR_WHITE, world->getPlayer(player).color);
    }
    if((nowId >= maxId - 4) && (nowId < maxId)) // zurücklaufen teil 2
    {
        LOADER.building_cache[nation][BuildingType::Mill].door.drawFull(drawPt);
        LOADER.getBobSprite(nation, Job::Miller, Direction::NorthWest, nowId % 8)
          .draw(drawPt + walkoffsets[7 - (nowId % 8)], COLOR_WHITE, world->getPlayer(player).color);
        rotateSails = false;
    }

    if(rotateSails)
    {
        // Flügel der Mühle
        LOADER.getNationImage(nation, 250 + 5 * (42 + ((nowId + 4) % 8)))->drawFull(drawPt);
        // Schatten der Flügel
        LOADER.getNationImage(nation, 250 + (5 * (42 + ((nowId + 4) % 8))) + 1)->drawFull(drawPt, COLOR_SHADOW);

        // Mühlensound abspielen in zufälligen Intervallen
        if(VIDEODRIVER.getTickCount() - last_sound > next_interval)
        {
            world->getSoundMgr().playNOSound(77, *this, nowId);
            was_sounding = true;

            last_sound = VIDEODRIVER.getTickCount();
            next_interval = 500 + rand() % 1400;
        }
    } else
    {
        // Flügel der Mühle
        LOADER.getNationImage(nation, 250 + 5 * 49)->drawFull(drawPt);
        // Schatten der Flügel
        LOADER.getNationImage(nation, 250 + 5 * 49 + 1)->drawFull(drawPt, COLOR_SHADOW);
    }
}

helpers::OptionalEnum<GoodType> nofMiller::produceWare()
{
    return GoodType::Flour;
}
