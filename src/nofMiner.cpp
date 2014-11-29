// $Id: nofMiner.cpp 9501 2014-11-29 10:45:47Z marcus $
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
#include "nofMiner.h"

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

nofMiner::nofMiner(const unsigned short x, const unsigned short y, const unsigned char player, nobUsual* workplace)
    : nofWorkman(JOB_MINER, x, y, player, workplace)
{
}

nofMiner::nofMiner(SerializedGameData* sgd, const unsigned obj_id) : nofWorkman(sgd, obj_id)
{
}

void nofMiner::DrawWorking(int x, int y)
{
    const signed char offsets[40] = //work animation offset in x,y granite, coal, iron, gold 
    {
        5, 3, 5, 3, 5, 3, 5, 3,		//africans
        4, 1, 4, 1, 4, 1, 4, 1,		//japanese
        9, 4, 9, 4, 9, 4, 9, 4,		//romans
        10, 3, 10, 3, 10, 3, 10, 3, //vikings
        8, 3, 8, 3, 8, 3, 8, 3		//babylonians
    };

    unsigned now_id = GAMECLIENT.Interpolate(160, current_ev);
    if(workplace->GetNation() == 2)
        LOADER.GetImageN("rom_bobs", 92 + now_id % 8)->Draw(x + offsets[workplace->GetNation() * 8 + (workplace->GetBuildingType() - BLD_GRANITEMINE) * 2],
                y + offsets[workplace->GetNation() * 8 + (workplace->GetBuildingType() - BLD_GRANITEMINE) * 2 + 1], 0, 0, 0, 0, 0, 0, COLOR_WHITE, COLOR_WHITE);
    else
        LOADER.GetImageN("rom_bobs", 1799 + now_id % 4)
        ->Draw(x + offsets[workplace->GetNation() * 8 + (workplace->GetBuildingType() - BLD_GRANITEMINE) * 2],
               y + offsets[workplace->GetNation() * 8 + (workplace->GetBuildingType() - BLD_GRANITEMINE) * 2 + 1], 0, 0, 0, 0, 0, 0, COLOR_WHITE, COLOR_WHITE);
    // 1799

    if(now_id % 8 == 3)
    {
        SoundManager::inst().PlayNOSound(59, this, now_id);
        was_sounding = true;
    }
}

unsigned short nofMiner::GetCarryID() const
{
    switch(workplace->GetBuildingType())
    {
        case BLD_GOLDMINE: return 65;
        case BLD_IRONMINE: return 66;
        case BLD_COALMINE: return 67;
        default: return 68;
    }
}

GoodType nofMiner::ProduceWare()
{
    switch(workplace->GetBuildingType())
    {
        case BLD_GOLDMINE: return GD_GOLD;
        case BLD_IRONMINE: return GD_IRONORE;
        case BLD_COALMINE: return GD_COAL;
        default: return GD_STONES;
    }
}
