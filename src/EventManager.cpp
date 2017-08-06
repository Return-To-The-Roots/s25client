// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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
#include "libutil/src/Log.h"
#include <vector>

EventManager::EventManager(unsigned startGF) : currentGF(startGF), curActiveEvent(NULL)
{
}

EventManager::~EventManager()
{
    for(EventMap::iterator it = events.begin(); it != events.end(); ++it)
    {
        for(EventList::iterator e_it = it->second.begin(); e_it != it->second.end(); ++e_it)
            delete *e_it;
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

GameEvent* EventManager::AddEvent(GameEvent* event)
{
    // Should be in the future!
    RTTR_Assert(event->GetTargetGF() > currentGF);
    // Make sure the linked object is not an event itself
    RTTR_Assert(!dynamic_cast<GameEvent*>(event->obj));
    events[event->GetTargetGF()].push_back(event);
    return event;
}

GameEvent* EventManager::AddEvent(GameObject* obj, const unsigned gf_length, const unsigned id)
{
    RTTR_Assert(obj);
    RTTR_Assert(gf_length);

    return AddEvent(new GameEvent(obj, currentGF, gf_length, id));
}

GameEvent* EventManager::AddEvent(SerializedGameData& sgd, const unsigned obj_id)
{
    return AddEvent(new GameEvent(sgd, obj_id));
}

GameEvent* EventManager::AddEvent(GameObject* obj, const unsigned gf_length, const unsigned id, const unsigned gf_elapsed)
{
    RTTR_Assert(gf_length > gf_elapsed);
    // Anfang des Events in die Vergangenheit zurückverlegen
    RTTR_Assert(currentGF >= gf_elapsed);
    return AddEvent(new GameEvent(obj, currentGF - gf_elapsed, gf_length, id));
}

void EventManager::ExecuteNextGF()
{
    currentGF++;

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

void EventManager::Serialize(SerializedGameData& sgd) const
{
    // Kill list must be empty (do not store to-be-killed objects)
    RTTR_Assert(killList.empty());

    // Gather all events, that are not yet serialized
    std::vector<const GameEvent*> save_events;
    for(EventMap::const_iterator it = events.begin(); it != events.end(); ++it)
    {
        for(EventList::const_iterator e_it = it->second.begin(); e_it != it->second.end(); ++e_it)
        {
            if(!sgd.IsObjectSerialized((*e_it)->GetObjId()))
                save_events.push_back(*e_it);
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

bool EventManager::IsEventActive(const GameObject* const obj, const unsigned id) const
{
    for(EventMap::const_iterator it = events.begin(); it != events.end(); ++it)
    {
        for(EventList::const_iterator e_it = it->second.begin(); e_it != it->second.end(); ++e_it)
        {
            if((*e_it)->id == id && (*e_it)->obj == obj)
            {
                return true;
            }
        }
    }

    return false;
}

bool EventManager::ObjectHasEvents(GameObject* obj)
{
    for(EventMap::iterator it = events.begin(); it != events.end(); ++it)
    {
        EventList& curEvents = it->second;
        for(std::list<GameEvent*>::iterator e_it = curEvents.begin(); e_it != curEvents.end(); ++e_it)
        {
            if((*e_it)->obj == obj)
                return true;
        }
    }
    return false;
}

bool EventManager::ObjectIsInKillList(GameObject* obj)
{
    return helpers::contains(killList, obj);
}

void EventManager::RemoveEvent(GameEvent*& ep)
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

    EventMap::iterator itEventsAtTime = events.find(ep->GetTargetGF());
    if(itEventsAtTime != events.end())
    {
        EventList& eventsAtTime = itEventsAtTime->second;
        EventList::iterator e_it = std::find(eventsAtTime.begin(), eventsAtTime.end(), ep);
        if(e_it != eventsAtTime.end())
        {
            eventsAtTime.erase(e_it);
            RTTR_Assert(!helpers::contains(eventsAtTime, ep)); // Event existed multiple times?
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

    deletePtr(ep);
}

void EventManager::AddToKillList(GameObject* obj)
{
    RTTR_Assert(obj);
    RTTR_Assert(!helpers::contains(killList, obj));
    killList.push_back(obj);
}
