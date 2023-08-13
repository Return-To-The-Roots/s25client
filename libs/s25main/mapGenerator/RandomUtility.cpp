// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "mapGenerator/RandomUtility.h"

#include <ctime>
#include <numeric>

namespace rttr::mapGenerator {

RandomUtility::RandomUtility()
{
    auto seed = static_cast<uint64_t>(time(nullptr));
    rng_.seed(static_cast<UsedRNG::result_type>(seed));
}

RandomUtility::RandomUtility(uint64_t seed)
{
    rng_.seed(static_cast<UsedRNG::result_type>(seed));
}

bool RandomUtility::ByChance(unsigned percentage)
{
    return static_cast<unsigned>(RandomValue(1, 100)) <= percentage;
}

unsigned RandomUtility::Index(const size_t& size)
{
    return RandomValue(0u, static_cast<unsigned>(size - 1));
}

MapPoint RandomUtility::Point(const MapExtent& size)
{
    return MapPoint(Index(size.x), Index(size.y));
}

double RandomUtility::RandomDouble(double min, double max)
{
    std::uniform_real_distribution<double> distr(min, max);
    return distr(rng_);
}

} // namespace rttr::mapGenerator
