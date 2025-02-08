// Copyright (C) 2005 - 2025 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "helpers/random.h"
#include <random>

namespace AI {

std::minstd_rand& getRandomGenerator();

template<typename T>
T randomValue(T min = std::numeric_limits<T>::min(), T max = std::numeric_limits<T>::max())
{
    return helpers::randomValue(getRandomGenerator(), min, max);
}

} // namespace AI
