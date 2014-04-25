// $Id: nofScout_Free.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
#include "nofScout_Free.h"

#include "GameConsts.h"
#include "noFlag.h"
#include "GameWorld.h"
#include "Random.h"
#include "EventManager.h"
#include "GameClient.h"
#include "SerializedGameData.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

nofScout_Free::nofScout_Free(const MapCoord x, const MapCoord y, const unsigned char player, noRoadNode* goal)
    : nofFlagWorker(JOB_SCOUT, x, y, player, goal), next_x(x), next_y(y), rest_way(0)
{
}

void nofScout_Free::Serialize_nofScout_Free(SerializedGameData* sgd) const
{
    Serialize_nofFlagWorker(sgd);

    sgd->PushUnsignedShort(next_x);
    sgd->PushUnsignedShort(next_y);
}

nofScout_Free::nofScout_Free(SerializedGameData* sgd, const unsigned obj_id) : nofFlagWorker(sgd, obj_id),
    next_x(sgd->PopUnsignedShort()),
    next_y(sgd->PopUnsignedShort())
{
}

void nofScout_Free::Draw(int x, int y)
{
    //assert(this->GetObjId()!= 8215505);
    // normales Laufen zeichnen
    DrawWalking(x, y);
}


void nofScout_Free::GoalReached()
{
    /// Bestimmte Anzahl an Punkten abklappern, leicht variieren
    rest_way = 80 + RANDOM.Rand(__FILE__, __LINE__, obj_id, 20);

    state = STATE_SCOUT_SCOUTING;

    // Loslegen
    GoToNewNode();
}

void nofScout_Free::Walked()
{
    switch(state)
    {
        default: break;
        case STATE_GOTOFLAG:
        {
            GoToFlag();
        } break;
        case STATE_SCOUT_SCOUTING:
        {
            Scout();
        } break;
    }
}

void nofScout_Free::HandleDerivedEvent(const unsigned int id)
{
}

void nofScout_Free::LostWork()
{
    flag = 0;

    switch(state)
    {
        default: break;
            // Wenn wir noch hingehen, dann zurückgehen
        case STATE_FIGUREWORK:
        {
            GoHome();
        } break;
        case STATE_GOTOFLAG:
        case STATE_SCOUT_SCOUTING:
        {
            // dann sofort rumirren, wenn wir zur Flagge gehen
            StartWandering();
            state = STATE_FIGUREWORK;
        } break;
    }
}

/// Erkundet (quasi ein Umherirren)
void nofScout_Free::Scout()
{
    // Erkundet er noch, ist aber schon seine maximale Wegstrecke abgelaufen?
    if(--rest_way == 0)
    {
        // Wieder zur Flagge zurückgehen
        state = STATE_GOTOFLAG;
        GoToFlag();
        return;
    }

    // Bin ich schon an dem Punkt angekommen?
    if(x == next_x && y == next_y)
    {
        // Nächsten Punkt suchen
        GoToNewNode();
    }
    else
    {
        // Weg suchen
        dir = gwg->FindHumanPath(x, y, next_x, next_y, 30);

        // Wenns keinen gibt, neuen suchen, ansonsten hinlaufen
        if(dir == 0xFF)
            // Neuen Punkt suchen
            GoToNewNode();

        else
            StartWalking(dir);
    }
}

const unsigned SCOUT_RANGE = 16;

void nofScout_Free::GoToNewNode()
{
    list< Point<MapCoord> > available_points;

    for(MapCoord tx = gwg->GetXA(flag->GetX(), flag->GetY(), 0), r = 1; r < SCOUT_RANGE; tx = gwg->GetXA(tx, flag->GetY(), 0), ++r)
    {
        MapCoord tx2 = tx, ty2 = flag->GetY();
        for(unsigned i = 2; i < 8; ++i)
        {
            for(MapCoord r2 = 0; r2 < r; gwg->GetPointA(tx2, ty2, i % 6), ++r2)
            {
                // Liegt Punkt im Nebel und für Figuren begehbar?
                if(gwg->GetNode(tx2, ty2).fow[player].visibility != VIS_VISIBLE
                        && gwg->IsNodeForFigures(tx2, ty2))
                {
                    Point<MapCoord> p (tx2, ty2);
                    available_points.push_back(p);
                }
            }
        }
    }

    // Ein Objekt zufällig heraussuchen
    bool found_point = false;
    while(available_points.size() && !found_point)
    {
        list< Point<MapCoord> >::iterator p = available_points[RANDOM.Rand(__FILE__, __LINE__, obj_id, available_points.size())];

        // Existiert ein Weg zu diesem Punkt und ist dieser Punkt auch noch von der Flagge noch in
        // einigermaßen vernünftiger Entfernung zu erreichen, um das Drumherumlaufen um Berge usw. zu
        // verhindern
        if(gwg->FindHumanPath(x, y, p->x, p->y, SCOUT_RANGE * 2) != 0xFF && gwg->FindHumanPath(flag->GetX(), flag->GetY(), p->x, p->y, SCOUT_RANGE + SCOUT_RANGE / 4) != 0xFF)
        {
            // Als neues Ziel nehmen
            next_x = p->x;
            next_y = p->y;

            Scout();

            found_point = true;

            break;
        }

        available_points.erase(p);


    }

    // Gibt es überhaupt einen Punkt, wo ich hingehen kann?
    if(!found_point)
    {
        // Wieder zur Flagge zurückgehen
        state = STATE_GOTOFLAG;
        GoToFlag();
    }
}

/// Gibt den Sichtradius dieser Figur zurück (0, falls nicht-spähend)
unsigned nofScout_Free::GetVisualRange() const
{
    return VISUALRANGE_SCOUT;
}
