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
    constexpr InventorySetting(const EInventorySetting setting) noexcept : state(makeBitField(setting)) {}
    constexpr explicit InventorySetting(uint8_t state) noexcept : state(state) { makeValid(); }
    constexpr bool isSet(EInventorySetting setting) const noexcept;
    constexpr InventorySetting toggle(EInventorySetting setting) noexcept;
    constexpr void makeValid() noexcept;
    constexpr explicit operator uint8_t() const noexcept { return state; }
    friend constexpr bool operator==(const InventorySetting& lhs, const InventorySetting& rhs) noexcept;

private:
    static constexpr uint8_t makeBitField(EInventorySetting setting) noexcept;
    // Current state as a bitfield!
    uint8_t state = 0;
};

//////////////////////////////////////////////////////////////////////////
// Implementation
//////////////////////////////////////////////////////////////////////////

constexpr bool InventorySetting::isSet(const EInventorySetting setting) const noexcept
{
    return (state & makeBitField(setting)) != 0;
}

constexpr InventorySetting InventorySetting::toggle(const EInventorySetting setting) noexcept
{
    state ^= makeBitField(setting);
    // If we changed collect, then allow only collect to be set
    // Else clear collect (Collect with anything else makes no sense)
    if(setting == EInventorySetting::Collect)
        state &= makeBitField(EInventorySetting::Collect);
    else
        state &= ~makeBitField(EInventorySetting::Collect);
    return *this;
}

constexpr uint8_t InventorySetting::makeBitField(const EInventorySetting setting) noexcept
{
    return static_cast<uint8_t>(1 << static_cast<unsigned>(setting));
}

constexpr void InventorySetting::makeValid() noexcept
{
    switch(state)
    {
        case makeBitField(EInventorySetting::Stop):
        case makeBitField(EInventorySetting::Send):
        case makeBitField(EInventorySetting::Collect):
        case static_cast<uint8_t>(makeBitField(EInventorySetting::Stop) | makeBitField(EInventorySetting::Send)): break;
        default: state = 0; break;
    }
}

constexpr bool operator!=(const InventorySetting& lhs, const InventorySetting& rhs) noexcept
{
    return !(lhs == rhs);
}

constexpr bool operator==(const InventorySetting& lhs, const InventorySetting& rhs) noexcept
{
    return lhs.state == rhs.state;
}
