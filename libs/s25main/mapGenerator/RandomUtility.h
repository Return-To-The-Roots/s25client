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

#pragma once

#include "random/XorShift.h"
#include "gameTypes/MapCoordinates.h"
#include <random>
#include <vector>

namespace rttr { namespace mapGenerator {

    using UsedRNG = XorShift;

    class RandomUtility
    {
    private:
        UsedRNG rng_;

    public:
        RandomUtility();
        RandomUtility(uint64_t seed);

        /**
         * Returns 'true' by a %-chance.
         *
         * @param percentage likelihood to return true
         */
        bool ByChance(unsigned percentage);

        /**
         * Generates a random index based on the specified size.
         *
         * @param size range of indices (0 to size - 1)
         *
         * @return a random index based on the specified size.
         */
        unsigned Index(const size_t& size);

        /**
         * Creates a random point within the specified size.
         *
         * @param size size of the map
         *
         * @return a random point on the map.
         */
        MapPoint Point(const MapExtent& size);

        /**
         * Returns a random item.
         *
         * @param items collection of all items to choose from
         *
         * @returns a random item out of the specified collection of items.
         */
        template<typename T>
        T RandomItem(const std::vector<T>& items)
        {
            return items[Index(items.size())];
        }

        /**
         * Generates a random value between the specified minimum and maximum values.
         *
         * @param min minimum value for the random number
         * @param max maximum value for the random number
         *
         * @return a random number between min and max (inclusive).
         */
        template<typename T>
        T RandomValue(T min, T max)
        {
            static_assert(std::is_integral<T>::value, "T must be an integral type.");
            std::uniform_int_distribution<T> distr(min, max);
            return distr(rng_);
        }

        /**
         * Generates a random floating point value between the specified minimum and maximum values.
         *
         * @param min minimum value for the random number
         * @param max maximum value for the random number
         *
         * @return a random float value between min and max (inclusive).
         */
        double RandomDouble(double min, double max);
    };

}} // namespace rttr::mapGenerator
