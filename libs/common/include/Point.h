// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "helpers/mathFuncs.h"
#include <algorithm>
#include <cstdint>
#include <limits>
#include <type_traits>

namespace detail {

template<class T>
struct type_identity
{
    using type = T;
};

/// Convert the type T to a signed type if the condition is true (safe for float types)
template<bool cond, typename T>
using make_signed_if_t =
  typename std::conditional_t<cond && !std::is_signed<T>::value, std::make_signed<T>, type_identity<T>>::type;

// clang-format off

/// Creates a mixed type out of types T and U which is
/// the larger type of T & U AND signed iff either is signed
/// Will be a floating point type if either T or U is floating point
template<typename T, typename U>
using mixed_type_t =
  make_signed_if_t<
    std::is_signed<T>::value || std::is_signed<U>::value,
    typename std::conditional_t<
        std::is_floating_point<T>::value == std::is_floating_point<U>::value, // both are FP or not FP?
        std::conditional<(sizeof(T) > sizeof(U)), T, U>, // Take the larger type
        std::conditional<std::is_floating_point<T>::value, T, U> // Take the floating point type
    >::type
  >;

template<typename T, typename U>
using IsNonLossyOp = std::integral_constant<bool,
    // We can do T = T <op> U (except overflow) if:
    std::is_floating_point<T>::value || std::is_signed<T>::value || std::is_unsigned<U>::value
>;

// clang-format on

template<typename T, typename U>
using require_nonLossyOp = std::enable_if_t<IsNonLossyOp<T, U>::value>;
template<typename T>
using require_arithmetic = std::enable_if_t<std::is_arithmetic<T>::value>;

} // namespace detail

/// Type for describing a 2D value (position, size, offset...)
/// Note: Combining a signed with an unsigned point will result in a signed type!
/// Allowed operations:
/// pt <op> pt, for <op> in +,-,*,/,+=,-=,*=,/=
/// pt * x, x * pt, pt / x for arithmetic types x
/// pt *= y, pt /= y for float types y and if either all are signed or y is unsigned
template<typename T>
struct Point //-V690
{
    using ElementType = T;
    static_assert(std::is_arithmetic<ElementType>::value, "Requires an arithmetic type");

    static constexpr auto MinElementValue = std::numeric_limits<T>::min();
    static constexpr auto MaxElementValue = std::numeric_limits<T>::max();

    struct Truncate_t
    {
        constexpr explicit Truncate_t() = default;
    };

    static constexpr Truncate_t Truncate{};

    T x, y;
    constexpr Point() noexcept : x(getInvalidValue()), y(getInvalidValue()) {}
    constexpr Point(const T x, const T y) noexcept : x(x), y(y) {}
    template<typename U, std::enable_if_t<!(std::is_integral<T>::value && std::is_floating_point<U>::value), int> = 0>
    constexpr explicit Point(const Point<U>& pt) noexcept : x(static_cast<T>(pt.x)), y(static_cast<T>(pt.y))
    {}
    /// Convert floating-point to integer by truncating
    template<typename U, std::enable_if_t<std::is_integral<T>::value && std::is_floating_point<U>::value, int> = 0>
    constexpr explicit Point(Truncate_t, const Point<U>& pt) noexcept : x(static_cast<T>(pt.x)), y(static_cast<T>(pt.y))
    {}
    /// Convert floating-point to integer with rounding (default behavior)
    template<typename U, std::enable_if_t<std::is_integral<T>::value && std::is_floating_point<U>::value, int> = 0>
    constexpr explicit Point(const Point<U>& pt) noexcept : x(helpers::iround<T>(pt.x)), y(helpers::iround<T>(pt.y))
    {}
    constexpr Point(const Point&) = default;
    constexpr Point& operator=(const Point&) = default;

    static constexpr Point Invalid() noexcept { return Point(); }
    /// Create a new point with all coordinates set to value
    static constexpr Point all(const T value) noexcept { return Point(value, value); }
    constexpr bool isValid() const noexcept { return *this != Invalid(); }

    /// Calculate the euclidean distance to the origin
    constexpr auto distance() const noexcept { return std::hypot(x, y); }

    /// Calculate the euclidean distance to another point
    template<typename U, std::enable_if_t<!(std::is_unsigned<T>::value && std::is_unsigned<U>::value), int> = 0>
    constexpr auto distance(const Point<U>& pt) const noexcept
    {
        using Res = ::detail::mixed_type_t<T, U>;
        return std::hypot(static_cast<Res>(x) - static_cast<Res>(pt.x), static_cast<Res>(y) - static_cast<Res>(pt.y));
    }

    /// Calculate the euclidean distance to another point
    template<typename U, std::enable_if_t<std::is_unsigned<T>::value && std::is_unsigned<U>::value, int> = 0>
    constexpr auto distance(const Point<U>& pt) const noexcept
    {
        using Res = ::detail::mixed_type_t<T, U>;
        const auto x1 = static_cast<Res>(x), x2 = static_cast<Res>(pt.x);
        const auto y1 = static_cast<Res>(y), y2 = static_cast<Res>(pt.y);
        // Calculate difference of usigned values without overflow
        return std::hypot((x1 > x2 ? x1 - x2 : x2 - x1), (y1 > y2 ? y1 - y2 : y2 - y1));
    }

