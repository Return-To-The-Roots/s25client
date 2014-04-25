// $Id: nofPigbreeder.cpp 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "main.h"
#include "nofPigbreeder.h"

#include "Loader.h"
#include "macros.h"
#include "GameClient.h"
#include "nobUsual.h"
#include "SoundManager.h"

#include "glSmartBitmap.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

nofPigbreeder::nofPigbreeder(const unsigned short x, const unsigned short y, const unsigned char player, nobUsual* workplace)
    : nofWorkman(JOB_PIGBREEDER, x, y, player, workplace)
{
}

nofPigbreeder::nofPigbreeder(SerializedGameData* sgd, const unsigned obj_id) : nofWorkman(sgd, obj_id)
{
}

void nofPigbreeder::DrawWorking(int x, int y)
{
    //148
    signed char offsets[NATION_COUNT][2] = { {10, 2}, {10, 2}, {10, 2}, {10, 2}, {10, 2} };
    signed char walkstart[NATION_COUNT][2] = { { -6, -6}, { -6, -6}, { -6, -6}, { -6, -6}, { -6, -6} };

    unsigned int max_id = 240;
    int now_id = GAMECLIENT.Interpolate(max_id, current_ev);
    unsigned char wpNation = workplace->GetNation();
    unsigned int plColor = gwg->GetPlayer(player)->color;
    int walksteps = 16;

//  LOADER.GetImageN("rom_bobs", 148+(now_id = GAMECLIENT.Interpolate(12,current_ev)%12))
//      ->Draw(x+offsets[workplace->GetNation()][0],y+offsets[workplace->GetNation()][1],COLORS[gwg->GetPlayer(workplace->GetPlayer())->color]);

    if(now_id < 16)
    {
        if (now_id < 8) LOADER.GetNationImageN(wpNation, 250 + 5 * BLD_PIGFARM + 4)->Draw(x, y, 0, 0, 0, 0, 0, 0);
        int walkx = x + walkstart[wpNation][0] + (((offsets[wpNation][0] - walkstart[wpNation][0]) * (now_id) / walksteps));
        int walky = y + walkstart[wpNation][1] + (((offsets[wpNation][1] - walkstart[wpNation][1])) * (now_id) / walksteps);

        Loader::bob_jobs_cache[wpNation][JOB_PIGBREEDER][4][now_id % 8].draw(walkx, walky, COLOR_WHITE, COLORS[plColor]);
//        LOADER.GetBobN("jobs")->Draw(14,4,false,now_id%8,walkx,walky,COLORS[plColor]);
    }
    if(now_id >= 16 && now_id < 40)
    {
        LOADER.GetImageN("rom_bobs", 148 + (now_id - 16) / 2)
        ->Draw(x + offsets[wpNation][0], y + offsets[wpNation][1], 0, 0, 0, 0, 0, 0, COLOR_WHITE, COLORS[plColor]);

        // Evtl Sound abspielen
        if((now_id - 16) == 10)
        {
            SoundManager::inst().PlayNOSound(65, this, 0);
            was_sounding = true;
        }
    }
    if(now_id >= 40 && now_id < 56)
    {
        if(now_id > 46) LOADER.GetNationImageN(wpNation, 250 + 5 * BLD_PIGFARM + 4)->Draw(x, y, 0, 0, 0, 0, 0, 0);
        int walkx = x + offsets[wpNation][0] + (((walkstart[wpNation][0] - offsets[wpNation][0])) * (now_id - 40) / walksteps);
        int walky = y + offsets[wpNation][1] + (((walkstart[wpNation][1] - offsets[wpNation][1])) * (now_id - 40) / walksteps);
        Loader::bob_jobs_cache[wpNation][JOB_PIGBREEDER][1][(now_id - 40) % 8].draw(walkx, walky, COLOR_WHITE, COLORS[plColor]);
//        LOADER.GetBobN("jobs")->Draw(14,1,false,(now_id-40)%8,walkx,walky,COLORS[plColor]);
    }

}

GoodType nofPigbreeder::ProduceWare()
{
    return GD_HAM;
}


void nofPigbreeder::MakePigSounds()
{
    /// Ist es wieder Zeit für einen Schweine-Sound?
    if(GameClient::inst().GetGFNumber() - last_id > 600 + unsigned(rand() % 200) - unsigned((*workplace->GetProduktivityPointer()) * 5) &&
            (GameClient::inst().GetGFNumber() != last_id) && !GAMECLIENT.IsPaused())
    {
        // "Oink"
        LOADER.GetSoundN("sound", 86)->Play(255, false);

        last_id = GameClient::inst().GetGFNumber();
    }

}
