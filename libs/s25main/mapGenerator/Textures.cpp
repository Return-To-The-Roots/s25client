// Copyright (c) 2017 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "mapGenerator/Textures.h"
#include "mapGenerator/Algorithms.h"
#include "mapGenerator/TextureHelper.h"

#include <algorithm>
#include <set>
#include <stdexcept>

namespace rttr { namespace mapGenerator {

    void TextureMap::Set(const MapPoint& pt, const DescIdx<TerrainDesc>& texture)
    {
        const auto& triangles = GetTriangles(pt, GetSize());

        for(const Triangle& triangle : triangles)
        {
            Set(triangle, texture);
        }
    }

    void TextureMap::Set(const Triangle& triangle, const DescIdx<TerrainDesc>& texture)
    {
        if(triangle.rsu)
        {
            nodes[GetIdx(triangle.position)].rsu = texture;
        } else
        {
            nodes[GetIdx(triangle.position)].lsd = texture;
        }
    }

    DescIdx<TerrainDesc> TextureMap::Get(const Triangle& triangle) const
    {
        if(triangle.rsu)
        {
            return nodes[GetIdx(triangle.position)].rsu;
        } else
        {
            return nodes[GetIdx(triangle.position)].lsd;
        }
    }

    std::vector<DescIdx<TerrainDesc>> Texturizer::CreateTextureMapping(unsigned mountainLevel)
    {
        if(seaLevel_ + 2 > mountainLevel)
        {
            throw std::invalid_argument("sea level must be below mountain level by at least 2");
        }

        const uint8_t maximum = z_.GetRange().maximum;

        std::vector<DescIdx<TerrainDesc>> mapping(maximum + 1);

        auto waterTexture = textures_.Find(IsShipableWater);

        auto landTextures = textures_.FindAll(IsBuildableLand);
        textures_.Sort(landTextures, ByHumidity);

        auto mountainTextures = textures_.FindAll(IsMinableMountain);
        mountainTextures.push_back(textures_.Find(IsSnowOrLava));

        for(uint8_t z = 0; z <= maximum; z++)
        {
            if(z <= seaLevel_)
            {
                mapping[z] = waterTexture;
            } else if(z < mountainLevel)
            {
                ValueRange<uint8_t> range(seaLevel_ + 1, mountainLevel - 1);
                mapping[z] = landTextures[MapValueToIndex(z, range, landTextures.size())];
            } else
            {
                ValueRange<uint8_t> range(mountainLevel, maximum);
                mapping[z] = mountainTextures[MapValueToIndex(z, range, mountainTextures.size())];
            }
        }

        return mapping;
    }

    void Texturizer::ApplyTexturingByHeightMap(unsigned mountainLevel)
    {
        const auto& mapping = CreateTextureMapping(mountainLevel);

        const MapExtent size = textures_.GetSize();
        const auto& z = this->z_;

        auto interpolateEdges = [&size, z](const Triangle& triangle) {
            const auto& edges = GetTriangleEdges(triangle, size);

            // Assumptions:
            // 1) sum of 3 height values is always < 256 (1 byte)
            // 2) we always want to round to the next integer (ceil)
            return static_cast<uint8_t>(std::ceil(static_cast<double>(z[edges[0]] + z[edges[1]] + z[edges[2]]) / 3));
        };

        RTTR_FOREACH_PT(MapPoint, size)
        {
            if(textures_[pt].rsu.value == DescIdx<TerrainDesc>::INVALID)
            {
                textures_[pt].rsu = mapping[interpolateEdges(Triangle(true, pt))];
            }
            if(textures_[pt].lsd.value == DescIdx<TerrainDesc>::INVALID)
            {
                textures_[pt].lsd = mapping[interpolateEdges(Triangle(false, pt))];
            }
        }
    }

