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

#pragma once

#include "gameTypes/BuildingType.h"
#include "gameTypes/Direction.h"
#include "gameTypes/MapCoordinates.h"
#include "gameTypes/Resource.h"

namespace AIEvent {
enum EventType
{
    BuildingDestroyed,
    BuildingConquered,
    BuildingLost,
    BorderChanged,
    NoMoreResourcesReachable,
    BuildingFinished,
    ExpeditionWaiting,
    TreeChopped,
    ShipBuilt,
    ResourceUsed,
    RoadConstructionComplete,
    RoadConstructionFailed,
    NewColonyFounded,
    LuaConstructionOrder,
    ResourceFound,
    LostLand
};

class Base
{
public:
    Base(EventType type) : type(type) {}
    virtual ~Base() = default;
    EventType GetType() const { return type; }

protected:
    EventType type;
};

class Location : public Base
{
public:
    Location(EventType type, const MapPoint pt) : Base(type), pos(pt) {}
    ~Location() override = default;
    MapCoord GetX() const { return pos.x; }
    MapCoord GetY() const { return pos.y; }
    MapPoint GetPos() const { return pos; }

protected:
    MapPoint pos;
};

class Direction : public Location
{
public:
    Direction(EventType type, const MapPoint pt, ::Direction direction) : Location(type, pt), direction(direction) {}
    ~Direction() override = default;
    ::Direction GetDirection() const { return direction; }

protected:
    ::Direction direction;
};

class Building : public Location
{
public:
    Building(EventType type, const MapPoint pt, BuildingType building) : Location(type, pt), building(building) {}
    ~Building() override = default;
    BuildingType GetBuildingType() const { return building; }

protected:
    BuildingType building;
};

class Resource : public Location
{
public:
    const ::Resource resType;
    Resource(EventType type, const MapPoint& pt, ::Resource resType) : Location(type, pt), resType(resType) {}
};
} // namespace AIEvent
