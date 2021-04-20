// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GameEvent.h"
#include "GameObject.h"
#include "SerializedGameData.h"

GameEvent::GameEvent(unsigned instanceId, GameObject* obj, unsigned startGF, unsigned length, unsigned id)
    : instanceId(instanceId), obj(obj), startGF(startGF), length(length), id(id)
{
    RTTR_Assert(length > 0); // Events cannot be executed in the same GF as they are added
    RTTR_Assert(obj);        // Events without an object are pointless
}

GameEvent::GameEvent(SerializedGameData& sgd, const unsigned instanceId)
    : instanceId(sgd.AddEvent(instanceId, this)), obj(sgd.PopObject<GameObject>()), startGF(sgd.PopUnsignedInt()),
      length(sgd.PopUnsignedInt()), id(sgd.PopUnsignedInt())
{
    RTTR_Assert(obj);
}

void GameEvent::Serialize(SerializedGameData& sgd) const
{
    sgd.PushObject(obj);
    sgd.PushUnsignedInt(startGF);
    sgd.PushUnsignedInt(length);
    sgd.PushUnsignedInt(id);
}
