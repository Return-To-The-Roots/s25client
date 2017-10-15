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
#ifndef SERIALIZED_GAME_DATA_H_
#define SERIALIZED_GAME_DATA_H_

#pragma once

#include "FOWObjects.h"
#include "helpers/GetInsertIterator.hpp"
#include "helpers/ReserveElements.hpp"
#include "gameTypes/GO_Type.h"
#include "gameTypes/MapCoordinates.h"
#include "libutil/Serializer.h"
#include <boost/static_assert.hpp>
#include <map>
#include <set>
#include <stdexcept>

class GameObject;
class GameWorld;
class EventManager;
class BinaryFile;
class GameEvent;

/// Kümmert sich um das Serialisieren der GameDaten fürs Speichern und Resynchronisieren
class SerializedGameData : public Serializer
{
public:
    /// Exception that is thrown if an error during (de)serialization occured
    class Error : public std::runtime_error
    {
    public:
        explicit Error(const std::string& msg) : std::runtime_error(msg) {}
    };

    SerializedGameData();

    /// Nimmt das gesamte Spiel auf und speichert es im Buffer
    void MakeSnapshot(GameWorld& gw);
    /// Liest den Buffer aus einer Datei
    void ReadFromFile(BinaryFile& file) override;

    /// Reads the snapshot from the internal buffer
    void ReadSnapshot(GameWorld& gw);

    unsigned GetGameDataVersion() const { return gameDataVersion; }

    //////////////////////////////////////////////////////////////////////////
    // Write methods
    //////////////////////////////////////////////////////////////////////////

    /// Objekt(referenzen) kopieren
    template<class T>
    void PushObject(const T* go, const bool known)
    {
        /* The assert below basically checks the virtual function table.
           If the dynamic_cast failes, we tried to push an object of another type or it was deleted */
        const GameObject* goTmp = static_cast<const GameObject*>(go);
        RTTR_Assert(dynamic_cast<const T*>(goTmp) == go);
        PushObject_(goTmp, known);
    }

    void PushObject(const GameEvent* event, const bool known);

    /// Copies a container of GameObjects
    template<typename T>
    void PushObjectContainer(const T& gos, const bool known);

    /// Pushes a container of values
    template<typename T>
    void PushContainer(const T& container);

    /// FoW-Objekt
    void PushFOWObject(const FOWObject* fowobj);

    template<typename T>
    void PushPoint(const Point<T>& pt);

    /// Point of map coords
    void PushMapPoint(const MapPoint pt) { PushPoint(pt); }

    //////////////////////////////////////////////////////////////////////////
    // Read methods
    //////////////////////////////////////////////////////////////////////////

    /// Objekt(referenzen) lesen
    template<typename T>
    T* PopObject(GO_Type got)
    {
        return static_cast<T*>(PopObject_(got));
    }

    const GameEvent* PopEvent();

    /// FoW-Objekt
    FOWObject* PopFOWObject();

    /// Liest einen Vektor von GameObjects
    template<typename T>
    void PopObjectContainer(T& gos, GO_Type got);

    /// Reads a container of values, param NOT used. Only for automatic type deduction
    template<typename T>
    T PopContainer(const T& = T());

    template<typename T>
    Point<T> PopPoint();

    /// Point of map coords
    MapPoint PopMapPoint() { return PopPoint<MapPoint::ElementType>(); }

    /// Adds a deserialized object to the storage. Must be called exactly once per read GameObject
    void AddObject(GameObject* go);

    /// Returns whether the object with the given id was already serialized (only valid during writing)
    bool IsObjectSerialized(const unsigned obj_id) const;

    bool debugMode;

private:
    static unsigned short GetSafetyCode(const GameObject& go);

    // Version of the game data that is read. Gets set to the current version for writing
    unsigned gameDataVersion;

    /// Stores the ids of all written objects (-> only valid during writing)
    std::set<unsigned> writtenObjIds;
    /// Maps already read object ids to GameObjects (-> only valid during reading)
    std::map<unsigned, GameObject*> readObjects;

    /// Aktuelle Anzahl an Objekten
    unsigned objectsCount;
    /// Expected number of objects to be read/written
    unsigned expectedObjectsCount;

    /// EventManager, used during deserialization to add events, NULL otherwise
    EventManager* em;
    /// Is set to true when currently in read mode
    bool isReading;

    /// Starts reading or writing according to the param
    void Prepare(bool reading);
    /// Erzeugt GameObject
    GameObject* Create_GameObject(const GO_Type got, const unsigned obj_id);
    /// Erzeugt FOWObject
    FOWObject* Create_FOWObject(const FOW_Type fowtype);

    void PushObject_(const GameObject* go, const bool known);
    /// Objekt(referenzen) lesen
    GameObject* PopObject_(GO_Type got);

    /// Returns the object with the given id when it was read, NULL otherwhise (only valid during reading)
    GameObject* GetReadGameObject(const unsigned obj_id) const;
};

////////////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////////////

template<>
inline void Serializer::Push(bool val)
{
    PushBool(val);
}

template<>
inline bool Serializer::Pop<bool>()
{
    return PopBool();
}

template<typename T>
void SerializedGameData::PushObjectContainer(const T& gos, const bool known)
{
    // Anzahl
    PushUnsignedInt(gos.size());
    // einzelne Objekte
    for(typename T::const_iterator it = gos.begin(); it != gos.end(); ++it)
        PushObject(*it, known);
}

template<typename T>
void SerializedGameData::PopObjectContainer(T& gos, GO_Type got)
{
    typedef typename T::value_type ObjectPtr;
    typedef typename helpers::remove_pointer<ObjectPtr>::type Object;

    unsigned size = PopUnsignedInt();
    helpers::ReserveElements<T>::reserve(gos, size);
    typename helpers::GetInsertIterator<T>::iterator it = helpers::GetInsertIterator<T>::get(gos);
    for(unsigned i = 0; i < size; ++i)
        *it = PopObject<Object>(got);
}

template<typename T>
void SerializedGameData::PushContainer(const T& container)
{
    typedef typename T::value_type Type;
    BOOST_STATIC_ASSERT_MSG(boost::is_integral<Type>::value, "Only integral types are possible");
    PushUnsignedInt(container.size());
    for(typename T::const_iterator it = container.begin(); it != container.end(); ++it)
    {
        // Explicit template argument required for bool vector -.-
        Push<Type>(*it);
    }
}

template<typename T>
T SerializedGameData::PopContainer(const T&)
{
    typedef typename T::value_type Type;
    BOOST_STATIC_ASSERT_MSG(boost::is_integral<Type>::value, "Only integral types are possible");

    T result;
    unsigned size = PopUnsignedInt();
    helpers::ReserveElements<T>::reserve(result, size);
    typename helpers::GetInsertIterator<T>::iterator it = helpers::GetInsertIterator<T>::get(result);
    for(unsigned i = 0; i < size; ++i)
    {
        *it = Pop<Type>();
    }
    return result;
}

template<typename T>
void SerializedGameData::PushPoint(const Point<T>& pt)
{
    Push(pt.x);
    Push(pt.y);
}

template<typename T>
Point<T> SerializedGameData::PopPoint()
{
    Point<T> pt;
    pt.x = Pop<T>();
    pt.y = Pop<T>();
    return pt;
}

#endif // !SERIALIZED_GAME_DATA_H_
