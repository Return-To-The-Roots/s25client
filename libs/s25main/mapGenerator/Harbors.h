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

#ifndef Harbors_h__
#define Harbors_h__

#include "mapGenerator/Brush.h"
#include "mapGenerator/Map.h"

namespace rttr {
namespace mapGenerator {

// NEW WORLD

/**
 * Finds all islands on the map which consist of at least the specified minimum number of nodes.
 * @param map reference to the map to search for islands
 * @param minNodes minimum number of nodes of an island
 * @returns vector of islands which are themselves a vector of points on the map.
 */
std::vector<std::vector<MapPoint>> FindIslands(const Map& map, unsigned minNodes);

/**
 * Finds all coast points within the specified area of the map. A point on the map is considered as part of the coast if at least one
 * neighboring triangle contains a water texture and at least another one a land texture.
 * @param map reference to the map
 * @param area area to search for coast points
 * @returns all coast points within the specified area.
 */
std::vector<MapPoint> FindCoastland(const Map& map, const std::vector<MapPoint>& area);

/**
 * Finds all suitable harbor positions within the specified area. A map point is considered suitable for a harbor position if the map
 * point is coast and not next to a tiny river.
 * @param map reference to the map
 * @param area area to search for suitable harbor positions
 * @returns all suitable harbor positions.
 */
std::vector<MapPoint> SuitableHarborPositions(const Map& map, const std::vector<MapPoint>& area);

/**
 * Places a harbor position on the map.
 * @param map reference to the map to place the harbor position on
 * @param position position for the harbor
 */
void PlaceHarborPosition(Map& map, const MapPoint& position);

/**
 * Places harbors on the specified map in suitable positions based on the given parameters.
 * @param minimumIslandSize minimum size of an island to consider it for harbor placement
 * @param minimumCoastSize minimum size of a coastline of an island to consider it for harbor placement
 * @param maximumIslandHarbors maximum number of harbors per island
 * @param map map to place harbors on
 */
void PlaceHarbors(int minimumIslandSize, int minimumCoastSize, int maximumIslandHarbors, Map& map);

// OLD WORLD

struct HarborParams
{
    HarborParams(Map_& map, TextureMapping_& mapping) : map(map), mapping(mapping) {}
    Map_& map;
    TextureMapping_& mapping;
    unsigned char waterLevel;
    Texture texture;
};

struct TextureParams
{
    TextureParams(Map_& map, Texture texture) : map(map), number(0), texture(texture) {}
    Map_& map;
    int number;
    Texture texture;
};

/**
 * Brush to ensure the harbor has water access.
 * @param params parameters to prepare the landscape around the harbor
 * @param index index of the map vertex to prepare
 * @param rightSideUp whether to prepare the right-side-up texture (true) of
 *      the vertex or the left-side-down texture (false)
 */
void EnsureWaterAccess(HarborParams& params, int index, bool rsu); // NOT REQUIRED

/**
 * Brush method for creating a harbor position.
 * @param params parameters to prepare the landscape around the harbor
 * @param index index of the map vertex to prepare
 * @param rightSideUp whether to prepare the right-side-up texture (true) of
 *      the vertex or the left-side-down texture (false)
 */
void EnsureHarborIsBuildable(HarborParams& params, int index, bool rsu); // NOT REQUIRED

/**
 * Brush method for preparing tiles around the harbor position.
 * @param params parameters to prepare the landscape around the harbor
 * @param index index of the map vertex to prepare
 * @param rightSideUp whether to prepare the right-side-up texture (true) of
 *      the vertex or the left-side-down texture (false)
 */
void EnsureTerrainIsBuildable(HarborParams& params, int index, bool rsu); // NOT REQUIRED

// ToDo: use correct grid representation for z-values (not the brush which is applied to textures)
/**
 * Flattens the land around the harbor position.
 * @param params parameters to prepare the landscape around the harbor
 * @param index index of the map vertex to prepare
 * @param rightSideUp whether to prepare the right-side-up texture (true) of
 *      the vertex or the left-side-down texture (false)
 */
void EnsureTerrainIsFlat(HarborParams& params, int index, bool rsu); // NOT REQUIRED

/**
 * Counts the number of textures specified in the params.
 */
void TextureTileCounter(TextureParams& params, int index, bool rsu); // NOT REQUIRED

/**
 * Gets the corresponding brush shape for the specified direction vector.
 * @param vector direction vector
 * @param water water texture
 */
BrushSettings GetBrushShape(Map_& map, Texture water, const Position& vector); // NOT REQUIRED

/**
 * Places a harbor position on specified map at the specified position on a coastline.
 * The function ensures that the area around the harbor is cleared and flattened.
 * @param map map to place harbor position on
 * @param mapping texture mapping to access texture related game data
 * @param coast location of the harbor on the coastline
 */
void PlaceHarborPosition(Map_& map, TextureMapping_& mapping, const CoastNode& coast); // DONE

/**
 * Calculates the distance to the closest harbor position.
 * @param coast coast area for possible harbor positions
 * @param harbors existing harbors already placed on the map
 * @param size size of the map
 * @return distance values for the entire coastline to the closest harbor position
 */
std::vector<double> NextHarborDistance(const Coast& coast,
                                       const std::vector<Position>& harbors,
                                       const MapExtent& size);


/**
 * Places harbors on the specified map in suitable positions based on the given parameters.
 * @param minimumIslandSize minimum size of an island to consider it for harbor placement
 * @param minimumCoastSize minimum size of a coastline of an island to consider it for harbor placement
 * @param maximumIslandHarbors maximum number of harbors per island
 * @param map map to place harbors on
 * @param mapping texture mapping to access texture related game data
 */
void PlaceHarbors(int minimumIslandSize,
                  int minimumCoastSize,
                  int maximumIslandHarbors,
                  Map_& map,
                  TextureMapping_& mapping);

}}

#endif // Harbors_h__
