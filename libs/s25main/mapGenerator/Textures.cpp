// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "mapGenerator/Textures.h"
#include "mapGenerator/Algorithms.h"
#include "mapGenerator/TextureHelper.h"

#include <algorithm>
#include <set>
#include <stdexcept>

namespace rttr::mapGenerator {

void TextureMap::set(const MapPoint& pt, DescIdx<TerrainDesc> texture)
{
    const auto& triangles = GetTriangles(pt, textures_.getSize());

    for(const Triangle& triangle : triangles)
    {
        set(triangle, texture);
    }
}

void TextureMap::set(const Triangle& triangle, DescIdx<TerrainDesc> texture)
{
    if(triangle.rsu)
    {
        textures_[triangle.position].rsu = texture;
    } else
    {
        textures_[triangle.position].lsd = texture;
    }
}

DescIdx<TerrainDesc> TextureMap::get(const Triangle& triangle) const
{
    if(triangle.rsu)
    {
        return textures_[triangle.position].rsu;
    } else
    {
        return textures_[triangle.position].lsd;
    }
}

std::vector<DescIdx<TerrainDesc>> Texturizer::createTextureMapping(unsigned mountainLevel)
{
    if(seaLevel_ + 2 > mountainLevel)
    {
        throw std::invalid_argument("sea level must be below mountain level by at least 2");
    }

    const uint8_t maximum = *std::max_element(z_.begin(), z_.end());
    std::vector<DescIdx<TerrainDesc>> mapping(maximum + 1);
    const auto waterTexture = textureMap_.Find(isShipableWater);
    auto landTextures = textureMap_.FindAll(isBuildableLand);
    textureMap_.sort(landTextures, byHumidity);
    auto mountainTextures = textureMap_.FindAll(isMinableMountain);
    mountainTextures.push_back(textureMap_.Find(isSnowOrLava));

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

void Texturizer::applyTexturingByHeightMap(unsigned mountainLevel)
{
    const auto& mapping = createTextureMapping(mountainLevel);
    const MapExtent size = z_.getSize();
    const auto& z = z_;

    auto interpolateEdges = [&size, z](const Triangle& triangle) {
        const auto& edges = GetTriangleEdges(triangle, size);

        // Assumptions:
        // 1) sum of 3 height values is always < 256 (1 byte)
        // 2) we always want to round to the next integer (ceil)
        return static_cast<uint8_t>(std::ceil(static_cast<double>(z[edges[0]] + z[edges[1]] + z[edges[2]]) / 3));
    };

    RTTR_FOREACH_PT(MapPoint, size)
    {
        if(!textures_[pt].rsu)
            textures_[pt].rsu = mapping[interpolateEdges(Triangle(true, pt))];
        if(!textures_[pt].lsd)
            textures_[pt].lsd = mapping[interpolateEdges(Triangle(false, pt))];
    }
}

void Texturizer::applyCoastTexturing(const std::vector<MapPoint>& coast, unsigned width)
{
    std::set<MapPoint, MapPointLess> visited;
    std::copy(coast.begin(), coast.end(), std::inserter(visited, visited.begin()));

    // setup transition textures from water to land
    auto sand = textureMap_.Find(isCoastTerrain);
    auto sandGrass = textureMap_.FindAll(isBuildableCoast);
    textureMap_.sort(sandGrass, byHumidity);
    std::array<DescIdx<TerrainDesc>, 3> transitionTextures{sand, sandGrass[0], sandGrass[1]};

    // exclude mountain & water textures from being replaced by water-land transition
    std::set<DescIdx<TerrainDesc>> excludedTextures;
    const auto mountainTextures = textureMap_.FindAll(isMountainOrSnowOrLava);
    const auto water = textureMap_.Find(isShipableWater);
    excludedTextures.insert(mountainTextures.begin(), mountainTextures.end());
    excludedTextures.insert(water);

    for(unsigned i = 0; i < transitionTextures.size(); ++i)
    {
        // for nodes covered by water & coast, replace only neighboring coast textures
        unsigned appliedWidth = width - (i == 0 ? 1 : 0);
        replaceTextures(textures_, appliedWidth, visited, transitionTextures[i], excludedTextures);

        // don't replace already applied transition textures
        excludedTextures.insert(transitionTextures[i]);
    }
}

void Texturizer::applyMountainTransitions(const std::vector<MapPoint>& mountainFoot)
{
    std::set<DescIdx<TerrainDesc>> excludedTextures{textureMap_.Find(isShipableWater)};

    const auto& mountainTextures = textureMap_.FindAll(isMountainOrSnowOrLava);

    for(const auto& mountainTexture : mountainTextures)
    {
        excludedTextures.insert(mountainTexture);
    }

    auto mountainTransition = textureMap_.Find(isBuildableMountain);

    for(const MapPoint& point : mountainFoot)
    {
        replaceTextureForPoint(textures_, point, mountainTransition, excludedTextures);
    }
}

void Texturizer::applyMountainWaterTransitions(const std::vector<MapPoint>& transitions)
{
    std::set<MapPoint, MapPointLess> nodes;
    nodes.insert(transitions.begin(), transitions.end());
    const auto water = textureMap_.Find(IsWater);
    const auto boulder = textureMap_.Find(isBuildableMountain);
    const auto swamp = textureMap_.Find(isSwamp);
    replaceTextures(textures_, 0, nodes, swamp, {water});
    replaceTextures(textures_, 1, nodes, boulder, {swamp, water});
}

void Texturizer::addTextures(unsigned mountainLevel, unsigned coastline)
{
    applyTexturingByHeightMap(mountainLevel);

    auto coast = SelectPoints(
      [this](const MapPoint& pt) { return this->textureMap_.any(pt, isLand) && this->textureMap_.any(pt, IsWater); },
      this->textures_.getSize());

    applyCoastTexturing(coast, 1); // small for tiny rivers

    auto isRiver = [this](const MapPoint& pt) {
        auto suroundedByWater = [this](const MapPoint& pt) { return this->textureMap_.all(pt, IsWater); };
        return !helpers::contains_if(this->textures_.getNeighbours(pt), suroundedByWater);
    };
    helpers::erase_if(coast, isRiver);

    applyCoastTexturing(coast, coastline);

    const auto mountainFoot = SelectPoints(
      [this](const MapPoint& pt) {
          return this->textureMap_.any(pt, isMinableMountain) && !this->textureMap_.all(pt, isMountainOrSnowOrLava);
      },
      this->textures_.getSize());

    applyMountainTransitions(mountainFoot);

    const auto moutainWaterTransition = SelectPoints(
      [this](const MapPoint& pt) {
          return this->textureMap_.any(pt, isMinableMountain) && this->textureMap_.any(pt, IsWater);
      },
      this->textures_.getSize());

    applyMountainWaterTransitions(moutainWaterTransition);
}

void replaceTextureForPoint(NodeMapBase<TexturePair>& textures, const MapPoint& point, DescIdx<TerrainDesc> texture,
                            const std::set<DescIdx<TerrainDesc>>& excluded)
{
    auto triangles = GetTriangles(point, textures.getSize());

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

void replaceTextures(NodeMapBase<TexturePair>& textures, unsigned radius, std::set<MapPoint, MapPointLess>& nodes,
                     DescIdx<TerrainDesc> texture, const std::set<DescIdx<TerrainDesc>>& excluded)
{
    if(radius == 0)
    {
        for(const MapPoint& pt : nodes)
        {
            replaceTextureForPoint(textures, pt, texture, excluded);
        }

        return;
    }

    std::set<MapPoint, MapPointLess> initialNodes(nodes);

    for(const MapPoint& pt : initialNodes)
    {
        auto points = textures.getPointsInRadius(pt, radius);

        for(const MapPoint& p : points)
        {
            replaceTextureForPoint(textures, p, texture, excluded);
            nodes.insert(p);
        }
    }
}

} // namespace rttr::mapGenerator
