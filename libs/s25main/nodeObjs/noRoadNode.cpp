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

#include "noRoadNode.h"

#include "GamePlayer.h"
#include "RoadSegment.h"
#include "SerializedGameData.h"
#include "world/GameWorld.h"
#include "s25util/warningSuppression.h"

noRoadNode::noRoadNode(const NodalObjectType nop, const MapPoint pos, const unsigned char player)
    : noCoordBase(nop, pos), player(player)
{
    for(const auto dir : helpers::EnumRange<Direction>{})
        routes[dir] = nullptr;
    last_visit = 0;
}

noRoadNode::~noRoadNode() = default;

void noRoadNode::Destroy()
{
    DestroyAllRoads();
    noCoordBase::Destroy();
}

void noRoadNode::Serialize(SerializedGameData& sgd) const
{
    noCoordBase::Serialize(sgd);

    sgd.PushUnsignedChar(player);

    // the trick only seems to work for flags
    if(this->GetGOT() == GO_Type::Flag)
    {
        // this is a trick:
        // -> initialize routes for flag with nullptr
        // -> RoadSegment will set these later
        for(const auto i : helpers::EnumRange<Direction>{})
        {
            RTTR_UNUSED(i);
            sgd.PushObject(static_cast<GameObject*>(nullptr), true);
        }
    } else
    {
        for(const auto dir : helpers::EnumRange<Direction>{})
        {
            sgd.PushObject(routes[dir], true);
        }
    }
}

noRoadNode::noRoadNode(SerializedGameData& sgd, const unsigned obj_id)
    : noCoordBase(sgd, obj_id), player(sgd.PopUnsignedChar())
{
    for(const auto dir : helpers::EnumRange<Direction>{})
    {
        routes[dir] = sgd.PopObject<RoadSegment>(GO_Type::Roadsegment);
    }

    last_visit = 0;
}

void noRoadNode::UpgradeRoad(const Direction dir) const
{
    if(GetRoute(dir))
        GetRoute(dir)->UpgradeDonkeyRoad();
}

void noRoadNode::DestroyRoad(const Direction dir)
{
    RoadSegment* route = GetRoute(dir);
    if(!route)
        return;
    MapPoint t = route->GetF1()->GetPos();
    for(unsigned z = 0; z < route->GetLength(); ++z)
    {
        world->SetPointRoad(t, route->GetRoute(z), PointRoad::None);
        world->RecalcBQForRoad(t);
        t = world->GetNeighbour(t, route->GetRoute(z));
    }

    noRoadNode* otherFlag;

    if(route->GetF1() == this)
        otherFlag = route->GetF2();
    else
        otherFlag = route->GetF1();

    for(const auto z : helpers::EnumRange<Direction>{})
    {
        if(otherFlag->routes[z] == route)
        {
            otherFlag->routes[z] = nullptr;
            break;
        }
    }

    SetRoute(dir, nullptr);

    route->Destroy();
    delete route;

    // Spieler Bescheid sagen
    world->GetPlayer(player).RoadDestroyed();
}

/// Vernichtet Alle Straße um diesen Knoten
void noRoadNode::DestroyAllRoads()
{
    // Alle Straßen um mich herum zerstören
    for(const auto dir : helpers::EnumRange<Direction>{})
        DestroyRoad(dir);
}
