// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <type_traits>

namespace helpers {

/// Returns the greatest common divisor of a and b
/// That is the greatest number x with a % x == b % x == 0
int gcd(int a, int b) noexcept;
/// Returns the result of "dividend / divisor" rounded to the nearest integer value
unsigned roundedDiv(unsigned dividend, unsigned divisor) noexcept;
constexpr unsigned divCeil(unsigned dividend, unsigned divisor) noexcept
{
    return (dividend + divisor - 1)
           / divisor; // Standard trick using truncating division for smalish values (no overflow)
}
/// Clamp the value into [min, max]
template<typename T>
constexpr T clamp(T val, T min, T max) noexcept
{
    if(val <= min)
        return min;
    else if(val >= max)
        return max;
    else
        return val;
}
template<typename T, typename U>
constexpr U clamp(T val, U min, U max) noexcept
{
    using Common = std::common_type_t<T, U>;
    if(std::is_signed<T>::value && !std::is_signed<U>::value)
    {
        // min/max is unsigned -> No negative values possible
        if(val < 0)
            return min;
    } else if(!std::is_signed<T>::value && std::is_signed<U>::value)
    {
        // min/max is signed
        if(max < 0)
            return max;
        if(min < 0)
            min = 0;
    }
    // Here all values are positive or have the same signedness
    return static_cast<U>(clamp(static_cast<Common>(val), static_cast<Common>(min), static_cast<Common>(max)));
}
// Linear interpolation between [startVal, endVal]. Difference between those 2 and elapsedTime should be smallish
template<typename T, typename U, typename V>
constexpr T interpolate(const T startVal, const T endVal, const U elapsedTime, const V duration) noexcept
{
    // Allow only 1 order of magnitude between the time units (i.e. seconds and milliseconds)
    static_assert(U(1) / V(1) <= 1000 && V(1) / U(1) <= 1000,
                  "Time units are to different so result would likely overflow");
    if(elapsedTime < U(0))
        return startVal;
    if(elapsedTime > duration)
        return endVal;
    if(startVal <= endVal)
        return static_cast<T>(startVal + ((endVal - startVal) * elapsedTime) / duration);
    else // Special case for unsigned values
        return static_cast<T>(startVal - ((startVal - endVal) * elapsedTime) / duration);
}
} // namespace helpers
