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
        EventManager(): curActiveEvent(NULL){}
        ~EventManager();

        /// führt alle Events des aktuellen GameFrames aus.
        void NextGF();
        /// fügt ein Event der Eventliste hinzu.
        GameEvent* AddEvent(GameObject* obj, const unsigned int gf_length, const unsigned int id = 0);
        /// Deserialisiert ein Event und fügt es hinzu
        GameEvent* AddEvent(SerializedGameData& sgd, const unsigned obj_id);
        /// Fügt ein schon angebrochenes Event hinzu (Events, wenn jemand beim Laufen stehengeblieben ist z.B.)
        /// Ein altes Event wird also quasi fortgeführt (um gf_elapsed in der Vergangenheit angelegt)
        GameEvent* AddEvent(GameObject* obj, const unsigned int gf_length, const unsigned int id, const unsigned gf_elapsed);

        /// Löscht alle Listen für Spielende
        void Clear();
        /// Removes an event and sets the pointer to NULL
        void RemoveEvent(GameEvent*& ep);
        /// Objekt will gekillt werden
        void AddToKillList(GameObject* obj);

        /// Serialisieren
        void Serialize(SerializedGameData& sgd) const;
        /// Deserialisieren
        void Deserialize(SerializedGameData& sgd);

        /// Ist ein Event mit bestimmter id für ein bestimmtes Objekt bereits vorhanden?
        bool IsEventActive(const GameObject* const obj, const unsigned id) const;

        void RemoveAllEventsOfObject(GameObject* obj);
        bool ObjectHasEvents(GameObject* obj);
        bool ObjectIsInKillList(GameObject* obj);
    private:
        typedef std::list<GameEvent*> EventList;
        typedef std::map<unsigned, EventList> EventMap;
        typedef std::list<GameObject*> GameObjList;
        EventMap events;      /// Mapping of GF to Events to be executed in this GF
        GameObjList killList; /// Objects that will be killed after current GF
        GameEvent* curActiveEvent;

        GameEvent* AddEvent(GameEvent* event);
};

#endif // !EVENTMANAGER_H_INCLUDED
