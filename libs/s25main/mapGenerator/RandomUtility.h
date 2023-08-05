// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "random/XorShift.h"
#include "gameTypes/MapCoordinates.h"
#include <random>
#include <vector>

namespace rttr::mapGenerator {

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

} // namespace rttr::mapGenerator
