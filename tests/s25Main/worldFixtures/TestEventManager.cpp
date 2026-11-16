// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "TestEventManager.h"
#include "GameEvent.h"
#include "GamePlayer.h"
#include "world/GameWorldBase.h"

unsigned TestEventManager::doExecuteNextEvent(unsigned maxGF)
{
    if(getCurrentGF() >= maxGF)
        return 0;
    if(events.empty())
    {
        unsigned numGFs = maxGF - getCurrentGF();
        currentGF = maxGF;
        return numGFs;
    }
    auto itEvents = events.begin();
    if(itEvents->first > maxGF)
    {
        unsigned numGFs = maxGF - getCurrentGF();
        currentGF = maxGF;
        return numGFs;
    }
    unsigned numGFs = itEvents->first - getCurrentGF();
    currentGF = itEvents->first;
    executeEvents(itEvents);
    destroyCurrentObjects();
    return numGFs;
}

unsigned TestEventManager::executeNextEvent(unsigned maxGF)
{
    const auto numGFs = doExecuteNextEvent(maxGF);
    if(numGFs > 0)
    {
        for(auto& player : world_->getPlayers())
        {
            if(player.isUsed())
            {
                player.testForEmergencyProgramm();
                player.testPacts();
            }
        }
    }
    return numGFs;
}

std::vector<const GameEvent*> TestEventManager::getObjEvents(const GameObject& obj) const
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

bool TestEventManager::isEventActive(const GameObject& obj, const unsigned id) const
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

const GameEvent* TestEventManager::rescheduleEvent(const GameEvent* event, unsigned targetGF)
{
    removeEventFromQueue(*event);
    // Hacky but we need to preserve the location (pointer) of the event as objects store it
    const_cast<GameEvent*>(event)->length = targetGF - event->startGF;
    return addEventToQueue(event);
}

std::vector<const GameEvent*> TestEventManager::getEvents() const
{
    return EventManager::getEvents();
}
