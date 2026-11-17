// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofBaker.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "SoundManager.h"
#include "buildings/nobUsual.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "ogl/glSmartBitmap.h"
#include "world/GameWorld.h"
#include "gameTypes/Direction.h"

nofBaker::nofBaker(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofWorkman(Job::Baker, pos, player, workplace)
{}

nofBaker::nofBaker(SerializedGameData& sgd, const unsigned obj_id) : nofWorkman(sgd, obj_id) {}

void nofBaker::drawWorking(DrawPoint drawPt)
{
    constexpr helpers::EnumArray<DrawPoint, Nation> workOffset = {{{40, -4}, {-16, 8}, {-5, 9}, {-8, 7}, {-16, 8}}};
    constexpr helpers::EnumArray<std::array<DrawPoint, 8>, Nation> walkOffsets = {
      {// nation, step
       {{{10, 10}, {17, 12}, {24, 14}, {32, 14}, {34, 9}, {36, 4}, {38, -1}, {40, -4}}},
       {{{9, 11}, {11, 13}, {7, 17}, {3, 20}, {-1, 17}, {-5, 14}, {-9, 12}, {-13, 10}}},
       {{{9, 9}, {11, 11}, {9, 13}, {7, 15}, {4, 13}, {1, 11}, {-2, 9}, {-5, 9}}},
       {{{9, 11}, {11, 13}, {9, 15}, {7, 17}, {4, 15}, {1, 13}, {-2, 11}, {-5, 9}}},
       {{{9, 11}, {11, 13}, {7, 17}, {3, 20}, {-1, 17}, {-5, 14}, {-9, 12}, {-13, 10}}}}};
    constexpr helpers::EnumArray<std::array<Direction, 6>, Nation> walkdirection = {
      {{Direction::East, Direction::East, Direction::NorthEast, Direction::SouthWest, Direction::West, Direction::West},
       {Direction::SouthEast, Direction::SouthWest, Direction::West, Direction::East, Direction::NorthEast,
        Direction::NorthWest},
       {Direction::SouthEast, Direction::SouthWest, Direction::West, Direction::East, Direction::NorthEast,
        Direction::NorthWest},
       {Direction::SouthEast, Direction::SouthWest, Direction::West, Direction::East, Direction::NorthEast,
        Direction::NorthWest},
       {Direction::SouthEast, Direction::SouthWest, Direction::West, Direction::East, Direction::NorthEast,
        Direction::NorthWest}}};

    unsigned maxId = 120;
    unsigned nowId = GAMECLIENT.interpolate(maxId, current_ev);
    const Nation wpNation = workplace->getNation();
    unsigned plColor = world->getPlayer(player).color;

    // position zum rauslaufen berechnen
    DrawPoint walkOutPos = drawPt + walkOffsets[wpNation][nowId % 8];
    // position zum reinlaufen berechnen
    DrawPoint walkInPos = drawPt + walkOffsets[wpNation][7 - (nowId % 8)];

    if(nowId < 2) // hinauslaufen teil 1
    {
        LOADER.building_cache[wpNation][BuildingType::Bakery].door.drawFull(drawPt);
        LOADER.getBobSprite(wpNation, Job::Baker, walkdirection[wpNation][0], nowId)
          .draw(walkOutPos, COLOR_WHITE, plColor);
    }
    if((nowId >= 2) && (nowId < 4)) // hinauslaufen teil 2
    {
        LOADER.building_cache[wpNation][BuildingType::Bakery].door.drawFull(drawPt);
        LOADER.getBobSprite(wpNation, Job::Baker, walkdirection[wpNation][1], nowId)
          .draw(walkOutPos, COLOR_WHITE, plColor);
    }
    if((nowId >= 4) && (nowId < 8)) // hinauslaufen teil 3
    {
        LOADER.getBobSprite(wpNation, Job::Baker, walkdirection[wpNation][2], nowId)
          .draw(walkOutPos, COLOR_WHITE, plColor);
    }
    if((nowId >= 8) && (nowId < 16)) // brot in den ofen schieben
    {
        LOADER.getPlayerImage("rom_bobs", 182 + (nowId - 8))
          ->drawFull(drawPt + workOffset[wpNation], COLOR_WHITE, plColor);

        // "Brot-rein/raus"-Sound
        if((nowId % 8) == 4)
        {
            world->getSoundMgr().playNOSound(68, *this, nowId);
            was_sounding = true;
        }
    }
    if((nowId >= 16) && (nowId < maxId - 16)) // warten
    {
        LOADER.getPlayerImage("rom_bobs", 189)->drawFull(drawPt + workOffset[wpNation], COLOR_WHITE, plColor);
    }
    if((nowId >= maxId - 16) && (nowId < maxId - 8)) // brot aus dem ofen holen
    {
        LOADER.getPlayerImage("rom_bobs", 182 + 7 - (nowId % 8))
          ->drawFull(drawPt + workOffset[wpNation], COLOR_WHITE, plColor);

        // "Brot-rein/raus"-Sound
        if((nowId % 8) == 4)
        {
            world->getSoundMgr().playNOSound(68, *this, nowId);
            was_sounding = true;
        }
    }
    if((nowId >= maxId - 8) && (nowId < maxId - 4)) // reingehn teil 1
    {
        LOADER.getBobSprite(wpNation, Job::Baker, walkdirection[wpNation][3], nowId % 8)
          .draw(walkInPos, COLOR_WHITE, plColor);
    }
    if((nowId >= maxId - 4) && (nowId < maxId - 2)) // reingehn teil 1
    {
        LOADER.building_cache[wpNation][BuildingType::Bakery].door.drawFull(drawPt);
        LOADER.getBobSprite(wpNation, Job::Baker, walkdirection[wpNation][4], nowId % 8)
          .draw(walkInPos, COLOR_WHITE, plColor);
    }
    if((nowId >= maxId - 2) && (nowId < maxId)) // reingehn teil 2
    {
        LOADER.building_cache[wpNation][BuildingType::Bakery].door.drawFull(drawPt);
        LOADER.getBobSprite(wpNation, Job::Baker, walkdirection[wpNation][5], nowId % 8)
          .draw(walkInPos, COLOR_WHITE, plColor);
    }
}

helpers::OptionalEnum<GoodType> nofBaker::produceWare()
{
    return GoodType::Bread;
}