    constexpr bool operator==(const Point& second) const noexcept;
    constexpr bool operator!=(const Point& second) const noexcept;

private:
    static constexpr T getInvalidValue() noexcept;
};

/// Type for describing a position/offset etc. (signed type)
using Position = Point<int>;
/// Type for describing an extent/size etc. (unsigned type)
using Extent = Point<unsigned>;
using PointF = Point<float>;
//-V:all:810

//////////////////////////////////////////////////////////////////////////

template<typename T>
constexpr T Point<T>::getInvalidValue() noexcept
{
    return std::numeric_limits<T>::has_quiet_NaN ? std::numeric_limits<T>::quiet_NaN() : std::numeric_limits<T>::max();
}

template<typename T>
constexpr bool Point<T>::operator==(const Point<T>& second) const noexcept
{
    return (x == second.x && y == second.y);
}

template<typename T>
constexpr bool Point<T>::operator!=(const Point<T>& second) const noexcept
{
    return !(*this == second);
}

/// Compute the element wise minimum
template<typename T>
constexpr Point<T> elMin(const Point<T>& lhs, const Point<T>& rhs) noexcept
{
    return Point<T>(std::min(lhs.x, rhs.x), std::min(lhs.y, rhs.y));
}

/// Compute the element wise maximum
template<typename T>
constexpr Point<T> elMax(const Point<T>& lhs, const Point<T>& rhs) noexcept
{
    return Point<T>(std::max(lhs.x, rhs.x), std::max(lhs.y, rhs.y));
}

/// Compute pt.x * pt.y
/// The result type is T iff T is a floating point value, else a >=32 bit integer type with the same signednes as T
template<typename T>
constexpr auto prodOfComponents(const Point<T>& pt) noexcept
{
    // Let the compiler handle conversion to at least 32 bits keeping float types
    using op_type = decltype(T{} * uint32_t{});
    using ResultType = ::detail::make_signed_if_t<std::is_signed<T>::value, op_type>;
    return static_cast<ResultType>(pt.x * pt.y);
}

//////////////////////////////////////////////////////////////////////////
// Math ops: add/subtract/negate of Point(s). multiply/divide of points and or scalars

/// Unary negate
template<typename T>
constexpr auto operator-(const Point<T>& pt) noexcept
{
    using Res = ::detail::make_signed_if_t<true, T>;
    return Point<Res>(-static_cast<Res>(pt.x), -static_cast<Res>(pt.y));
}

/// General arithmetic
#define RTTR_GEN_ARITH(op)                                                          \
    template<typename T>                                                            \
    constexpr auto operator op(const Point<T>& lhs, const Point<T>& rhs) noexcept   \
    {                                                                               \
        return Point<T>(lhs.x op rhs.x, lhs.y op rhs.y);                            \
    }                                                                               \
    template<typename T, typename U>                                                \
    constexpr auto operator op(const Point<T>& lhs, const Point<U>& rhs) noexcept   \
    {                                                                               \
        using Res = ::detail::mixed_type_t<T, U>;                                   \
        return Point<Res>(lhs) op Point<Res>(rhs);                                  \
    }                                                                               \
                                                                                    \
    template<typename T>                                                            \
    constexpr Point<T>& operator op##=(Point<T>& lhs, const Point<T>& rhs) noexcept \
    {                                                                               \
        lhs.x op## = rhs.x;                                                         \
        lhs.y op## = rhs.y;                                                         \
        return lhs;                                                                 \
    }

RTTR_GEN_ARITH(+)
RTTR_GEN_ARITH(-)
RTTR_GEN_ARITH(*)
RTTR_GEN_ARITH(/)
#undef RTTR_GEN_ARITH

// Scaling operators

template<typename T, typename U, class = detail::require_nonLossyOp<T, U>>
constexpr Point<T>& operator*=(Point<T>& lhs, U factor) noexcept
{
    return lhs *= Point<T>::all(factor);
}

template<typename T, typename U, class = detail::require_arithmetic<U>>
constexpr auto operator*(const Point<T>& pt, const U factor) noexcept
{
    return pt * Point<U>::all(factor);
}

template<typename T, typename U, class = detail::require_arithmetic<T>>
constexpr auto operator*(const T left, const Point<U>& factor) noexcept
{
    return factor * left;
}

template<typename T, typename U, class = detail::require_nonLossyOp<T, U>>
constexpr Point<T>& operator/=(Point<T>& lhs, U div) noexcept
{
    return lhs /= Point<T>::all(div);
}

template<typename T, typename U, class = detail::require_arithmetic<U>>
constexpr auto operator/(const Point<T>& lhs, const U div) noexcept
{
    return lhs / Point<U>::all(div);
}
