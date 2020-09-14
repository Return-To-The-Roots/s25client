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

#include <algorithm>
#include <cstdint>
#include <limits>
#include <type_traits>

/// Type for describing a 2D value (position, size, offset...)
/// Note: Combining a signed with an unsigned point will result in a signed type!
template<typename T>
struct Point //-V690
{
    using ElementType = T;

    T x, y;
    constexpr Point() noexcept : x(getInvalidValue()), y(getInvalidValue()) {}
    constexpr Point(const T x, const T y) noexcept : x(x), y(y) {}
    constexpr Point(const Point&) = default;
    template<typename U>
    constexpr explicit Point(const Point<U>& pt) noexcept : x(static_cast<T>(pt.x)), y(static_cast<T>(pt.y))
    {}

    static constexpr Point Invalid() { return Point(); }
    /// Create a new point with all coordinates set to value
    static constexpr Point all(const T& value);
    constexpr bool isValid() const;

    constexpr bool operator==(const Point& second) const;
    constexpr bool operator!=(const Point& second) const;

private:
    static constexpr T getInvalidValue();
};

/// Type for describing a position/offset etc. (signed type)
using Position = Point<int>;
/// Type for describing an extent/size etc. (unsigned type)
using Extent = Point<unsigned>;
//-V:all:810

//////////////////////////////////////////////////////////////////////////

template<typename T>
constexpr T Point<T>::getInvalidValue()
{
    return std::numeric_limits<T>::has_quiet_NaN ? std::numeric_limits<T>::quiet_NaN() : std::numeric_limits<T>::max();
}

template<typename T>
constexpr Point<T> Point<T>::all(const T& val)
{
    return Point(val, val);
}

template<typename T>
constexpr bool Point<T>::isValid() const
{
    return *this != Invalid();
}

template<typename T>
constexpr bool Point<T>::operator==(const Point<T>& second) const
{
    return (x == second.x && y == second.y);
}

template<typename T>
constexpr bool Point<T>::operator!=(const Point<T>& second) const
{
    return !(*this == second);
}

//////////////////////////////////////////////////////////////////////////
// operations on points and points and scalars
//////////////////////////////////////////////////////////////////////////

/// Compute the element wise minimum
template<typename T>
constexpr Point<T> elMin(const Point<T>& lhs, const Point<T>& rhs)
{
    using std::min;
    return Point<T>(min(lhs.x, rhs.x), min(lhs.y, rhs.y));
}

/// Compute the element wise maximum
template<typename T>
constexpr Point<T> elMax(const Point<T>& lhs, const Point<T>& rhs)
{
    using std::max;
    return Point<T>(max(lhs.x, rhs.x), max(lhs.y, rhs.y));
}

template<typename T, bool T_isFloat = std::is_floating_point<T>::value>
struct PointProductType
{
    using type = T;
};

template<typename T>
struct PointProductType<T, false>
{
    static constexpr bool is64Bit = sizeof(T) > 4u;
    using type32Bit = std::conditional_t<std::is_signed<T>::value, int32_t, uint32_t>;
    using type = std::conditional_t<is64Bit, T, type32Bit>;
};

/// Compute pt.x * pt.y
/// The result type is T iff T is a floating point value, else a >=32 bit integer type with the same signednes as T
template<typename T>
constexpr typename PointProductType<T>::type prodOfComponents(const Point<T>& pt)
{
    return pt.x * pt.y;
}

//////////////////////////////////////////////////////////////////////////
// Math ops: add/subtract/negate of Point(s). multiply/divide of points and or scalars

