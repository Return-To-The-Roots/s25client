// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "nofBaker.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "SoundManager.h"
#include "buildings/nobUsual.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "ogl/glSmartBitmap.h"
#include "world/GameWorldGame.h"
#include "gameTypes/Direction.h"

nofBaker::nofBaker(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofWorkman(Job::Baker, pos, player, workplace)
{}

nofBaker::nofBaker(SerializedGameData& sgd, const unsigned obj_id) : nofWorkman(sgd, obj_id) {}

void nofBaker::DrawWorking(DrawPoint drawPt)
{
    static const std::array<DrawPoint, NUM_NATIONS> workOffset = {{{40, -4}, {-16, 8}, {-5, 9}, {-8, 7}, {-16, 8}}};
    static const helpers::MultiArray<DrawPoint, NUM_NATIONS, 8> walkOffsets = // nation, schrit, x-y
      {{{{10, 10}, {17, 12}, {24, 14}, {32, 14}, {34, 9}, {36, 4}, {38, -1}, {40, -4}},
        {{9, 11}, {11, 13}, {7, 17}, {3, 20}, {-1, 17}, {-5, 14}, {-9, 12}, {-13, 10}},
        {{9, 9}, {11, 11}, {9, 13}, {7, 15}, {4, 13}, {1, 11}, {-2, 9}, {-5, 9}},
        {{9, 11}, {11, 13}, {9, 15}, {7, 17}, {4, 15}, {1, 13}, {-2, 11}, {-5, 9}},
        {{9, 11}, {11, 13}, {7, 17}, {3, 20}, {-1, 17}, {-5, 14}, {-9, 12}, {-13, 10}}}};
    static const helpers::MultiArray<Direction, NUM_NATIONS, 6> walkdirection = {
      {{Direction::EAST, Direction::EAST, Direction::NORTHEAST, Direction::SOUTHWEST, Direction::WEST, Direction::WEST},
       {Direction::SOUTHEAST, Direction::SOUTHWEST, Direction::WEST, Direction::EAST, Direction::NORTHEAST,
        Direction::NORTHWEST},
       {Direction::SOUTHEAST, Direction::SOUTHWEST, Direction::WEST, Direction::EAST, Direction::NORTHEAST,
        Direction::NORTHWEST},
       {Direction::SOUTHEAST, Direction::SOUTHWEST, Direction::WEST, Direction::EAST, Direction::NORTHEAST,
        Direction::NORTHWEST},
       {Direction::SOUTHEAST, Direction::SOUTHWEST, Direction::WEST, Direction::EAST, Direction::NORTHEAST,
        Direction::NORTHWEST}}};

    unsigned max_id = 120;
    unsigned now_id = GAMECLIENT.Interpolate(max_id, current_ev);
    const Nation wpNation = workplace->GetNation();
    unsigned plColor = gwg->GetPlayer(player).color;

    // position zum rauslaufen berechnen
    DrawPoint walkOutPos = drawPt + walkOffsets[wpNation][now_id % 8];
    // position zum reinlaufen berechnen
    DrawPoint walkInPos = drawPt + walkOffsets[wpNation][7 - (now_id % 8)];

    if(now_id < 2) // hinauslaufen teil 1
    {
        LOADER.GetNationImage(wpNation, 250 + 5 * BLD_BAKERY + 4)->DrawFull(drawPt);
        LOADER.getBobSprite(wpNation, Job::Baker, walkdirection[wpNation][0], now_id)
          .draw(walkOutPos, COLOR_WHITE, plColor);
    }
    if((now_id >= 2) && (now_id < 4)) // hinauslaufen teil 2
    {
        LOADER.GetNationImage(wpNation, 250 + 5 * BLD_BAKERY + 4)->DrawFull(drawPt);
        LOADER.getBobSprite(wpNation, Job::Baker, walkdirection[wpNation][1], now_id)
          .draw(walkOutPos, COLOR_WHITE, plColor);
    }
    if((now_id >= 4) && (now_id < 8)) // hinauslaufen teil 3
    {
        LOADER.getBobSprite(wpNation, Job::Baker, walkdirection[wpNation][2], now_id)
          .draw(walkOutPos, COLOR_WHITE, plColor);
    }
    if((now_id >= 8) && (now_id < 16)) // brot in den ofen schieben
    {
        LOADER.GetPlayerImage("rom_bobs", 182 + (now_id - 8))
          ->DrawFull(drawPt + workOffset[wpNation], COLOR_WHITE, plColor);

        // "Brot-rein/raus"-Sound
        if((now_id % 8) == 4)
        {
            SOUNDMANAGER.PlayNOSound(68, this, now_id);
            was_sounding = true;
        }
    }
    if((now_id >= 16) && (now_id < max_id - 16)) // warten
    {
        LOADER.GetPlayerImage("rom_bobs", 189)->DrawFull(drawPt + workOffset[wpNation], COLOR_WHITE, plColor);
    }
    if((now_id >= max_id - 16) && (now_id < max_id - 8)) // brot aus dem ofen holen
    {
        LOADER.GetPlayerImage("rom_bobs", 182 + 7 - (now_id % 8))
          ->DrawFull(drawPt + workOffset[wpNation], COLOR_WHITE, plColor);

        // "Brot-rein/raus"-Sound
        if((now_id % 8) == 4)
        {
            SOUNDMANAGER.PlayNOSound(68, this, now_id);
            was_sounding = true;
        }
    }
    if((now_id >= max_id - 8) && (now_id < max_id - 4)) // reingehn teil 1
    {
        LOADER.getBobSprite(wpNation, Job::Baker, walkdirection[wpNation][3], now_id % 8)
          .draw(walkInPos, COLOR_WHITE, plColor);
    }
    if((now_id >= max_id - 4) && (now_id < max_id - 2)) // reingehn teil 1
    {
        LOADER.GetNationImage(wpNation, 250 + 5 * BLD_BAKERY + 4)->DrawFull(drawPt);
        LOADER.getBobSprite(wpNation, Job::Baker, walkdirection[wpNation][4], now_id % 8)
          .draw(walkInPos, COLOR_WHITE, plColor);
    }
    if((now_id >= max_id - 2) && (now_id < max_id)) // reingehn teil 2
    {
        LOADER.GetNationImage(wpNation, 250 + 5 * BLD_BAKERY + 4)->DrawFull(drawPt);
        LOADER.getBobSprite(wpNation, Job::Baker, walkdirection[wpNation][5], now_id % 8)
          .draw(walkInPos, COLOR_WHITE, plColor);
    }
}

helpers::OptionalEnum<GoodType> nofBaker::ProduceWare()
{
    return GoodType::Bread;
}
