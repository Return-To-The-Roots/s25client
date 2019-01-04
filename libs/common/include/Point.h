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

#ifndef Point_h__
#define Point_h__

#include <algorithm>
#include <limits>
#include <stdint.h>
#include <type_traits>

/// Type for describing a 2D value (position, size, offset...)
/// Note: Combining a signed with an unsigned point will result in a signed type!
template<typename T>
struct Point //-V690
{
    typedef T ElementType;

    T x, y;
    constexpr Point(const T x, const T y) : x(x), y(y) {}
    constexpr Point(const Point& other = Invalid()) : x(other.x), y(other.y) {}
    template<typename U>
    constexpr explicit Point(const Point<U>& pt) : x(static_cast<T>(pt.x)), y(static_cast<T>(pt.y))
    {}

    static constexpr Point Invalid();
    /// Create a new point with all coordinates set to value
    static constexpr Point all(const T& value);
    constexpr bool isValid() const;

    bool operator==(const Point& second) const;
    bool operator!=(const Point& second) const;
};

/// Type for describing a position/offset etc. (signed type)
typedef Point<int> Position;
/// Type for describing an extent/size etc. (unsigned type)
typedef Point<unsigned> Extent;
//-V:all:810

//////////////////////////////////////////////////////////////////////////

template<typename T>
inline constexpr Point<T> Point<T>::Invalid()
{
    return Point::all(std::numeric_limits<T>::has_quiet_NaN ? std::numeric_limits<T>::quiet_NaN() : std::numeric_limits<T>::max());
}

template<typename T>
inline constexpr Point<T> Point<T>::all(const T& val)
{
    return Point(val, val);
}

template<typename T>
inline constexpr bool Point<T>::isValid() const
{
    return *this != Invalid();
}

template<typename T>
inline bool Point<T>::operator==(const Point<T>& second) const
{
    return (x == second.x && y == second.y);
}

template<typename T>
inline bool Point<T>::operator!=(const Point<T>& second) const
{
    return !(*this == second);
}

//////////////////////////////////////////////////////////////////////////
// operations on points and points and scalars
//////////////////////////////////////////////////////////////////////////

/// Compute the element wise minimum
template<typename T>
inline Point<T> elMin(const Point<T>& lhs, const Point<T>& rhs)
{
    using std::min;
    return Point<T>(min(lhs.x, rhs.x), min(lhs.y, rhs.y));
}

/// Compute the element wise maximum
template<typename T>
inline Point<T> elMax(const Point<T>& lhs, const Point<T>& rhs)
{
    using std::max;
    return Point<T>(max(lhs.x, rhs.x), max(lhs.y, rhs.y));
}

template<typename T, bool T_isFloat = std::is_floating_point<T>::value>
struct PointProductType
{
    typedef T type;
};

template<typename T>
struct PointProductType<T, false>
{
    static constexpr bool is64Bit = sizeof(T) > 4u;
    using type32Bit = std::conditional_t<std::is_signed<T>::value, int32_t, uint32_t>;
    typedef std::conditional_t<is64Bit, T, type32Bit> type;
};

/// Compute pt.x * pt.y
/// The result type is T iff T is a floating point value, else a 32 bit integer type with the same signednes as T
template<typename T>
inline typename PointProductType<T>::type prodOfComponents(const Point<T>& pt)
{
    return pt.x * pt.y;
}

//////////////////////////////////////////////////////////////////////////
// Math ops: add/subtract/negate of Point(s). multiply/divide of points and or scalars

namespace detail {
template<typename T>
struct TryMakeSigned
{
    typedef typename std::conditional<std::is_integral<T>::value, std::make_signed<T>, std::common_type<T>>::type::type type;
};
/// Creates a mixed type out of types T and U which is
/// the common type of T & U AND signed iff either is signed
/// fails for non-numeric types with SFINAE
template<typename T, typename U, bool T_areNumeric = std::is_arithmetic<T>::value&& std::is_arithmetic<U>::value>
struct MixedType;

template<typename T, typename U>
struct MixedType<T, U, true>
{
    typedef typename std::common_type<T, U>::type Common;
    // Convert to signed iff least one value is signed
    typedef
      typename std::conditional<std::is_signed<T>::value || std::is_signed<U>::value, typename TryMakeSigned<Common>::type, Common>::type
        type;
};
} // namespace detail

/// Unary negate
template<typename T>
inline Point<typename std::make_signed<T>::type> operator-(const Point<T>& pt)
{
    typedef typename std::make_signed<T>::type Res;
    return Point<Res>(-static_cast<Res>(pt.x), -static_cast<Res>(pt.y));
}

/// Add and subtract operations
template<typename T>
inline Point<T>& operator+=(Point<T>& lhs, const Point<T>& right)
{
    lhs.x += right.x;
    lhs.y += right.y;
    return lhs;
}

template<typename T, typename U>
inline Point<typename detail::MixedType<T, U>::type> operator+(const Point<T>& lhs, const Point<U>& rhs)
{
    typedef typename detail::MixedType<T, U>::type Res;
    return Point<Res>(lhs.x + rhs.x, lhs.y + rhs.y);
}

template<typename T>
inline Point<T>& operator-=(Point<T>& lhs, const Point<T>& right)
{
    lhs.x -= right.x;
    lhs.y -= right.y;
    return lhs;
}

template<typename T, typename U>
inline Point<typename detail::MixedType<T, U>::type> operator-(const Point<T>& lhs, const Point<U>& rhs)
{
    typedef typename detail::MixedType<T, U>::type Res;
    return Point<Res>(lhs.x - rhs.x, lhs.y - rhs.y);
}

//////////////////////////////////////////////////////////////////////////
// Multiply/divide

template<typename T, typename U>
inline Point<typename detail::MixedType<T, U>::type> operator*(const Point<T>& pt, const U factor)
{
    return Point<typename detail::MixedType<T, U>::type>(pt.x * factor, pt.y * factor);
}

template<typename T, typename U>
inline Point<typename detail::MixedType<T, U>::type> operator*(const T left, const Point<U>& factor)
{
    return factor * left;
}

template<typename T, typename U>
inline Point<typename detail::MixedType<T, U>::type> operator*(const Point<T>& lhs, const Point<U>& rhs)
{
    return Point<typename detail::MixedType<T, U>::type>(lhs.x * rhs.x, lhs.y * rhs.y);
}

template<typename T>
inline Point<T>& operator*=(Point<T>& lhs, const Point<T>& right)
{
    lhs.x *= right.x;
    lhs.y *= right.y;
    return lhs;
}

template<typename T, typename U>
inline Point<typename detail::MixedType<T, U>::type> operator/(const Point<T>& pt, const U div)
{
    return Point<typename detail::MixedType<T, U>::type>(pt.x / div, pt.y / div);
}

template<typename T, typename U>
inline Point<typename detail::MixedType<T, U>::type> operator/(const T left, const Point<U>& div)
{
    return Point<typename detail::MixedType<T, U>::type>(left / div.x, left / div.y);
}

template<typename T, typename U>
inline Point<typename detail::MixedType<T, U>::type> operator/(const Point<T>& lhs, const Point<U>& rhs)
{
    return Point<typename detail::MixedType<T, U>::type>(lhs.x / rhs.x, lhs.y / rhs.y);
}

template<typename T>
inline Point<T>& operator/=(Point<T>& lhs, const Point<T>& right)
{
    lhs.x /= right.x;
    lhs.y /= right.y;
    return lhs;
}

#endif // Point_h__
