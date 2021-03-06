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
    void Clear();

    unsigned GetNumActiveEvents() const { return numActiveEvents; }
    unsigned GetEventInstanceCtr() const { return eventInstanceCtr; }

    /// Increase the GF# and execute all events of that GF
    void ExecuteNextGF();
    /// Add an event for the given object
    /// @param length Number of GFs after which it is executed (>0)
    /// @param id     ID of the event (passed to OnEvent)
    const GameEvent* AddEvent(GameObject* obj, unsigned gf_length, unsigned id = 0);
    /// Add an event that was started before, but paused (e.g. removed as someone stopped walking due to an obstacle)
    /// @param elapsed Number of GFs that have already elapsed of the length. Passing 0 is equal to adding a regular
    /// event
    const GameEvent* AddEvent(GameObject* obj, unsigned gf_length, unsigned id, unsigned gf_elapsed);
    /// Remove an event and sets the pointer to nullptr
    void RemoveEvent(const GameEvent*& ep);
    /// Add an object to be destroyed after current GF
    void AddToKillList(GameObject* obj);
    void AddToKillList(std::unique_ptr<GameObject> obj);

    void Serialize(SerializedGameData& sgd) const;
    void Deserialize(SerializedGameData& sgd);

    unsigned GetNextEventInstanceId();

    unsigned GetCurrentGF() const { return currentGF; }

    /// Return true if the object has any active events. SLOW!
    bool ObjectHasEvents(const GameObject& obj);
    /// Return true if the object will be destroyed after the current GF
    bool IsObjectInKillList(const GameObject& obj);

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

    const GameEvent* AddEventToQueue(const GameEvent* event);
    void RemoveEventFromQueue(const GameEvent& event);
    /// Execute all events of the current GF
    void ExecuteCurrentEvents();
    /// Execute the events from the given iterator
    void ExecuteEvents(const EventMap::iterator& itEvents);
    /// Destroy all objects in the kill list
    void DestroyCurrentObjects();
    /// Get all events in the order they will be processed
    std::vector<const GameEvent*> GetEvents() const;
};
