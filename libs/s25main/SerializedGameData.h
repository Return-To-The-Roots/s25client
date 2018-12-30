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
#include <boost/shared_ptr.hpp>
#include <boost/static_assert.hpp>
#include <map>
#include <set>
#include <stdexcept>

class GameObject;
class GameWorld;
class EventManager;
class BinaryFile;
class GameEvent;
class Game;

/// Kümmert sich um das Serialisieren der GameDaten fürs Speichern und Resynchronisieren
class SerializedGameData : public Serializer
{
public:
    /// Exception that is thrown if an error during (de)serialization occurred
    class Error : public std::runtime_error
    {
    public:
        explicit Error(const std::string& msg) : std::runtime_error(msg) {}
    };

    SerializedGameData();

    /// Nimmt das gesamte Spiel auf und speichert es im Buffer
    void MakeSnapshot(boost::shared_ptr<Game> game);

    /// Reads the snapshot from the internal buffer
    void ReadSnapshot(boost::shared_ptr<Game> game);

    unsigned GetGameDataVersion() const { return gameDataVersion; }

    //////////////////////////////////////////////////////////////////////////
    // Write methods
    //////////////////////////////////////////////////////////////////////////

    /// Write a GameObject
    template<class T>
    void PushObject(const T* go, const bool known)
    {
        /* The assert below basically checks the virtual function table.
           If the dynamic_cast fails, we tried to push an object of another type or it was deleted */
        const GameObject* goTmp = static_cast<const GameObject*>(go);
        RTTR_Assert(dynamic_cast<const T*>(goTmp) == go);
        PushObject_(goTmp, known);
    }

    void PushEvent(const GameEvent* event);

    /// Write a container of GameObjects
    template<typename T>
    void PushObjectContainer(const T& gos, const bool known);

    /// Push a container of values
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

    /// Read a GameObject
    template<typename T>
    T* PopObject(GO_Type got)
    {
        return static_cast<T*>(PopObject_(got));
    }

    const GameEvent* PopEvent();

    /// FoW-Objekt
    FOWObject* PopFOWObject();

    /// Read a container of GameObjects
    template<typename T>
    void PopObjectContainer(T& gos, GO_Type got);

    /// Read a container of values
    template<typename T>
    void PopContainer(T& result);

    template<typename T>
    Point<T> PopPoint();

    /// Point of map coords
    MapPoint PopMapPoint() { return PopPoint<MapPoint::ElementType>(); }

    /// Adds a deserialized object to the storage. Must be called exactly once per read GameObject
    void AddObject(GameObject* go);
    /// Adds a deserialized event to the storage. Return instanceId. Must be called exactly once per read event
    unsigned AddEvent(unsigned instanceId, GameEvent* ev);
    /// Only valid during writing
    bool IsEventSerialized(unsigned evInstanceid) const;
    bool debugMode;

private:
    static unsigned short GetSafetyCode(const GameObject& go);
    static unsigned short GetSafetyCode(const GameEvent& ev);

    /// Version of the game data that is read. Gets set to the current version for writing
    unsigned gameDataVersion;

    /// Stores the ids of all written objects (-> only valid during writing)
    std::set<unsigned> writtenObjIds;
    std::set<unsigned> writtenEventIds;
    /// Maps already read object ids to GameObjects (-> only valid during reading)
    std::map<unsigned, GameObject*> readObjects;
    std::map<unsigned, GameEvent*> readEvents;

    /// Expected number of objects to be read/written
    unsigned expectedNumObjects;

    /// EventManager, used during deserialization to add events, NULL otherwise
    EventManager* em;
    /// EventManager, used during serialization to add events, NULL otherwise
    const EventManager* writeEm;
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

    /// Returns the object with the given id when it was read, NULL otherwise (only valid during reading)
    GameObject* GetReadGameObject(const unsigned obj_id) const;
    /// Returns whether the object with the given id was already serialized (only valid during writing)
    bool IsObjectSerialized(unsigned obj_id) const;
};

////////////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////////////

template<typename T>
void SerializedGameData::PushObjectContainer(const T& gos, const bool known)
{
    // Anzahl
    PushVarSize(gos.size());
    // einzelne Objekte
    for(typename T::const_iterator it = gos.begin(); it != gos.end(); ++it)
        PushObject(*it, known);
}

template<typename T>
void SerializedGameData::PopObjectContainer(T& gos, GO_Type got)
{
    typedef typename T::value_type ObjectPtr;
    typedef typename helpers::remove_pointer<ObjectPtr>::type Object;

    unsigned size = (GetGameDataVersion() >= 2) ? PopVarSize() : PopUnsignedInt();
    gos.clear();
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
    PushVarSize(container.size());
    for(typename T::const_iterator it = container.begin(); it != container.end(); ++it)
    {
        // Explicit template argument required for bool vector -.-
        Push<Type>(*it);
    }
}

template<typename T>
void SerializedGameData::PopContainer(T& result)
{
    typedef typename T::value_type Type;
    BOOST_STATIC_ASSERT_MSG(boost::is_integral<Type>::value, "Only integral types are possible");

    unsigned size = (GetGameDataVersion() >= 2) ? PopVarSize() : PopUnsignedInt();
    result.clear();
    helpers::ReserveElements<T>::reserve(result, size);
    typename helpers::GetInsertIterator<T>::iterator it = helpers::GetInsertIterator<T>::get(result);
    for(unsigned i = 0; i < size; ++i)
    {
        *it = Pop<Type>();
    }
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
