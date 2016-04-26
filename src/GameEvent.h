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
    GameObject* const obj;
    const unsigned gf;
    const unsigned gf_length;
    const unsigned gf_next;
    const unsigned id;

    GameEvent(GameObject* const  obj, const unsigned gf, const unsigned gf_length, const unsigned id)
        : obj(obj), gf(gf), gf_length(gf_length), gf_next(gf + gf_length), id(id)
    {
        RTTR_Assert(obj); // Events without an object are pointless
    }

    GameEvent(SerializedGameData& sgd, const unsigned obj_id);

    void Destroy() override{}

    /// Serialisierungsfunktionen
protected: void Serialize_Event(SerializedGameData& sgd) const;
public: void Serialize(SerializedGameData& sgd) const override { Serialize_Event(sgd); }

    GO_Type GetGOT() const override { return GOT_EVENT; }

    // Vergleichsoperatur für chronologisches Einfügen nach Ziel-GF
    bool operator<= (const GameEvent& other) const { return gf_next <= other.gf_next; }
    bool operator< (const GameEvent& other) const { return gf_next < other.gf_next; }
};

#endif // GameEvent_h__
