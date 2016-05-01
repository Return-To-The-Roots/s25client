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

#ifndef GameEvent_h__
#define GameEvent_h__

#include "GameObject.h"

class GameEvent: public GameObject
{
public:
    /// Object that will handle this event
    GameObject* const obj;
    /// GF at which this event was added
    const unsigned startGF;
    /// Number of GF till event will be executed
    const unsigned length;
    /// ID of the event (meaning dependent on object)
    const unsigned id;

    GameEvent(GameObject* const obj, const unsigned startGF, const unsigned length, const unsigned id)
        : obj(obj), startGF(startGF), length(length), id(id)
    {
        RTTR_Assert(length > 0); // Events cannot be executed in the same GF as they are added
        RTTR_Assert(obj); // Events without an object are pointless
    }

    GameEvent(SerializedGameData& sgd, const unsigned obj_id);
    void Serialize(SerializedGameData& sgd) const override;

    void Destroy() override{}

    GO_Type GetGOT() const override { return GOT_EVENT; }
    /// Return GF at which this event will be executed
    unsigned GetTargetGF() const { return startGF + length; }
};

#endif // GameEvent_h__
