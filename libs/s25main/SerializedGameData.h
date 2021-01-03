// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "FOWObjects.h"
#include "RTTR_Assert.h"
#include "helpers/GetInsertIterator.hpp"
#include "helpers/MaxEnumValue.h"
#include "helpers/OptionalEnum.h"
#include "helpers/ReserveElements.hpp"
#include "helpers/serializeEnums.h"
#include "gameTypes/GO_Type.h"
#include "gameTypes/MapCoordinates.h"
#include "s25util/Serializer.h"
#include "s25util/warningSuppression.h"
#include <limits>
#include <map>
#include <memory>
#include <set>
#include <stdexcept>
#include <type_traits>

class GameObject;
class EventManager;
class GameEvent;
class Game;
class ILocalGameState;

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
    void MakeSnapshot(const std::shared_ptr<Game>& game);

    /// Reads the snapshot from the internal buffer
    void ReadSnapshot(const std::shared_ptr<Game>& game, ILocalGameState& localGameState);

    /// Get the format version the data is saved in. Deserializing methods can use this to support
    /// loading data saved in an earlier format.
    /// See `currentGameDataVersion` in SerializedGameData.cpp for version history
    unsigned GetGameDataVersion() const { return gameDataVersion; }

    //////////////////////////////////////////////////////////////////////////
    // Write methods
    //////////////////////////////////////////////////////////////////////////

    /// Write a GameObject
    template<class T>
    void PushObject(const T* go, bool known)
    {
        /* The assert below basically checks the virtual function table.
           If the dynamic_cast fails, we tried to push an object of another type or it was deleted */
        const auto* goTmp = static_cast<const GameObject*>(go);
        RTTR_Assert(dynamic_cast<const T*>(goTmp) == go); //-V547
        PushObject_(goTmp, known);
    }

    void PushEvent(const GameEvent* event);

    /// Write a container of GameObjects
    template<typename T>
    void PushObjectContainer(const T& gos, bool known);

    /// Push a container of values. Values must have fixed-width types!
    template<typename T>
    void PushContainer(const T& container);

    /// FoW-Objekt
    void PushFOWObject(const FOWObject* fowobj);

    template<typename T>
    void PushPoint(const Point<T>& pt);

    /// Point of map coords
    void PushMapPoint(const MapPoint pt) { PushPoint(pt); }

    /// Serialize an enum as T_SavedType which must be the underlying type (as that is what is deserialized by Pop<T>)
    /// Requires MaxEnumValue<T> to be specialized and T_SavedType to be able to hold all enumerators (checked only for
    /// max value)
    template<typename T_SavedType, typename T>
    void PushEnum(const T val)
    {
        helpers::pushEnum<T_SavedType>(*this, val);
    }
    template<typename T_SavedType, typename T>
    void PushOptionalEnum(const helpers::OptionalEnum<T> val)
    {
        if(val)
            PushEnum<T_SavedType>(*val);
        else
            Push<T_SavedType>(helpers::OptionalEnum<T>::invalidValue);
    }

    //////////////////////////////////////////////////////////////////////////
    // Read methods
    //////////////////////////////////////////////////////////////////////////

    /* FIXME: This function may be used to pop and cast incomplete objects due to a dependency cycle.
     Example: Builder->Building->Builder
     In this case Builder cannot be complete when it is stored into Building */
    /// Read a GameObject
    template<typename T>
    RTTR_ATTRIBUTE_NO_UBSAN(vptr)
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

    /// Read a container of values.  Values must have fixed-width types!
    template<typename T>
    void PopContainer(T& result);

    template<typename T>
    Point<T> PopPoint();

    /// Point of map coords
    MapPoint PopMapPoint() { return PopPoint<MapPoint::ElementType>(); }

    template<typename T>
    helpers::OptionalEnum<T> PopOptionalEnum();

    /// Read a trivial type (integral, enum, ...)
    template<typename T>
    std::enable_if_t<std::is_enum<T>::value, T> Pop();
    template<typename T>
    std::enable_if_t<!std::is_enum<T>::value, T> Pop();

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
    static Error makeOutOfRange(unsigned value, unsigned maxValue);

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

    /// EventManager, used during deserialization to add events, nullptr otherwise
    EventManager* em;
    /// EventManager, used during serialization to add events, nullptr otherwise
    const EventManager* writeEm;
    /// Is set to true when currently in read mode
    bool isReading;

    /// Starts reading or writing according to the param
    void Prepare(bool reading);
    /// Erzeugt GameObject
    GameObject* Create_GameObject(GO_Type got, unsigned obj_id);
    /// Erzeugt FOWObject
    FOWObject* Create_FOWObject(FoW_Type fowtype);

    void PushObject_(const GameObject* go, bool known);
    /// Objekt(referenzen) lesen
    GameObject* PopObject_(GO_Type got);

    /// Returns the object with the given id when it was read, nullptr otherwise (only valid during reading)
    GameObject* GetReadGameObject(unsigned obj_id) const;
    /// Returns whether the object with the given id was already serialized (only valid during writing)
    bool IsObjectSerialized(unsigned obj_id) const;
};

