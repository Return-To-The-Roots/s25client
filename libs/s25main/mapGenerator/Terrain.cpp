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

#include "mapGenerator/Terrain.h"
#include "mapGenerator/Algorithms.h"
#include "mapGenerator/TextureHelper.h"

#include <algorithm>
#include <set>
#include <stdexcept>

namespace rttr { namespace mapGenerator {

    void Restructure(Map& map, const std::vector<MapPoint>& focusArea, double weight)
    {
        const MapExtent& size = map.size;

        auto inFocusArea = [&focusArea](const MapPoint& pt) { return helpers::contains(focusArea, pt); };

        auto& z = map.z;
        auto distances = Distances(size, inFocusArea);
        auto maximum = distances.GetMaximum();

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

        auto isCoast = [&z, &map](const MapPoint& pt) { return z[pt] == map.height.minimum; };

        auto coastDistance = Distances(map.size, isCoast);

        auto minimum = static_cast<unsigned>(map.height.minimum);
        auto maximum = coastDistance.GetMaximum();

        if(maximum > map.height.maximum)
        {
            maximum = map.height.maximum;
            Scale(coastDistance, minimum, maximum);
        }

        RTTR_FOREACH_PT(MapPoint, map.size)
        {
            if(coastDistance[pt] > minimum)
            {
                const int base = coastDistance[pt];

                const int min = std::max(static_cast<int>(minimum + 1), base);
                const int max = std::min(static_cast<int>(maximum), base + 1);

                z[pt] = rnd.RandomInt(min, max);
            }
        }
    }

}} // namespace rttr::mapGenerator
