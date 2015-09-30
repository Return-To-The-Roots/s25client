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
#ifndef SERIALIZED_GAME_DATA_H_
#define SERIALIZED_GAME_DATA_H_

#pragma once

#include "GameObject.h"
#include "FOWObjects.h"
#include "Serializer.h"
#include "helpers/ReserveElements.hpp"
#include "helpers/GetInsertIterator.hpp"

class noBase;
class GameObject;
class GameWorld;
class EventManager;

/// Kümmert sich um das Serialisieren der GameDaten fürs Speichern und Resynchronisieren
class SerializedGameData : public Serializer
{
public:

    SerializedGameData();

    /// Nimmt das gesamte Spiel auf und speichert es im Buffer
    void MakeSnapshot(GameWorld& gw, EventManager& em);
    /// Liest den Buffer aus einer Datei
    void ReadFromFile(BinaryFile& file);

    void PrepareDeserialization(EventManager* const em) { this->em = em; }

    //////////////////////////////////////////////////////////////////////////
    // Write methods
    //////////////////////////////////////////////////////////////////////////

    /// Objekt(referenzen) kopieren
    void PushObject(const GameObject* go, const bool known);

    /// Copies a container of GameObjects
    template <typename T>
    void PushObjectContainer(const T& gos, const bool known);

    /// FoW-Objekt
    void PushFOWObject(const FOWObject* fowobj);

    /// Point of map coords
    void PushMapPoint(const MapPoint p);

    //////////////////////////////////////////////////////////////////////////
    // Read methods
    //////////////////////////////////////////////////////////////////////////

    /// Objekt(referenzen) lesen
    template <typename T>
    T* PopObject(GO_Type got) { return static_cast<T*>(PopObject_(got)); }

    /// FoW-Objekt
    FOWObject* PopFOWObject();

    /// Liest einen Vektor von GameObjects
    template <typename T>
    void PopObjectContainer(T& gos, GO_Type got);

    /// Point of map coords
    MapPoint PopMapPoint();

    /// Fügt ein gelesenes Objekt zur globalen Objektliste dazu
    void AddObject(GameObject* go);

    /// Sucht ein Objekt, falls vorhanden
    const GameObject* GetConstGameObject(const unsigned obj_id) const;
    GameObject* GetGameObject(const unsigned obj_id) const;

private:
    /// Objektreferenzen
    union
    {
        const GameObject** objects_write;
        GameObject** objects_read;
    };
    /// Voraussichtliche Gesamtanzahl an Objekten (nur beim Laden)
    unsigned total_objects_count;
    /// Aktuelle Anzahl an Objekten
    unsigned objects_count;

    EventManager* em;

    /// Erzeugt GameObject
    GameObject* Create_GameObject(const GO_Type got, const unsigned obj_id);
    /// Erzeugt FOWObject
    FOWObject* Create_FOWObject(const FOW_Type fowtype);

    /// Objekt(referenzen) lesen
    GameObject* PopObject_(GO_Type got);
};


////////////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////////////

template <typename T>
void SerializedGameData::PushObjectContainer(const T& gos, const bool known)
{
    // Anzahl
    PushUnsignedInt(gos.size());
    // einzelne Objekte
    for (typename T::const_iterator it = gos.begin(); it != gos.end(); ++it)
        PushObject(*it, known);
}

template <typename T>
void SerializedGameData::PopObjectContainer(T& gos, GO_Type got)
{
    typedef typename T::value_type ObjectPtr;
    typedef typename helpers::remove_pointer<ObjectPtr>::type Object;

    unsigned size = PopUnsignedInt();
    helpers::ReserveElements<T>::reserve(gos, size);
    typename helpers::GetInsertIterator<T>::iterator it = helpers::GetInsertIterator<T>::get(gos);
    for (unsigned i = 0; i < size; ++i)
        *it = PopObject<Object>(got);
}

#endif // !SERIALIZED_GAME_DATA_H_
