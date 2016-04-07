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
#include "nofMetalworker.h"

#include "Loader.h"
#include "GameClient.h"
#include "GameClientPlayer.h"
#include "buildings/nobUsual.h"
#include "Random.h"
#include "SoundManager.h"
#include "SerializedGameData.h"
#include "ingameWindows/iwTools.h"
#include "PostMsg.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "gameTypes/MessageTypes.h"
#include "Log.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

nofMetalworker::nofMetalworker(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofWorkman(JOB_METALWORKER, pos, player, workplace), nextProducedTool(GD_NOTHING)
{
}

nofMetalworker::nofMetalworker(SerializedGameData& sgd, const unsigned obj_id) : nofWorkman(sgd, obj_id), nextProducedTool(GoodType(sgd.PopUnsignedChar()))
{
    if(state == STATE_ENTERBUILDING && current_ev == NULL && ware == GD_NOTHING && nextProducedTool == GD_NOTHING)
    {
        LOG.lprintf("Found invalid metalworker. Assuming corrupted savegame -> Trying to fix this. If you encounter this with a new game, report this!");
        state = STATE_WAITINGFORWARES_OR_PRODUCTIONSTOPPED;
        current_ev = em->AddEvent(this, 1000, 2);
    }
}

void nofMetalworker::Serialize(SerializedGameData& sgd) const
{
    nofWorkman::Serialize(sgd);
    sgd.PushUnsignedChar(nextProducedTool);
}


void nofMetalworker::DrawWorking(int x, int y)
{
    signed char offsets[NAT_COUNT][2] = { { -11, -13}, {31, 5}, {32, 6}, {30, 10}, {28, 5} };

    const unsigned now_id = GAMECLIENT.Interpolate(230, current_ev);

    LOADER.GetPlayerImage("rom_bobs", 190 + (now_id % 23))
    ->Draw(x + offsets[workplace->GetNation()][0], y + offsets[workplace->GetNation()][1], 0, 0, 0, 0, 0, 0, COLOR_WHITE, gwg->GetPlayer(workplace->GetPlayer()).color);

    // Hämmer-Sound
    if(now_id % 23 == 3 || now_id % 23 == 7)
    {
        SOUNDMANAGER.PlayNOSound(72, this, now_id, 100);
        was_sounding = true;
    }
    // Säge-Sound 1
    else if(now_id % 23 == 9)
    {
        SOUNDMANAGER.PlayNOSound(54, this, now_id);
        was_sounding = true;
    }
    else if(now_id % 23 == 17)
    {
        SOUNDMANAGER.PlayNOSound(55, this, now_id);
        was_sounding = true;
    }

    last_id = now_id;
}



// Zuordnungnen Richtige IDs - Trage-IDs in der JOBS.BOB
const unsigned short CARRYTOOLS_IDS[14] =
{
    78, 79, 80, 91, 81, 82, 83, 84, 85, 86, 0, 87, 88, 88
};

unsigned short nofMetalworker::GetCarryID() const
{
    return CARRYTOOLS_IDS[ware - GD_TONGS];
}

/// Zuordnungen Werkzeugeinstellungs-ID - Richtige IDs
const GoodType TOOLS_SETTINGS_IDS[TOOL_COUNT] =
{
    GD_TONGS,       // Zange
    GD_AXE,         // Axt,
    GD_SAW,         // Säge
    GD_PICKAXE,     // Spitzhacke
    GD_HAMMER,      // Hammer
    GD_SHOVEL,      // Schaufel
    GD_CRUCIBLE,    // Schmelztiegel
    GD_RODANDLINE,  // Angel
    GD_SCYTHE,      // Sense
    GD_CLEAVER,     // Beil
    GD_ROLLINGPIN,  // Nudelholz
    GD_BOW          // Bogen
};

unsigned nofMetalworker::ToolsOrderedTotal() const
{
    unsigned sum = 0;
    for (unsigned i = 0; i < TOOL_COUNT; ++i)
        sum += gwg->GetPlayer(player).tools_ordered[i];
    return sum;
}

GoodType nofMetalworker::GetOrderedTool()
{
    // qx:tools
    int prio = -1;
    int tool = -1;

    GameClientPlayer& owner = gwg->GetPlayer(player);
    for (unsigned i = 0; i < TOOL_COUNT; ++i)
    {
        if (owner.tools_ordered[i] > 0 && (owner.toolsSettings_[i] > prio) )
        {
            prio = owner.toolsSettings_[i];
            tool = i;
        }
    }

    if (tool != -1)
    {
        --owner.tools_ordered[tool];

        if ( (player == GAMECLIENT.GetPlayerID()) && (ToolsOrderedTotal() == 0) )
        {
            GAMECLIENT.SendPostMessage( new PostMsg( _("Completed the ordered amount of tools."), PMC_GENERAL ) );
        }

        iwTools::UpdateOrders();
        return TOOLS_SETTINGS_IDS[tool];
    }
    return GD_NOTHING;
}

GoodType nofMetalworker::GetRandomTool()
{
    // Je nach Werkzeugeinstellungen zufällig ein Werkzeug produzieren, je größer der Balken,
    // desto höher jeweils die Wahrscheinlichkeit
    unsigned short all_size = 0;

    for(unsigned int i = 0; i < TOOL_COUNT; ++i)
        all_size += gwg->GetPlayer(player).toolsSettings_[i];

	// if they're all zero
    if(all_size == 0)
	{
	    // do nothing if addon is enabled, otherwise produce random ware (orig S2 behaviour)
		if (GAMECLIENT.GetGGS().isEnabled(AddonId::METALWORKSBEHAVIORONZERO) && GAMECLIENT.GetGGS().getSelection(AddonId::METALWORKSBEHAVIORONZERO) == 1)
			return GD_NOTHING;
		else
			return TOOLS_SETTINGS_IDS[RANDOM.Rand(__FILE__, __LINE__, GetObjId(), 12)];
	}

    // Ansonsten Array mit den Werkzeugtypen erstellen und davon dann eins zufällig zurückliefern, je höher Wahr-
    // scheinlichkeit (Balken), desto öfter im Array enthalten
    std::vector<unsigned char> random_array(all_size);
    unsigned curIdx = 0;

    for(unsigned int i = 0; i < TOOL_COUNT; ++i)
    {
        for(unsigned g = 0; g < gwg->GetPlayer(player).toolsSettings_[i]; ++g)
            random_array[curIdx++] = i;
    }

    GoodType tool = TOOLS_SETTINGS_IDS[random_array[RANDOM.Rand(__FILE__, __LINE__, GetObjId(), all_size)]];

    return tool;
}

bool nofMetalworker::ReadyForWork()
{
    nextProducedTool = GetOrderedTool();
    if(nextProducedTool == GD_NOTHING)
        nextProducedTool = GetRandomTool();

    if(current_ev)
    {
        RTTR_Assert(current_ev->id == 2 && state == STATE_WAITINGFORWARES_OR_PRODUCTIONSTOPPED);
        em->RemoveEvent(current_ev);
    }
    if(nextProducedTool != GD_NOTHING)
        return true;

    // Try again in some time (3000GF ~= 2min at 40ms/GF)
    current_ev = em->AddEvent(this, 3000, 2);
    return false;
}

GoodType nofMetalworker::ProduceWare()
{
    return nextProducedTool;
}

void nofMetalworker::HandleDerivedEvent(const unsigned int id)
{
    if(id != 2)
    {
        nofWorkman::HandleDerivedEvent(id);
        return;
    }
    RTTR_Assert(state == STATE_WAITINGFORWARES_OR_PRODUCTIONSTOPPED);
    current_ev = NULL;
    TryToWork();
}
