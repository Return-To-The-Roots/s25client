// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

class GameObject;
class SerializedGameData;

class GameEvent
{
    const unsigned instanceId; /// unique ID
public:
    /// Object that will handle this event
    GameObject* obj;
    /// GF at which this event was added
    unsigned startGF;
    /// Number of GF till event will be executed
    unsigned length;
    /// ID of the event (meaning dependent on object)
    unsigned id;

    GameEvent(unsigned instanceId, GameObject* obj, unsigned startGF, unsigned length, unsigned id);
    GameEvent(SerializedGameData& sgd, unsigned instanceId);
    void Serialize(SerializedGameData& sgd) const;

    /// Return GF at which this event will be executed
    unsigned GetTargetGF() const { return startGF + length; }
    unsigned GetInstanceId() const { return instanceId; }
};
