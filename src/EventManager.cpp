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

#include "rttrDefines.h" // IWYU pragma: keep
#include "EventManager.h"
#include "GameEvent.h"
#include "GameObject.h"
#include "SerializedGameData.h"
#include "helpers/containerUtils.h"
#include "helpers/mapTraits.h"
#include "libutil/Log.h"
#include <boost/foreach.hpp>
#include <boost/format.hpp>

EventManager::EventManager(unsigned startGF) : numActiveEvents(0), eventInstanceCtr(1), currentGF(startGF), curActiveEvent(NULL) {}

EventManager::~EventManager()
{
    Clear();
}

void EventManager::Clear()
{
    for(EventMap::iterator it = events.begin(); it != events.end(); ++it)
    {
        BOOST_FOREACH(GameEvent* ev, it->second)
        {
            delete ev;
            RTTR_Assert(numActiveEvents > 0u);
            numActiveEvents--;
        }
    }
    events.clear();
    RTTR_Assert(numActiveEvents == 0u);

    for(GameObjList::iterator it = killList.begin(); it != killList.end(); ++it)
    {
        GameObject* obj = *it;
        *it = NULL;
        delete obj;
    }
    killList.clear();

    // Reset counters (next should already be 0 but just to be sure)
    numActiveEvents = 0u;
    // 0 == unused -> start at 1
    eventInstanceCtr = 1u;
}

GameEvent* EventManager::AddEventToQueue(GameEvent* event)
{
    // Should be in the future!
    RTTR_Assert(event->GetTargetGF() > currentGF);
    events[event->GetTargetGF()].push_back(event);
    ++numActiveEvents;
    return event;
}

const GameEvent* EventManager::AddEvent(GameObject* obj, unsigned gf_length, unsigned id)
{
    RTTR_Assert(obj);
    RTTR_Assert(gf_length);

    return AddEventToQueue(new GameEvent(GetNextEventInstanceId(), obj, currentGF, gf_length, id));
}

const GameEvent* EventManager::AddEvent(GameObject* obj, unsigned gf_length, unsigned id, unsigned gf_elapsed)
{
    RTTR_Assert(gf_length > gf_elapsed);
    // Anfang des Events in die Vergangenheit zurückverlegen
    RTTR_Assert(currentGF >= gf_elapsed);
    return AddEventToQueue(new GameEvent(GetNextEventInstanceId(), obj, currentGF - gf_elapsed, gf_length, id));
}

unsigned EventManager::GetNextEventInstanceId()
{
    unsigned result = eventInstanceCtr++;
    // Overflow detection. Highly unlikely
    RTTR_Assert(eventInstanceCtr != 0u);
    return result;
}

void EventManager::ExecuteNextGF()
{
    currentGF++;

    ExecuteCurrentEvents();
    DestroyCurrentObjects();
}

void EventManager::DestroyCurrentObjects()
{
    // Remove all objects
    for(GameObjList::iterator it = killList.begin(); it != killList.end(); ++it)
    {
        GameObject* obj = *it;
        // Object is no longer in the kill list (some may check this upon destruction)
        *it = NULL;
        obj->Destroy();
        delete obj;
    }

    killList.clear();
}

void EventManager::ExecuteCurrentEvents()
{
    if(events.empty())
        return;
    // Get list of events to be executed next
    EventMap::iterator itCurEvents = events.begin();

    RTTR_Assert(itCurEvents->first >= currentGF);

    if(itCurEvents->first == currentGF)
        ExecuteEvents(itCurEvents);
}

void EventManager::ExecuteEvents(const EventMap::iterator& itEvents)
{
    EventList& curEvents = itEvents->second;
    // We have to allow 2 cases:
    // 1) Adding of events to current GF -> std::list allows this without invalidating any iterators
    // 2) Checking for events -> Remove all deleted events so only valid ones are in the list
    for(EventList::iterator e_it = curEvents.begin(); e_it != curEvents.end(); e_it = curEvents.erase(e_it))
    {
        GameEvent* ev = (*e_it);
        RTTR_Assert(ev->obj);
        RTTR_Assert(ev->obj->GetObjId() <= GameObject::GetObjIDCounter());

        curActiveEvent = ev;
        ev->obj->HandleEvent(ev->id);

        delete ev;
        --numActiveEvents;
    }
    curActiveEvent = NULL;
    events.erase(itEvents);
}

