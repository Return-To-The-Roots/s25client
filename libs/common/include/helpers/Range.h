// Copyright (c) 2020 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include <boost/config.hpp>
#include <type_traits>

namespace helpers {
/// Can be used in range-based for loops to iterate over each value in [0, endValue)
/// Requires: T must be an integral
template<typename T>
struct Range
{
    static_assert(std::is_integral<T>::value, "Must be an integral!");
    class iterator
    {
        T value;

    public:
        explicit BOOST_FORCEINLINE constexpr iterator(T value) : value(value) {}
        BOOST_FORCEINLINE constexpr T operator*() const noexcept { return value; }
        BOOST_FORCEINLINE void operator++() noexcept { ++value; }
        BOOST_FORCEINLINE constexpr bool operator!=(iterator rhs) const noexcept { return value != rhs.value; }
    };

    BOOST_FORCEINLINE constexpr iterator begin() const noexcept { return iterator(0); }
    BOOST_FORCEINLINE constexpr iterator end() const noexcept { return iterator(endValue); }

    const T endValue;
};

template<typename T>
constexpr Range<T> range(T endValue)
{
    return {endValue};
}

} // namespace helpers
