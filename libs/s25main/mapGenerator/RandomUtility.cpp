// Copyright (c) 2017 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "mapGenerator/RandomUtility.h"

#include <ctime>
#include <random>

namespace rttr {
namespace mapGenerator {

RandomUtility::RandomUtility()
{
    auto seed = static_cast<uint64_t>(time(nullptr));
    rng_.seed(static_cast<UsedRNG::result_type>(seed));
}

RandomUtility::RandomUtility(uint64_t seed)
{
    rng_.seed(static_cast<UsedRNG::result_type>(seed));
}

bool RandomUtility::ByChance(int percentage)
{
    return Rand(1, 100) <= percentage;
}

int RandomUtility::Index(const size_t& size)
{
    return Rand(0, (int)size - 1);
}

int RandomUtility::Rand(int min, int max)
{
    std::uniform_int_distribution<int> distr(min, max);
    return distr(rng_);
}

Position RandomUtility::RandomPoint(const MapExtent& size)
{
    return Position(Rand(0, size.x-1), Rand(0, size.y-1));
}

double RandomUtility::DRand(double min, double max)
{
    std::uniform_real_distribution<double> distr(min, max);
    return distr(rng_);
}

std::vector<int> RandomUtility::ShuffledRange(int n)
{
    std::vector<int> indices(n);
    std::iota(indices.begin(), indices.end(), 0);
    std::shuffle(indices.begin(), indices.end(), rng_);
    
    return indices;
}

void RandomUtility::Shuffle(std::vector<Position>& area)
{
    std::shuffle(area.begin(), area.end(), rng_);
}

}}
