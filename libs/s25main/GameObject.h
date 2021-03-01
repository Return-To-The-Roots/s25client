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

#include "commonDefines.h"
#include "gameTypes/GO_Type.h"
#include <memory>
#include <string>

class SerializedGameData;
class GameWorldGame;
class EventManager;
class PostMsg;

/// Basisklasse für alle Spielobjekte
class GameObject
{
public:
    GameObject();
    GameObject(SerializedGameData& sgd, unsigned obj_id);
    GameObject(const GameObject& go);
    virtual ~GameObject();
    GameObject& operator=(const GameObject&) = delete;

    /// zerstört das Objekt.
    virtual void Destroy() = 0;

    /// Benachrichtigen, wenn neuer GF erreicht wurde.
    virtual void HandleEvent(unsigned /*id*/) {}

    /// Return the unique ID of an object. Always non-zero!
    unsigned GetObjId() const { return objId; }

    /// Serialisierungsfunktion.
    virtual void Serialize(SerializedGameData& sgd) const = 0;
    /// Liefert den GOT (siehe oben)
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
    static void AttachWorld(GameWorldGame* gameWorld);
    /// Remove the world from all game objects
    static void DetachWorld(GameWorldGame* gameWorld);
    /// Return the number of objects alive
    static unsigned GetNumObjs() { return objCounter_; }
    /// Gibt Obj-ID-Counter zurück
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
    /// Zugriff auf übrige Spielwelt
    static GameWorldGame* gwg;

private:
    static unsigned objIdCounter_; /// Objekt-ID-Counter (number of objects created)
    static unsigned objCounter_;   /// Objekt-Counter (number of objects alive)
};

/// Calls destroy on a GameObject and then deletes it setting the ptr to nullptr
template<typename T>
void destroyAndDelete(T*& obj)
{
    obj->Destroy();
    deletePtr(obj);
}