void EventManager::Serialize(SerializedGameData& sgd) const
{
    static boost::format eventCtError("Event count mismatch. Found events: %1%. Expected: %2%.\n");

    // Kill list must be empty (do not store to-be-killed objects)
    RTTR_Assert(killList.empty());

    // Gather all events, that are not yet serialized
    unsigned numEvents = 0;
    std::vector<const GameEvent*> save_events;
    for(EventMap::const_iterator it = events.begin(); it != events.end(); ++it)
    {
        BOOST_FOREACH(const GameEvent* ev, it->second)
        {
            numEvents++;
            if(!sgd.IsEventSerialized(ev->GetInstanceId()))
                save_events.push_back(ev);
        }
    }
    if(numEvents != numActiveEvents)
        throw SerializedGameData::Error((eventCtError % numEvents % numActiveEvents).str());

    sgd.PushUnsignedInt(save_events.size());
    BOOST_FOREACH(const GameEvent* ev, save_events)
        sgd.PushEvent(ev);
    sgd.PushUnsignedInt(eventInstanceCtr);
    sgd.PushUnsignedInt(numActiveEvents);
}

void EventManager::Deserialize(SerializedGameData& sgd)
{
    static boost::format eventCtError("Event count mismatch. Found events: %1%. Expected: %2%.\n");
    static boost::format eventIdError("Invalid event instance id. Found: %1%. Expected less than %2%.\n");

    // This are just the not yet deserialized events
    unsigned numEvents = sgd.PopUnsignedInt();
    // Pop all events, but do NOT add them. Deserialization will already do so
    for(unsigned i = 0; i < numEvents; ++i)
        sgd.PopEvent();

    eventInstanceCtr = sgd.PopUnsignedInt();

    // Validation
    unsigned expectedNumActiveEvents = sgd.PopUnsignedInt();
    if(expectedNumActiveEvents != numActiveEvents)
        throw SerializedGameData::Error((eventCtError % numActiveEvents % expectedNumActiveEvents).str());
    for(EventMap::const_iterator it = events.begin(); it != events.end(); ++it)
    {
        BOOST_FOREACH(const GameEvent* ev, it->second)
        {
            if(ev->GetInstanceId() >= eventInstanceCtr)
                throw SerializedGameData::Error((eventIdError % ev->GetInstanceId() % eventInstanceCtr).str());
        }
    }
}

const GameEvent* EventManager::AddEvent(SerializedGameData& sgd, unsigned instanceId)
{
    return AddEventToQueue(new GameEvent(sgd, instanceId));
}

bool EventManager::ObjectHasEvents(const GameObject& obj)
{
    for(EventMap::iterator it = events.begin(); it != events.end(); ++it)
    {
        BOOST_FOREACH(GameEvent* ev, it->second)
        {
            if(ev->obj == &obj)
                return true;
        }
    }
    return false;
}

bool EventManager::IsObjectInKillList(const GameObject& obj)
{
    return helpers::contains(killList, &obj);
}

void EventManager::RemoveEvent(const GameEvent*& ep)
{
    if(!ep)
        return;

    if(ep == curActiveEvent)
    {
        RTTR_Assert(false);
        LOG.write("Bug detected: Active event deleted");
        ep = NULL;
        return;
    }
    RemoveEventFromQueue(*ep);
    deletePtr(ep);
}

void EventManager::RemoveEventFromQueue(const GameEvent& event)
{
    RTTR_Assert(curActiveEvent != &event);
    EventMap::iterator itEventsAtTime = events.find(event.GetTargetGF());
    if(itEventsAtTime != events.end())
    {
        EventList& eventsAtTime = itEventsAtTime->second;
        EventList::iterator e_it = helpers::find(eventsAtTime, &event);
        if(e_it != eventsAtTime.end())
        {
            eventsAtTime.erase(e_it);
            --numActiveEvents;
            RTTR_Assert(!helpers::contains(eventsAtTime, &event)); // Event existed multiple times?
        } else
        {
            RTTR_Assert(false);
            LOG.write("Bug detected: Event to be removed did not exist");
        }

        // Note: Removing this is possible, as it cannot be the currently processed list
        //       because there is always the curActiveEvent left, which cannot be removed (check above)
        if(eventsAtTime.empty())
            events.erase(itEventsAtTime);
    } else
    {
        RTTR_Assert(false);
        LOG.write("Bug detected: GF of event to be removed did not exist");
    }
}

void EventManager::AddToKillList(GameObject* obj)
{
    RTTR_Assert(obj);
    RTTR_Assert(!IsObjectInKillList(*obj));
    killList.push_back(obj);
}
