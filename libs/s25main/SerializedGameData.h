// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "FOWObjects.h"
#include "RTTR_Assert.h"
#include "helpers/GetInsertIterator.hpp"
#include "helpers/MaxEnumValue.h"
#include "helpers/OptionalEnum.h"
#include "helpers/ReserveElements.hpp"
#include "helpers/serializeContainers.h"
#include "helpers/serializeEnums.h"
#include "helpers/serializePoint.h"
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
    void MakeSnapshot(const Game& game);

    /// Reads the snapshot from the internal buffer
    void ReadSnapshot(Game& game, ILocalGameState& localGameState);

    /// Get the format version the data is saved in. Deserializing methods can use this to support
    /// loading data saved in an earlier format.
    /// See `currentGameDataVersion` in SerializedGameData.cpp for version history
    unsigned GetGameDataVersion() const { return gameDataVersion; }

    //////////////////////////////////////////////////////////////////////////
    // Write methods
    //////////////////////////////////////////////////////////////////////////

    /// Write a GameObject
    template<class T>
    void PushObject(const T* go, bool known = false)
    {
        /* The assert below basically checks the virtual function table.
           If the dynamic_cast fails, we tried to push an object of another type or it was deleted */
        const auto* goTmp = static_cast<const GameObject*>(go);
        RTTR_Assert(dynamic_cast<const T*>(goTmp) == go); //-V547
        PushObject_(goTmp, known);
    }
    template<class T>
    void PushObject(const std::unique_ptr<T>& go, bool known = false)
    {
        PushObject(go.get(), known);
    }

    void PushEvent(const GameEvent* event);

    /// Write a container of GameObjects
    template<typename T>
    void PushObjectContainer(const T& gos, bool known = false);

    /// FoW-Objekt
    void PushFOWObject(const FOWObject* fowobj);

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
    T* PopObject(helpers::OptionalEnum<GO_Type> got = {})
    {
        return static_cast<T*>(PopObject_(got));
    }

    const GameEvent* PopEvent();

    /// FoW-Objekt
    std::unique_ptr<FOWObject> PopFOWObject();

    /// Read a container of GameObjects
    template<typename T>
    void PopObjectContainer(T& gos, helpers::OptionalEnum<GO_Type> got = {});

    /// Read a container of values.  Values must have fixed-width types!
    template<typename T>
    void PopContainer(T& result);

    template<typename T>
    helpers::OptionalEnum<T> PopOptionalEnum();

    /// Read a trivial type (integral, enum, ...)
    template<typename T>
    std::enable_if_t<std::is_enum<T>::value, T> Pop();
    template<typename T>
    std::enable_if_t<!std::is_enum<T>::value, T> Pop();

    MapPoint PopMapPoint() { return helpers::popPoint<MapPoint>(*this); }

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
    std::unique_ptr<GameObject> Create_GameObject(GO_Type got, unsigned obj_id);
    /// Erzeugt FOWObject
    std::unique_ptr<FOWObject> Create_FOWObject(FoW_Type fowtype);

    void PushObject_(const GameObject* go, bool known);
    /// Objekt(referenzen) lesen
    GameObject* PopObject_(helpers::OptionalEnum<GO_Type> got);

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
    for(const auto& go : gos)
        PushObject(go, known);
}

template<typename T>
void SerializedGameData::PopObjectContainer(T& gos, helpers::OptionalEnum<GO_Type> got)
{
    using Elements = typename T::value_type;
    using Object = std::remove_reference_t<decltype(**gos.begin())>;

    unsigned size = (GetGameDataVersion() >= 2) ? PopVarSize() : PopUnsignedInt();
    gos.clear();
    helpers::ReserveElements<T>::reserve(gos, size);
    auto it = helpers::GetInsertIterator<T>::get(gos);
    for(unsigned i = 0; i < size; ++i)
        *it = Elements(PopObject<Object>(got)); // Conversion required to support unique_ptr
}

template<typename T>
void SerializedGameData::PopContainer(T& result)
{
    // Remove this method after raising GameDataVersion
    if(GetGameDataVersion() >= 2)
        helpers::popContainer(*this, result);
    else
    {
        result.resize(PopUnsignedInt());
        helpers::popContainer(*this, result, true);
    }
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
