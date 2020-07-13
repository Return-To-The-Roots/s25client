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

#include "TestEventManager.h"
#include "GameEvent.h"

unsigned TestEventManager::ExecuteNextEvent(unsigned maxGF)
{
    if(GetCurrentGF() >= maxGF)
        return 0;
    if(events.empty())
    {
        unsigned numGFs = maxGF - GetCurrentGF();
        currentGF = maxGF;
        return numGFs;
    }
    auto itEvents = events.begin();
    if(itEvents->first > maxGF)
    {
        unsigned numGFs = maxGF - GetCurrentGF();
        currentGF = maxGF;
        return numGFs;
    }
    unsigned numGFs = itEvents->first - GetCurrentGF();
    currentGF = itEvents->first;
    ExecuteEvents(itEvents);
    DestroyCurrentObjects();
    return numGFs;
}

std::vector<const GameEvent*> TestEventManager::GetObjEvents(const GameObject& obj) const
{
    std::vector<const GameEvent*> objEvnts;
    for(const auto& event : events)
    {
        for(const GameEvent* ev : event.second)
        {
            if(ev->obj == &obj)
                objEvnts.push_back(ev);
        }
    }
    return objEvnts;
}

bool TestEventManager::IsEventActive(const GameObject& obj, const unsigned id) const
{
    for(const auto& event : events)
    {
        for(const GameEvent* ev : event.second)
        {
            if(ev->id == id && ev->obj == &obj)
                return true;
        }
    }

    return false;
}

const GameEvent* TestEventManager::RescheduleEvent(const GameEvent* event, unsigned targetGF)
{
    RemoveEventFromQueue(*event);
    // Hacky but we need to preserve the location (pointer) of the event as objects store it
    const_cast<GameEvent*>(event)->length = targetGF - event->startGF;
    return AddEventToQueue(event);
}

std::vector<const GameEvent*> TestEventManager::GetEvents() const
{
    return EventManager::GetEvents();
}
