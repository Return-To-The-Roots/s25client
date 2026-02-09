// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "helpers/random.h"
#include <random>

namespace AI {

std::minstd_rand& getRandomGenerator();

/// Return a random value (min and max are included)
template<typename T>
T randomValue(T min = std::numeric_limits<T>::min(), T max = std::numeric_limits<T>::max())
{
    return helpers::randomValue(getRandomGenerator(), min, max);
}

/// Return a true in `chance` out of `total` cases:
///   randomChance()       ... return true|false with 50% chance each
///   randomChance(15)     ... return true in 1/15 of the cases
///   randomChance(20, 5)  ... return true in 5 out of 20 cases, i.e. a probability of 25%. Sames as randomChance(4, 1)
inline bool randomChance(unsigned total = 2u, unsigned chance = 1u)
{
    return helpers::randomChance(getRandomGenerator(), total, chance);
}

template<typename ContainerT>
unsigned randomIndex(const ContainerT& container)
{
    RTTR_Assert(!container.empty());
    return helpers::getRandomIndex(getRandomGenerator(), container.size());
}

template<typename ContainerT>
auto randomElement(const ContainerT& container)
{
    return helpers::getRandomElement(getRandomGenerator(), container);
}

/// Return random enumerator
template<typename Enum>
auto randomEnum()
{
    return helpers::randomEnum<Enum>(getRandomGenerator());
}

} // namespace AI
