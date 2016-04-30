// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#include "defines.h" // IWYU pragma: keep
#include "nofPigbreeder.h"

#include "Loader.h"
#include "GameClient.h"
#include "world/GameWorldGame.h"
#include "buildings/nobUsual.h"
#include "SoundManager.h"
#include "EventManager.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "ogl/glArchivItem_Sound.h"
#include "ogl/glSmartBitmap.h"

nofPigbreeder::nofPigbreeder(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofWorkman(JOB_PIGBREEDER, pos, player, workplace)
{
}

nofPigbreeder::nofPigbreeder(SerializedGameData& sgd, const unsigned obj_id) : nofWorkman(sgd, obj_id)
{
}

void nofPigbreeder::DrawWorking(int x, int y)
{
    signed char offsets[NAT_COUNT][2] = { {10, 2}, {10, 2}, {10, 2}, {10, 2}, {10, 2} };
    signed char walkstart[NAT_COUNT][2] = { { -6, -6}, { -6, -6}, { -6, -6}, { -6, -6}, { -6, -6} };

    unsigned int max_id = 240;
    int now_id = GAMECLIENT.Interpolate(max_id, current_ev);
    unsigned char wpNation = workplace->GetNation();
    unsigned int plColor = gwg->GetPlayer(player).color;
    int walksteps = 16;

    if(now_id < 16)
    {
        if (now_id < 8) LOADER.GetNationImage(wpNation, 250 + 5 * BLD_PIGFARM + 4)->Draw(x, y, 0, 0, 0, 0, 0, 0);
        int walkx = x + walkstart[wpNation][0] + ((offsets[wpNation][0] - walkstart[wpNation][0]) * now_id / walksteps);
        int walky = y + walkstart[wpNation][1] + ((offsets[wpNation][1] - walkstart[wpNation][1]) * now_id / walksteps);

        LOADER.bob_jobs_cache[wpNation][JOB_PIGBREEDER][4][now_id % 8].draw(walkx, walky, COLOR_WHITE, plColor);
    }
    if(now_id >= 16 && now_id < 40)
    {
        LOADER.GetPlayerImage("rom_bobs", 148 + (now_id - 16) / 2)
        ->Draw(x + offsets[wpNation][0], y + offsets[wpNation][1], 0, 0, 0, 0, 0, 0, COLOR_WHITE, plColor);

        // Evtl Sound abspielen
        if((now_id - 16) == 10)
        {
            SOUNDMANAGER.PlayNOSound(65, this, 0);
            was_sounding = true;
        }
    }
    if(now_id >= 40 && now_id < 56)
    {
        if(now_id > 46) LOADER.GetNationImage(wpNation, 250 + 5 * BLD_PIGFARM + 4)->Draw(x, y, 0, 0, 0, 0, 0, 0);
        int walkx = x + offsets[wpNation][0] + (((walkstart[wpNation][0] - offsets[wpNation][0])) * (now_id - 40) / walksteps);
        int walky = y + offsets[wpNation][1] + (((walkstart[wpNation][1] - offsets[wpNation][1])) * (now_id - 40) / walksteps);
        LOADER.bob_jobs_cache[wpNation][JOB_PIGBREEDER][1][(now_id - 40) % 8].draw(walkx, walky, COLOR_WHITE, plColor);
    }

}

GoodType nofPigbreeder::ProduceWare()
{
    return GD_HAM;
}


void nofPigbreeder::MakePigSounds()
{
    /// Ist es wieder Zeit für einen Schweine-Sound?
    unsigned timeDiffSound = 600 + unsigned(rand() % 200) - workplace->GetProductivity() * 5u;
    if(GetEvMgr().GetCurrentGF() - last_id > timeDiffSound)
    {
        // "Oink"
        SOUNDMANAGER.PlayNOSound(86, this, 1);
        last_id = GetEvMgr().GetCurrentGF();
    }

}
