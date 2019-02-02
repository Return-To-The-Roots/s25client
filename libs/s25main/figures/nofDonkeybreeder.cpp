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
#include "nofDonkeybreeder.h"

#include "GamePlayer.h"
#include "Loader.h"
#include "buildings/nobUsual.h"
#include "network/GameClient.h"
#include "nofCarrier.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "ogl/glSmartBitmap.h"
#include "world/GameWorldGame.h"
#include "libutil/colors.h"

nofDonkeybreeder::nofDonkeybreeder(const MapPoint pos, unsigned char player, nobUsual* workplace)
    : nofWorkman(JOB_DONKEYBREEDER, pos, player, workplace)
{}

nofDonkeybreeder::nofDonkeybreeder(SerializedGameData& sgd, unsigned obj_id) : nofWorkman(sgd, obj_id) {}

/**
 *  Zeichnet ihn beim Arbeiten.
 */
void nofDonkeybreeder::DrawWorking(DrawPoint drawPt)
{
    const Nation nation = workplace->GetNation();
    const DrawPointInit walk_start[NUM_NATS] = {{2, 2}, {-6, -6}, {-7, -7}, {-7, -7}, {-6, -6}};
    const signed char walk_length[NUM_NATS] = {22, 19, 19, 23, 19};
    const unsigned color = gwg->GetPlayer(player).color;

    unsigned now_id = GAMECLIENT.Interpolate(9600, current_ev);
    DrawPoint walkBasePos = drawPt + walk_start[nation];

    if(now_id < 400)
    {
        LOADER.GetNationImage(workplace->GetNation(), 250 + 5 * BLD_DONKEYBREEDER + 4)->DrawFull(drawPt);
        LOADER.bob_jobs_cache[workplace->GetNation()][JOB_DONKEYBREEDER][4][(now_id / 70) % 8].draw(
          walkBasePos + DrawPoint(now_id / 100, now_id / 100), COLOR_WHITE, color);
    } else if(now_id < 1200)
        LOADER.bob_jobs_cache[workplace->GetNation()][JOB_DONKEYBREEDER][3][((now_id - 400) / 70) % 8].draw(
          walkBasePos + DrawPoint((now_id - 400) / 800, 4), COLOR_WHITE, color);
    else if(now_id < 2000)
        LOADER.GetPlayerImage("rom_bobs", 291 + (now_id - 1200) / 100)
          ->DrawFull(walkBasePos + DrawPoint(walk_length[nation] + 4, 4), COLOR_WHITE, color);
    else if(now_id < 2800)
        LOADER.bob_jobs_cache[workplace->GetNation()][JOB_DONKEYBREEDER][0][((now_id - 2000) / 70) % 8].draw(
          walkBasePos + DrawPoint(4 + walk_length[nation] * (2800 - now_id) / 800, 4), COLOR_WHITE, color);
    else if(now_id < 3200)
    {
        LOADER.GetNationImage(workplace->GetNation(), 250 + 5 * BLD_DONKEYBREEDER + 4)->DrawFull(drawPt);
        LOADER.bob_jobs_cache[workplace->GetNation()][JOB_DONKEYBREEDER][1][((now_id - 2800) / 70) % 8].draw(
          walkBasePos + DrawPoint((3200 - now_id) / 100, (3200 - now_id) / 100), COLOR_WHITE, color);
    }
}

/**
 *  Der Arbeiter erzeugt eine Ware.
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
    donkey->InitializeRoadWalking(gwg->GetSpecObj<noRoadNode>(pos)->GetRoute(Direction::SOUTHEAST), 0, true);

    // Wenn keine Straße gefunden wurde, muss er nach Hause gehen
    if(!road)
        donkey->GoHome();
    else
        // ansonsten Arbeitsplatz Bescheid sagen
        road->GotDonkey(donkey);

    // Esel absetzen
    gwg->AddFigure(pos, donkey);

    // In die neue Welt laufen
    donkey->ActAtFirst();
}
