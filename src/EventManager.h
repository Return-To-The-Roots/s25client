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

#include "GameObject.h"

#include <list>
#include <map>

class SerializedGameData;

class EventManager
{
    public:
        class Event : public GameObject
        {
            public:
                GameObject* const obj;
                const unsigned gf;
                const unsigned gf_length;
                const unsigned gf_next;
                const unsigned id;

            public:

                Event(GameObject* const  obj, const unsigned int gf, const unsigned int gf_length, const unsigned int id)
                    : obj(obj), gf(gf), gf_length(gf_length), gf_next(gf + gf_length), id(id)
                {
                    RTTR_Assert(obj); // Events without an object are pointless
                }

                Event(SerializedGameData& sgd, const unsigned obj_id);

                void Destroy() override{}

                /// Serialisierungsfunktionen
            protected: void Serialize_Event(SerializedGameData& sgd) const;
            public: void Serialize(SerializedGameData& sgd) const override { Serialize_Event(sgd); }

                GO_Type GetGOT() const override { return GOT_EVENT; }

                // Vergleichsoperatur für chronologisches Einfügen nach Ziel-GF
                bool operator<= (const Event& other) const { return gf_next <= other.gf_next; }
                bool operator< (const Event& other) const { return gf_next < other.gf_next; }
        };
        typedef Event* EventPointer;

    public:
        EventManager(): curActiveEvent(NULL){}
        ~EventManager();

        /// führt alle Events des aktuellen GameFrames aus.
        void NextGF();
        /// fügt ein Event der Eventliste hinzu.
        EventPointer AddEvent(GameObject* obj, const unsigned int gf_length, const unsigned int id = 0);
        /// Deserialisiert ein Event und fügt es hinzu
        EventPointer AddEvent(SerializedGameData& sgd, const unsigned obj_id);
        /// Fügt ein schon angebrochenes Event hinzu (Events, wenn jemand beim Laufen stehengeblieben ist z.B.)
        /// Ein altes Event wird also quasi fortgeführt (um gf_elapsed in der Vergangenheit angelegt)
        EventPointer AddEvent(GameObject* obj, const unsigned int gf_length, const unsigned int id, const unsigned gf_elapsed);

        /// Löscht alle Listen für Spielende
        void Clear();
        /// Removes an event and sets the pointer to NULL
        void RemoveEvent(EventPointer& ep);
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
        typedef std::list<EventPointer> EventList;
        typedef std::map<unsigned, EventList> EventMap;
        typedef std::list<GameObject*> GameObjList;
        EventMap events;     /// Liste der Events für die einzelnen Objekte
        GameObjList kill_list; /// Liste mit Objekten die unmittelbar nach NextGF gekillt werden sollen
        EventPointer curActiveEvent;

        EventPointer AddEvent(EventPointer event);
};

#endif // !EVENTMANAGER_H_INCLUDED
