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

#include "mapGenerator/Rivers.h"
#include "mapGenerator/Terrain.h"
#include "mapGenerator/TextureHelper.h"

namespace rttr { namespace mapGenerator {

    River CreateStream(RandomUtility& rnd, Map& map, const MapPoint& source, Direction direction, unsigned length, unsigned splitRate)
    {
        const MapExtent& size = map.size;

        River river;
        std::vector<Direction> exlcuded{direction + 2, direction + 3, direction + 4};

        auto& textures = map.textures;
        auto water = textures.Find(IsWater);

        MapPoint currentNode = source;
        Direction currentDir = direction;

        for(unsigned node = 0; node < length; ++node)
        {
            const auto& triangles = GetTriangles(currentNode, size, currentDir);

            for(const auto& triangle : triangles)
            {
                textures.Set(triangle, water);

                const auto& edges = GetTriangleEdges(triangle, size);
                for(const MapPoint& edge : edges)
                {
                    river.insert(edge);
                }
            }

            const auto lastDir = currentDir;

            while(currentDir == lastDir || helpers::contains(exlcuded, currentDir))
            {
                currentDir = lastDir + (rnd.ByChance(50) ? 5 : 1);
            }

            currentNode = textures.GetNeighbour(currentNode, lastDir);

            if(rnd.ByChance(splitRate))
            {
                auto splitDirection = direction + (rnd.ByChance(50) ? 5 : 1);
                auto anotherRivers = CreateStream(rnd, map, currentNode, splitDirection, length / 2, splitRate / 2);

                for(const MapPoint& node : anotherRivers)
                {
                    river.insert(node);
                }
            }
        }

        auto seaLevel = static_cast<unsigned>(map.height.minimum);
        auto& z = map.z;

        for(const MapPoint& node : river)
        {
            if(z[node] > 0)
            {
                z[node] = std::max(static_cast<unsigned>(z[node] - 1), seaLevel);
            } else
            {
                z[node] = seaLevel;
            }
        }

        return river;
    }

}} // namespace rttr::mapGenerator
