// Copyright (c) 2020 - 2021 Settlers Freaks (sf-team at siedler25.org)
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

#include <iterator>
#include <random>

namespace helpers {
/// Return a random number in the interval [0, upperBound).
template<typename RandomT>
unsigned getRandomIndex(RandomT& rng, unsigned size)
{
    RTTR_Assert(size > 0);
    std::uniform_int_distribution<unsigned> dist(0, size - 1);
    return dist(rng);
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
    std::advance(it, getRandomIndex(rng, container.size()));
    return *it;
}
} // namespace helpers
