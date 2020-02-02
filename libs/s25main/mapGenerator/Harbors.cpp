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

#include "mapGenerator/Harbors.h"
#include "mapGenerator/Islands.h"
#include "mapGenerator/DistanceByProperty.h"
#include "mapGenerator/Textures.h"
#include "mapGenerator/Algorithms.h"
#include "mapGenerator/GridUtility.h"

namespace rttr {
namespace mapGenerator {

// NEW WORLD

std::vector<std::vector<MapPoint>> FindIslands(const Map& map, unsigned minNodes)
{
    std::set<MapPoint, MapPoint_compare> visited;
    std::vector<std::vector<MapPoint>> islands;
    
    auto containsLand = [&map] (const MapPoint& pt) {
        return map.AnyTexture(pt, [](const TerrainDesc& terrain) {
            return terrain.kind == TerrainKind::LAND;
        });
    };
    
    RTTR_FOREACH_PT(MapPoint, map.size)
    {
        if (visited.insert(pt).second)
        {
            auto island = Collect(map.textures, pt, containsLand);
            
            std::copy(island.begin(), island.end(), std::inserter(visited, visited.end()));
            
            if (island.size() >= minNodes)
            {
                islands.push_back(island);
            }
        }
    }
    
    return islands;
}

std::vector<MapPoint> FindCoastland(const Map& map, const std::vector<MapPoint>& area)
{
    std::vector<MapPoint> coast;
    
    auto isLand = [] (const auto& terrain) { return terrain.kind == TerrainKind::LAND; };
    auto isWater = [] (const auto& terrain) { return terrain.kind == TerrainKind::WATER; };
    
    for (auto point : area)
    {
        if (map.AnyTexture(point, isLand) && map.AnyTexture(point, isWater))
        {
            coast.push_back(point);
        }
    }
    
    return coast;
}

std::vector<MapPoint> SuitableHarborPositions(const Map& map, const std::vector<MapPoint>& area)
{
    auto suitablePositions = FindCoastland(map, area);

    auto suroundedByWater = [&map] (const MapPoint& pt) {
        return map.AllTextures(pt, [](const TerrainDesc& terrain) {
            return terrain.kind == TerrainKind::WATER;
        });
    };
    
    auto insufficientWater = [&map, &suroundedByWater] (const MapPoint& pt) {
        return !helpers::contains_if(map.textures.GetNeighbours(pt), suroundedByWater);
    };
    
    helpers::remove_if(suitablePositions, insufficientWater);
    
    return suitablePositions;
}

void PlaceHarborPosition(Map& map, const MapPoint& position)
{
    // Find lowest height around harbor position

    auto& z = map.z;
    auto neighbors = z.GetNeighbours(position);

    auto compareHeight = [&z] (MapPoint p1, MapPoint p2) {
        return z[p1] < z[p2];
    };

    auto lowestPoint = *std::min_element(neighbors.begin(), neighbors.end(), compareHeight);
    auto lowestHeight = std::min(z[position], z[lowestPoint]);
    
    // Flatten the terrain around the harbor position
    
    z[position] = lowestHeight;
    for (MapPoint p : neighbors)
    {
        z[p] = lowestHeight;
    }

    // Make textures around harbor buildable
    
    auto harborTexture = map.FindTexture([] (const TerrainDesc& t) {
        return
            t.GetBQ() == TerrainBQ::CASTLE &&
            t.IsVital() &&
            t.humidity < 100;
    });

    auto triangles = GetTriangles(position, map.size);
    
    for (Triangle t : triangles)
    {
        if (t.rsu)
        {
            map.textures[t.position].rsu = harborTexture;
        }
        else
        {
            map.textures[t.position].lsd = harborTexture;
        }
        
        map.MarkAsHarbor(t);
    }
}

// OLD WORLD

void PlaceHarborPosition(Map_& map, TextureMapping_& mapping, const CoastNode& coast)
{
    HarborParams params(map, mapping);
    unsigned char waterLevel = map.z[coast.water.x + coast.water.y * map.size.x];
    
    Brush<HarborParams> flattenBrush(EnsureTerrainIsFlat);
    params.waterLevel = waterLevel + 2;
    flattenBrush.Paint(params, Large, coast.land, map.size);
    params.waterLevel = waterLevel + 1;
    flattenBrush.Paint(params, Medium, coast.land, map.size);
    params.waterLevel = waterLevel;
    flattenBrush.Paint(params, Small, coast.land, map.size);

    Brush<HarborParams> terrainBrush(EnsureTerrainIsBuildable);
    params.texture = mapping.GetCoastTerrain(1);
    terrainBrush.Paint(params, Medium, coast.land, map.size);

    Brush<HarborParams> harborBrush(EnsureHarborIsBuildable);
    params.texture = mapping.GetCoastTerrain(2);
    harborBrush.Paint(params, Small, coast.land, map.size);

    Brush<HarborParams> waterBrush(EnsureWaterAccess);
    params.texture = mapping.water;
    waterBrush.Paint(params, GetBrushShape(map, mapping.water, coast.land), coast.land, map.size);
}

BrushSettings GetBrushShape(Map_& map, Texture water, const Position& vector)
{
    auto coveredWater = [&map, water, &vector] (auto d1, auto d2) {
        
        Brush<TextureParams> counter(TextureTileCounter);
        
        TextureParams params1(map, water);
        counter.Paint(params1, d1, vector, map.size);
        
        TextureParams params2(map, water);
        counter.Paint(params2, d2, vector, map.size);

        return params1.number < params2.number;
    };
    
    return *std::max_element(BrushDirections.begin(), BrushDirections.end(), coveredWater);
}

void EnsureTerrainIsFlat(HarborParams& params, int index, bool rsu)
{
    
    if ((params.map.textureRsu[index] != params.mapping.water && rsu) ||
        (params.map.textureLsd[index] != params.mapping.water && !rsu))
    {
        params.map.z[index] =
            std::min(params.waterLevel,
                     params.map.z[index]);
    }
    
    params.map.objectInfo[index] = libsiedler2::OI_Empty;
    params.map.objectType[index] = libsiedler2::OT_Empty;
}

void EnsureTerrainIsBuildable(HarborParams& params, int index, bool rsu)
{
    if (rsu)
    {
        if (params.map.textureRsu[index] == params.mapping.GetCoastTerrain())
        {
            params.map.textureRsu[index] = params.texture;
        }
    }
    else
    {
        if (params.map.textureLsd[index] == params.mapping.GetCoastTerrain())
        {
            params.map.textureLsd[index] = params.texture;
        }
    }
}

void EnsureHarborIsBuildable(HarborParams& params, int index, bool rsu)
{
    if (rsu)
    {
        params.map.textureRsu[index] = params.texture;
        params.map.harborsRsu.insert(index);
    }
    else
    {
        params.map.textureLsd[index] = params.texture;
        params.map.harborsLsd.insert(index);
    }
}

void EnsureWaterAccess(HarborParams& params, int index, bool rsu)
{
    if (rsu)
    {
        params.map.textureRsu[index] = params.texture;
    }
    else
    {
        params.map.textureLsd[index] = params.texture;
    }
}

void TextureTileCounter(TextureParams& params, int index, bool rsu)
{
    if (rsu)
    {
        if (params.map.textureRsu[index] == params.texture)
        {
            params.number++;
        }
    }
    else
    {
        if (params.map.textureLsd[index] == params.texture)
        {
            params.number++;
        }
    }
}

std::vector<double> NextHarborDistance(const Coast& coast,
                                       const std::vector<Position>& harbors,
                                       const MapExtent& size)
{
    auto currentDistance = static_cast<double>(size.x + size.y);
    auto coastNodes = static_cast<int>(coast.size());
    
    std::vector<double> distance(coastNodes, currentDistance);
    
    for (int i = 0; i < coastNodes; ++i)
    {
        for (const auto& harbor : harbors)
        {
            currentDistance = GridDistance(harbor, coast[i].land, size);
            distance[i] = std::min(distance[i], currentDistance);
        }
    }
    
    return distance;
}

void PlaceHarbors(int minimumIslandSize,
                  int minimumCoastSize,
                  int maximumIslandHarbors,
                  Map_& map,
                  TextureMapping_& mapping)
{
    
    auto size = map.size;
    auto waterMap = CreateWaterMap(map, mapping);

    Islands islands = FindIslands(waterMap, size, minimumIslandSize);
    
    // iterate over all islands ...
    for (const auto& island : islands)
    {
        const int islandSize = island.size();
        if (islandSize < minimumIslandSize)
        {
            continue;
        }
        
        auto coastline = FindCoast(island, waterMap, size);
        
        const int coastSize = coastline.size();
        if (coastSize < minimumCoastSize)
        {
            continue;
        }
        
        const int harborsForCoastSize = coastSize / minimumCoastSize;
        const int harborPositions = std::max(1,
                                             maximumIslandHarbors > 0 ?
                                             std::min(maximumIslandHarbors,
                                                      harborsForCoastSize) : harborsForCoastSize);
        
        std::vector<Position> harbors;
        
        PlaceHarborPosition(map, mapping, coastline[0]);
        harbors.push_back(coastline[0].land);
        
        for (int i = 1; i < harborPositions; i++)
        {
            const auto& distance = NextHarborDistance(coastline, harbors, size);
            const auto maximum = std::max_element(distance.begin(), distance.end());
            const auto maxIndex = std::distance(distance.begin(), maximum);
            
            PlaceHarborPosition(map, mapping, coastline[maxIndex]);
            harbors.push_back(coastline[maxIndex].land);
        }
    }
}

}}
