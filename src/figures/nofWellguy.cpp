// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "rttrDefines.h" // IWYU pragma: keep
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
#include "world/GameWorldGame.h"

nofWellguy::nofWellguy(const MapPoint pos, const unsigned char player, nobUsual* workplace) : nofWorkman(JOB_HELPER, pos, player, workplace)
{}

nofWellguy::nofWellguy(const MapPoint pos, const unsigned char player, nobBaseWarehouse* goalWh)
    : nofWorkman(JOB_HELPER, pos, player, goalWh)
{}

nofWellguy::nofWellguy(SerializedGameData& sgd, const unsigned obj_id) : nofWorkman(sgd, obj_id) {}

void nofWellguy::DrawWorking(DrawPoint drawPt)
{
    const DrawPointInit offsets[NUM_NATS] = {{-20, 17}, {-18, 17}, {-20, 13}, {-20, 15}, {-18, 17}};

    const DrawPointInit walkoffsets[NUM_NATS][8] = // nation, schritt
      {{{7, 7}, {9, 9}, {5, 12}, {2, 14}, {-1, 17}, {-4, 17}, {-7, 17}, {-10, 17}},
       {{4, 4}, {8, 8}, {5, 12}, {2, 14}, {-1, 17}, {-3, 19}, {-6, 19}, {-8, 19}},
       {{5, 5}, {8, 8}, {5, 10}, {2, 13}, {-1, 13}, {-4, 13}, {-7, 13}, {-10, 13}},
       {{5, 5}, {8, 8}, {5, 10}, {2, 13}, {-1, 15}, {-4, 15}, {-7, 15}, {-10, 15}},
       {{4, 4}, {8, 8}, {5, 12}, {2, 14}, {-1, 17}, {-3, 19}, {-6, 19}, {-8, 19}}};
    signed char walkdirection[NUM_NATS][6] = {
      {4, 5, 0, 3, 2, 1}, {4, 5, 0, 3, 2, 1}, {4, 5, 0, 3, 2, 1}, {4, 5, 0, 3, 2, 1}, {4, 5, 0, 3, 2, 1}};

    unsigned max_id = 112;
    unsigned now_id = GAMECLIENT.Interpolate(max_id, current_ev);
    unsigned char wpNation = workplace->GetNation();
    unsigned plColor = gwg->GetPlayer(player).color;

    // position zum rauslaufen berechnen
    DrawPoint walkOutPos = drawPt + walkoffsets[wpNation][now_id % 8];
    // position zum reinlaufen berechnen
    DrawPoint walkInPos = drawPt + walkoffsets[wpNation][7 - (now_id % 8)];

    if(now_id < 2) // laufen 1
    {
        if(wpNation == NAT_ROMANS)
            LOADER.GetNationImage(wpNation, 250 + 5 * BLD_WELL + 4)->DrawFull(drawPt);
        LOADER.carrier_cache[10][walkdirection[wpNation][0]][now_id % 8][false].draw(walkOutPos, COLOR_WHITE, plColor);
    } else if(now_id < 4) // laufen 2
    {
        if(wpNation == NAT_ROMANS)
            LOADER.GetNationImage(wpNation, 250 + 5 * BLD_WELL + 4)->DrawFull(drawPt);
        LOADER.carrier_cache[10][walkdirection[wpNation][1]][now_id % 8][false].draw(walkOutPos, COLOR_WHITE, plColor);
    } else if(now_id < 8) // laufen 3
    {
        LOADER.carrier_cache[10][walkdirection[wpNation][2]][now_id % 8][false].draw(walkOutPos, COLOR_WHITE, plColor);
    } else if(now_id < 16) // eimer runter lassen
    {
        if(now_id == 8)
            LOADER.GetPlayerImage("rom_bobs", 346)->DrawFull(drawPt + offsets[wpNation], COLOR_WHITE, plColor);
        else
            LOADER.GetPlayerImage("rom_bobs", 346 + (now_id % 8) - 1)->DrawFull(drawPt + offsets[wpNation], COLOR_WHITE, plColor);
    } else if(now_id < max_id - 16) // kurbeln
    {
        LOADER.GetPlayerImage("rom_bobs", 330 + (now_id % 8))->DrawFull(drawPt + offsets[wpNation], COLOR_WHITE, plColor);
    } else if(now_id < max_id - 8) // eimer rauf kurbeln
    {
        LOADER.GetPlayerImage("rom_bobs", 338 + (now_id % 8))->DrawFull(drawPt + offsets[wpNation], COLOR_WHITE, plColor);
    } else if(now_id < max_id - 4) // laufen 3
    {
        LOADER.carrier_cache[11][walkdirection[wpNation][3]][now_id % 8][false].draw(walkInPos, COLOR_WHITE, plColor);
    } else if(now_id < max_id - 2) // laufen 2
    {
        if(wpNation == NAT_ROMANS)
            LOADER.GetNationImage(wpNation, 250 + 5 * BLD_WELL + 4)->DrawFull(drawPt);
        LOADER.carrier_cache[11][walkdirection[wpNation][4]][now_id % 8][false].draw(walkInPos, COLOR_WHITE, plColor);
    } else // laufen 1
    {
        if(wpNation == NAT_ROMANS)
            LOADER.GetNationImage(wpNation, 250 + 5 * BLD_WELL + 4)->DrawFull(drawPt);
        LOADER.carrier_cache[11][walkdirection[wpNation][5]][now_id % 8][false].draw(walkInPos, COLOR_WHITE, plColor);
    }

    if((now_id >= 8) && (now_id < max_id - 8) && now_id % 8 == 4)
    {
        SOUNDMANAGER.PlayNOSound(82, this, now_id);
        was_sounding = true;
    }
}

GoodType nofWellguy::ProduceWare()
{
    return GD_WATER;
}

bool nofWellguy::AreWaresAvailable() const
{
    // Check for water
    return FindPointWithResource(Resource::Water).isValid();
}

bool nofWellguy::StartWorking()
{
    MapPoint resPt = FindPointWithResource(Resource::Water);
    if(!resPt.isValid())
        return false;
    if(gwg->GetGGS().getSelection(AddonId::EXHAUSTIBLE_WELLS) == 2)
        gwg->ReduceResource(resPt);
    return nofWorkman::StartWorking();
}
