// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "commonDefines.h"
#include "gameTypes/GO_Type.h"
#include <memory>
#include <string>

class SerializedGameData;
class GameWorld;
class EventManager;
class PostMsg;

/// Base class for all game objects
class GameObject
{
public:
    GameObject();
    GameObject(SerializedGameData& sgd, unsigned obj_id);
    virtual ~GameObject();

protected:
    // We store GameObject references by address, so they must not be copied
    explicit GameObject(const GameObject&);

public:
    GameObject& operator=(const GameObject&) = delete;

    /// Handle destruction before deleting the instance
    virtual void Destroy() = 0;

    /// Called for each expiring event (i.e. target GF reached) that this instance has registered
    virtual void HandleEvent(unsigned /*id*/) {}

    /// Return the unique ID of an object. Always non-zero!
    unsigned GetObjId() const { return objId; }

    virtual void Serialize(SerializedGameData& sgd) const = 0;
    virtual GO_Type GetGOT() const = 0;

    virtual std::string ToString() const;

protected:
    // Following are some "sandbox methods". They avoid dependencies of subclasses to commonly used functions
    static EventManager& GetEvMgr();
    /// Send the msg to given player
    static void SendPostMessage(unsigned player, std::unique_ptr<PostMsg> msg);

private:
    unsigned objId; /// unique ID

    // Static members
public:
    /// Set the currently active world for all game objects
    static void AttachWorld(GameWorld* gameWorld);
    /// Remove the world from all game objects
    static void DetachWorld(GameWorld* gameWorld);
    /// Return the number of objects alive
    static unsigned GetNumObjs() { return objCounter_; }
    /// Return ID counter, i.e. the last object ID used
    static unsigned GetObjIDCounter() { return objIdCounter_; }
    /// Reset the object counter and the object ID counter to 0
    static void ResetCounters()
    {
        objIdCounter_ = 0;
        objCounter_ = 0;
    }
    /// Set the objIdCounter to the given value and resets the object counter to 1 (noNodeObj)
    static void ResetCounters(unsigned objIdCounter)
    {
        objIdCounter_ = objIdCounter;
        objCounter_ = 1;
    }

protected:
    /// Access to the currently active game world
    static GameWorld* world;

private:
    static unsigned objIdCounter_; /// Object-ID-Counter (number of objects created)
    static unsigned objCounter_;   /// Object-Counter (number of objects alive)
};

/// Calls destroy on a GameObject and then deletes it setting the ptr to nullptr
template<typename T>
void destroyAndDelete(T*& obj)
{
    obj->Destroy();
    deletePtr(obj);
}
/// Same but for smart pointers
template<typename T>
void destroyAndDelete(T& obj)
{
    obj->Destroy();
    obj.reset();
}
