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

#include "nofPigbreeder.h"
#include "EventManager.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "SoundManager.h"
#include "buildings/nobUsual.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "ogl/glSmartBitmap.h"
#include "world/GameWorld.h"

nofPigbreeder::nofPigbreeder(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofWorkman(Job::PigBreeder, pos, player, workplace)
{}

nofPigbreeder::nofPigbreeder(SerializedGameData& sgd, const unsigned obj_id) : nofWorkman(sgd, obj_id) {}

void nofPigbreeder::DrawWorking(DrawPoint drawPt)
{
    constexpr helpers::EnumArray<DrawPoint, Nation> offsets = {{{10, 2}, {10, 2}, {10, 2}, {10, 2}, {10, 2}}};
    constexpr helpers::EnumArray<DrawPoint, Nation> walkstart = {{{-6, -6}, {-6, -6}, {-6, -6}, {-6, -6}, {-6, -6}}};

    unsigned max_id = 240;
    int now_id = GAMECLIENT.Interpolate(max_id, current_ev);
    const Nation wpNation = workplace->GetNation();
    unsigned plColor = world->GetPlayer(player).color;
    int walksteps = 16;

    if(now_id < 16)
    {
        if(now_id < 8)
            LOADER.building_cache[wpNation][BuildingType::PigFarm].door.DrawFull(drawPt);
        // TODO: Use GlobalAnimation?
        DrawPoint walkPos =
          drawPt + walkstart[wpNation] + (offsets[wpNation] - walkstart[wpNation]) * now_id / walksteps;

        LOADER.getBobSprite(wpNation, Job::PigBreeder, Direction::SouthEast, now_id % 8)
          .draw(walkPos, COLOR_WHITE, plColor);
    } else if(now_id < 40)
    {
        LOADER.GetPlayerImage("rom_bobs", 148 + (now_id - 16) / 2)
          ->DrawFull(drawPt + offsets[wpNation], COLOR_WHITE, plColor);

        // Evtl Sound abspielen
        if((now_id - 16) == 10)
        {
            world->GetSoundMgr().playNOSound(65, *this, 0);
            was_sounding = true;
        }
    } else if(now_id < 56)
    {
        if(now_id > 46)
            LOADER.building_cache[wpNation][BuildingType::PigFarm].door.DrawFull(drawPt);
        // TODO: Use GlobalAnimation?
        DrawPoint walkPos =
          drawPt + walkstart[wpNation] + (walkstart[wpNation] - offsets[wpNation]) * (now_id - 40) / walksteps;
        LOADER.getBobSprite(wpNation, Job::PigBreeder, Direction::NorthWest, (now_id - 40) % 8)
          .draw(walkPos, COLOR_WHITE, plColor);
    }
}

helpers::OptionalEnum<GoodType> nofPigbreeder::ProduceWare()
{
    return GoodType::Ham;
}

void nofPigbreeder::MakePigSounds()
{
    /// Ist es wieder Zeit für einen Schweine-Sound?
    unsigned timeDiffSound = 600 + unsigned(rand() % 200) - workplace->GetProductivity() * 5u;
    if(GetEvMgr().GetCurrentGF() - last_id > timeDiffSound)
    {
        // "Oink"
        world->GetSoundMgr().playNOSound(86, *this, 1);
        last_id = GetEvMgr().GetCurrentGF();
    }
}
