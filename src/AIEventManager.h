// $Id: AIEventManager.h 9555 2014-12-16 15:25:13Z marcus $
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
#ifndef AIEVENTMANAGER_H_INCLUDED
#define AIEVENTMANAGER_H_INCLUDED

#pragma once

#include "main.h"
#include "MapConsts.h"
#include "GameConsts.h"
#include <queue>

namespace AIEvent
{
    enum AIEventType
    {
        BuildingDestroyed,
        BuildingConquered,
        BuildingLost,
        BuildingOccupied,
        BorderChanged,
        TerritoryLost,
        NoMoreResourcesReachable,
        BuildingFinished,
        ExpeditionWaiting,
        TreeChopped,
        ShipBuilt,
        ResourceUsed,
        RoadConstructionComplete,
        RoadConstructionFailed,
        NewColonyFounded,
		LuaConstructionOrder
    };


    class Base
    {
        public:
            Base(AIEventType type) : type(type) { }
            virtual ~Base() { }
            AIEventType GetType() const { return type; }

        protected:
            AIEventType type;
    };


    class Location : public Base
    {
        public:
            Location(AIEventType type, MapCoord x, MapCoord y) : Base(type), x(x), y(y) { }
            ~Location() { }
            MapCoord GetX() const { return x; }
            MapCoord GetY() const { return y; }

        protected:
            MapCoord x, y;
    };

    class Direction : public Location
    {
        public:
            Direction(AIEventType type, MapCoord x, MapCoord y, unsigned char direction) : Location(type, x, y), direction(direction) { }
            ~Direction() { }
            unsigned char GetDirection() const { return direction; }

        protected:
            unsigned char direction;
    };

    class Building : public Location
    {
        public:
            Building(AIEventType type, MapCoord x, MapCoord y, BuildingType building) : Location(type, x, y), building(building) { }
            ~Building() { }
            BuildingType GetBuildingType() const { return building; }

        protected:
            BuildingType building;
    };


}

class AIEventManager
{
    public:
        AIEventManager(void);
        ~AIEventManager(void);
        void AddAIEvent(AIEvent::Base* ev) { events.push(ev); }
        AIEvent::Base* GetEvent();
        bool EventAvailable() const { return events.size() > 0; }
        unsigned GetEventNum() const { return events.size(); }

    protected:
        std::queue<AIEvent::Base*> events;
};


#endif // !AIEVENTMANAGER_H_INCLUDED