    void Texturizer::ApplyCoastTexturing(const std::vector<MapPoint>& coast, unsigned width)
    {
        std::vector<DescIdx<TerrainDesc>> transition;
        std::set<DescIdx<TerrainDesc>> excludedTextures;
        std::set<MapPoint, MapPointLess> visited;
        std::copy(coast.begin(), coast.end(), std::inserter(visited, visited.begin()));

        auto water = textures_.Find(IsShipableWater);
        auto coastland = textures_.FindAll(IsBuildableCoast);
        auto mountain = textures_.FindAll(IsMountainOrSnowOrLava);

        for(const auto& texture : mountain)
        {
            excludedTextures.insert(texture);
        }

        textures_.Sort(coastland, ByHumidity);

        transition.push_back(textures_.Find(IsCoastTerrain));
        transition.push_back(coastland[0]);
        transition.push_back(coastland[1]);

        excludedTextures.insert(water);

        for(unsigned i = 0; i < 3; ++i)
        {
            unsigned appliedWidth = width - (i == 0 ? 1 : 0);

            ReplaceTextures(textures_, appliedWidth, visited, transition[i], excludedTextures);

            excludedTextures.insert(transition[i]);
        }
    }

    void Texturizer::ApplyMountainTransitions(const std::vector<MapPoint>& mountainFoot)
    {
        std::set<DescIdx<TerrainDesc>> excludedTextures{textures_.Find(IsShipableWater)};

        const auto& mountainTextures = textures_.FindAll(IsMountainOrSnowOrLava);

        for(const auto& mountainTexture : mountainTextures)
        {
            excludedTextures.insert(mountainTexture);
        }

        auto mountainTransition = textures_.Find(IsBuildableMountain);

        for(const MapPoint& point : mountainFoot)
        {
            ReplaceTextureForPoint(textures_, point, mountainTransition, excludedTextures);
        }
    }

    void Texturizer::AddTextures(unsigned mountainLevel, unsigned coastline)
    {
        ApplyTexturingByHeightMap(mountainLevel);

        const auto& textures = textures_;
        const auto& size = textures.GetSize();

        std::vector<MapPoint> coast;

        RTTR_FOREACH_PT(MapPoint, size)
        {
            if(textures.Any(pt, IsLand) && textures.Any(pt, IsWater))
            {
                coast.push_back(pt);
            }
        }

        ApplyCoastTexturing(coast, 1); // small for tiny rivers

        auto isRiver = [&textures](const MapPoint& pt) {
            auto suroundedByWater = [&textures](const MapPoint& pt) { return textures.All(pt, IsWater); };

            return !helpers::contains_if(textures.GetNeighbours(pt), suroundedByWater);
        };

        helpers::remove_if(coast, isRiver);

        ApplyCoastTexturing(coast, coastline);

        std::vector<MapPoint> footOfMountain;

        RTTR_FOREACH_PT(MapPoint, size)
        {
            if(textures.Any(pt, IsMinableMountain) && !textures.All(pt, IsMountainOrSnowOrLava))
            {
                footOfMountain.push_back(pt);
            }
        }

        ApplyMountainTransitions(footOfMountain);
    }

    void ReplaceTextureForPoint(NodeMapBase<TexturePair>& textures, const MapPoint& point,
                                const DescIdx<TerrainDesc>& texture, const std::set<DescIdx<TerrainDesc>>& excluded)
    {
        auto triangles = GetTriangles(point, textures.GetSize());

        for(const Triangle& triangle : triangles)
        {
            auto& pair = textures[triangle.position];
            auto& currentTexture = triangle.rsu ? pair.rsu : pair.lsd;

            if(!helpers::contains(excluded, currentTexture))
            {
                currentTexture = texture;
            }
        }
    }

    void ReplaceTextures(NodeMapBase<TexturePair>& textures, unsigned radius, std::set<MapPoint, MapPointLess>& nodes,
                         const DescIdx<TerrainDesc>& texture, const std::set<DescIdx<TerrainDesc>>& excluded)
    {
        if(radius == 0)
        {
            for(const MapPoint& pt : nodes)
            {
                ReplaceTextureForPoint(textures, pt, texture, excluded);
            }

            return;
        }

        std::set<MapPoint, MapPointLess> initialNodes(nodes);

        for(const MapPoint& pt : initialNodes)
        {
            auto points = textures.GetPointsInRadius(pt, radius);

            for(const MapPoint& p : points)
            {
                ReplaceTextureForPoint(textures, p, texture, excluded);
                nodes.insert(p);
            }
        }
    }

}} // namespace rttr::mapGenerator
