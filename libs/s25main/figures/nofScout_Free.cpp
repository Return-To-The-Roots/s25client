// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofScout_Free.h"

#include "SerializedGameData.h"
#include "pathfinding/PathConditionHuman.h"
#include "random/Random.h"
#include "world/GameWorld.h"
#include "nodeObjs/noFlag.h"
#include "gameData/GameConsts.h"
#include "gameData/MilitaryConsts.h"
#include <algorithm>
class noRoadNode;

nofScout_Free::nofScout_Free(const MapPoint pos, const unsigned char player, noRoadNode* goal)
    : nofFlagWorker(Job::Scout, pos, player, goal), nextPos(pos), rest_way(0)
{}

void nofScout_Free::Serialize(SerializedGameData& sgd) const
{
    nofFlagWorker::Serialize(sgd);
    helpers::pushPoint(sgd, nextPos);
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
    rest_way = 80 + RANDOM_RAND(20);

    state = State::ScoutScouting;

    // Loslegen
    GoToNewNode();
}

void nofScout_Free::Walked()
{
    switch(state)
    {
        default: break;
        case State::GoToFlag: GoToFlag(); break;
        case State::ScoutScouting: Scout(); break;
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
        case State::FigureWork:
        {
            GoHome();
        }
        break;
        case State::GoToFlag:
        case State::ScoutScouting:
        {
            // dann sofort rumirren, wenn wir zur Flagge gehen
            StartWandering();
            state = State::FigureWork;
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
        state = State::GoToFlag;
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
        const auto dir = world->FindHumanPath(pos, nextPos, 30);

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
    const GameWorld& world;
    IsScoutable(const unsigned char player, const GameWorld& world) : player(player), world(world) {}

    bool operator()(const MapPoint& pt) const
    {
        // Liegt Punkt im Nebel und für Figuren begehbar?
        return world.CalcVisiblityWithAllies(pt, player) != Visibility::Visible
               && PathConditionHuman(world).IsNodeOk(pt);
    }
};
} // namespace

void nofScout_Free::GoToNewNode()
{
    std::vector<MapPoint> available_points =
      world->GetMatchingPointsInRadius<-1>(flag->GetPos(), SCOUT_RANGE, IsScoutable(player, *world));
    RANDOM_SHUFFLE(available_points);
    for(MapPoint pt : available_points)
    {
        // Is there a path to this point and is the point also not to far away from the flag?
        // (Second check avoids running around mountains with a very far way back)
        if(world->FindHumanPath(pos, pt, SCOUT_RANGE * 2)
           && world->FindHumanPath(flag->GetPos(), pt, SCOUT_RANGE + SCOUT_RANGE / 4))
        {
            // Take it
            nextPos = pt;
            Scout();
            return;
        }
    }

    // Nothing found -> Go back
    state = State::GoToFlag;
    GoToFlag();
}

/// Gibt den Sichtradius dieser Figur zurück (0, falls nicht-spähend)
unsigned nofScout_Free::GetVisualRange() const
{
    return VISUALRANGE_SCOUT;
}
