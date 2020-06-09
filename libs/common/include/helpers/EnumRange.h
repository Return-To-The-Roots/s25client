// Copyright (c) 2015 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef RttrEnumRange_h__
#define RttrEnumRange_h__

#include "MaxEnumValue.h"
#include <boost/config.hpp>
#include <type_traits>

namespace helpers {
/// Can be used in range-based for loops to iterate over each enum value
/// Requires: T must be an enum with MaxEnumValue trait specialized
///           T must be simple: Start at zero, no gaps
template<class T>
struct EnumRange
{
    static_assert(std::is_enum<T>::value, "Must be an enum!");
    class iterator
    {
        unsigned value;

    public:
        explicit BOOST_FORCEINLINE iterator(unsigned value) : value(value) {}
        BOOST_FORCEINLINE T operator*() const { return static_cast<T>(value); }
        BOOST_FORCEINLINE void operator++() { ++value; }
        BOOST_FORCEINLINE bool operator!=(iterator rhs) const { return value != rhs.value; }
    };

    BOOST_FORCEINLINE iterator begin() const { return iterator(0); }
    BOOST_FORCEINLINE iterator end() const { return iterator(MaxEnumValue_v<T> + 1u); }
};
} // namespace helpers

#endif // RttrEnumRange_h__
