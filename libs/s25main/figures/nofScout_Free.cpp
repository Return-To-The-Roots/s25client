// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofScout_Free.h"

#include "SerializedGameData.h"
#include "pathfinding/PathConditionHuman.h"
#include "random/Random.h"
#include "world/GameWorld.h"
#include "nodeObjs/noFlag.h"
#include "gameData/MilitaryConsts.h"
#include <algorithm>
class noRoadNode;

nofScout_Free::nofScout_Free(const MapPoint pos, const unsigned char player, noRoadNode* goal)
    : nofFlagWorker(Job::Scout, pos, player, goal), nextPos(pos), rest_way(0)
{}

void nofScout_Free::serialize(SerializedGameData& sgd) const
{
    nofFlagWorker::serialize(sgd);
    helpers::pushPoint(sgd, nextPos);
    sgd.pushUnsignedInt(rest_way);
}

nofScout_Free::nofScout_Free(SerializedGameData& sgd, const unsigned obj_id)
    : nofFlagWorker(sgd, obj_id), nextPos(sgd.popMapPoint()), rest_way(sgd.popUnsignedInt())
{}

void nofScout_Free::draw(DrawPoint drawPt)
{
    // normales Laufen zeichnen
    drawWalking(drawPt);
}

void nofScout_Free::goalReached()
{
    /// Bestimmte Anzahl an Punkten abklappern, leicht variieren
    rest_way = 80 + RANDOM_RAND(20);

    state = State::ScoutScouting;

    // Loslegen
    goToNewNode();
}

void nofScout_Free::walked()
{
    switch(state)
    {
        default: break;
        case State::GoToFlag: goToFlag(); break;
        case State::ScoutScouting: scout(); break;
    }
}

void nofScout_Free::handleDerivedEvent(const unsigned /*id*/) {}

void nofScout_Free::lostWork()
{
    flag = nullptr;

    switch(state)
    {
        default: break;
        // Wenn wir noch hingehen, dann zurückgehen
        case State::FigureWork:
        {
            goHome();
        }
        break;
        case State::GoToFlag:
        case State::ScoutScouting:
        {
            // dann sofort rumirren, wenn wir zur Flagge gehen
            startWandering();
        }
        break;
    }
}

/// Erkundet (quasi ein Umherirren)
void nofScout_Free::scout()
{
    // Erkundet er noch, ist aber schon seine maximale Wegstrecke abgelaufen?
    if(--rest_way == 0)
    {
        // Wieder zur Flagge zurückgehen
        state = State::GoToFlag;
        goToFlag();
        return;
    }

    // Bin ich schon an dem Punkt angekommen?
    if(pos == nextPos)
    {
        // Nächsten Punkt suchen
        goToNewNode();
    } else
    {
        // Weg suchen
        const auto dir = world->findHumanPath(pos, nextPos, 30);

        // Wenns keinen gibt, neuen suchen, ansonsten hinlaufen
        if(dir)
            startWalking(*dir);
        else
        {
            // Neuen Punkt suchen
            goToNewNode();
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
        return world.calcVisiblityWithAllies(pt, player) != Visibility::Visible
               && PathConditionHuman(world).isNodeOk(pt);
    }
};
} // namespace

void nofScout_Free::goToNewNode()
{
    std::vector<MapPoint> availablePoints =
      world->getMatchingPointsInRadius<-1>(flag->getPos(), SCOUT_RANGE, IsScoutable(player, *world));
    RANDOM_SHUFFLE(availablePoints);
    for(MapPoint pt : availablePoints)
    {
        // Is there a path to this point and is the point also not to far away from the flag?
        // (Second check avoids running around mountains with a very far way back)
        if(world->findHumanPath(pos, pt, SCOUT_RANGE * 2)
           && world->findHumanPath(flag->getPos(), pt, SCOUT_RANGE + SCOUT_RANGE / 4))
        {
            // Take it
            nextPos = pt;
            scout();
            return;
        }
    }

    // Nothing found -> Go back
    state = State::GoToFlag;
    goToFlag();
}

/// Gibt den Sichtradius dieser Figur zurück (0, falls nicht-spähend)
unsigned nofScout_Free::getVisualRange() const
{
    return VISUALRANGE_SCOUT;
}
