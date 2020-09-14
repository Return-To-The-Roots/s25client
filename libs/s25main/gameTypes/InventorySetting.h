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

#pragma once

#include <array>

/// Setting for each item in a warehouses inventory
enum class EInventorySetting : unsigned
{
    STOP = 0,
    SEND = 1,
    COLLECT = 2
};

struct InventorySetting
{
    InventorySetting() : state(0) {}
    InventorySetting(const EInventorySetting setting) : state(MakeBitField(setting)) {}
    explicit InventorySetting(unsigned char state) : state(state) { MakeValid(); }
    inline bool IsSet(EInventorySetting setting) const;
    inline InventorySetting Toggle(EInventorySetting setting);
    inline void MakeValid();
    unsigned char ToUnsignedChar() const { return state; }
    friend bool operator==(const InventorySetting& lhs, const InventorySetting& rhs);

private:
    inline static unsigned char MakeBitField(EInventorySetting setting);
    // Current state as a bitfield!
    unsigned char state;
};

//////////////////////////////////////////////////////////////////////////
// Implementation
//////////////////////////////////////////////////////////////////////////

bool InventorySetting::IsSet(const EInventorySetting setting) const
{
    return (state & MakeBitField(setting)) != 0;
}

InventorySetting InventorySetting::Toggle(const EInventorySetting setting)
{
    state ^= MakeBitField(setting);
    // If we changed collect, then allow only collect to be set
    // Else clear collect (Collect with anything else makes no sense)
    if(setting == EInventorySetting::COLLECT)
        state &= MakeBitField(EInventorySetting::COLLECT);
    else
        state &= ~MakeBitField(EInventorySetting::COLLECT);
    return *this;
}

unsigned char InventorySetting::MakeBitField(const EInventorySetting setting)
{
    return static_cast<unsigned char>(1 << static_cast<unsigned>(setting));
}

void InventorySetting::MakeValid()
{
    static const std::array<unsigned char, 4> validStates = {
      {MakeBitField(EInventorySetting::STOP), MakeBitField(EInventorySetting::SEND), MakeBitField(EInventorySetting::COLLECT),
       static_cast<unsigned char>(MakeBitField(EInventorySetting::STOP) | MakeBitField(EInventorySetting::SEND))}};
    for(unsigned char validState : validStates)
    {
        if(state == validState)
            return;
    }
    state = 0;
}

inline bool operator!=(const InventorySetting& lhs, const InventorySetting& rhs)
{
    return !(lhs == rhs);
}

inline bool operator==(const InventorySetting& lhs, const InventorySetting& rhs)
{
    return lhs.state == rhs.state;
}
