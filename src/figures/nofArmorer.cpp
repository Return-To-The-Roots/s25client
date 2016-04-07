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

///////////////////////////////////////////////////////////////////////////////
// Header


#include "defines.h" // IWYU pragma: keep
#include "nofArmorer.h"
#include "Loader.h"
#include "GameClient.h"
#include "buildings/nobUsual.h"
#include "SoundManager.h"
#include "SerializedGameData.h"
#include "gameData/ShieldConsts.h"
#include "gameData/JobConsts.h"
#include "ogl/glArchivItem_Bitmap_Player.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

nofArmorer::nofArmorer(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofWorkman(JOB_ARMORER, pos, player, workplace), sword_shield(false)
{
}

void nofArmorer::Serialize_nofArmorer(SerializedGameData& sgd) const
{
    Serialize_nofWorkman(sgd);

    sgd.PushBool(sword_shield);
}

nofArmorer::nofArmorer(SerializedGameData& sgd, const unsigned obj_id) : nofWorkman(sgd, obj_id),
    sword_shield(sgd.PopBool())
{
}

void nofArmorer::DrawWorking(int x, int y)
{
    signed char offsets[NAT_COUNT][2] = { { -10, 15}, { -11, 9}, { -14, 16}, { -19, 1}, { -11, 9} };

    unsigned int max_id = 280;
    unsigned now_id = GAMECLIENT.Interpolate(max_id, current_ev);
    unsigned char wpNation = workplace->GetNation();
    unsigned int plColor = GAMECLIENT.GetPlayer(player).color;

    if(now_id < 200)
    {
        LOADER.GetPlayerImage("rom_bobs", 16 + (now_id % 8))
        ->Draw(x + offsets[workplace->GetNation()][0], y + offsets[wpNation][1], 0, 0, 0, 0, 0, 0, COLOR_WHITE, plColor);

        if((now_id % 8) == 5)
        {
            SOUNDMANAGER.PlayNOSound(52, this, now_id / 8);
            was_sounding = true;
        }
    }
}

unsigned short nofArmorer::GetCarryID() const
{
    if(sword_shield)
        return 56;
    else
    {
        // Je nach Nation einen bestimmtem Schild fertigen
        switch(GAMECLIENT.GetPlayer(player).nation)
        {
            case 0: return 60; //-V525
            case 1: return 58;
            case 2: return 57;
            case 3: return 59;
			case 4: return 58; //babylonians use japanese shield carry-animation
            default: return 0;
        }
    }
}

void nofArmorer::HandleDerivedEvent(const unsigned int  /*id*/)
{	
    switch(state)
    {
        case STATE_WAITING1:
        {
			if(!GAMECLIENT.GetGGS().isEnabled(AddonId::HALF_COST_MIL_EQUIP) || !sword_shield)
			{
				//LOG.lprintf("armorer handlewait1 - consume wares %i \n",player);
				nofWorkman::HandleStateWaiting1();
			}
			else
			{
				// Nach 1. Warten wird gearbeitet
				current_ev = em->AddEvent(this, JOB_CONSTS[job_].work_length, 1);
				state = STATE_WORK;
				workplace->is_working = true;
				//LOG.lprintf("armorer handlewait1 - no consume wares %i \n",player);
			}
        } break;
        case STATE_WORK:
        {
            HandleStateWork();
        } break;
        case STATE_WAITING2:
        {
            HandleStateWaiting2();
        } break;
        default:
            break;
	}
}

bool nofArmorer::AreWaresAvailable()
{
    return workplace->WaresAvailable() || (GAMECLIENT.GetGGS().isEnabled(AddonId::HALF_COST_MIL_EQUIP) && sword_shield );
}

GoodType nofArmorer::ProduceWare()
{
    sword_shield = !sword_shield;

    if(sword_shield)
        return GD_SWORD;
    else
	    return SHIELD_TYPES[GAMECLIENT.GetPlayer(player).nation];
}
