// $Id: EventManager.h 9363 2014-04-26 15:00:08Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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

#include "Singleton.h"
#include "GameConsts.h"
#include "list.h"
#include "GameObject.h"

#include <list>
#include <map>

class GameObject;
class SerializedGameData;

class EventManager
{
    public:
        class Event : public GameObject
        {
            public:

                GameObject* obj;
                unsigned int gf;
                unsigned int gf_length;
                unsigned int gf_next;
                unsigned int id;

            public:

                Event(GameObject* const  obj, const unsigned int gf, const unsigned int gf_length, const unsigned int id)
                    : obj(obj), gf(gf), gf_length(gf_length), id(id)
                {
                    gf_next = gf + gf_length;
                }

                Event(SerializedGameData* sgd, const unsigned obj_id);

                void Destroy(void);

                /// Serialisierungsfunktionen
            protected: void Serialize_Event(SerializedGameData* sgd) const;
            public: void Serialize(SerializedGameData* sgd) const { Serialize_Event(sgd); }

                GO_Type GetGOT() const { return GOT_EVENT; }

                // Vergleichsoperatur für chronologisches Einfügen nach Ziel-GF
                bool operator <= (const Event& other) const { return gf + gf_length <= other.gf + other.gf_length; }
        };
        typedef Event* EventPointer;

    public:
        ~EventManager();

        /// führt alle Events des aktuellen GameFrames aus.
        void NextGF();
        /// fügt ein Event der Eventliste hinzu.
        EventPointer AddEvent(GameObject* obj, const unsigned int gf_length, const unsigned int id = 0);
        /// Deserialisiert ein Event und fügt es hinzu
        EventPointer AddEvent(SerializedGameData* sgd, const unsigned obj_id);
        /// Fügt ein schon angebrochenes Event hinzu (Events, wenn jemand beim Laufen stehengeblieben ist z.B.)
        /// Ein altes Event wird also quasi fortgeführt (um gf_elapsed in der Vergangenheit angelegt)
        EventPointer AddEvent(GameObject* obj, const unsigned int gf_length, const unsigned int id, const unsigned gf_elapsed);

        /// Löscht alle Listen für Spielende
        void Clear() { eis.clear(); kill_list.clear(); }
        /// Event entfernen
        void RemoveEvent(EventPointer ep);
        /// Objekt will gekillt werden
        void AddToKillList(GameObject* obj) { assert(std::count(kill_list.begin(), kill_list.end(), obj) == 0); kill_list.push_back(obj); }

        /// Serialisieren
        void Serialize(SerializedGameData* sgd) const;
        /// Deserialisieren
        void Deserialize(SerializedGameData* sgd);

        /// Ist ein Event mit bestimmter id für ein bestimmtes Objekt bereits vorhanden?
        bool IsEventActive(const GameObject* const obj, const unsigned id) const;

        void RemoveAllEventsOfObject(GameObject* obj);
    private:
        std::map<unsigned, std::list<Event*> > eis;     ///< Liste der Events für die einzelnen Objekte
        std::list<GameObject*> kill_list; ///< Liste mit Objekten die unmittelbar nach NextGF gekillt werden sollen
};

#define EVENTMANAGER EventManager::inst()

#endif // !EVENTMANAGER_H_INCLUDED
