// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <list>
#include <map>
#include <memory>
#include <vector>

class SerializedGameData;
class GameEvent;
class GameObject;

class EventManager
{
public:
    explicit EventManager(unsigned startGF);
    ~EventManager();

    /// Deletes all events and objects to be killed. Then resets counters
    void clear();

    unsigned getNumActiveEvents() const { return numActiveEvents; }
    unsigned getEventInstanceCtr() const { return eventInstanceCtr; }

    /// Increase the GF# and execute all events of that GF
    void executeNextGF();
    /// Add an event for the given object
    /// @param length Number of GFs after which it is executed (>0)
    /// @param id     ID of the event (passed to OnEvent)
    const GameEvent* addEvent(GameObject* obj, unsigned gf_length, unsigned id = 0);
    /// Add an event that was started before, but paused (e.g. removed as someone stopped walking due to an obstacle)
    /// @param elapsed Number of GFs that have already elapsed of the length. Passing 0 is equal to adding a regular
    /// event
    const GameEvent* addEvent(GameObject* obj, unsigned gf_length, unsigned id, unsigned gf_elapsed);
    /// Remove an event and sets the pointer to nullptr
    void removeEvent(const GameEvent*& ep);
    /// Add an object to be destroyed after current GF
    void addToKillList(GameObject* obj);
    void addToKillList(std::unique_ptr<GameObject> obj);

    void serialize(SerializedGameData& sgd) const;
    void deserialize(SerializedGameData& sgd);

    unsigned getNextEventInstanceId();

    unsigned getCurrentGF() const { return currentGF; }

    /// Return true if the object has any active events. SLOW!
    bool objectHasEvents(const GameObject& obj);
    /// Return true if the object will be destroyed after the current GF
    bool isObjectInKillList(const GameObject& obj);

protected:
    // Use list to allow removing of events while iterating (Event A can cause Event B in the same GF to be removed)
    using EventList = std::list<const GameEvent*>;
    using EventMap = std::map<unsigned, EventList>;
    // Use list to allow adding events while iterating (Destroying 1 object may lead to destruction of another)
    using GameObjList = std::list<GameObject*>;
    unsigned numActiveEvents;
    /// Instances created. Must be != 0
    unsigned eventInstanceCtr;
    unsigned currentGF;
    EventMap events;      /// Mapping of GF to Events to be executed in this GF
    GameObjList killList; /// Objects that will be killed after current GF
    const GameEvent* curActiveEvent;

    const GameEvent* addEventToQueue(const GameEvent* event);
    void removeEventFromQueue(const GameEvent& event);
    /// Execute all events of the current GF
    void executeCurrentEvents();
    /// Execute the events from the given iterator
    void executeEvents(const EventMap::iterator& itEvents);
    /// Destroy all objects in the kill list
    void destroyCurrentObjects();
    /// Get all events in the order they will be processed
    std::vector<const GameEvent*> getEvents() const;
};
