// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef GreenlandGenerator_h__
#define GreenlandGenerator_h__

#include "mapGenerator/Generator.h"
#include <cstdlib>

/**
 * Random map generator for Greenland.
 */
class GreenlandGenerator : public Generator
{
    public:
    
    /**
     * Creates a new GreenlandGenerator with the specified landscape properties.
     * The radius is measured in map length: min(width/2, height/2).
     * @param radiusPlayerMin minimum radius each player must be placed away from the center point of the map
     * @param radiusPlayerMax maximum radius each player can be placed away from the center point of the map
     * @param radiusInnerLand maximum radius from the map's center point to consider land as "inner land"
     * @param radiusIslands minimum radius from the map's center point to consider land for the generation of islands
     * @üaram radiusSmallIslands minimum radius from the map's center point to consider land for the generation of smaller islands
     * @param radiusWaterOnly minimum radius from the map's center point to consider land for water only
     * @param likelyhoodStone likelyhood in percentage that a stone pile is generated for a vertex
     * @param likelyhoodTree likelyhood in percentage that a tree is generated for a vertex
     */
    GreenlandGenerator(double radiusPlayerMin,
                       double radiusPlayerMax,
                       double radiusInnerLand,
                       double radiusIslands,
                       double radiusSmallIslands,
                       double radiusWaterOnly,
                       int likelyhoodStone,
                       int likelyhoodTree) :
    _radiusPlayerMin(radiusPlayerMin),
    _radiusPlayerMax(radiusPlayerMax),
    _radiusInnerLand(radiusInnerLand),
    _radiusIslands(radiusIslands),
    _radiusSmallIslands(radiusSmallIslands),
    _radiusWaterOnly(radiusWaterOnly),
    _likelyhoodStone(likelyhoodStone),
    _likelyhoodTree(likelyhoodTree){}
    
    /**
     * Creates a new GreenlandGenerator with random landscape properties.
     */
    GreenlandGenerator()
    {
        _radiusPlayerMin = DRand(0.3, 0.5);
        _radiusPlayerMax = DRand(_radiusPlayerMin, _radiusPlayerMin + 0.2);
        _radiusInnerLand = DRand(0.0, 0.5);
        _radiusIslands = DRand(_radiusInnerLand, _radiusInnerLand + 1.8);
        _radiusSmallIslands = DRand(_radiusIslands, _radiusIslands + 1.0);
        _radiusWaterOnly = DRand(_radiusSmallIslands, _radiusSmallIslands + 0.5);
        _likelyhoodStone = 1 + rand() % 8;
        _likelyhoodTree = 5 + rand() % 30;
    }
    
    protected:
    
    /**
     * Generates a new random map with the specified settings.
     * @param settings settings used to generate the random map
     */
    Map* GenerateMap(const MapSettings& settings);
    
    private:
    
    /**
     * Minimum radius each player must be placed away from the center point of the map.
     * The radius is measured in map length: min(width/2, height/2).
     */
    double _radiusPlayerMin;
    
    /**
     * Maximum radius each player can be placed away from the center point of the map.
     * The radius is measured in map length: min(width/2, height/2).
     */
    double _radiusPlayerMax;
    
    /**
     * Maximum radius from the map's center point to consider land as "inner land" (higher hills).
     * The radius is measured in map length: min(width/2, height/2).
     */
    double _radiusInnerLand;
    
    /**
     * Minimum radius from the map's center point to consider land for the generation of islands.
     * The radius is measured in map length: min(width/2, height/2).
     */
    double _radiusIslands;
    
    /**
     * Minimum radius from the map's center point to consider land for the generation of smaller islands.
     * The radius is measured in map length: min(width/2, height/2).
     */
    double _radiusSmallIslands;
    
    /**
     * Minimum radius from the map's center point to consider land for water only.
     * The radius is measured in map length: min(width/2, height/2).
     */
    double _radiusWaterOnly;
    
    /**
     * Likelyhood in percentage that a stone pile is generated for a vertex.
     */
    int _likelyhoodStone;
    
    /**
     * Likelyhood in percentage that a tree is generated for a vertex.
     */
    int _likelyhoodTree;
    
    /**
     * Create a new, empty terain for the specified map.
     * @param settings settings used for map generation
     * @param map map to modify
     */
    void CreateEmptyTerrain(const MapSettings& settings, Map* map);
    
    /**
     * Create player positions (headquarters) for the specified map.
     * @param settings settings used for map generation
     * @param map map to modify
     */
    void PlacePlayers(const MapSettings& settings, Map* map);

    /**
     * Create standard resoures for each player.
     * @param settings settings used for map generation
     * @param map map to modify
     */
    void PlacePlayerResources(const MapSettings& settings, Map* map);
    
    /**
     * Create a elevation (hills) for the specified map.
     * @param settings settings used for map generation
     * @param map map to modify
     */
    void CreateHills(const MapSettings& settings, Map* map);
    
    /**
     * Fill the remaining terrain (apart from the player positions) according to the generated hills.
     * @param settings settings used for map generation
     * @param map map to modify
     */
    void FillRemainingTerrain(const MapSettings& settings, Map* map);

    /**
     * Generates a random number between min and max.
     * @param min minimum values
     * @param max maximum value
     */
    double DRand(const double min, const double max)
    {
        return min + static_cast<double>(rand()) / (static_cast<double>(RAND_MAX/(max - min)));
    }
 };

#endif // GreenlandGenerator_h__
