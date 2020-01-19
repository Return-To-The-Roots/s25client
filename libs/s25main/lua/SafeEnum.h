// Copyright (c) 2015 - 2019 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef SafeEnum_h__
#define SafeEnum_h__

#include "helpers/MaxEnumValue.h"
#include "lua/LuaHelpers.h"

namespace lua {
/// Wrapper around an enum which checks the int passed on ctor to be in range of the enum
template<class T_Enum, unsigned maxValue = helpers::MaxEnumValue_v<T_Enum>>
struct SafeEnum
{
    static T_Enum cast(int value)
    {
        assertTrue(value >= 0 && static_cast<unsigned>(value) <= maxValue, "Enum value is out of range");
        return static_cast<T_Enum>(value);
    }
    T_Enum value_;
    SafeEnum(int value) : value_(cast(value)) {}
    constexpr operator T_Enum() const noexcept { return value_; };
};
} // namespace lua

#endif // SafeEnum_h__
