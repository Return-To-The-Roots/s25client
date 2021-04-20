// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

void nofMiller::DrawWorking(DrawPoint drawPt)
{
    constexpr helpers::EnumArray<DrawPoint, Nation> offsets = {{{20, 8}, {20, 8}, {20, 8}, {20, 8}, {20, 8}}};
    constexpr helpers::EnumArray<DrawPoint, Nation> offsets_sitdown = {{{23, 8}, {23, 8}, {23, 8}, {23, 8}, {23, 8}}};
    const std::array<DrawPoint, 8> walkoffsets = {
      {{8, 8}, {10, 9}, {12, 10}, {14, 11}, {16, 10}, {18, 9}, {20, 8}, {22, 8}}};

    unsigned max_id = 120;
    unsigned now_id = GAMECLIENT.Interpolate(max_id, current_ev);
    bool rotate_sails = true;
    const Nation nation = workplace->GetNation();

    if(now_id < 4) // hinauslaufen teil 1
    {
        LOADER.building_cache[nation][BuildingType::Mill].door.DrawFull(drawPt);
        LOADER.getBobSprite(nation, Job::Miller, Direction::SouthEast, now_id % 8)
          .draw(drawPt + walkoffsets[now_id], COLOR_WHITE, world->GetPlayer(player).color);
        rotate_sails = false;
    }
    if((now_id >= 4) && (now_id < 8)) // hinauslaufen teil 2
    {
        LOADER.getBobSprite(nation, Job::Miller, Direction::East, now_id % 8)
          .draw(drawPt + walkoffsets[now_id], COLOR_WHITE, world->GetPlayer(player).color);
    }
    if((now_id >= 8) && (now_id < 16)) // hinsetzen
    {
        LOADER.GetPlayerImage("rom_bobs", 166 + (now_id % 8))
          ->DrawFull(drawPt + offsets_sitdown[nation], COLOR_WHITE, world->GetPlayer(workplace->GetPlayer()).color);
    }
    if((now_id >= 16) && (now_id < max_id - 16)) // schlafen
    {
        LOADER.GetPlayerImage("rom_bobs", 174 + (now_id % 8))
          ->DrawFull(drawPt + offsets[nation], COLOR_WHITE, world->GetPlayer(workplace->GetPlayer()).color);
    }
    if((now_id >= max_id - 16) && (now_id < max_id - 8)) // aufstehn
    {
        LOADER.GetPlayerImage("rom_bobs", 166 + 7 - (now_id % 8))
          ->DrawFull(drawPt + offsets_sitdown[nation], COLOR_WHITE, world->GetPlayer(workplace->GetPlayer()).color);
    }
    if((now_id >= max_id - 8) && (now_id < max_id - 4)) // zurücklaufen teil 1
    {
        LOADER.getBobSprite(nation, Job::Miller, Direction::West, now_id % 8)
          .draw(drawPt + walkoffsets[7 - (now_id % 8)], COLOR_WHITE, world->GetPlayer(player).color);
    }
    if((now_id >= max_id - 4) && (now_id < max_id)) // zurücklaufen teil 2
    {
        LOADER.building_cache[nation][BuildingType::Mill].door.DrawFull(drawPt);
        LOADER.getBobSprite(nation, Job::Miller, Direction::NorthWest, now_id % 8)
          .draw(drawPt + walkoffsets[7 - (now_id % 8)], COLOR_WHITE, world->GetPlayer(player).color);
        rotate_sails = false;
    }

    if(rotate_sails)
    {
        // Flügel der Mühle
        LOADER.GetNationImage(nation, 250 + 5 * (42 + ((now_id + 4) % 8)))->DrawFull(drawPt);
        // Schatten der Flügel
        LOADER.GetNationImage(nation, 250 + (5 * (42 + ((now_id + 4) % 8))) + 1)->DrawFull(drawPt, COLOR_SHADOW);

        // Mühlensound abspielen in zufälligen Intervallen
        if(VIDEODRIVER.GetTickCount() - last_sound > next_interval)
        {
            world->GetSoundMgr().playNOSound(77, *this, now_id);
            was_sounding = true;

            last_sound = VIDEODRIVER.GetTickCount();
            next_interval = 500 + rand() % 1400;
        }
    } else
    {
        // Flügel der Mühle
        LOADER.GetNationImage(nation, 250 + 5 * 49)->DrawFull(drawPt);
        // Schatten der Flügel
        LOADER.GetNationImage(nation, 250 + 5 * 49 + 1)->DrawFull(drawPt, COLOR_SHADOW);
    }
}

helpers::OptionalEnum<GoodType> nofMiller::ProduceWare()
{
    return GoodType::Flour;
}
