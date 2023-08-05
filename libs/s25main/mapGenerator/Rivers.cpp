// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "mapGenerator/Rivers.h"
#include "mapGenerator/Terrain.h"
#include "mapGenerator/TextureHelper.h"

namespace rttr::mapGenerator {

    River CreateStream(RandomUtility& rnd, Map& map, const MapPoint& source, Direction direction, unsigned length,
                       unsigned splitRate)
    {
        const MapExtent& size = map.size;

        River river;
        std::vector<Direction> exlcuded{direction + 2, direction + 3, direction + 4};

        auto& textures = map.textureMap;
        const auto water = textures.Find(IsWater);

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

            if(map.z[currentNode] == map.height.minimum)
            {
                return river;
            }

            const auto lastDir = currentDir;

            while(currentDir == lastDir || helpers::contains(exlcuded, currentDir))
            {
                currentDir = lastDir + (rnd.ByChance(50) ? 5 : 1);
            }

            currentNode = map.getTextures().GetNeighbour(currentNode, lastDir);

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

} // namespace rttr::mapGenerator
