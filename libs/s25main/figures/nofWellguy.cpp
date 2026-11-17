// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofWellguy.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "Loader.h"
#include "SoundManager.h"
#include "addons/const_addons.h"
#include "buildings/nobUsual.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "ogl/glSmartBitmap.h"
#include "world/GameWorld.h"

nofWellguy::nofWellguy(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofWorkman(Job::Helper, pos, player, workplace)
{}

nofWellguy::nofWellguy(SerializedGameData& sgd, const unsigned obj_id) : nofWorkman(sgd, obj_id) {}

void nofWellguy::drawWorking(DrawPoint drawPt)
{
    constexpr helpers::EnumArray<DrawPoint, Nation> offsets = {{{-20, 17}, {-18, 17}, {-20, 13}, {-20, 15}, {-18, 17}}};

    constexpr helpers::EnumArray<std::array<DrawPoint, 8>, Nation> walkoffsets = // nation, schritt
      {{{{{7, 7}, {9, 9}, {5, 12}, {2, 14}, {-1, 17}, {-4, 17}, {-7, 17}, {-10, 17}}},
        {{{4, 4}, {8, 8}, {5, 12}, {2, 14}, {-1, 17}, {-3, 19}, {-6, 19}, {-8, 19}}},
        {{{5, 5}, {8, 8}, {5, 10}, {2, 13}, {-1, 13}, {-4, 13}, {-7, 13}, {-10, 13}}},
        {{{5, 5}, {8, 8}, {5, 10}, {2, 13}, {-1, 15}, {-4, 15}, {-7, 15}, {-10, 15}}},
        {{{4, 4}, {8, 8}, {5, 12}, {2, 14}, {-1, 17}, {-3, 19}, {-6, 19}, {-8, 19}}}}};
    constexpr helpers::EnumArray<std::array<Direction, 6>, Nation> walkdirection = {{
      {Direction::SouthEast, Direction::SouthWest, Direction::West, Direction::East, Direction::NorthEast,
       Direction::NorthWest},
      {Direction::SouthEast, Direction::SouthWest, Direction::West, Direction::East, Direction::NorthEast,
       Direction::NorthWest},
      {Direction::SouthEast, Direction::SouthWest, Direction::West, Direction::East, Direction::NorthEast,
       Direction::NorthWest},
      {Direction::SouthEast, Direction::SouthWest, Direction::West, Direction::East, Direction::NorthEast,
       Direction::NorthWest},
      {Direction::SouthEast, Direction::SouthWest, Direction::West, Direction::East, Direction::NorthEast,
       Direction::NorthWest},
    }};

    unsigned maxId = 112;
    unsigned nowId = GAMECLIENT.interpolate(maxId, current_ev);
    const Nation wpNation = workplace->getNation();
    unsigned plColor = world->getPlayer(player).color;

    // position zum rauslaufen berechnen
    DrawPoint walkOutPos = drawPt + walkoffsets[wpNation][nowId % 8];
    // position zum reinlaufen berechnen
    DrawPoint walkInPos = drawPt + walkoffsets[wpNation][7 - (nowId % 8)];

    if(nowId < 2) // laufen 1
    {
        if(wpNation == Nation::Romans)
            LOADER.building_cache[wpNation][BuildingType::Well].door.drawFull(drawPt);
        LOADER.getCarrierSprite(GoodType::WaterEmpty, false, walkdirection[wpNation][0], nowId % 8)
          .draw(walkOutPos, COLOR_WHITE, plColor);
    } else if(nowId < 4) // laufen 2
    {
        if(wpNation == Nation::Romans)
            LOADER.building_cache[wpNation][BuildingType::Well].door.drawFull(drawPt);
        LOADER.getCarrierSprite(GoodType::WaterEmpty, false, walkdirection[wpNation][1], nowId % 8)
          .draw(walkOutPos, COLOR_WHITE, plColor);
    } else if(nowId < 8) // laufen 3
    {
        LOADER.getCarrierSprite(GoodType::WaterEmpty, false, walkdirection[wpNation][2], nowId % 8)
          .draw(walkOutPos, COLOR_WHITE, plColor);
    } else if(nowId < 16) // eimer runter lassen
    {
        if(nowId == 8)
            LOADER.getPlayerImage("rom_bobs", 346)->drawFull(drawPt + offsets[wpNation], COLOR_WHITE, plColor);
        else
            LOADER.getPlayerImage("rom_bobs", 346 + (nowId % 8) - 1)
              ->drawFull(drawPt + offsets[wpNation], COLOR_WHITE, plColor);
    } else if(nowId < maxId - 16) // kurbeln
    {
        LOADER.getPlayerImage("rom_bobs", 330 + (nowId % 8))
          ->drawFull(drawPt + offsets[wpNation], COLOR_WHITE, plColor);
    } else if(nowId < maxId - 8) // eimer rauf kurbeln
    {
        LOADER.getPlayerImage("rom_bobs", 338 + (nowId % 8))
          ->drawFull(drawPt + offsets[wpNation], COLOR_WHITE, plColor);
    } else if(nowId < maxId - 4) // laufen 3
    {
        LOADER.getCarrierSprite(GoodType::Water, false, walkdirection[wpNation][3], nowId % 8)
          .draw(walkInPos, COLOR_WHITE, plColor);
    } else if(nowId < maxId - 2) // laufen 2
    {
        if(wpNation == Nation::Romans)
            LOADER.building_cache[wpNation][BuildingType::Well].door.drawFull(drawPt);
        LOADER.getCarrierSprite(GoodType::Water, false, walkdirection[wpNation][4], nowId % 8)
          .draw(walkInPos, COLOR_WHITE, plColor);
    } else // laufen 1
    {
        if(wpNation == Nation::Romans)
            LOADER.building_cache[wpNation][BuildingType::Well].door.drawFull(drawPt);
        LOADER.getCarrierSprite(GoodType::Water, false, walkdirection[wpNation][5], nowId % 8)
          .draw(walkInPos, COLOR_WHITE, plColor);
    }

    if((nowId >= 8) && (nowId < maxId - 8) && nowId % 8 == 4)
    {
        world->getSoundMgr().playNOSound(82, *this, nowId);
        was_sounding = true;
    }
}

helpers::OptionalEnum<GoodType> nofWellguy::produceWare()
{
    return GoodType::Water;
}

bool nofWellguy::areWaresAvailable() const
{
    // Check for water
    const bool hasWater = findPointWithResource(ResourceType::Water).isValid();
    if(!hasWater)
        workplace->onOutOfResources();
    return hasWater;
}

bool nofWellguy::startWorking()
{
    MapPoint resPt = findPointWithResource(ResourceType::Water);
    if(!resPt.isValid())
    {
        workplace->onOutOfResources();
        return false;
    }
    if(world->getGGS().getSelection(AddonId::EXHAUSTIBLE_WATER) == 2)
        world->reduceResource(resPt);
    return nofWorkman::startWorking();
}
