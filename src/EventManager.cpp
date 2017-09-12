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
#include "EventManager.h"
#include "GameEvent.h"
#include "SerializedGameData.h"
#include "helpers/containerUtils.h"
#include "helpers/mapTraits.h"
#include "libutil/Log.h"
#include <boost/foreach.hpp>

EventManager::EventManager(unsigned startGF) : currentGF(startGF), curActiveEvent(NULL)
{
}

EventManager::~EventManager()
{
    for(EventMap::iterator it = events.begin(); it != events.end(); ++it)
    {
        BOOST_FOREACH(GameEvent* ev, it->second)
            delete ev;
    }
    events.clear();

    for(GameObjList::iterator it = killList.begin(); it != killList.end(); ++it)
    {
        GameObject* obj = *it;
        *it = NULL;
        delete obj;
    }
    killList.clear();
}

GameEvent* EventManager::AddEventToQueue(GameEvent* event)
{
    // Should be in the future!
    RTTR_Assert(event->GetTargetGF() > currentGF);
    // Make sure the linked object is not an event itself
    RTTR_Assert(!dynamic_cast<GameEvent*>(event->obj));
    events[event->GetTargetGF()].push_back(event);
    return event;
}

const GameEvent* EventManager::AddEvent(GameObject* obj, unsigned gf_length, unsigned id)
{
    RTTR_Assert(obj);
    RTTR_Assert(gf_length);

    return AddEventToQueue(new GameEvent(obj, currentGF, gf_length, id));
}

const GameEvent* EventManager::AddEvent(GameObject* obj, unsigned gf_length, unsigned id, unsigned gf_elapsed)
{
    RTTR_Assert(gf_length > gf_elapsed);
    // Anfang des Events in die Vergangenheit zurückverlegen
    RTTR_Assert(currentGF >= gf_elapsed);
    return AddEventToQueue(new GameEvent(obj, currentGF - gf_elapsed, gf_length, id));
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
    RTTR_Assert(events.empty() || events.begin()->first >= currentGF);

    // Get list of events for current GF
    EventMap::iterator itCurEvents = events.find(currentGF);
    if(itCurEvents != events.end())
    {
        EventList& curEvents = itCurEvents->second;
        // We have to allow 2 cases:
        // 1) Adding of events to current GF -> std::list allows this without invalidating any iterators
        // 2) Checking for events -> Remove all deleted events so only valid ones are in the list
        for(EventList::iterator e_it = curEvents.begin(); e_it != curEvents.end(); e_it = curEvents.erase(e_it))
        {
            GameEvent* ev = (*e_it);
            RTTR_Assert(ev->obj);
            RTTR_Assert(ev->obj->GetObjId() < GameObject::GetObjIDCounter());

            curActiveEvent = ev;
            ev->obj->HandleEvent(ev->id);

            delete ev;
        }
        curActiveEvent = NULL;
        events.erase(itCurEvents);
    }
}

void EventManager::Serialize(SerializedGameData& sgd) const
{
    // Kill list must be empty (do not store to-be-killed objects)
    RTTR_Assert(killList.empty());

    // Gather all events, that are not yet serialized
    std::vector<const GameEvent*> save_events;
    for(EventMap::const_iterator it = events.begin(); it != events.end(); ++it)
    {
        BOOST_FOREACH(const GameEvent* ev, it->second)
        {
            if(!sgd.IsObjectSerialized(ev->GetObjId()))
                save_events.push_back(ev);
        }
    }

    sgd.PushObjectContainer(save_events, true);
}

void EventManager::Deserialize(SerializedGameData& sgd)
{
    unsigned size = sgd.PopUnsignedInt();
    // Pop all events, but do NOT add them. Deserialization will already do so
    for(unsigned i = 0; i < size; ++i)
        sgd.PopEvent();
}

GameEvent* EventManager::AddEvent(SerializedGameData& sgd, unsigned obj_id)
{
    return AddEventToQueue(new GameEvent(sgd, obj_id));
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
