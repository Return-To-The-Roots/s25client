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

#pragma once

#include "RandomConfig.h"
#include "mapGenerator/Map.h"
#include "libsiedler2/enumTypes.h"

struct TerrainDesc;

/**
 * Utility class to place object, textures and animals on a map.
 */
class ObjectGenerator
{
    RandomConfig& config;

public:
    ObjectGenerator(RandomConfig& config) : config(config) {}

    /**
     * Creates a new texture for the specified terrain type.
     * @param map map to place the texture on
     * @param index vertex index to place the texture on
     * @param harbor whether or not enable harbor placement on the texture. To enable the player to
     *      place a harbor at the position of the texture, it need to be close to water. Also keep
     *      in mind, only terrain types which allow buildings also support harbor placement.
     */
    void CreateTexture(Map& map, int index, DescIdx<TerrainDesc> terrain, bool harbor = false) const;

    /**
     * Checks whether or not the specified texture is representing the specified terrain.
     * @param map map of the vertex to check
     * @param index vertex index of the texture to check
     * @param terrain terrain to compare the input texture to
     * @return true if at least one of the texture-triangles matches the terrain, false otherwise
     */
    bool IsTexture(const Map& map, int index, DescIdx<TerrainDesc> terrain) const;
    template<class T_Predicate>
    bool IsTexture(const Map& map, int index, T_Predicate predicate) const;

    /**
     * Checks whether or not it is allowed to build a harbor on the specified terrain.
     * @param terrain terrain to check
     * @return true of it is allowed to build a harbor on the terrain
     */
    bool IsHarborAllowed(DescIdx<TerrainDesc> terrain) const;

    /**
     * Creates a new, empty object.
     * @param map map to place the empty object on
     * @param index vertex index to place the empty object on
     */
    static void CreateEmpty(Map& map, int index);

    /**
     * Creates a new headquarter for the specified player.
     * @param map map to place the headquarter on
     * @param index vertex index to place the headquarter on
     * @param i player number
     */
    static void CreateHeadquarter(Map& map, int index, unsigned i);

    /**
     * Checks whether or not the specified object is empty.
     * @param map map of the vertex to check
     * @param index vertex index to check
     */
    static bool IsEmpty(const Map& map, int index);

    /**
     * Creates a new duck.
     * @param likelihood likelihood for object generation in percent
     * @return a new duck animal
     */
    libsiedler2::Animal CreateDuck(int likelihood);

    /**
     * Creates a new sheep.
     * @param likelihood likelihood for object generation in percent
     * @return a new sheep animal
     */
    libsiedler2::Animal CreateSheep(int likelihood);

    /**
     * Creates a new, random animal to be placed inside of a forest.
     * @param likelihood likelihood for object generation in percent
     * @return a new forest animal
     */
    libsiedler2::Animal CreateRandomForestAnimal(int likelihood);

    /**
     * Creates a new random mountain resources (gold, coal, granite, iron).
     * @param ratioGold ratio of gold placed as mountain resource on the map
     * @param ratioIron ratio of iron placed as mountain resource on the map
     * @param ratioCoal ratio of coal placed as mountain resource on the map
     * @param ratioGranite ratio of granite placed as mountain resource on the map
     * @return random piles of gold, coal, granite or iron
     */
    uint8_t CreateRandomResource(unsigned ratioGold, unsigned ratioIron, unsigned ratioCoal, unsigned ratioGranite);

    /**
     * Creates a new, random ground animal.
     * @param likelihood likelihood for object generation in percent
     * @return a new ground animal
     */
    libsiedler2::Animal CreateRandomAnimal(int likelihood);

    /**
     * Checks whether or not the specified object is a tree.
     * @param map map of the vertex to check
     * @param index index of the vertex to check
     * @return true if the specified object is a tree, false otherwise
     */
    static bool IsTree(const Map& map, int index);

    /**
     * Creates a new, random tree (excluding palm trees).
     * @param map map to place the object upon
     * @param index index of the vertex for the new object
     */
    void CreateRandomTree(Map& map, int index);

    /**
     * Creates a new, random palm.
     * @param map map of the vertex to create a new tree on
     * @param index index of the vertex to create a new tree on
     */
    void CreateRandomPalm(Map& map, int index);

    /**
     * Creates a new, random tree (including palm trees).
     * @param map map of the vertex to create a new tree on
     * @param index index of the vertex to create a new tree on
     */
    void CreateRandomMixedTree(Map& map, int index);

    /**
     * Creates a random amount of stone.
     * @param map map of the vertex to create a new stone pile on
     * @param index index of the vertex to create a new stone pile on
     */
    void CreateRandomStone(Map& map, int index);
};

template<class T_Predicate>
inline bool ObjectGenerator::IsTexture(const Map& map, int index, T_Predicate predicate) const
{
    return predicate(config.GetTerrainByS2Id(map.textureRsu[index]))
           || predicate(config.GetTerrainByS2Id(map.textureLsd[index]));
}
