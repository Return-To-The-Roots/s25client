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

#ifndef TextureMapping_h__
#define TextureMapping_h__

#include "mapGenerator/Types.h"
#include <functional>

namespace rttr {
namespace mapGenerator {

// NEW WORLD

using Predicate = std::function<bool(TerrainDesc)>;

class TextureMapping
{
public:
    
    /**
     * Gets a list of all textures for the specified landscape.
     * @param landscape landscape description identifier
     * @returns a list of all textures available for the specified landscape.
     */
    virtual Textures GetTextures(const DescIdx<LandscapeDesc>& landscape) const = 0;
    
    /**
     * Checks whether or not the texture fulfills the specified predicate.
     * @param texture texture to check
     * @param predicate predicate to verify
     * @returns true if the texture fulfills the specified predicate, false otherwise.
     */
    virtual bool CheckTexture(const Texture& texture, Predicate predicate) const = 0;
    
    /**
     * Translates the specified landscape description index into a S2 landscape index.
     * @param landscape landscape description index
     * @returns the S2 landscape index for the specified landscape.
     */
    virtual uint8_t GetLandscapeIndex(const DescIdx<LandscapeDesc>& landscape) const = 0;

    /**
     * Translates the specified terrain description index into a S2 terrain index.
     * @param terrain terrain description index
     * @returns the S2 terrain index for the specified terrain.
     */
    virtual uint8_t GetTerrainIndex(const DescIdx<TerrainDesc>& terrain) const = 0;
};

class RttrMapping : public TextureMapping
{
private:

    WorldDescription& worldDesc_;
    
public:
    
    /**
     * Creates a new instance of RttrMapping with the specified world description.
     * @param worldDesc reference to the world description
     */
    RttrMapping(WorldDescription& worldDesc);
    
    /**
     * Gets a list of all textures for the specified landscape.
     * @param landscape landscape description identifier
     * @returns a list of all textures available for the specified landscape.
     */
    Textures GetTextures(const DescIdx<LandscapeDesc>& landscape) const;
    
    /**
     * Checks whether or not the texture fulfills the specified predicate.
     * @param texture texture to check
     * @param predicate predicate to verify
     * @returns true if the texture fulfills the specified predicate, false otherwise.
     */
    bool CheckTexture(const Texture& texture, Predicate predicate) const;
    
    /**
     * Translates the specified landscape description index into a S2 landscape index.
     * @param landscape landscape description index
     * @returns the S2 landscape index for the specified landscape.
     */
    uint8_t GetLandscapeIndex(const DescIdx<LandscapeDesc>& landscape) const;
    
    /**
     * Translates the specified terrain description index into a S2 terrain index.
     * @param terrain terrain description index
     * @returns the S2 terrain index for the specified terrain.
     */
    uint8_t GetTerrainIndex(const DescIdx<TerrainDesc>& terrain) const;
};

// OLD WORLD

/**
 * Maps the specified value of a range between the minimum and maximum to the corresponding index of an array
 * of the specified size. It's important to note, that this function make sure that following ratios are equal:
 * v / (max - min) = MapRangeToIndex(v, min, max, size) / size
 *
 * @param value value to map to the new range
 * @param minimum minimum value of the current range
 * @param maximum maximum value of the current range
 * @param size size of the container to find an index for
 *
 * @returns an index for a container which corresponds to the specified value within the specified range.
 */
int MapRangeToIndex(int value, int minimum, int maximum, size_t size);

class TextureMapping_
{
public:
    
    /**
     * Creates a vector of terrains where the indices represent height values.
     * @param peak maximum height value used on the map
     * @param sea maximum height value the sea can reach
     * @param mountains minimum height value from which the mountains start
     * @returns a mapping from height values to terrain descriptions.
     */
    virtual Textures MapHeightsToTerrains(Height peak, Height sea, Height mountains) = 0;

    /**
     * Gets terrain for the coast depending on it's distance to water.
     * @param distance distance to water
     * @returns the coast terrain description.
     */
    virtual Texture GetCoastTerrain(int distance = 0) const = 0;
    