namespace detail {
template<typename T>
using TryMakeSigned = std::conditional_t<std::is_integral<T>::value, std::make_signed<T>, std::common_type<T>>;
template<typename T>
using TryMakeSigned_t = typename TryMakeSigned<T>::type;

/// Creates a mixed type out of types T and U which is
/// the larger type of T & U AND signed iff either is signed
/// Will be a floating point type if either T or U is floating point
/// fails for non-numeric types with SFINAE
template<typename T, typename U, bool T_areNumeric = std::is_arithmetic<T>::value&& std::is_arithmetic<U>::value>
struct MixedType;

template<typename T, typename U>
struct MixedType<T, U, true>
{
    static constexpr bool isTBigger = sizeof(T) > sizeof(U);
    // If both are floating point or both are not
    using Common = std::conditional_t<std::is_floating_point<T>::value == std::is_floating_point<U>::value,
                                      std::conditional_t<isTBigger, T, U>,                       // Take the larger type
                                      std::conditional_t<std::is_floating_point<T>::value, T, U> // Take the floating point type
                                      >;
    // Convert to signed iff at least one value is signed
    using type = std::conditional_t<std::is_signed<T>::value || std::is_signed<U>::value, TryMakeSigned_t<Common>, Common>;
};
template<typename T, typename U>
using MixedType_t = typename MixedType<T, U>::type;

template<typename T, typename U>
struct IsNonLossyOp
{
    // We can do T <op> U (except overflow) if:
    static constexpr bool value = std::is_floating_point<T>::value || std::is_signed<T>::value || std::is_unsigned<U>::value;
};
template<typename T, typename U>
using require_nonLossyOp = std::enable_if_t<IsNonLossyOp<T, U>::value>;
template<typename T>
using require_arithmetic = std::enable_if_t<std::is_arithmetic<T>::value>;
} // namespace detail

/// Unary negate
template<typename T>
constexpr auto operator-(const Point<T>& pt)
{
    using Res = detail::TryMakeSigned_t<T>;
    return Point<Res>(-static_cast<Res>(pt.x), -static_cast<Res>(pt.y));
}

/// Add and subtract operations
template<typename T, typename U>
constexpr auto operator+(const Point<T>& lhs, const Point<U>& rhs) -> Point<detail::MixedType_t<T, U>>
{
    return Point<detail::MixedType_t<T, U>>(lhs.x + rhs.x, lhs.y + rhs.y);
}

template<typename T>
constexpr Point<T>& operator+=(Point<T>& lhs, const Point<T>& rhs)
{
    return lhs = lhs + rhs; // Single return assignment for MSVC2015
}

template<typename T, typename U>
constexpr auto operator-(const Point<T>& lhs, const Point<U>& rhs) -> Point<detail::MixedType_t<T, U>>
{
    return Point<detail::MixedType_t<T, U>>(lhs.x - rhs.x, lhs.y - rhs.y);
}

template<typename T>
constexpr Point<T>& operator-=(Point<T>& lhs, const Point<T>& rhs)
{
    return lhs = lhs - rhs;
}

//////////////////////////////////////////////////////////////////////////
// Multiply

template<typename T, typename U>
constexpr auto operator*(const Point<T>& lhs, const Point<U>& rhs)
{
    using Res = detail::MixedType_t<T, U>;
    return Point<Res>{static_cast<Res>(Res(lhs.x) * Res(rhs.x)), static_cast<Res>(Res(lhs.y) * Res(rhs.y))};
}

template<typename T>
constexpr Point<T>& operator*=(Point<T>& lhs, const Point<T>& rhs)
{
    return lhs = lhs * rhs;
}

template<typename T, typename U, class = detail::require_nonLossyOp<T, U>>
constexpr Point<T>& operator*=(Point<T>& lhs, U factor)
{
    return lhs *= Point<T>::all(factor);
}

template<typename T, typename U, class = detail::require_arithmetic<U>>
constexpr auto operator*(const Point<T>& pt, const U factor)
{
    return pt * Point<U>::all(factor);
}

template<typename T, typename U, class = detail::require_arithmetic<T>>
constexpr auto operator*(const T left, const Point<U>& factor)
{
    return factor * left;
}

//////////////////////////////////////////////////////////////////////////
// Divide

template<typename T, typename U>
constexpr auto operator/(const Point<T>& lhs, const Point<U>& rhs)
{
    using Res = detail::MixedType_t<T, U>;
    return Point<Res>{static_cast<Res>(Res(lhs.x) / Res(rhs.x)), static_cast<Res>(Res(lhs.y) / Res(rhs.y))};
}

template<typename T>
constexpr Point<T>& operator/=(Point<T>& lhs, const Point<T>& rhs)
{
    return lhs = lhs / rhs;
}

template<typename T, typename U, class = detail::require_nonLossyOp<T, U>>
constexpr Point<T>& operator/=(Point<T>& lhs, U div)
{
    return lhs /= Point<T>::all(div);
}

template<typename T, typename U, class = detail::require_arithmetic<U>>
constexpr auto operator/(const Point<T>& lhs, const U div)
{
    return lhs / Point<U>::all(div);
}

template<typename T, typename U, class = detail::require_arithmetic<U>>
constexpr auto operator/(const U rhs, const Point<T>& div)
{
    return Point<U>::all(rhs) / div;
}
