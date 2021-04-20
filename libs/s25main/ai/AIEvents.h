// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameTypes/BuildingType.h"
#include "gameTypes/Direction.h"
#include "gameTypes/MapCoordinates.h"
#include "gameTypes/Resource.h"

namespace AIEvent {
enum class EventType
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
