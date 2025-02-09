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

// Return a random bool:
//   random()           ... will return true|false with 50% chance each
//   random(15)         ... will return true in 1/15 of the cases
inline bool random(unsigned total = 1u)
{
    return helpers::randomValue(getRandomGenerator(), 0u, total) == 0u;
}

//   random(20, 5)      ... will return true in ~3/4 of the cases (random(20) > 5)
inline bool random(unsigned total, unsigned chance)
{
    return helpers::randomValue(getRandomGenerator(), 0u, total) > chance;
}

template<typename ContainerT>
inline unsigned randomIndex(const ContainerT& container)
{
    RTTR_Assert(!container.empty());
    return helpers::randomValue(getRandomGenerator(), 0u, static_cast<unsigned>(container.size()) - 1u);
}

} // namespace AI
