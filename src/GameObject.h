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
#ifndef GAMEOBJECT_H_INCLUDED
#define GAMEOBJECT_H_INCLUDED

#pragma once

#include "gameTypes/GO_Type.h"
#include <string>
#include <sstream>

class SerializedGameData;
class GameWorldGame;
class EventManager;

/// Basisklasse für alle Spielobjekte
class GameObject
{
    public:
        GameObject();
        GameObject(SerializedGameData& sgd, const unsigned obj_id);
        GameObject(const GameObject& go);
        virtual ~GameObject();

        GameObject& operator=(const GameObject& obj);

        /// zerstört das Objekt.
        virtual void Destroy() = 0;

        /// Benachrichtigen, wenn neuer GF erreicht wurde.
        virtual void HandleEvent(const unsigned int  /*id*/) {}

        /// Gibt Objekt-ID zurück.
        unsigned GetObjId() const { return objId; }

        /// Serialisierungsfunktion.
        virtual void Serialize(SerializedGameData& sgd) const = 0;
        /// Liefert den GOT (siehe oben)
        virtual GO_Type GetGOT() const = 0;

        /// Setzt Pointer auf GameWorld und EventManager
        static void SetPointers(GameWorldGame* const gameWorld, EventManager* const eventManager){ GameObject::gwg = gameWorld; GameObject::em = eventManager; }
        /// setzt den Objekt und Objekt-ID-Counter zurück
        static void ResetCounter() { objIdCounter_ = 1; objCounter_ = 0; };
        /// Gibt Anzahl Objekte zurück.
        static unsigned GetObjCount() { return objCounter_; }
        /// Setzt Anzahl der Objekte (NUR FÜR DAS LADEN!)
        static void SetObjCount(const unsigned obj_count) { objCounter_ = obj_count; }
        /// Gibt Obj-ID-Counter zurück (NUR FÜR DAS SPEICHERN!)
        static unsigned GetObjIDCounter() { return objIdCounter_; }
        /// Setzt Counter (NUR FÜR DAS LADEN!)
        static void SetObjIDCounter(const unsigned obj_id_counter) { objIdCounter_ = obj_id_counter; }

        virtual std::string ToString() const {std::stringstream s; s << "GameObject(" << objId << ")"; return s.str();}
    protected:

        /// Serialisierungsfunktion.
        void Serialize_GameObject(SerializedGameData&  /*sgd*/) const {}

        /// Zugriff auf übrige Spielwelt
        static GameWorldGame* gwg;
        static EventManager* em;

    private:
        unsigned int objId; /// eindeutige Objekt-ID

        static unsigned objIdCounter_; /// Objekt-ID-Counter
        static unsigned objCounter_;    /// Objekt-Counter
};

#endif /// GAMEOBJECT_H_INCLUDED
