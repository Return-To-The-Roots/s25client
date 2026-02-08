// Copyright (C) 2005 - 2025 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "RTTR_Assert.h"
#include "helpers/MaxEnumValue.h"
#include <iterator>
#include <limits>
#include <random>
#include <type_traits>

namespace helpers {
/// Return a random value in the given bounds. Bound default to the types min and max value
template<typename T, typename RandomT>
T randomValue(RandomT& rng, T min = std::numeric_limits<T>::min(), T max = std::numeric_limits<T>::max())
{
    // 1 byte types are not supported, expand to 2 bytes instead
    using IntDistribution = std::uniform_int_distribution<std::conditional_t<sizeof(T) == 1, int16_t, T>>;
    using Distribution =
      std::conditional_t<std::is_floating_point_v<T>, std::uniform_real_distribution<T>, IntDistribution>;
    Distribution distr(min, max);
    return static_cast<T>(distr(rng));
}

/// Return a true in `chance` out of `total` cases:
///   randomChance()       ... return true|false with 50% chance each
///   randomChance(15)     ... return true in 1/15 of the cases
///   randomChance(20, 5)  ... return true in 5 out of 20 cases, i.e. a probability of 25%. Sames as randomChance(4, 1)
template<typename RandomT>
bool randomChance(RandomT& rng, unsigned total = 2u, unsigned chance = 1u)
{
    RTTR_Assert(total > 0u);
    return (chance >= total) || randomValue(rng, 1u, total) <= chance;
}

/// Return a random enumerator from the given enum
template<typename T, typename RandomT>
T randomEnum(RandomT& rng)
{
    return T(randomValue<std::underlying_type_t<T>>(rng, 0, helpers::MaxEnumValue_v<T>));
}

/// Return a random number in the interval [0, upperBound).
template<typename RandomT>
unsigned getRandomIndex(RandomT& rng, unsigned size)
{
    RTTR_Assert(size > 0);
    return randomValue(rng, 0u, size - 1u);
}

/// Return a randomly seeded random number generator
inline auto getRandomGenerator()
{
    std::random_device rnd_dev;
    return std::mt19937(rnd_dev());
}

/// Return a random element from the container using the rng
template<typename RandomT, typename ContainerT>
auto getRandomElement(RandomT& rng, ContainerT& container)
{
    RTTR_Assert(!container.empty());
    auto it = container.begin();
    if(container.size() > 1u)
        std::advance(it, getRandomIndex(rng, container.size()));
    return *it;
}
} // namespace helpers
