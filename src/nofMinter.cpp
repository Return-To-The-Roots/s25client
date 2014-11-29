// $Id: nofMinter.cpp 9503 2014-11-29 10:47:02Z marcus $
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
#include "nofMinter.h"

#include "Loader.h"
#include "macros.h"
#include "GameClient.h"
#include "nobUsual.h"
#include "SoundManager.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

nofMinter::nofMinter(const unsigned short x, const unsigned short y, const unsigned char player, nobUsual* workplace)
    : nofWorkman(JOB_MINTER, x, y, player, workplace)
{
}

void nofMinter::Serialize_nofMinter(SerializedGameData* sgd) const
{
    Serialize_nofWorkman(sgd);
}

nofMinter::nofMinter(SerializedGameData* sgd, const unsigned obj_id) : nofWorkman(sgd, obj_id)
{
}

void nofMinter::DrawWorking(int x, int y)
{
    signed char offsets[NATION_COUNT][2] = { {19, -20}, {19, -11}, {22, -12}, {28, 1}, {16, -12} };

    unsigned now_id = GAMECLIENT.Interpolate(136, current_ev);

    if(now_id < 91)
    {
        LOADER.GetImageN("rom_bobs", 84 + (now_id) % 8)
        ->Draw(x + offsets[workplace->GetNation()][0], y + offsets[workplace->GetNation()][1], 0, 0, 0, 0, 0, 0, COLOR_WHITE, COLORS[gwg->GetPlayer(workplace->GetPlayer())->color]);

        // Evtl Sound abspielen
        if(now_id % 8 == 3)
        {
            SoundManager::inst().PlayNOSound(58, this, now_id);
            was_sounding = true;
        }
    }

    last_id = now_id;
}

GoodType nofMinter::ProduceWare()
{
    gwg->GetPlayer(player)->ChangeStatisticValue(STAT_GOLD, 1);
    return GD_COINS;
}
