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

#include "nofScout_Free.h"

#include "SerializedGameData.h"
#include "pathfinding/PathConditionHuman.h"
#include "random/Random.h"
#include "world/GameWorldGame.h"
#include "nodeObjs/noFlag.h"
#include "gameData/GameConsts.h"
#include "gameData/MilitaryConsts.h"
#include <algorithm>
class noRoadNode;

nofScout_Free::nofScout_Free(const MapPoint pos, const unsigned char player, noRoadNode* goal)
    : nofFlagWorker(JOB_SCOUT, pos, player, goal), nextPos(pos), rest_way(0)
{}

void nofScout_Free::Serialize_nofScout_Free(SerializedGameData& sgd) const
{
    Serialize_nofFlagWorker(sgd);
    sgd.PushMapPoint(nextPos);
    sgd.PushUnsignedInt(rest_way);
}

nofScout_Free::nofScout_Free(SerializedGameData& sgd, const unsigned obj_id)
    : nofFlagWorker(sgd, obj_id), nextPos(sgd.PopMapPoint()), rest_way(sgd.PopUnsignedInt())
{}

void nofScout_Free::Draw(DrawPoint drawPt)
{
    // normales Laufen zeichnen
    DrawWalking(drawPt);
}

void nofScout_Free::GoalReached()
{
    /// Bestimmte Anzahl an Punkten abklappern, leicht variieren
    rest_way = 80 + RANDOM.Rand(__FILE__, __LINE__, GetObjId(), 20);

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
        }
        break;
        case STATE_SCOUT_SCOUTING:
        {
            Scout();
        }
        break;
    }
}

void nofScout_Free::HandleDerivedEvent(const unsigned /*id*/) {}

void nofScout_Free::LostWork()
{
    flag = nullptr;

    switch(state)
    {
        default: break;
        // Wenn wir noch hingehen, dann zurückgehen
        case STATE_FIGUREWORK:
        {
            GoHome();
        }
        break;
        case STATE_GOTOFLAG:
        case STATE_SCOUT_SCOUTING:
        {
            // dann sofort rumirren, wenn wir zur Flagge gehen
            StartWandering();
            state = STATE_FIGUREWORK;
        }
        break;
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
    if(pos == nextPos)
    {
        // Nächsten Punkt suchen
        GoToNewNode();
    } else
    {
        // Weg suchen
        const auto dir = gwg->FindHumanPath(pos, nextPos, 30);

        // Wenns keinen gibt, neuen suchen, ansonsten hinlaufen
        if(dir)
            StartWalking(*dir);
        else
        {
            // Neuen Punkt suchen
            GoToNewNode();
        }
    }
}

const unsigned SCOUT_RANGE = 16;

namespace {
struct IsScoutable
{
    const unsigned char player;
    const GameWorldGame& gwg;
    IsScoutable(const unsigned char player, const GameWorldGame& gwg) : player(player), gwg(gwg) {}

    bool operator()(const MapPoint& pt) const
    {
        // Liegt Punkt im Nebel und für Figuren begehbar?
        return gwg.CalcVisiblityWithAllies(pt, player) != VIS_VISIBLE && PathConditionHuman(gwg).IsNodeOk(pt);
    }
};
} // namespace

void nofScout_Free::GoToNewNode()
{
    std::vector<MapPoint> available_points =
      gwg->GetPointsInRadius<-1>(flag->GetPos(), SCOUT_RANGE, Identity<MapPoint>(), IsScoutable(player, *gwg));
    RANDOM_SHUFFLE(available_points);
    for(MapPoint pt : available_points)
    {
        // Is there a path to this point and is the point also not to far away from the flag?
        // (Second check avoids running around mountains with a very far way back)
        if(gwg->FindHumanPath(pos, pt, SCOUT_RANGE * 2)
           && gwg->FindHumanPath(flag->GetPos(), pt, SCOUT_RANGE + SCOUT_RANGE / 4))
        {
            // Take it
            nextPos = pt;
            Scout();
            return;
        }
    }

    // Nothing found -> Go back
    state = STATE_GOTOFLAG;
    GoToFlag();
}

/// Gibt den Sichtradius dieser Figur zurück (0, falls nicht-spähend)
unsigned nofScout_Free::GetVisualRange() const
{
    return VISUALRANGE_SCOUT;
}
