// $Id: GameObject.h 9357 2014-04-25 15:35:25Z FloSoft $
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
#ifndef GAMEOBJECT_H_INCLUDED
#define GAMEOBJECT_H_INCLUDED

#pragma once

#include <string>
#include <sstream>

class SerializedGameData;
class GameWorldGame;
class EventManager;
class GameClientPlayerList;

/// IDs für jede Klasse AM ENDE DER Hierarchie für die Speicherfunktionen
/// To be able to load old savegames and keep ids unique, please insert new
/// items at the end of the list.
enum GO_Type
{
    GOT_UNKNOWN = 0,
    GOT_NOTHING,
    GOT_NOB_HQ,
    GOT_NOB_MILITARY,
    GOT_NOB_STOREHOUSE,
    GOT_NOB_USUAL,
    GOT_NOB_SHIPYARD,
    GOT_NOB_HARBORBUILDING,
    GOT_BUILDINGSITE,
    GOT_NOF_AGGRESSIVEDEFENDER,
    GOT_NOF_ATTACKER,
    GOT_NOF_DEFENDER,
    GOT_NOF_PASSIVESOLDIER,
    GOT_NOF_WELLGUY,
    GOT_NOF_CARRIER,
    GOT_NOF_WOODCUTTER,
    GOT_NOF_FISHER,
    GOT_NOF_FORESTER,
    GOT_NOF_CARPENTER,
    GOT_NOF_STONEMASON,
    GOT_NOF_HUNTER,
    GOT_NOF_FARMER,
    GOT_NOF_MILLER,
    GOT_NOF_BAKER,
    GOT_NOF_BUTCHER,
    GOT_NOF_MINER,
    GOT_NOF_BREWER,
    GOT_NOF_PIGBREEDER,
    GOT_NOF_DONKEYBREEDER,
    GOT_NOF_IRONFOUNDER,
    GOT_NOF_MINTER,
    GOT_NOF_METALWORKER,
    GOT_NOF_ARMORER,
    GOT_NOF_BUILDER,
    GOT_NOF_PLANER,
    GOT_NOF_GEOLOGIST,
    GOT_NOF_SHIPWRIGHT,
    GOT_NOF_SCOUT_FREE,
    GOT_NOF_SCOUT_LOOKOUTTOWER,
    GOT_NOF_WAREHOUSEWORKER,
    GOT_NOF_CATAPULTMAN,
    GOT_NOF_PASSIVEWORKER,
    GOT_NOF_CHARBURNER,
    GOT_EXTENSION,
    GOT_ENVOBJECT,
    GOT_FIRE,
    GOT_FLAG,
    GOT_GRAINFIELD,
    GOT_GRANITE,
    GOT_SIGN,
    GOT_SKELETON,
    GOT_STATICOBJECT,
    GOT_DISAPPEARINGMAPENVOBJECT,
    GOT_TREE,
    GOT_ANIMAL,
    GOT_FIGHTING,
    GOT_EVENT,
    GOT_ROADSEGMENT,
    GOT_WARE,
    GOT_CATAPULTSTONE,
    GOT_BURNEDWAREHOUSE,
    GOT_SHIPBUILDINGSITE,
    GOT_SHIP,
    GOT_CHARBURNERPILE,
    GOT_NOF_TRADELEADER,
    GOT_NOF_TRADEDONKEY
};

/// Basisklasse für alle Spielobjekte
class GameObject
{
    public:
        /// Konstruktor von @p GameObject.
        GameObject(void);
        /// Deserialisierungskonstruktor
        GameObject(SerializedGameData* sgd, const unsigned obj_id);
        /// Copy-Konstruktor
        GameObject(const GameObject& go);
        /// Destruktor von @p GameObject.
        virtual ~GameObject(void);

        /// zerstört das Objekt.
        virtual void Destroy(void) = 0;

        /// Benachrichtigen, wenn neuer GF erreicht wurde.
        virtual void HandleEvent(const unsigned int id) {}

        /// Gibt Objekt-ID zurück.
        unsigned GetObjId() const { return obj_id; }

        /// Serialisierungsfunktion.
        virtual void Serialize(SerializedGameData* sgd) const = 0;
        /// Liefert den GOT (siehe oben)
        virtual GO_Type GetGOT(void) const = 0;

        /// Setzt Pointer auf GameWorld und EventManager
        static void SetPointers(GameWorldGame* const gwg, EventManager* const em, GameClientPlayerList* players)
        { GameObject::gwg = gwg; GameObject::em = em; GameObject::players = players; }
        /// setzt den Objekt und Objekt-ID-Counter zurück
        static void ResetCounter(void) { obj_id_counter = 1; obj_counter = 0; };
        /// Gibt Anzahl Objekte zurück.
        static unsigned GetObjCount() { return obj_counter; }
        /// Setzt Anzahl der Objekte (NUR FÃœR DAS LADEN!)
        static void SetObjCount(const unsigned obj_count)
        { obj_counter = obj_count; }
        /// Gibt Obj-ID-Counter zurück (NUR FÃœR DAS SPEICHERN!)
        static unsigned GetObjIDCounter() { return obj_id_counter; }
        /// Setzt Counter (NUR FÃœR DAS LADEN!)
        static void SetObjIDCounter(const unsigned obj_id_counter)
        { GameObject::obj_id_counter = obj_id_counter; }

        virtual std::string ToString() const {std::stringstream s; s << "GameObject(" << obj_id << ")"; return s.str();}
    protected:

        /// Serialisierungsfunktion.
        void Serialize_GameObject(SerializedGameData* sgd) const {}

    protected:
        const unsigned int obj_id; ///< eindeutige Objekt-ID

        /// Zugriff auf übrige Spielwelt
        static GameWorldGame* gwg;
        static EventManager* em;
        static GameClientPlayerList* players;

    private:

        static unsigned obj_id_counter; ///< Objekt-ID-Counter
        static unsigned obj_counter;    ///< Objekt-Counter
};

#endif /// GAMEOBJECT_H_INCLUDED
