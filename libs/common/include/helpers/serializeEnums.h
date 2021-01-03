// Copyright (c) 2021 - 2021 Settlers Freaks (sf-team at siedler25.org)
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
    static_assert(std::is_same<T_SavedType, std::underlying_type_t<T>>::value,
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
