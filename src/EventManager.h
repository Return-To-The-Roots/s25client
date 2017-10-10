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
#ifndef EVENTMANAGER_H_INCLUDED
#define EVENTMANAGER_H_INCLUDED

#pragma once

#include <list>
#include <map>

class SerializedGameData;
class GameEvent;
class GameObject;

class EventManager
{
public:
    explicit EventManager(unsigned startGF);
    ~EventManager();

    /// Increase the GF# and execute all events of that GF
    void ExecuteNextGF();
    /// Add an event for the given object
    /// @param length Number of GFs after which it is executed (>0)
    /// @param id     ID of the event (passed to OnEvent)
    GameEvent* AddEvent(GameObject* obj, const unsigned length, const unsigned id = 0);
    /// Add an event that was started before, but paused (e.g. removed as someone stopped walking due to an obstacle)
    /// @param elapsed Number of GFs that have already elapsed of the length. Passing 0 is equal to adding a regular event
    GameEvent* AddEvent(GameObject* obj, const unsigned length, const unsigned id, const unsigned elapsed);
    /// Remove an event and sets the pointer to NULL
    void RemoveEvent(GameEvent*& ep);
    /// Add an object to be destroyed after current GF
    void AddToKillList(GameObject* obj);

    void Serialize(SerializedGameData& sgd) const;
    void Deserialize(SerializedGameData& sgd);
    /// Deserializes an event and adds it
    GameEvent* AddEvent(SerializedGameData& sgd, const unsigned obj_id);

    unsigned GetCurrentGF() const { return currentGF; }

    // Debugging
    /// Check if there is already an event of the given id for this object
    bool IsEventActive(const GameObject* const obj, const unsigned id) const;
    bool ObjectHasEvents(GameObject* obj);
    bool ObjectIsInKillList(GameObject* obj);

private:
    // Use list to allow removing of events while iterating (Event A can cause Event B in the same GF to be removed)
    typedef std::list<GameEvent*> EventList;
    typedef std::map<unsigned, EventList> EventMap;
    // Use list to allow adding events while iterating (Destroying 1 object may lead to destruction of another)
    typedef std::list<GameObject*> GameObjList;
    unsigned currentGF;
    EventMap events;      /// Mapping of GF to Events to be executed in this GF
    GameObjList killList; /// Objects that will be killed after current GF
    GameEvent* curActiveEvent;

    GameEvent* AddEvent(GameEvent* event);
};

#endif // !EVENTMANAGER_H_INCLUDED
