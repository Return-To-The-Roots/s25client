// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "nofPlaner.h"
#include "EventManager.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "SoundManager.h"
#include "buildings/noBuildingSite.h"
#include "network/GameClient.h"
#include "ogl/glArchivItem_Bitmap_Player.h"
#include "random/Random.h"
#include "world/GameWorldGame.h"
#include "gameData/JobConsts.h"

nofPlaner::nofPlaner(const MapPoint pos, const unsigned char player, noBuildingSite* building_site)
    : noFigure(Job::Planer, pos, player, building_site), building_site(building_site), state(PlanerState::FigureWork),
      pd(PlaningDir::NotWorking)
{}

void nofPlaner::Serialize(SerializedGameData& sgd) const
{
    noFigure::Serialize(sgd);

    sgd.PushEnum<uint8_t>(state);
    sgd.PushObject(building_site, true);
    sgd.PushEnum<uint8_t>(pd);
}

nofPlaner::nofPlaner(SerializedGameData& sgd, const unsigned obj_id) : noFigure(sgd, obj_id)
{
    state = sgd.Pop<PlanerState>();
    building_site = sgd.PopObject<noBuildingSite>(GO_Type::Buildingsite);
    pd = sgd.Pop<PlaningDir>();
}

void nofPlaner::GoalReached()
{
    state = PlanerState::Walking;

    // Zufällig Uhrzeigersinn oder dagegen
    pd = (RANDOM_RAND(2) == 0) ? (PlaningDir::Clockwise) : (PlaningDir::Counterclockwise);

    // Je nachdem erst nach rechts oder links gehen
    StartWalking((pd == PlaningDir::Clockwise) ? Direction::SouthWest : Direction::East);
}

void nofPlaner::Walked()
{
    /// Zur Baustelle zurückgelaufen? (=fertig)
    if(pos == building_site->GetPos())
    {
        // Baustelle Bescheid sagen
        building_site->PlaningFinished();

        state = PlanerState::FigureWork;

        // Nach Hause laufen bzw. auch rumirren
        rs_pos = 0;
        rs_dir = true;
        cur_rs = gwg->GetSpecObj<noRoadNode>(pos)->GetRoute(Direction::SouthEast);
        building_site = nullptr;

        GoHome();
        StartWalking(Direction::SouthEast);
    } else
    {
        /// Anfangen zu arbeiten
        current_ev = GetEvMgr().AddEvent(this, JOB_CONSTS[Job::Planer].work_length, 1);
        state = PlanerState::Planing;
    }
}

void nofPlaner::AbrogateWorkplace()
{
    if(building_site)
    {
        state = PlanerState::FigureWork;
        building_site->Abrogate();
        building_site = nullptr;
    }
}

void nofPlaner::LostWork()
{
    building_site = nullptr;

    if(state == PlanerState::FigureWork)
        GoHome();
    else
    {
        // Event ggf. abmelden
        if(state == PlanerState::Planing)
        {
            GetEvMgr().RemoveEvent(current_ev);
            /// Sounds abmelden
            SOUNDMANAGER.WorkingFinished(this);
        }

        StartWandering();
        // wenn wir schon laufen, nicht nochmal laufen!
        if(state != PlanerState::Walking)
            Wander();

        state = PlanerState::FigureWork;
    }
}

void nofPlaner::Draw(DrawPoint drawPt)
{
    switch(state)
    {
        case PlanerState::FigureWork:
        case PlanerState::Walking:
        {
            DrawWalkingBobJobs(drawPt, Job::Planer);
        }
        break;
        case PlanerState::Planing:
        {
            // 41

            /// Animation des Planierers
            unsigned now_id = GAMECLIENT.Interpolate(69, current_ev);

            // spezielle Animation am Ende
            const std::array<unsigned, 21> ANIMATION = {273, 273, 273, 273, 273, 274, 274, 275, 276, 276, 276,
                                                        276, 276, 276, 276, 276, 276, 276, 277, 277, 278};

            unsigned bobId;
            if(now_id < 20)
                bobId = 253 + now_id;
            else if(now_id < 41)
                bobId = ANIMATION[now_id - 20];
            else if(now_id < 55)
                bobId = 253 + now_id - 41;
            else
                bobId = 253 + now_id - 55;
            LOADER.GetPlayerImage("rom_bobs", bobId)
              ->DrawFull(drawPt, COLOR_WHITE, gwg->GetPlayer(building_site->GetPlayer()).color);

            // Schaufel-Sound
            if(now_id == 5 || now_id == 46 || now_id == 60)
                SOUNDMANAGER.PlayNOSound(76, this, now_id, 200);
            // Tret-Sound
            else if(now_id == 20 || now_id == 28)
                SOUNDMANAGER.PlayNOSound(66, this, now_id, 200);
        }
        break;
    }
}

void nofPlaner::HandleDerivedEvent(const unsigned id)
{
    if(id == 1)
    {
        // Planieren (falls Baustelle noch existiert)
        if(building_site)
            gwg->ChangeAltitude(pos, gwg->GetNode(building_site->GetPos()).altitude);
        /// Sounds abmelden
        SOUNDMANAGER.WorkingFinished(this);

        state = PlanerState::Walking;

        // Planierung fertig --> weiterlaufen
        Direction curDir = GetCurMoveDir();

        // Das erste Mal gelaufen?
        if((pd == PlaningDir::Clockwise && curDir == Direction::SouthWest)
           || (pd == PlaningDir::Counterclockwise && curDir == Direction::East))
            StartWalking(Direction::NorthWest);
        // Fertig -> zur Baustelle zurücklaufen
        else if(pd == PlaningDir::Clockwise && curDir == Direction::SouthEast)
            StartWalking(Direction::West);
        else if(pd == PlaningDir::Counterclockwise && curDir == Direction::SouthEast)
            StartWalking(Direction::NorthEast);

        // In nächste Richtung gehen
        else if(pd == PlaningDir::Clockwise)
            StartWalking(curDir + 1u);
        else
            StartWalking(curDir - 1u);
    }
}
