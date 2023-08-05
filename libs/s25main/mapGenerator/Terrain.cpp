// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "mapGenerator/Terrain.h"
#include "mapGenerator/Algorithms.h"
#include "mapGenerator/TextureHelper.h"

#include <algorithm>
#include <stdexcept>

namespace rttr::mapGenerator {

    void Restructure(Map& map, const std::function<bool(const MapPoint&)>& predicate, double weight)
    {
        auto& z = map.z;
        const MapExtent& size = map.size;
        const auto distances = DistancesTo(size, predicate);
        const auto maximum = *std::max_element(distances.begin(), distances.end());

        RTTR_FOREACH_PT(MapPoint, size)
        {
            // value between 0 and 1 - depending on the distance to focused area
            auto value = static_cast<double>(maximum - distances[pt]) / maximum;

            // combine weight, value and actual z-value
            z[pt] = static_cast<uint8_t>(round(std::pow(value, weight) * z[pt]));
        }

        Scale(z, map.height.minimum, map.height.maximum);
    }

    void ResetSeaLevel(Map& map, RandomUtility& rnd, unsigned seaLevel)
    {
        auto& z = map.z;

        RTTR_FOREACH_PT(MapPoint, map.size)
        {
            if(z[pt] <= seaLevel)
            {
                z[pt] = map.height.minimum;
            } else
            {
                z[pt] -= seaLevel;
            }
        }

        const auto isCoast = [&z, &map](const MapPoint& pt) { return z[pt] == map.height.minimum; };
        auto coastDistance = DistancesTo(map.size, isCoast);
        auto minimum = static_cast<unsigned>(map.height.minimum);
        auto maximum = *std::max_element(coastDistance.begin(), coastDistance.end());

        if(maximum > map.height.maximum)
        {
            maximum = map.height.maximum;
            Scale(coastDistance, minimum, maximum);
        }

        RTTR_FOREACH_PT(MapPoint, map.size)
        {
            if(coastDistance[pt] > minimum)
            {
                const unsigned base = coastDistance[pt];

                z[pt] = rnd.RandomValue(std::max(minimum + 1, base), std::min(maximum, base + 1));
            }
        }
    }

} // namespace rttr::mapGenerator