    /**
     * Gets the humidity of the texture.
     * @param texture reference to the texture
     * @returns humidity value for the texture.
     */
    virtual int GetHumidity(const Texture& texture) const = 0;
    
    /**
     * Checks whether or not the specified texture is a coast texture.
     * @param texture texture to check
     * @param distance distance to water
     * @returns true of the texture is a coast texture with the specified distance to water, false otherwise.
     */
    virtual bool IsCoast(const Texture& texture, int distance = 0) const = 0;
    
    /**
     * Checks whether or not the specified texture is a mountain texture.
     * @param texture texture to check
     * @returns true of the texture is a mountain texture, false otherwise.
     */
    virtual bool IsMountain(const Texture& texture) const = 0;
    
    /**
     * Checks whether or not the specified texture is a minable mountain texture.
     * @param texture texture to check
     * @returns true of the texture is a minable mountain texture, false otherwise.
     */
    virtual bool IsMineableMountain(const Texture& texture) const = 0;

    /**
     * Checks whether or not the specified texture is a grass texture.
     * @param texture texture to check
     * @returns true of the texture is a grass texture, false otherwise.
     */
    virtual bool IsGrassland(const Texture& texture) const = 0;
    
    /**
     * Checks whether or not the specified texture is a builable texture.
     * @param texture texture to check
     * @returns true of the texture is a builable texture, false otherwise.
     */
    virtual bool IsBuildable(const Texture& texture) const = 0;
    
    /**
     * Checks whether or not the specified texture is a water texture.
     * @param texture texture to check
     * @returns true of the texture is a water texture, false otherwise.
     */
    virtual bool IsWater(const Texture& texture) const = 0;
    
    /**
     * Water texture for selected landscape.
     */
    Texture water;
    
    /**
     * Grass with flowers for selected landscape
     */
    Texture flowers;
    
    /**
     * Coast texture for selected landscape.
     */
    Texture coast;
    
    /**
     * Mountain transition textures for selected landscape.
     * This texture represents the transition from greenland to mountain.
     */
    Texture mountainTransition;
};

class RttrTextureMapping : public TextureMapping_
{
private:
    
    WorldDescription& worldDesc_;
    
    Textures terrains_;
    Textures coast_;
    Textures grassland_;
    Textures mountain_;

    /**
     * Finds all terrains fulfilling the specified predicate.
     * @param predicate condition the terrain has to fulfill
     * @returns all terrains fulfilling the predicate.
     */
    template<class T_Predicate>
    inline Textures FindTerrains(T_Predicate predicate) const
    {
        Textures result;
        
        for (Texture t : terrains_)
        {
            if (predicate(worldDesc_.get(t)))
            {
                result.push_back(t);
            }
        }
        return result;
    }
    
    /**
     * Initializes the terrain member variables for later usage.
     * @param landscape type of landscape to load terrain for
     */
    void InitializeTerrains(DescIdx<LandscapeDesc> landscape);

    /**
     * Gets the actual terrain description from the specified index.
     * @param texture texture to unwrap
     * @returns the terrain description for the texture.
     */
    TerrainDesc Unwrap(const Texture& texture) const;
    
public:
    
    /**
     * Creates a new TextureMapping instance for the specified landscape type.
     * @param worldDesc description of the world
     * @param landscape description of the type of landscape used for texture mapping
     */
    RttrTextureMapping(WorldDescription& worldDesc, DescIdx<LandscapeDesc> landscape);
    
    Textures MapHeightsToTerrains(Height peak, Height sea, Height mountains);
    Texture GetCoastTerrain(int distance = 0) const;
    int GetHumidity(const Texture& texture) const;
    bool IsCoast(const Texture& texture, int distance = 0) const;
    bool IsMountain(const Texture& texture) const;
    bool IsMineableMountain(const Texture& texture) const;
    bool IsGrassland(const Texture& texture) const;
    bool IsBuildable(const Texture& texture) const;
    bool IsWater(const Texture& texture) const;
};

}}

#endif // TextureMapping_h__
