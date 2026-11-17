// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "EventManager.h"
#include "GameEvent.h"
#include "GameObject.h"
#include "SerializedGameData.h"
#include "helpers/containerUtils.h"
#include "s25util/Log.h"
#include <mygettext/mygettext.h>

EventManager::EventManager(unsigned startGF)
    : numActiveEvents(0), eventInstanceCtr(1), currentGF(startGF), curActiveEvent(nullptr)
{}

EventManager::~EventManager()
{
    clear();
}

void EventManager::clear()
{
    for(const auto& event : events)
    {
        for(const GameEvent* ev : event.second)
        {
            delete ev;
            RTTR_Assert(numActiveEvents > 0u);
            numActiveEvents--;
        }
    }
    events.clear();
    RTTR_Assert(numActiveEvents == 0u);

    for(auto* it : killList)
    {
        GameObject* obj = it;
        it = nullptr;
        delete obj;
    }
    killList.clear();

    // Reset counters (next should already be 0 but just to be sure)
    numActiveEvents = 0u;
    // 0 == unused -> start at 1
    eventInstanceCtr = 1u;
}

const GameEvent* EventManager::addEventToQueue(const GameEvent* event)
{
    // Should be in the future!
    RTTR_Assert(event->getTargetGF() > currentGF);
    events[event->getTargetGF()].push_back(event);
    ++numActiveEvents;
    return event;
}

const GameEvent* EventManager::addEvent(GameObject* obj, unsigned gf_length, unsigned id)
{
    RTTR_Assert(obj);
    RTTR_Assert(gf_length);

    return addEventToQueue(new GameEvent(getNextEventInstanceId(), obj, currentGF, gf_length, id));
}

const GameEvent* EventManager::addEvent(GameObject* obj, unsigned gf_length, unsigned id, unsigned gf_elapsed)
{
    RTTR_Assert(gf_length > gf_elapsed);
    // Anfang des Events in die Vergangenheit zurückverlegen
    RTTR_Assert(currentGF >= gf_elapsed);
    return addEventToQueue(new GameEvent(getNextEventInstanceId(), obj, currentGF - gf_elapsed, gf_length, id));
}

unsigned EventManager::getNextEventInstanceId()
{
    unsigned result = eventInstanceCtr++;
    // Overflow detection. Highly unlikely
    RTTR_Assert(eventInstanceCtr != 0u);
    return result;
}

void EventManager::executeNextGF()
{
    currentGF++;

    executeCurrentEvents();
    destroyCurrentObjects();
}

void EventManager::destroyCurrentObjects()
{
    // Remove all objects
    for(auto& it : killList)
    {
        GameObject* obj = it;
        // Object is no longer in the kill list (some may check this upon destruction)
        it = nullptr;
        obj->destroy();
        RTTR_Assert(!objectHasEvents(*obj));
        delete obj;
    }

    killList.clear();
}

std::vector<const GameEvent*> EventManager::getEvents() const
{
    std::vector<const GameEvent*> nextEv;
    for(const auto& event : events)
        nextEv.insert(nextEv.end(), event.second.begin(), event.second.end());
    return nextEv;
}

void EventManager::executeCurrentEvents()
{
    if(events.empty())
        return;
    // Get list of events to be executed next
    auto itCurEvents = events.begin();

    RTTR_Assert(itCurEvents->first >= currentGF);

    if(itCurEvents->first == currentGF)
        executeEvents(itCurEvents);
}

void EventManager::executeEvents(const EventMap::iterator& itEvents)
{
    EventList& curEvents = itEvents->second;
    // We have to allow 2 cases:
    // 1) Adding of events to current GF -> std::list allows this without invalidating any iterators
    // 2) Checking for events -> Remove all deleted events so only valid ones are in the list
    for(auto e_it = curEvents.begin(); e_it != curEvents.end(); e_it = curEvents.erase(e_it))
    {
        const GameEvent* ev = *e_it;
        RTTR_Assert(ev->obj);
        RTTR_Assert(ev->obj->getObjId() <= GameObject::getObjIDCounter());

        curActiveEvent = ev;
        ev->obj->handleEvent(ev->id);

        delete ev;
        --numActiveEvents;
    }
    curActiveEvent = nullptr;
    events.erase(itEvents);
}

void EventManager::serialize(SerializedGameData& sgd) const
{
    // Kill list must be empty (do not store to-be-killed objects)
    RTTR_Assert(killList.empty());

    // Gather all events in the correct order
    std::vector<const GameEvent*> saveEvents = getEvents();
    if(saveEvents.size() != numActiveEvents)
    {
        boost::format eventCtError(_("Event count mismatch. Found events: %1%. Expected: %2%.\n"));
        throw SerializedGameData::Error((eventCtError % saveEvents.size() % numActiveEvents).str());
    }

    sgd.pushUnsignedInt(saveEvents.size());
    for(const GameEvent* ev : saveEvents)
        sgd.pushEvent(ev);
    sgd.pushUnsignedInt(eventInstanceCtr);
}

void EventManager::deserialize(SerializedGameData& sgd)
{
    if(numActiveEvents != 0)
        throw SerializedGameData::Error(_("Cannot deserialize event manager when there are still events active!"));

    unsigned numEvents = sgd.popUnsignedInt();
    for(unsigned i = 0; i < numEvents; ++i)
        addEventToQueue(sgd.popEvent());

    eventInstanceCtr = sgd.popUnsignedInt();

    // Validation
    if(numEvents != numActiveEvents)
    {
        boost::format eventCtError(_("Event count mismatch. Read events: %1%. Expected: %2%.\n"));
        throw SerializedGameData::Error((eventCtError % numActiveEvents % numEvents).str());
    }
    for(const auto& it : events)
    {
        for(const GameEvent* ev : it.second)
        {
            if(ev->getInstanceId() >= eventInstanceCtr)
            {
                boost::format eventIdError(_("Invalid event instance id. Found: %1%. Expected less than %2%.\n"));
                throw SerializedGameData::Error((eventIdError % ev->getInstanceId() % eventInstanceCtr).str());
            }
        }
    }
}

bool EventManager::objectHasEvents(const GameObject& obj)
{
    for(const auto& event : events)
    {
        for(const GameEvent* ev : event.second)
        {
            if(ev->obj == &obj)
                return true;
        }
    }
    return false;
}

bool EventManager::isObjectInKillList(const GameObject& obj)
{
    return helpers::contains(killList, &obj);
}

void EventManager::removeEvent(const GameEvent*& ep)
{
    if(!ep)
        return;

    if(ep == curActiveEvent)
    {
        RTTR_Assert(false);
        LOG.write("Bug detected: Active event deleted");
        ep = nullptr;
        return;
    }
    removeEventFromQueue(*ep);
    deletePtr(ep);
}

void EventManager::removeEventFromQueue(const GameEvent& event)
{
    RTTR_Assert(curActiveEvent != &event);
    auto itEventsAtTime = events.find(event.getTargetGF());
    if(itEventsAtTime != events.end())
    {
        EventList& eventsAtTime = itEventsAtTime->second;
        auto e_it = helpers::find(eventsAtTime, &event);
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

void EventManager::addToKillList(GameObject* obj)
{
    RTTR_Assert(!isObjectInKillList(assertNonNull(obj)));
    killList.emplace_back(obj);
}

void EventManager::addToKillList(std::unique_ptr<GameObject> obj)
{
    addToKillList(obj.release());
}
