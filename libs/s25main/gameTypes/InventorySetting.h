// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdint>

/// Setting for each item in a warehouses inventory
enum class EInventorySetting : unsigned
{
    Stop,
    Send,
    Collect
};

struct InventorySetting
{
    constexpr InventorySetting() noexcept = default;
    constexpr InventorySetting(const EInventorySetting setting) noexcept : state(MakeBitField(setting)) {}
    constexpr explicit InventorySetting(uint8_t state) noexcept : state(state) { MakeValid(); }
    constexpr bool IsSet(EInventorySetting setting) const noexcept;
    constexpr InventorySetting Toggle(EInventorySetting setting) noexcept;
    constexpr void MakeValid() noexcept;
    constexpr explicit operator uint8_t() const noexcept { return state; }
    friend constexpr bool operator==(const InventorySetting& lhs, const InventorySetting& rhs);

private:
    static constexpr uint8_t MakeBitField(EInventorySetting setting) noexcept;
    // Current state as a bitfield!
    uint8_t state = 0;
};

//////////////////////////////////////////////////////////////////////////
// Implementation
//////////////////////////////////////////////////////////////////////////

constexpr bool InventorySetting::IsSet(const EInventorySetting setting) const noexcept
{
    return (state & MakeBitField(setting)) != 0;
}

constexpr InventorySetting InventorySetting::Toggle(const EInventorySetting setting) noexcept
{
    state ^= MakeBitField(setting);
    // If we changed collect, then allow only collect to be set
    // Else clear collect (Collect with anything else makes no sense)
    if(setting == EInventorySetting::Collect)
        state &= MakeBitField(EInventorySetting::Collect);
    else
        state &= ~MakeBitField(EInventorySetting::Collect);
    return *this;
}

constexpr uint8_t InventorySetting::MakeBitField(const EInventorySetting setting) noexcept
{
    return static_cast<uint8_t>(1 << static_cast<unsigned>(setting));
}

constexpr void InventorySetting::MakeValid() noexcept
{
    switch(state)
    {
        case MakeBitField(EInventorySetting::Stop):
        case MakeBitField(EInventorySetting::Send):
        case MakeBitField(EInventorySetting::Collect):
        case static_cast<uint8_t>(MakeBitField(EInventorySetting::Stop) | MakeBitField(EInventorySetting::Send)): break;
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
