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

#include "defines.h" // IWYU pragma: keep
#include "TestEventManager.h"
#include "GameEvent.h"
#include <boost/foreach.hpp>

unsigned TestEventManager::ExecuteNextEvent(unsigned maxGF)
{
    if(events.empty() || GetCurrentGF() >= maxGF)
        return 0;
    EventMap::iterator itEvents = events.begin();
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

std::vector<GameEvent*> TestEventManager::GetObjEvents(const GameObject& obj)
{
    std::vector<GameEvent*> objEvnts;
    for(EventMap::iterator it = events.begin(); it != events.end(); ++it)
    {
        BOOST_FOREACH(GameEvent* ev, it->second)
        {
            if(ev->obj == &obj)
                objEvnts.push_back(ev);
        }
    }
    return objEvnts;
}

bool TestEventManager::IsEventActive(const GameObject& obj, const unsigned id) const
{
    for(EventMap::const_iterator it = events.begin(); it != events.end(); ++it)
    {
        BOOST_FOREACH(GameEvent* ev, it->second)
        {
            if(ev->id == id && ev->obj == &obj)
                return true;
        }
    }

    return false;
}

void TestEventManager::RescheduleEvent(GameEvent& event, unsigned targetGF)
{
    RemoveEventFromQueue(event);
    event.length = targetGF - event.startGF;
    AddEventToQueue(&event);
}

std::vector<const GameEvent*> TestEventManager::GetEvents() const
{
    std::vector<const GameEvent*> nextEv;
    for(EventMap::const_iterator it = events.begin(); it != events.end(); ++it)
        nextEv.insert(nextEv.end(), it->second.begin(), it->second.end());
    return nextEv;
}
