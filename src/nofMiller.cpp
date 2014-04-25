// $Id: nofMiller.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
#include "nofMiller.h"

#include "Loader.h"
#include "macros.h"
#include "GameClient.h"
#include "nobUsual.h"
#include "VideoDriverWrapper.h"
#include "SoundManager.h"

#include "glSmartBitmap.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

nofMiller::nofMiller(const unsigned short x, const unsigned short y, const unsigned char player, nobUsual* workplace)
    : nofWorkman(JOB_MILLER, x, y, player, workplace), last_sound(0), next_interval(0)
{
}

void nofMiller::Serialize_nofMiller(SerializedGameData* sgd) const
{
    Serialize_nofWorkman(sgd);
}

nofMiller::nofMiller(SerializedGameData* sgd, const unsigned obj_id) : nofWorkman(sgd, obj_id), last_sound(0), next_interval(0)
{
}

void nofMiller::DrawWorking(int x, int y)
{
    signed char offsets[NATION_COUNT][2] = { {20, 8}, {20, 8}, {20, 8}, {20, 8}, {20, 8} };
    signed char offsets_sitdown[NATION_COUNT][2] = { {23, 8}, {23, 8}, {23, 8}, {23, 8}, {23, 8} };
    signed char walkoffsets[8][2] = { {8, 8}, {10, 9}, {12, 10}, {14, 11}, {16, 10}, {18, 9}, {20, 8}, {22, 8} };

    unsigned int max_id = 120;
    unsigned now_id = GAMECLIENT.Interpolate(max_id, current_ev);
    bool rotate_sails = true;

    if(now_id < 4) //hinauslaufen teil 1
    {
        LOADER.GetNationImageN(workplace->GetNation(), 250 + 5 * BLD_MILL + 4)->Draw(x, y, 0, 0, 0, 0, 0, 0);
        Loader::bob_jobs_cache[workplace->GetNation()][JOB_MILLER][4][now_id % 8].draw(x + walkoffsets[now_id % 8][0], y + walkoffsets[now_id % 8][1], COLOR_WHITE, COLORS[gwg->GetPlayer(player)->color]);
//        LOADER.GetBobN("jobs")->Draw(16,4,true,now_id%8,x+walkoffsets[now_id%8][0],y+walkoffsets[now_id%8][1],COLORS[gwg->GetPlayer(player)->color]);
        rotate_sails = false;
    }
    if( (now_id >= 4) && (now_id < 8) ) //hinauslaufen teil 2
    {
        Loader::bob_jobs_cache[workplace->GetNation()][JOB_MILLER][3][now_id % 8].draw(x + walkoffsets[now_id % 8][0], y + walkoffsets[now_id % 8][1], COLOR_WHITE, COLORS[gwg->GetPlayer(player)->color]);
//        LOADER.GetBobN("jobs")->Draw(16,3,true,now_id%8,x+walkoffsets[now_id%8][0],y+walkoffsets[now_id%8][1],COLORS[gwg->GetPlayer(player)->color]);
    }
    if( (now_id >= 8) && (now_id < 16)) //hinsetzen
    {
        LOADER.GetImageN("rom_bobs", 166 + (now_id % 8))
        ->Draw(x + offsets_sitdown[workplace->GetNation()][0], y + offsets_sitdown[workplace->GetNation()][1], 0, 0, 0, 0, 0, 0, COLOR_WHITE, COLORS[gwg->GetPlayer(workplace->GetPlayer())->color]);
    }
    if( (now_id >= 16) && (now_id < max_id - 16)) //schlafen
    {
        LOADER.GetImageN("rom_bobs", 174 + (now_id % 8))
        ->Draw(x + offsets[workplace->GetNation()][0], y + offsets[workplace->GetNation()][1], 0, 0, 0, 0, 0, 0, COLOR_WHITE, COLORS[gwg->GetPlayer(workplace->GetPlayer())->color]);
    }
    if( (now_id >= max_id - 16) && (now_id < max_id - 8)) //aufstehn
    {
        LOADER.GetImageN("rom_bobs", 166 + 7 - (now_id % 8))
        ->Draw(x + offsets_sitdown[workplace->GetNation()][0], y + offsets_sitdown[workplace->GetNation()][1], 0, 0, 0, 0, 0, 0, COLOR_WHITE, COLORS[gwg->GetPlayer(workplace->GetPlayer())->color]);
    }
    if( (now_id >= max_id - 8) && (now_id < max_id - 4)) //zurücklaufen teil 1
    {
        Loader::bob_jobs_cache[workplace->GetNation()][JOB_MILLER][0][now_id % 8].draw(x + walkoffsets[7 - (now_id % 8)][0], y + walkoffsets[7 - (now_id % 8)][1], COLOR_WHITE, COLORS[gwg->GetPlayer(player)->color]);
//        LOADER.GetBobN("jobs")->Draw(16,0,true,now_id%8,x+walkoffsets[7-(now_id%8)][0],y+walkoffsets[7-(now_id%8)][1],COLORS[gwg->GetPlayer(player)->color]);
    }
    if( (now_id >= max_id - 4) && (now_id < max_id)) //zurücklaufen teil 2
    {
        LOADER.GetNationImageN(workplace->GetNation(), 250 + 5 * BLD_MILL + 4)->Draw(x, y, 0, 0, 0, 0, 0, 0);
        Loader::bob_jobs_cache[workplace->GetNation()][JOB_MILLER][1][now_id % 8].draw(x + walkoffsets[7 - (now_id % 8)][0], y + walkoffsets[7 - (now_id % 8)][1], COLOR_WHITE, COLORS[gwg->GetPlayer(player)->color]);
//        LOADER.GetBobN("jobs")->Draw(16,1,true,now_id%8,x+walkoffsets[7-(now_id%8)][0],y+walkoffsets[7-(now_id%8)][1],COLORS[gwg->GetPlayer(player)->color]);
        rotate_sails = false;
    }

    if (rotate_sails)
    {
        // Flügel der Mühle
        LOADER.GetNationImageN(workplace->GetNation(), 250 + 5 * (42 + ((now_id + 4) % 8)))->Draw(x, y, 0, 0, 0, 0, 0, 0);
        // Schatten der Flügel
        LOADER.GetNationImageN(workplace->GetNation(), 250 + (5 * (42 + ((now_id + 4) % 8))) + 1)->Draw(x, y, 0, 0, 0, 0, 0, 0, COLOR_SHADOW);

        // Mühlensound abspielen in zufälligen Intervallen
        if(VideoDriverWrapper::inst().GetTickCount() - last_sound > next_interval)
        {
            SoundManager::inst().PlayNOSound(77, this, now_id);
            was_sounding = true;

            last_sound = VideoDriverWrapper::inst().GetTickCount();
            next_interval = 500 + rand() % 1400;
        }


    }
    else
    {
        // Flügel der Mühle
        LOADER.GetNationImageN(workplace->GetNation(), 250 + 5 * 49)->Draw(x, y, 0, 0, 0, 0, 0, 0);
        // Schatten der Flügel
        LOADER.GetNationImageN(workplace->GetNation(), 250 + 5 * 49 + 1)->Draw(x, y, 0, 0, 0, 0, 0, 0, COLOR_SHADOW);
    }

}

GoodType nofMiller::ProduceWare()
{
    return GD_FLOUR;
}
