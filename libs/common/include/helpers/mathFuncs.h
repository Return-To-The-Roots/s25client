// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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
