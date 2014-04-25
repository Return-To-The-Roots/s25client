// $Id: nofBaker.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
#include "nofBaker.h"
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

nofBaker::nofBaker(const unsigned short x, const unsigned short y, const unsigned char player, nobUsual* workplace)
    : nofWorkman(JOB_BAKER, x, y, player, workplace)
{
}

nofBaker::nofBaker(SerializedGameData* sgd, const unsigned obj_id) : nofWorkman(sgd, obj_id)
{
}

void nofBaker::DrawWorking(int x, int y)
{
    signed char offsets[NATION_COUNT][2] = { {40, -4}, { -16, 8}, { -5, 9}, { -8, 7}, { -16, 8} };
    signed char walkoffsets[NATION_COUNT][8][2] =   //nation, schrit, x-y
    {
        { {10, 10}, {17, 12}, {24, 14}, {32, 14}, {34, 9}, {36, 4}, {38, -1}, {40, -4} },
        { {9, 11}, {11, 13}, {7, 17}, {3, 20}, { -1, 17}, { -5, 14}, { -9, 12}, { -13, 10} },
        { {9, 9}, {11, 11}, {9, 13}, {7, 15}, {4, 13}, {1, 11}, { -2, 9}, { -5, 9} },
        { {9, 11}, {11, 13}, {9, 15}, {7, 17}, {4, 15}, {1, 13}, { -2, 11}, { -5, 9} },
        { {9, 11}, {11, 13}, {7, 17}, {3, 20}, { -1, 17}, { -5, 14}, { -9, 12}, { -13, 10} }
    };
    signed char walkdirection[NATION_COUNT][6] =
    {
        {3, 3, 2, 5, 0, 0},
        {4, 5, 0, 3, 2, 1},
        {4, 5, 0, 3, 2, 1},
        {4, 5, 0, 3, 2, 1},
        {4, 5, 0, 3, 2, 1}
    };

    unsigned int max_id = 120;
    unsigned now_id = GAMECLIENT.Interpolate(max_id, current_ev);
    unsigned char wpNation = workplace->GetNation();
    unsigned int plColor = GAMECLIENT.GetPlayer(player)->color;

    //position zum rauslaufen berechnen
    int walkx = x + walkoffsets[wpNation][now_id % 8][0];
    int walky = y + walkoffsets[wpNation][now_id % 8][1];
    //position zum reinlaufen berechnen
    int walkx_r = x + walkoffsets[wpNation][7 - (now_id % 8)][0];
    int walky_r = y + walkoffsets[wpNation][7 - (now_id % 8)][1];


    if(now_id < 2) //hinauslaufen teil 1
    {
        LOADER.GetNationImageN(wpNation, 250 + 5 * BLD_BAKERY + 4)->Draw(x, y, 0, 0, 0, 0, 0, 0);
        Loader::bob_jobs_cache[wpNation][JOB_BAKER][walkdirection[wpNation][0]][now_id % 8].draw(walkx, walky, COLOR_WHITE, COLORS[plColor]);
//        LOADER.GetBobN("jobs")->Draw(17,walkdirection[wpNation][0],true,now_id%8,walkx,walky,COLORS[plColor]);
    }
    if((now_id >= 2) && (now_id < 4) ) //hinauslaufen teil 2
    {
        LOADER.GetNationImageN(wpNation, 250 + 5 * BLD_BAKERY + 4)->Draw(x, y, 0, 0, 0, 0, 0, 0);
        Loader::bob_jobs_cache[wpNation][JOB_BAKER][walkdirection[wpNation][1]][now_id % 8].draw(walkx, walky, COLOR_WHITE, COLORS[plColor]);
//        LOADER.GetBobN("jobs")->Draw(17,walkdirection[wpNation][1],true,now_id%8,walkx,walky,COLORS[plColor]);
    }
    if((now_id >= 4) && (now_id < 8) ) //hinauslaufen teil 3
    {
        Loader::bob_jobs_cache[wpNation][JOB_BAKER][walkdirection[wpNation][2]][now_id % 8].draw(walkx, walky, COLOR_WHITE, COLORS[plColor]);
//        LOADER.GetBobN("jobs")->Draw(17,walkdirection[wpNation][2],true,now_id%8,walkx,walky,COLORS[plColor]);
    }
    if((now_id >= 8) && (now_id < 16) ) //brot in den ofen schieben
    {
        LOADER.GetImageN("rom_bobs", 182 + (now_id % 8))
        ->Draw(x + offsets[wpNation][0], y + offsets[wpNation][1], 0, 0, 0, 0, 0, 0, COLOR_WHITE, COLORS[plColor]);

        // "Brot-rein/raus"-Sound
        if((now_id % 8) == 4)
        {
            SoundManager::inst().PlayNOSound(68, this, now_id);
            was_sounding = true;
        }
    }
    if((now_id >= 16) && (now_id < max_id - 16) ) //warten
    {
        LOADER.GetImageN("rom_bobs", 189)
        ->Draw(x + offsets[wpNation][0], y + offsets[wpNation][1], 0, 0, 0, 0, 0, 0, COLOR_WHITE, COLORS[plColor]);
    }
    if((now_id >= max_id - 16) && (now_id < max_id - 8) ) //brot aus dem ofen holen
    {
        LOADER.GetImageN("rom_bobs", 182 + 7 - (now_id % 8))
        ->Draw(x + offsets[wpNation][0], y + offsets[wpNation][1], 0, 0, 0, 0, 0, 0, COLOR_WHITE, COLORS[plColor]);

        // "Brot-rein/raus"-Sound
        if((now_id % 8) == 4)
        {
            SoundManager::inst().PlayNOSound(68, this, now_id);
            was_sounding = true;
        }
    }
    if((now_id >= max_id - 8) && (now_id < max_id - 4) ) //reingehn teil 1
    {
        Loader::bob_jobs_cache[wpNation][JOB_BAKER][walkdirection[wpNation][3]][now_id % 8].draw(walkx_r, walky_r, COLOR_WHITE, COLORS[plColor]);
//        LOADER.GetBobN("jobs")->Draw(17,walkdirection[wpNation][3],true,now_id%8,walkx_r,walky_r,COLORS[plColor]);
    }
    if((now_id >= max_id - 4) && (now_id < max_id - 2) ) //reingehn teil 1
    {
        LOADER.LOADER.GetNationImageN(wpNation, 250 + 5 * BLD_BAKERY + 4)->Draw(x, y, 0, 0, 0, 0, 0, 0);
        Loader::bob_jobs_cache[wpNation][JOB_BAKER][walkdirection[wpNation][4]][now_id % 8].draw(walkx_r, walky_r, COLOR_WHITE, COLORS[plColor]);
//        LOADER.GetBobN("jobs")->Draw(17,walkdirection[wpNation][4],true,now_id%8,walkx_r,walky_r,COLORS[plColor]);
    }
    if((now_id >= max_id - 2) && (now_id < max_id) ) //reingehn teil 2
    {
        LOADER.LOADER.GetNationImageN(wpNation, 250 + 5 * BLD_BAKERY + 4)->Draw(x, y, 0, 0, 0, 0, 0, 0);
        Loader::bob_jobs_cache[wpNation][JOB_BAKER][walkdirection[wpNation][5]][now_id % 8].draw(walkx_r, walky_r, COLOR_WHITE, COLORS[plColor]);
//        LOADER.GetBobN("jobs")->Draw(17,walkdirection[wpNation][5],true,now_id%8,walkx_r,walky_r,COLORS[plColor]);
    }

}

GoodType nofBaker::ProduceWare()
{
    return GD_BREAD;
}

