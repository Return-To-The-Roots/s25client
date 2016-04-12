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
#include "nofDonkeybreeder.h"

#include "GameClient.h"

#include "buildings/nobUsual.h"
#include "nofCarrier.h"
#include "Loader.h"
#include "ogl/glSmartBitmap.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "libutil/src/colors.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep
class SerializedGameData;

nofDonkeybreeder::nofDonkeybreeder(const MapPoint pos, unsigned char player, nobUsual* workplace)
    : nofWorkman(JOB_DONKEYBREEDER, pos, player, workplace)
{
}

nofDonkeybreeder::nofDonkeybreeder(SerializedGameData& sgd, unsigned int obj_id)
    : nofWorkman(sgd, obj_id)
{
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Zeichnet ihn beim Arbeiten.
 *
 *  @author FloSoft
 */
void nofDonkeybreeder::DrawWorking(int x, int y)
{
    /// @todo KA was da gemacht werden muss
    const Nation nation = workplace->GetNation();
    const signed char walk_start[NAT_COUNT][2] = { {2, 2}, { -6, -6}, { -7, -7}, { -7, -7}, { -6, -6} };
    const signed char walk_length[NAT_COUNT] = { 22, 19, 19, 23, 19 };
    const unsigned int color = gwg->GetPlayer(player).color;

    unsigned now_id = GAMECLIENT.Interpolate(9600, current_ev);

    if(now_id < 400)
    {
        LOADER.GetNationImage(workplace->GetNation(), 250 + 5 * BLD_DONKEYBREEDER + 4)->Draw(x, y);
        LOADER.bob_jobs_cache[workplace->GetNation()][JOB_DONKEYBREEDER][4][(now_id / 70) % 8].draw(x + walk_start[nation][0] + now_id / 100, y + walk_start[nation][1] + now_id / 100, COLOR_WHITE, gwg->GetPlayer(player).color);
//      LOADER.GetBobN("jobs")->Draw(24,4,false,(now_id/70)%8,x+walk_start[nation][0]+now_id/100,y+walk_start[nation][1]+now_id/100,color);
    }
    else if(now_id < 1200)
        LOADER.bob_jobs_cache[workplace->GetNation()][JOB_DONKEYBREEDER][3][((now_id - 400) / 70) % 8].draw(x + walk_start[nation][0] + 4 + walk_length[nation] * (now_id - 400) / 800, y + walk_start[nation][1] + 4, COLOR_WHITE, gwg->GetPlayer(player).color);
//      LOADER.GetBobN("jobs")->Draw(24,3,false,((now_id-400)/70)%8,x+walk_start[nation][0]+4+walk_length[nation]*(now_id-400)/800,y+walk_start[nation][1]+4,color);
    else if(now_id < 2000)
        LOADER.GetPlayerImage("rom_bobs", 291 + (now_id - 1200) / 100)->Draw(x + walk_start[nation][0] + 4 + walk_length[nation], y + walk_start[nation][1] + 4, 0, 0, 0, 0, 0, 0, COLOR_WHITE, color);
    else if(now_id < 2800)
        LOADER.bob_jobs_cache[workplace->GetNation()][JOB_DONKEYBREEDER][0][((now_id - 2000) / 70) % 8].draw(x + walk_start[nation][0] + 4 + walk_length[nation] * (800 - (now_id - 2000)) / 800, y + walk_start[nation][1] + 4, COLOR_WHITE, gwg->GetPlayer(player).color);
//      LOADER.GetBobN("jobs")->Draw(24,0,false,((now_id-2000)/70)%8,x+walk_start[nation][0]+4+walk_length[nation]*(800-(now_id-2000))/800,y+walk_start[nation][1]+4,color);
    else if(now_id < 3200)
    {
        LOADER.GetNationImage(workplace->GetNation(), 250 + 5 * BLD_DONKEYBREEDER + 4)->Draw(x, y);
        LOADER.bob_jobs_cache[workplace->GetNation()][JOB_DONKEYBREEDER][1][((now_id - 2800) / 70) % 8].draw(x + walk_start[nation][0] + (400 - (now_id - 2800)) / 100, y + walk_start[nation][1] + (400 - (now_id - 2800)) / 100, COLOR_WHITE, gwg->GetPlayer(player).color);
//      LOADER.GetBobN("jobs")->Draw(24,1,false,((now_id-2800)/70)%8,x+walk_start[nation][0]+(400-(now_id-2800))/100,y+walk_start[nation][1]+(400-(now_id-2800))/100,color);
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Der Arbeiter erzeugt eine Ware.
 *
 *  @author FloSoft
 */
GoodType nofDonkeybreeder::ProduceWare()
{
    /// @todo Wie kann ich hier eine Person erzeugen?
    return GD_NOTHING;
}

void nofDonkeybreeder::WorkFinished()
{
    // Straße und Zielflagge für Esel suchen
    noRoadNode* flag_goal;
    RoadSegment* road = gwg->GetPlayer(player).FindRoadForDonkey(workplace, &flag_goal);

    // Esel erzeugen und zum Ziel beordern
    nofCarrier* donkey = new nofCarrier(nofCarrier::CT_DONKEY, pos, player, road, flag_goal);
    gwg->GetPlayer(player).IncreaseInventoryJob(JOB_PACKDONKEY, 1);
    donkey->InitializeRoadWalking(gwg->GetSpecObj<noRoadNode>(pos)->routes[4], 0, true);

    // Wenn keine Straße gefunden wurde, muss er nach Hause gehen
    if(!road)
        donkey->GoHome();
    else
        // ansonsten Arbeitsplatz Bescheid sagen
        road->GotDonkey(donkey);

    // Esel absetzen
    gwg->AddFigure(donkey, pos);

    // In die neue Welt laufen
    donkey->ActAtFirst();
}
