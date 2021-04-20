// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdint>

namespace helpers {
/// compute the next highest power of 2 of 32-bit v
/// Note: 0 will be rounded to 1 and maximum accepted value is 2^31 as maximum uint32_t value is 2^32-1
inline uint32_t roundToNextPowerOfTwo(uint32_t v)
{
    if(!v)
        return 1;
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
    v++;
    return v;
}
} // namespace helpers
