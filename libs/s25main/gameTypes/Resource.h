// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "helpers/MaxEnumValue.h"
#include <cstdint>

enum class ResourceType : uint8_t
{
    Nothing,
    Iron,
    Gold,
    Coal,
    Granite,
    Water,
    Fish
};
constexpr auto maxEnumValue(ResourceType)
{
    return ResourceType::Fish;
}

/// Holds a resource and its value.
/// Maximum number of resource types and amount is 15!
/// It makes sure that Type == Nothing => Amount == 0, but not vice versa!
/// Also: A value of 0 means Nothing|0
class Resource
{
    uint8_t value_;

public:
    constexpr Resource(ResourceType type, uint8_t amount);
    constexpr explicit Resource(uint8_t value = 0);
    constexpr explicit operator uint8_t() const { return value_; }
    constexpr uint8_t getValue() const { return value_; }
    constexpr ResourceType getType() const { return ResourceType(value_ >> 4); }
    constexpr uint8_t getAmount() const { return value_ & 0x0F; }
    constexpr void setType(ResourceType newType);
    constexpr void setAmount(uint8_t newAmount);
    /// True if we have a non-zero amount of the given resource. Always false for Nothing
    constexpr bool has(ResourceType type) const { return getAmount() > 0u && getType() == type; }
    constexpr bool operator==(Resource rhs) const { return value_ == rhs.value_; }
    constexpr bool operator!=(Resource rhs) const { return !(*this == rhs); }
};

constexpr Resource::Resource(ResourceType type, uint8_t amount)
    : value_((type == ResourceType::Nothing) ? 0 : (static_cast<uint8_t>(type) << 4) | (amount & 0x0F))
{}
constexpr Resource::Resource(uint8_t value) : value_(value)
{
    const uint8_t type = value >> 4u;
    if(type > helpers::MaxEnumValue_v<ResourceType> || type == static_cast<uint8_t>(ResourceType::Nothing))
        value_ = 0;
}
constexpr void Resource::setType(ResourceType newType)
{
    if(newType == ResourceType::Nothing)
        value_ = 0;
    else
        value_ = (static_cast<uint8_t>(newType) << 4) | getAmount();
}
constexpr void Resource::setAmount(uint8_t newAmount)
{
    if(getType() != ResourceType::Nothing)
        value_ = (value_ & 0xF0) | (newAmount & 0x0F);
}
