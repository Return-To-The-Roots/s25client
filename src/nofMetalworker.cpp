// $Id: nofMetalworker.cpp 9447 2014-06-20 21:40:55Z jh $
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
#include "nofMetalworker.h"

#include "Loader.h"
#include "macros.h"
#include "GameClient.h"
#include "GameClientPlayer.h"
#include "nobUsual.h"
#include "Random.h"
#include "SoundManager.h"

#include "iwTools.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

nofMetalworker::nofMetalworker(const unsigned short x, const unsigned short y, const unsigned char player, nobUsual* workplace)
    : nofWorkman(JOB_METALWORKER, x, y, player, workplace)
{
}

nofMetalworker::nofMetalworker(SerializedGameData* sgd, const unsigned obj_id) : nofWorkman(sgd, obj_id)
{
}


void nofMetalworker::DrawWorking(int x, int y)
{
    signed char offsets[NATION_COUNT][2] = { { -11, -13}, {31, 5}, {32, 6}, {30, 10}, {28, 5} };

    unsigned now_id;

    LOADER.GetImageN("rom_bobs", 190 + (now_id = GAMECLIENT.Interpolate(230, current_ev)) % 23)
    ->Draw(x + offsets[workplace->GetNation()][0], y + offsets[workplace->GetNation()][1], 0, 0, 0, 0, 0, 0, COLOR_WHITE, COLORS[gwg->GetPlayer(workplace->GetPlayer())->color]);

    // Hämmer-Sound
    if(now_id % 23 == 3 || now_id % 23 == 7)
    {
        SoundManager::inst().PlayNOSound(72, this, now_id, 100);
        was_sounding = true;
    }
    // Säge-Sound 1
    else if(now_id % 23 == 9)
    {
        SoundManager::inst().PlayNOSound(54, this, now_id);
        was_sounding = true;
    }
    else if(now_id % 23 == 17)
    {
        SoundManager::inst().PlayNOSound(55, this, now_id);
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
const GoodType TOOLS_SETTINGS_IDS[12] =
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
        sum += gwg->GetPlayer(player)->tools_ordered[i];
    return sum;
}

GoodType nofMetalworker::ProduceWare()
{
    // qx:tools
    {
        int prio = -1;
        int tool = -1;
        
        for (unsigned i = 0; i < TOOL_COUNT; ++i)
        {
            if (gwg->GetPlayer(player)->tools_ordered[i] > 0 && (gwg->GetPlayer(player)->tools_settings[i] > prio) )
            {
                prio = gwg->GetPlayer(player)->tools_settings[i];
                tool = i;
            }
        }
        
        if (tool != -1)
        {
            --gwg->GetPlayer(player)->tools_ordered[tool];
            
            if ( (player == GameClient::inst().GetPlayerID()) && (ToolsOrderedTotal() == 0) )
            {
                GameClient::inst().SendPostMessage( new PostMsg( _("Completed the ordered amount of tools."), PMC_GENERAL ) );
            }
            
            iwTools::UpdateOrders();
            return TOOLS_SETTINGS_IDS[tool];
        }
    }

    // Je nach Werkzeugeinstellungen zufällig ein Werkzeug produzieren, je größer der Balken,
    // desto höher jeweils die Wahrscheinlichkeit
    unsigned short all_size = 0;

    for(unsigned i = 0; i < 12; ++i)
        all_size += gwg->GetPlayer(player)->tools_settings[i];

    // Wenn alle auf 0 gesetzt sind, einfach eins zufällig auswählen
    if(!all_size)
        return TOOLS_SETTINGS_IDS[RANDOM.Rand(__FILE__, __LINE__, obj_id, 12)];

    // Ansonsten Array mit den Werkzeugtypen erstellen und davon dann eins zufällig zurückliefern, je höher Wahr-
    // scheinlichkeit (Balken), desto öfter im Array enthalten
    unsigned char* random_array = new unsigned char[all_size];
    unsigned pos = 0;

    for(unsigned i = 0; i < 12; ++i)
    {
        for(unsigned g = 0; g < gwg->GetPlayer(player)->tools_settings[i]; ++g)
            random_array[pos++] = i;
    }

    GoodType tool = TOOLS_SETTINGS_IDS[random_array[RANDOM.Rand(__FILE__, __LINE__, obj_id, all_size)]];

    delete [] random_array;

    return tool;

}
