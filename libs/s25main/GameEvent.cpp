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
    : instanceId(sgd.AddEvent(instanceId, this)), obj(sgd.PopObject<GameObject>(GOT_UNKNOWN)),
      startGF(sgd.PopUnsignedInt()), length(sgd.PopUnsignedInt()), id(sgd.PopUnsignedInt())
{
    RTTR_Assert(obj);
}

void GameEvent::Serialize(SerializedGameData& sgd) const
{
    sgd.PushObject(obj, false);
    sgd.PushUnsignedInt(startGF);
    sgd.PushUnsignedInt(length);
    sgd.PushUnsignedInt(id);
}
