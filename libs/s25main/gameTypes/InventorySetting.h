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

#include <cstdint>

/// Setting for each item in a warehouses inventory
enum class EInventorySetting : unsigned
{
    STOP,
    SEND,
    COLLECT
};

struct InventorySetting
{
    constexpr InventorySetting() = default;
    constexpr InventorySetting(const EInventorySetting setting) : state(MakeBitField(setting)) {}
    constexpr explicit InventorySetting(uint8_t state) : state(state) { MakeValid(); }
    constexpr bool IsSet(EInventorySetting setting) const;
    constexpr InventorySetting Toggle(EInventorySetting setting);
    constexpr void MakeValid();
    constexpr explicit operator uint8_t() const { return state; }
    friend constexpr bool operator==(const InventorySetting& lhs, const InventorySetting& rhs);

private:
    static constexpr uint8_t MakeBitField(EInventorySetting setting);
    // Current state as a bitfield!
    uint8_t state = 0;
};

//////////////////////////////////////////////////////////////////////////
// Implementation
//////////////////////////////////////////////////////////////////////////

constexpr bool InventorySetting::IsSet(const EInventorySetting setting) const
{
    return (state & MakeBitField(setting)) != 0;
}

constexpr InventorySetting InventorySetting::Toggle(const EInventorySetting setting)
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

constexpr uint8_t InventorySetting::MakeBitField(const EInventorySetting setting)
{
    return static_cast<uint8_t>(1 << static_cast<unsigned>(setting));
}

constexpr void InventorySetting::MakeValid()
{
    switch(state)
    {
        case MakeBitField(EInventorySetting::STOP):
        case MakeBitField(EInventorySetting::SEND):
        case MakeBitField(EInventorySetting::COLLECT):
        case static_cast<uint8_t>(MakeBitField(EInventorySetting::STOP) | MakeBitField(EInventorySetting::SEND)): break;
        default: state = 0; break;
    }
}

constexpr bool operator!=(const InventorySetting& lhs, const InventorySetting& rhs)
{
    return !(lhs == rhs);
}

constexpr bool operator==(const InventorySetting& lhs, const InventorySetting& rhs)
{
    return lhs.state == rhs.state;
}
