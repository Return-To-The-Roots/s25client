// $Id: nofArmorer.cpp 9567 2015-01-03 19:34:57Z marcus $
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
#include "nofArmorer.h"
#include "Loader.h"
#include "macros.h"
#include "GameClient.h"
#include "nobUsual.h"
#include "SoundManager.h"
#include "SerializedGameData.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

nofArmorer::nofArmorer(const unsigned short x, const unsigned short y, const unsigned char player, nobUsual* workplace)
    : nofWorkman(JOB_ARMORER, x, y, player, workplace), sword_shield(false)
{
}

void nofArmorer::Serialize_nofArmorer(SerializedGameData* sgd) const
{
    Serialize_nofWorkman(sgd);

    sgd->PushBool(sword_shield);
}

nofArmorer::nofArmorer(SerializedGameData* sgd, const unsigned obj_id) : nofWorkman(sgd, obj_id),
    sword_shield(sgd->PopBool())
{
}

void nofArmorer::DrawWorking(int x, int y)
{
    signed char offsets[NATION_COUNT][2] = { { -10, 15}, { -11, 9}, { -14, 16}, { -19, 1}, { -11, 9} };

    unsigned int max_id = 280;
    unsigned now_id = GAMECLIENT.Interpolate(max_id, current_ev);
    unsigned char wpNation = workplace->GetNation();
    unsigned int plColor = GAMECLIENT.GetPlayer(player)->color;

    if(now_id < 200)
    {
        LOADER.GetImageN("rom_bobs", 16 + (now_id % 8))
        ->Draw(x + offsets[workplace->GetNation()][0], y + offsets[wpNation][1], 0, 0, 0, 0, 0, 0, COLOR_WHITE, COLORS[plColor]);

        if((now_id % 8) == 5)
        {
            SoundManager::inst().PlayNOSound(52, this, now_id / 8);
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
        switch(GAMECLIENT.GetPlayer(player)->nation)
        {
            case 0: return 60;
            case 1: return 58;
            case 2: return 57;
            case 3: return 59;
			case 4: return 58; //babylonians use japanese shield carry-animation
            default: return 0;
        }
    }
}

void nofArmorer::HandleDerivedEvent(const unsigned int id)
{	
    switch(state)
    {
        case STATE_WAITING1:
        {
			if(!GameClient::inst().GetGGS().isEnabled(ADDON_HALF_COST_MIL_EQUIP) || !sword_shield)
			{
				//LOG.lprintf("armorer handlewait1 - consume wares %i \n",player);
				nofWorkman::HandleStateWaiting1();
			}
			else
			{
				// Nach 1. Warten wird gearbeitet
				current_ev = em->AddEvent(this, JOB_CONSTS[job].work_length, 1);
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

void nofArmorer::TryToWork()
{
    // Wurde die Produktion eingestellt?
    if(workplace->IsProductionDisabled())
    {
        state = STATE_WAITINGFORWARES_OR_PRODUCTIONSTOPPED;
        // Nun arbeite ich nich mehr
        StartNotWorking();
    }
    else if ( workplace->WaresAvailable() || (GameClient::inst().GetGGS().isEnabled(ADDON_HALF_COST_MIL_EQUIP) && sword_shield )) 
    {
        state = STATE_WAITING1;
        current_ev = em->AddEvent(this, JOB_CONSTS[job].wait1_length, 1);
        StopNotWorking();

    }
    else
    {
        state = STATE_WAITINGFORWARES_OR_PRODUCTIONSTOPPED;
        // Nun arbeite ich nich mehr
        StartNotWorking();
    }
}

GoodType nofArmorer::ProduceWare()
{
    sword_shield = !sword_shield;

    if(sword_shield)
        return GD_SWORD;
    else
	return SHIELD_TYPES[GAMECLIENT.GetPlayer(player)->nation];
}
