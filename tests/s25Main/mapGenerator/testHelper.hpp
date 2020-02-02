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

#include "mapGenerator/TextureMapping.h"
#include "helpers/containerUtils.h"
#include <stdexcept>

namespace rttr {
namespace mapGenerator {

// NEW WORLD

class MockTextureMapping : public TextureMapping
{
private:
    
    Textures textures_;

    const DescIdx<LandscapeDesc>& expectedLandscape_;

public:
    
    Textures expectedTextures;

    MockTextureMapping(const DescIdx<LandscapeDesc>& expectedLandscape) :
        expectedLandscape_(expectedLandscape)
    {
        
    }
    
    MockTextureMapping(const DescIdx<LandscapeDesc>& expectedLandscape,
                       const Textures& textures) :
        textures_(textures),
        expectedLandscape_(expectedLandscape)
    {
        
    }
    
    /**
     * Gets a list of all textures for the specified landscape.
     * @param landscape landscape description identifier
     * @returns a list of all textures available for the specified landscape.
     */
    Textures GetTextures(const DescIdx<LandscapeDesc>& landscape) const
    {
        if (landscape != expectedLandscape_)
        {
            throw "unexpected landscape";
        }
        
        return textures_;
    }
    
    /**
     * Checks whether or not the texture fulfills the specified predicate.
     * @param texture texture to check
     * @param predicate predicate to verify
     * @returns true if the texture fulfills the specified predicate, false otherwise.
     */
    bool CheckTexture(const Texture& texture, Predicate predicate) const
    {
        if (predicate) { }
        
        return helpers::contains(expectedTextures, texture);
    }
    
    /**
     * Translates the specified landscape description index into a S2 landscape index.
     * @param landscape landscape description index
     * @returns the S2 landscape index for the specified landscape.
     */
    uint8_t GetLandscapeIndex(const DescIdx<LandscapeDesc>& landscape) const
    {
        return landscape.value;
    }
    
    /**
     * Translates the specified terrain description index into a S2 terrain index.
     * @param terrain terrain description index
     * @returns the S2 terrain index for the specified terrain.
     */
    uint8_t GetTerrainIndex(const DescIdx<TerrainDesc>& terrain) const
    {
        return terrain.value;
    }
};

// OLD WORLD

class MockTextureMapping_ : public TextureMapping_
{
private:
    
    Textures grassland_;
    Textures mountain_;
    Textures terrains_;

public:

    Textures getCoastTerrain;
    Texture mountain;
    Texture grassland;
    Texture lava;
    
    MockTextureMapping_()
    {
        Texture texture1(0x1); // water
        Texture texture2(0x2); // coast
        Texture texture3(0x3); // coast, buildable
        Texture texture4(0x4); // coast, buildable
        Texture texture5(0x5); // grass, buildable
        Texture texture6(0x6); // flower, buildable
        Texture texture7(0x7); // transition, buildable
        Texture texture8(0x8); // mountain
        Texture texture9(0x9); // lava

        grassland_.push_back(texture5);
        grassland_.push_back(texture6);

        mountain_.push_back(texture7);
        mountain_.push_back(texture8);
        mountain_.push_back(texture9);

        this->water = texture1;
        this->coast = texture2;
        this->flowers = texture6;
        this->mountainTransition = texture7;

        grassland = texture5;
        mountain = texture8;
        lava = texture9;
        
        terrains_.push_back(texture1);
        terrains_.push_back(texture2);
        terrains_.push_back(texture3);
        terrains_.push_back(texture4);
        terrains_.push_back(texture5);
        terrains_.push_back(texture6);
        terrains_.push_back(texture7);
        terrains_.push_back(texture8);
        terrains_.push_back(texture9);

        getCoastTerrain.push_back(texture2);
        getCoastTerrain.push_back(texture3);
        getCoastTerrain.push_back(texture4);
    }
    
    Textures MapHeightsToTerrains(Height maximumHeight, Height sea, Height mountains)
    {
        Textures terrains(maximumHeight + 1);

        if (sea + 2 >= mountains)
        {
            throw std::invalid_argument("sea must be lower than mountains by more than 2");
        }
        
        if (sea > maximumHeight || mountains > maximumHeight)
        {
            throw std::invalid_argument("sea and mountains must be below maximum height");
        }
        
        for (Height z = 0; z <= maximumHeight; z++)
        {
            if (z <= sea)
            {
                terrains[z] = water;
            }
            
            if (z > sea && z < mountains)
            {
                int index = MapRangeToIndex(z, sea + 1, mountains - 1, grassland_.size());
                terrains[z] = grassland_[index];
            }
            
            if (z >= mountains)
            {
                int index = MapRangeToIndex(z, mountains, maximumHeight, mountain_.size());
                terrains[z] = mountain_[index];
            }
        }

        return terrains;
    }

    Texture GetCoastTerrain(int distance = 0) const
    {
        return getCoastTerrain[distance];
    }
    
    bool IsCoast(const Texture& texture, int distance = 0) const
    {
        return texture == GetCoastTerrain(distance);
    }
    
    bool IsMountain(const Texture& texture) const
    {
        return texture == mountain;
    }
    
    bool IsGrassland(const Texture& texture) const
    {
        return texture == grassland;
    }
    
    int GetHumidity(const Texture& texture) const
    {
        return texture.value;
    }
    
    bool IsMineableMountain(const Texture& texture) const
    {
        return IsMountain(texture);
    }
    
    bool IsBuildable(const Texture& texture) const
    {
        return
            texture == terrains_[2] ||
            texture == terrains_[3] ||
            texture == terrains_[4] ||
            texture == terrains_[5] ||
            texture == terrains_[6];
    }
    
    bool IsWater(const Texture& texture) const
    {
        return texture == water;
    }
};

}}