////////////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////////////

template<typename T>
void SerializedGameData::PushObjectContainer(const T& gos, bool known)
{
    // Anzahl
    PushVarSize(gos.size());
    // einzelne Objekte
    for(const auto* go : gos)
        PushObject(go, known);
}

template<typename T>
void SerializedGameData::PopObjectContainer(T& gos, GO_Type got)
{
    using ObjectPtr = typename T::value_type;
    using Object = std::remove_pointer_t<ObjectPtr>;

    unsigned size = (GetGameDataVersion() >= 2) ? PopVarSize() : PopUnsignedInt();
    gos.clear();
    helpers::ReserveElements<T>::reserve(gos, size);
    auto it = helpers::GetInsertIterator<T>::get(gos);
    for(unsigned i = 0; i < size; ++i)
        *it = PopObject<Object>(got);
}

template<typename T>
void SerializedGameData::PushContainer(const T& container)
{
    using Type = typename T::value_type;
    static_assert(std::is_integral<Type>::value || std::is_enum<Type>::value,
                  "Only integral types and enums are possible");
    using Integral =
      typename std::conditional_t<std::is_enum<Type>::value, std::underlying_type<Type>, std::common_type<Type>>::type;

    PushVarSize(container.size());
    for(const auto el : container)
    {
        // Cast also required for bool vector -.-
        Push(static_cast<Integral>(el));
    }
}

template<typename T>
void SerializedGameData::PopContainer(T& result)
{
    using Type = typename T::value_type;
    static_assert(std::is_integral<Type>::value || std::is_enum<Type>::value,
                  "Only integral types and enums are possible");

    unsigned size = (GetGameDataVersion() >= 2) ? PopVarSize() : PopUnsignedInt();
    result.clear();
    helpers::ReserveElements<T>::reserve(result, size);
    auto it = helpers::GetInsertIterator<T>::get(result);
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

template<typename T>
helpers::OptionalEnum<T> SerializedGameData::PopOptionalEnum()
{
    using Integral = std::underlying_type_t<T>;
    const auto value = Serializer::Pop<Integral>();
    if(value > helpers::MaxEnumValue_v<T> && value != helpers::OptionalEnum<T>::invalidValue)
        throw makeOutOfRange(value, helpers::MaxEnumValue_v<T>);
    // We can now safely convert to the enum value
    return static_cast<T>(value); // Avoid additional range checks
}

template<typename T>
std::enable_if_t<std::is_enum<T>::value, T> SerializedGameData::Pop()
{
    try
    {
        return helpers::popEnum<T>(*this);
    } catch(const std::range_error& e)
    {
        throw Error(e.what());
    }
}
template<typename T>
std::enable_if_t<!std::is_enum<T>::value, T> SerializedGameData::Pop()
{
    return Serializer::Pop<T>();
}
