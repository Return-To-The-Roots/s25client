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

#include "commonDefines.h" // IWYU pragma: keep
#include "helpers/mathFuncs.h"
#include <algorithm>
#include <cstdlib>

namespace helpers {
int gcd(int a, int b)
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

unsigned roundedDiv(unsigned dividend, unsigned divisor)
{
    RTTR_Assert(divisor > 0);
    RTTR_Assert(dividend + (divisor / 2) >= dividend); // Overflow check
    // Standard way for emulation mathematical rounding: floor(divident / divisor + 0.5)
    // Which is the same as: floor((divident + 0.5 * divisor) / divisor) == floor((divident + divisor / 2) / divisor)
    return (dividend + (divisor / 2)) / divisor;
}
} // namespace helpers
