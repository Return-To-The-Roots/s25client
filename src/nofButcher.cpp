// $Id: nofButcher.cpp 9503 2014-11-29 10:47:02Z marcus $
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
#include "nofButcher.h"
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

nofButcher::nofButcher(const unsigned short x, const unsigned short y, const unsigned char player, nobUsual* workplace)
    : nofWorkman(JOB_BUTCHER, x, y, player, workplace)
{
}

nofButcher::nofButcher(SerializedGameData* sgd, const unsigned obj_id) : nofWorkman(sgd, obj_id)
{
}

void nofButcher::DrawWorking(int x, int y)
{
    signed char offsets[NATION_COUNT][2] = { {38, 2}, { -3, 5}, {21, -1}, {26, -5}, { -7, 2} };

    unsigned now_id;

    LOADER.GetImageN("rom_bobs", 160 + (now_id = GAMECLIENT.Interpolate(136, current_ev)) % 6)
    ->Draw(x + offsets[workplace->GetNation()][0], y + offsets[workplace->GetNation()][1], 0, 0, 0, 0, 0, 0, COLOR_WHITE, COLORS[gwg->GetPlayer(workplace->GetPlayer())->color]);

    if(now_id % 6 == 5)
    {
        SoundManager::inst().PlayNOSound(66, this, now_id / 6);
        was_sounding = true;
    }

    last_id = now_id;
}

GoodType nofButcher::ProduceWare()
{
    return GD_MEAT;
}
