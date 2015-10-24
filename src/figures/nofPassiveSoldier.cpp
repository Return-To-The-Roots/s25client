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
#include "defines.h"
#include "nofPassiveSoldier.h"
#include "GameWorldGame.h"
#include "buildings/nobMilitary.h"
#include "Loader.h"
#include "Random.h"
#include "SerializedGameData.h"
#include "GameClientPlayer.h"
#include "gameData/MilitaryConsts.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

nofPassiveSoldier::nofPassiveSoldier(const nofSoldier& soldier) : nofSoldier(soldier),
    healing_event(0)
{
    // Soldat von einer Mission nach Hause gekommen --> ggf heilen!
    Heal();
    // Laufevent nullen, laufen ja nicht mehr
    current_ev = 0;
}

nofPassiveSoldier::nofPassiveSoldier(const MapPoint pos, const unsigned char player,
                                     nobBaseMilitary* const goal, nobBaseMilitary* const home, const unsigned char rank)
    : nofSoldier(pos, player, goal, home, rank), healing_event(0)
{
}


nofPassiveSoldier::~nofPassiveSoldier()
{
}


void nofPassiveSoldier::Destroy_nofPassiveSoldier()
{
    em->RemoveEvent(healing_event);
}

void nofPassiveSoldier::Serialize_nofPassiveSoldier(SerializedGameData& sgd) const
{
    Serialize_nofSoldier(sgd);

    sgd.PushObject(healing_event, true);
}

nofPassiveSoldier::nofPassiveSoldier(SerializedGameData& sgd, const unsigned obj_id) : nofSoldier(sgd, obj_id),
    healing_event(sgd.PopObject<EventManager::Event>(GOT_EVENT))
{
}

void nofPassiveSoldier::Draw(int x, int y)
{
    // Soldat normal laufend zeichnen
    DrawSoldierWalking(x, y);
}

void nofPassiveSoldier::HandleDerivedEvent(const unsigned int id)
{
    switch(id)
    {
            // "Heilungs-Event"
        case 1:
        {
            healing_event = 0;

            // Sind wir noch im Haus?
            if(fs == FS_JOB)
            {
                // Dann uns heilen, wenn wir nicht schon gesund sind
                if(hitpoints < HITPOINTS[gwg->GetPlayer(player).nation][job_ - JOB_PRIVATE])
                {
                    ++hitpoints;

                    // Sind wir immer noch nicht gesund? Dann neues Event anmelden!
                    if(hitpoints < HITPOINTS[gwg->GetPlayer(player).nation][job_ - JOB_PRIVATE])
                        healing_event = em->AddEvent(this, CONVALESCE_TIME + RANDOM.Rand(__FILE__, __LINE__, GetObjId(), CONVALESCE_TIME_RANDOM), 1);
                }
            }

        } break;
    }
}

void nofPassiveSoldier::Heal()
{
    // Schon ein Event angemeldet?
    // Dann muss dieses entfernt werden, wahrscheinlich war er zuvor draußen gewesen
    if(healing_event)
    {
        em->RemoveEvent(healing_event);
        healing_event = 0;
    }

    // Ist er verletzt?
    // Dann muss er geheilt werden
    if(hitpoints < HITPOINTS[gwg->GetPlayer(player).nation][job_ - JOB_PRIVATE])
        healing_event = em->AddEvent(this, CONVALESCE_TIME + RANDOM.Rand(__FILE__, __LINE__, GetObjId(), CONVALESCE_TIME_RANDOM), 1);
}

void nofPassiveSoldier::GoalReached()
{
    // im Militärgebäude angekommen

    // mich hinzufügen
    static_cast<nobMilitary*>(building)->AddPassiveSoldier(this);

    // und wir können uns auch aus der Laufliste erstmal entfernen
    gwg->RemoveFigure(this, pos);
}

void nofPassiveSoldier::InBuildingDestroyed()
{
    building = 0;

    // Auf die Karte setzen
    gwg->AddFigure(this, pos);
    // Erstmal in zufällige Richtung rammeln
    StartWandering();

    StartWalking(RANDOM.Rand(__FILE__, __LINE__, GetObjId(), 6));

}

void nofPassiveSoldier::LeaveBuilding()
{
    // Nach Hause in ein Lagerhaus gehen
    rs_dir = true;
    rs_pos = 1;
    cur_rs = building->routes[4];
    GoHome();

    building = 0;
}


void nofPassiveSoldier::Upgrade()
{
    assert(!building || !helpers::contains(static_cast<nobMilitary*>(building)->troops, this)); // We must not be in the buildings list while upgrading. This would destroy the ordered list
    // Einen Rang höher
    job_ = Job(unsigned(job_) + 1);

    // wieder heilen bzw. Hitpoints anpasen
    GameClientPlayer& owner = gwg->GetPlayer(player);
    hitpoints = HITPOINTS[owner.nation][job_ - JOB_PRIVATE];

    // Inventur entsprechend erhöhen und verringern
    owner.IncreaseInventoryJob(job_, 1);
    owner.DecreaseInventoryJob(Job(unsigned(job_) - 1), 1);
}

void nofPassiveSoldier::Walked()
{
    // Das dürfte nich passiern!
    assert(false);
}

void nofPassiveSoldier::NotNeeded()
{
    building = 0;
    GoHome();
}

