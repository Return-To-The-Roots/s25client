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

#ifndef ObjectGenerator_h__
#define ObjectGenerator_h__

#include "gameTypes/MapTypes.h"
#include "stdint.h"
#include <utility>

typedef std::pair<uint8_t, uint8_t> IntPair;

/**
 * TODO: probably handles to much (textures are not objects)
 * Object generator for map generation.
 */
class ObjectGenerator
{
    public:
    
    /**
     * Creates a new texture for the specified terrain type.
     * @param harbor whether or not enable harbor placement on the texture. To enable the player to
     *      place a harbor at the position of the texture, it need to be close to water. Also keep 
     *      in mind, only terrain types which allow buildings also support harbor placement.
     * @return the new texture, including two triangles for right-side-up and right-side-down
     */
    static IntPair CreateTexture(TerrainType terrain, const bool harbor = false);
    
    /**
     * Checks whether or not the specified texture is representing the specified terrain.
     * @param texture input texture to check
     * @param terrain terrain to compare the input texture to
     * @return true if at least one of the texture-triangles matches the terrain, false otherwise
     */
    static bool IsTexture(const IntPair& texture, TerrainType terrain);
    
    /**
     * Converts the input terrain into a texture id which can be stored in the original s2 map format. 
     * @param terrain terrain to get the texture id for
     * @return texture id of the terrain type
     */
    static uint8_t GetTextureId(TerrainType terrain);
    
    /**
     * Checks whether or not it is allowed to build a harbor on the specified terrain.
     * @param terrain terrain to check
     * @return true of it is allowed to build a harbor on the terrain
     */
    static bool IsHarborAllowed(TerrainType terrain);

    /**
     * Creates a new, empty object.
     * @return empty object
     */
    static IntPair CreateEmpty();

    /**
     * Creates a new headquarter for the specified player.
     * @param i player number
     * @return a new headquarter object
     */
    static IntPair CreateHeadquarter(const int i);

    /**
     * Checks whether or not the specified object is empty.
     * @param object object to check
     * @return true if the object is empty, false otherwise
     */
    static bool IsEmpty(const IntPair& object);

    /**
     * Creates a new duck.
     * @param likelyhood likelyhood for object generation in percent
     * @return a new duck animal
     */
    static uint8_t CreateDuck(const int likelyhood);

    /**
     * Creates a new sheep.
     * @param likelyhood likelyhood for object generation in percent
     * @return a new sheep animal
     */
    static uint8_t CreateSheep(const int likelyhood);

    /**
     * Creates a new, random animal to be placed inside of a forest.
     * @param likelyhood likelyhood for object generation in percent
     * @return a new forest animal
     */
    static uint8_t CreateRandomForestAnimal(const int likelyhood);
    
    /**
     * Creates a new random mountain resources (gold, coal, granite, iron).
     * @return random piles of gold, coal, granite or iron
     */
    static uint8_t CreateRandomResource();

    /**
     * Creates a new, random ground animal.
     * @param likelyhood likelyhood for object generation in percent
     * @return a new ground animal
     */
    static uint8_t CreateRandomAnimal(const int likelyhood);
    
    /**
     * Checks whether or not the specified object is a tree.
     * @param object object to check
     * @return true if the specified object is a tree, false otherwise
     */
    static bool IsTree(const IntPair& object);

    /**
     * Creates a new, random tree (excluding palm trees).
     * @return a new tree object
     */
    static IntPair CreateRandomTree();

    /**
     * Creates a new, random palm.
     * @return a new palm object
     */
    static IntPair CreateRandomPalm();
    
    /**
     * Creates a new, random tree (including palm trees).
     * @return a new tree object
     */
    static IntPair CreateRandomMixedTree();
    
    /**
     * Creates a random amount of stone.
     * @return a new stone object
     */
    static IntPair CreateRandomStone();
};

#endif // ObjectGenerator_h__
