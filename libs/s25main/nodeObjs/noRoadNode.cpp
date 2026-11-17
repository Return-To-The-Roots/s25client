// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "noRoadNode.h"
#include "GamePlayer.h"
#include "RoadSegment.h"
#include "SerializedGameData.h"
#include "world/GameWorld.h"

noRoadNode::noRoadNode(const NodalObjectType nop, const MapPoint pos, const unsigned char player)
    : noCoordBase(nop, pos), player(player)
{
    for(const auto dir : helpers::EnumRange<Direction>{})
        routes[dir] = nullptr;
    last_visit = 0;
}

noRoadNode::~noRoadNode() = default;

void noRoadNode::destroy()
{
    destroyAllRoads();
    noCoordBase::destroy();
}

void noRoadNode::serialize(SerializedGameData& sgd) const
{
    noCoordBase::serialize(sgd);

    sgd.pushUnsignedChar(player);

    // the trick only seems to work for flags
    if(this->getGOT() == GO_Type::Flag)
    {
        // this is a trick:
        // -> initialize routes for flag with nullptr
        // -> RoadSegment will set these later
        for([[maybe_unused]] const auto i : helpers::EnumRange<Direction>{})
            sgd.pushObject(static_cast<GameObject*>(nullptr), true);
    } else
    {
        for(const auto dir : helpers::EnumRange<Direction>{})
            sgd.pushObject(routes[dir], true);
    }
}

noRoadNode::noRoadNode(SerializedGameData& sgd, const unsigned obj_id)
    : noCoordBase(sgd, obj_id), player(sgd.popUnsignedChar())
{
    for(const auto dir : helpers::EnumRange<Direction>{})
    {
        routes[dir] = sgd.popObject<RoadSegment>(GO_Type::Roadsegment);
    }

    last_visit = 0;
}

void noRoadNode::UpgradeRoad(const Direction dir) const
{
    if(getRoute(dir))
        getRoute(dir)->upgradeDonkeyRoad();
}

void noRoadNode::DestroyRoad(const Direction dir)
{
    RoadSegment* route = getRoute(dir);
    if(!route)
        return;
    MapPoint t = route->getF1()->getPos();
    for(unsigned z = 0; z < route->getLength(); ++z)
    {
        world->setPointRoad(t, route->getRoute(z), PointRoad::None);
        world->recalcBQForRoad(t);
        t = world->getNeighbour(t, route->getRoute(z));
    }

    noRoadNode* otherFlag;

    if(route->getF1() == this)
        otherFlag = route->getF2();
    else
        otherFlag = route->getF1();

    for(const auto z : helpers::EnumRange<Direction>{})
    {
        if(otherFlag->routes[z] == route)
        {
            otherFlag->routes[z] = nullptr;
            break;
        }
    }

    setRoute(dir, nullptr);

    route->destroy();
    delete route;

    // Spieler Bescheid sagen
    world->getPlayer(player).roadDestroyed();
}

/// Vernichtet Alle Straße um diesen Knoten
void noRoadNode::destroyAllRoads()
{
    // Alle Straßen um mich herum zerstören
    for(const auto dir : helpers::EnumRange<Direction>{})
        DestroyRoad(dir);
}
