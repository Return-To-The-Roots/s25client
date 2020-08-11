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
#include "world/GameWorldGame.h"

nofMiller::nofMiller(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofWorkman(JOB_MILLER, pos, player, workplace), last_sound(0), next_interval(0)
{}

void nofMiller::Serialize_nofMiller(SerializedGameData& sgd) const
{
    Serialize_nofWorkman(sgd);
}

nofMiller::nofMiller(SerializedGameData& sgd, const unsigned obj_id) : nofWorkman(sgd, obj_id), last_sound(0), next_interval(0) {}

void nofMiller::DrawWorking(DrawPoint drawPt)
{
    const std::array<DrawPoint, NUM_NATIONS> offsets = {{{20, 8}, {20, 8}, {20, 8}, {20, 8}, {20, 8}}};
    const std::array<DrawPoint, NUM_NATIONS> offsets_sitdown = {{{23, 8}, {23, 8}, {23, 8}, {23, 8}, {23, 8}}};
    const std::array<DrawPoint, 8> walkoffsets = {{{8, 8}, {10, 9}, {12, 10}, {14, 11}, {16, 10}, {18, 9}, {20, 8}, {22, 8}}};

    unsigned max_id = 120;
    unsigned now_id = GAMECLIENT.Interpolate(max_id, current_ev);
    bool rotate_sails = true;

    if(now_id < 4) // hinauslaufen teil 1
    {
        LOADER.GetNationImage(workplace->GetNation(), 250 + 5 * BLD_MILL + 4)->DrawFull(drawPt);
        LOADER.bob_jobs_cache[workplace->GetNation()][JOB_MILLER][4][now_id % 8].draw(drawPt + walkoffsets[now_id], COLOR_WHITE,
                                                                                      gwg->GetPlayer(player).color);
        rotate_sails = false;
    }
    if((now_id >= 4) && (now_id < 8)) // hinauslaufen teil 2
    {
        LOADER.bob_jobs_cache[workplace->GetNation()][JOB_MILLER][3][now_id % 8].draw(drawPt + walkoffsets[now_id], COLOR_WHITE,
                                                                                      gwg->GetPlayer(player).color);
    }
    if((now_id >= 8) && (now_id < 16)) // hinsetzen
    {
        LOADER.GetPlayerImage("rom_bobs", 166 + (now_id % 8))
          ->DrawFull(drawPt + offsets_sitdown[workplace->GetNation()], COLOR_WHITE, gwg->GetPlayer(workplace->GetPlayer()).color);
    }
    if((now_id >= 16) && (now_id < max_id - 16)) // schlafen
    {
        LOADER.GetPlayerImage("rom_bobs", 174 + (now_id % 8))
          ->DrawFull(drawPt + offsets[workplace->GetNation()], COLOR_WHITE, gwg->GetPlayer(workplace->GetPlayer()).color);
    }
    if((now_id >= max_id - 16) && (now_id < max_id - 8)) // aufstehn
    {
        LOADER.GetPlayerImage("rom_bobs", 166 + 7 - (now_id % 8))
          ->DrawFull(drawPt + offsets_sitdown[workplace->GetNation()], COLOR_WHITE, gwg->GetPlayer(workplace->GetPlayer()).color);
    }
    if((now_id >= max_id - 8) && (now_id < max_id - 4)) // zurücklaufen teil 1
    {
        LOADER.bob_jobs_cache[workplace->GetNation()][JOB_MILLER][0][now_id % 8].draw(drawPt + walkoffsets[7 - (now_id % 8)], COLOR_WHITE,
                                                                                      gwg->GetPlayer(player).color);
    }
    if((now_id >= max_id - 4) && (now_id < max_id)) // zurücklaufen teil 2
    {
        LOADER.GetNationImage(workplace->GetNation(), 250 + 5 * BLD_MILL + 4)->DrawFull(drawPt);
        LOADER.bob_jobs_cache[workplace->GetNation()][JOB_MILLER][1][now_id % 8].draw(drawPt + walkoffsets[7 - (now_id % 8)], COLOR_WHITE,
                                                                                      gwg->GetPlayer(player).color);
        rotate_sails = false;
    }

    if(rotate_sails)
    {
        // Flügel der Mühle
        LOADER.GetNationImage(workplace->GetNation(), 250 + 5 * (42 + ((now_id + 4) % 8)))->DrawFull(drawPt);
        // Schatten der Flügel
        LOADER.GetNationImage(workplace->GetNation(), 250 + (5 * (42 + ((now_id + 4) % 8))) + 1)->DrawFull(drawPt, COLOR_SHADOW);

        // Mühlensound abspielen in zufälligen Intervallen
        if(VIDEODRIVER.GetTickCount() - last_sound > next_interval)
        {
            SOUNDMANAGER.PlayNOSound(77, this, now_id);
            was_sounding = true;

            last_sound = VIDEODRIVER.GetTickCount();
            next_interval = 500 + rand() % 1400;
        }
    } else
    {
        // Flügel der Mühle
        LOADER.GetNationImage(workplace->GetNation(), 250 + 5 * 49)->DrawFull(drawPt);
        // Schatten der Flügel
        LOADER.GetNationImage(workplace->GetNation(), 250 + 5 * 49 + 1)->DrawFull(drawPt, COLOR_SHADOW);
    }
}

helpers::OptionalEnum<GoodType> nofMiller::ProduceWare()
{
    return GD_FLOUR;
}
