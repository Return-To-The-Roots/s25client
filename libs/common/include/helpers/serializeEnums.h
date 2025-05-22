// Copyright (C) 2005 - 2025 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "RTTR_Assert.h"
#include "helpers/MaxEnumValue.h"
#include "s25util/Serializer.h"
#include <limits>
#include <stdexcept>

namespace helpers {

std::range_error makeOutOfRange(unsigned value, unsigned maxValue);

template<typename T_SavedType, typename T>
void pushEnum(Serializer& ser, const T val)
{
    static_assert(std::is_same_v<T_SavedType, std::underlying_type_t<T>>,
                  "Wrong saved type"); // Required for the popEnum method
    constexpr auto maxValue = helpers::MaxEnumValue_v<T>;
    static_assert(std::numeric_limits<T_SavedType>::max() >= maxValue, "SavedType cannot hold all enum values");

    const auto iVal = static_cast<T_SavedType>(val);
    RTTR_Assert(iVal <= maxValue);
    ser.Push(iVal);
}

template<typename T>
T popEnum(Serializer& ser)
{
    const auto value = ser.Pop<std::underlying_type_t<T>>();
    if(value > helpers::MaxEnumValue_v<T>)
        throw makeOutOfRange(value, helpers::MaxEnumValue_v<T>);
    return static_cast<T>(value);
}

} // namespace helpers
