// $Id: nofPlaner.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
#include "nofPlaner.h"

#include "Loader.h"
#include "GameClient.h"
#include "GameClientPlayer.h"
#include "GameWorld.h"
#include "noBuildingSite.h"
#include "Random.h"
#include "JobConsts.h"
#include "SoundManager.h"

#include "glSmartBitmap.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

nofPlaner::nofPlaner(const unsigned short x, const unsigned short y, const unsigned char player, noBuildingSite* building_site)
    : noFigure(JOB_PLANER, x, y, player, building_site), state(STATE_FIGUREWORK), building_site(building_site), pd(PD_NOTWORKING)
{
}

void nofPlaner::Serialize_nofPlaner(SerializedGameData* sgd) const
{
    Serialize_noFigure(sgd);

    sgd->PushUnsignedChar(static_cast<unsigned char>(state));
    sgd->PushObject(building_site, true);
    sgd->PushUnsignedChar(static_cast<unsigned char>(pd));
}

nofPlaner::nofPlaner(SerializedGameData* sgd, const unsigned obj_id) : noFigure(sgd, obj_id),
    state(PlanerState(sgd->PopUnsignedChar())),
    building_site(sgd->PopObject<noBuildingSite>(GOT_BUILDINGSITE)),
    pd(PlaningDir(sgd->PopUnsignedChar()))
{
}

void nofPlaner::GoalReached()
{
    state = STATE_WALKING;

    // Zufällig Uhrzeigersinn oder dagegen
    pd = ( Random::inst().Rand(__FILE__, __LINE__, obj_id, 2) == 0 ) ? (PD_CLOCKWISE) : (PD_COUNTERCLOCKWISE);

    // Je nachdem erst nach rechts oder links gehen
    StartWalking((pd == PD_CLOCKWISE) ? 5 : 3);
}

void nofPlaner::Walked()
{
    /// Zur Baustelle zurückgelaufen? (=fertig)
    if(x == building_site->GetX() && y == building_site->GetY())
    {
        // Baustelle Bescheid sagen
        building_site->PlaningFinished();

        state = STATE_FIGUREWORK;

        // Nach Hause laufen bzw. auch rumirren
        rs_pos = 0;
        rs_dir = true;
        cur_rs = gwg->GetSpecObj<noRoadNode>(x, y)->routes[4];
        building_site = 0;

        GoHome();
        StartWalking(4);
    }
    else
    {
        /// Anfangen zu arbeiten
        current_ev = em->AddEvent(this, JOB_CONSTS[JOB_PLANER].work_length, 1);
        state = STATE_PLANING;
    }
}

void nofPlaner::AbrogateWorkplace()
{
    if(building_site)
    {
        state = STATE_FIGUREWORK;
        building_site->Abrogate();
        building_site = 0;
    }
}

void nofPlaner::LostWork()
{
    building_site = 0;

    if(state == STATE_FIGUREWORK)
        GoHome();
    else
    {
        // Event ggf. abmelden
        if(state == STATE_PLANING)
        {
            em->RemoveEvent(current_ev);
            /// Sounds abmelden
            SoundManager::inst().WorkingFinished(this);
        }

        StartWandering();
        // wenn wir schon laufen, nicht nochmal laufen!
        if(state != STATE_WALKING)
            Wander();

        state = STATE_FIGUREWORK;
    }
}

void nofPlaner::Draw(int x, int y)
{
    switch(state)
    {
        case STATE_FIGUREWORK:
        case STATE_WALKING:
        {
            DrawWalkingBobJobs(x, y, JOB_PLANER);
//          DrawWalking(x,y,LOADER.GetBobN("jobs"),JOB_CONSTS[JOB_PLANER].jobs_bob_id,false);
        } break;
        case STATE_PLANING:
        {
            // 41

            /// Animation des Planierers
            unsigned now_id = GAMECLIENT.Interpolate(69, current_ev);

            // spezielle Animation am Ende
            const unsigned ANIMATION[21] =
            {
                273,
                273,
                273,
                273,
                273,
                274,
                274,
                275,
                276,
                276,
                276,
                276,
                276,
                276,
                276,
                276,
                276,
                276,
                277,
                277,
                278
            };

            if(now_id < 20)
                LOADER.GetImageN("rom_bobs", 253 + now_id)->Draw(x, y, 0, 0, 0, 0, 0, 0,  COLOR_WHITE, COLORS[gwg->GetPlayer(building_site->GetPlayer())->color]);
            else if(now_id < 41)
                LOADER.GetImageN("rom_bobs", ANIMATION[now_id - 20])->Draw(x, y, 0, 0, 0, 0, 0, 0,  COLOR_WHITE, COLORS[gwg->GetPlayer(building_site->GetPlayer())->color]);
            else if(now_id < 55)
                LOADER.GetImageN("rom_bobs", 253 + now_id - 41)->Draw(x, y, 0, 0, 0, 0, 0, 0,  COLOR_WHITE, COLORS[gwg->GetPlayer(building_site->GetPlayer())->color]);
            else
                LOADER.GetImageN("rom_bobs", 253 + now_id - 55)->Draw(x, y, 0, 0, 0, 0, 0, 0,  COLOR_WHITE, COLORS[gwg->GetPlayer(building_site->GetPlayer())->color]);

            // Schaufel-Sound
            if(now_id == 5 || now_id == 46 || now_id == 60)
                SoundManager::inst().PlayNOSound(76, this, now_id, 200);
            // Tret-Sound
            else if(now_id == 20 || now_id == 28)
                SoundManager::inst().PlayNOSound(66, this, now_id, 200);

        } break;



    }
}


void nofPlaner::HandleDerivedEvent(const unsigned int id)
{
    if(id == 1)
    {
        // Planieren (falls Baustelle noch existiert)
        if(building_site)
            gwg->ChangeAltitude(x, y, gwg->GetNode(building_site->GetX(), building_site->GetY()).altitude);
        /// Sounds abmelden
        SoundManager::inst().WorkingFinished(this);

        state = STATE_WALKING;

        // Planierung fertig --> weiterlaufen

        // Das erste Mal gelaufen?
        if(pd == PD_CLOCKWISE && dir == 5)
            StartWalking(1);
        else if(pd == PD_COUNTERCLOCKWISE && dir == 3)
            StartWalking(1);

        // Fertig -> zur Baustelle zurücklaufen
        else if(pd == PD_CLOCKWISE && dir == 4)
            StartWalking(0);
        else if(pd == PD_COUNTERCLOCKWISE && dir == 4)
            StartWalking(2);

        // In nächste Richtung gehen
        else if(pd == PD_CLOCKWISE)
            StartWalking((dir + 1) % 6);
        else
            StartWalking((6 + dir - 1) % 6);


    }
}
