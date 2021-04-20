// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "helpers/mathFuncs.h"
#include "RTTR_Assert.h"
#include <cstdlib>
#include <utility>

namespace helpers {
int gcd(int a, int b) noexcept
{
    a = std::abs(a);
    b = std::abs(b);
    using std::swap;
    if(a < b)
        swap(a, b);

    while(b > 0)
    {
        int remainder = a % b;
        a = b;
        b = remainder;
    }
    return a;
}

unsigned roundedDiv(unsigned dividend, unsigned divisor) noexcept
{
    RTTR_Assert(divisor > 0);
    RTTR_Assert(dividend + (divisor / 2) >= dividend); // Overflow check
    // Standard way for emulation mathematical rounding: floor(divident / divisor + 0.5)
    // Which is the same as: floor((divident + 0.5 * divisor) / divisor) == floor((divident + divisor / 2) / divisor)
    return (dividend + (divisor / 2)) / divisor;
}
} // namespace helpers
