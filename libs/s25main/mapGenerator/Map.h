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

#ifndef Map_h__
#define Map_h__

#include "mapGenerator/TextureMapping.h"
#include "mapGenerator/Triangles.h"
#include "mapGenerator/RandomUtility.h"
#include "mapGenerator/ValueMap.h"
#include "gameData/WorldDescription.h"
#include "helpers/containerUtils.h"
#include "libsiedler2/archives.h"

#include <string>
#include <set>

namespace rttr {
namespace mapGenerator {

class Map
{
private:
    
    TextureMapping& mapping_;
    std::vector<MapPoint> hqPositions_;
    std::vector<Triangle> harbors_;
    DescIdx<LandscapeDesc> landscape_;
    Textures terrains_;

public:
    
    ValueMap<Height> z;
    NodeMapBase<TexturePair> textures;
    NodeMapBase<libsiedler2::ObjectInfo> objectInfos;
    NodeMapBase<libsiedler2::ObjectType> objectTypes;
    NodeMapBase<libsiedler2::Resource> resources;
    NodeMapBase<libsiedler2::Animal> animals;
    
    const std::string name;
    const std::string author;
    const Range height;
    const uint8_t players;
    const MapExtent size;

    Map(TextureMapping& mapping, DescIdx<LandscapeDesc> landscape, uint8_t players, const MapExtent& size);

    /**
     * Marks a triangle as harbor position.
     * @param triangle triangle to mark as harbor position
     */
    void MarkAsHarbor(const Triangle& triangle);
    
    /**
     * Marks the position as HQ position if set to a valid position, otherwise unmarks previously marked position.
     * @param position position to mark or unmark as HQ position
     * @param index index of the player
     */
    void MarkAsHeadQuarter(const MapPoint& position, int index);

    /**
     * Creates a new archiv for this map.
     * @return a new archiv containing the information of this map
     */
    libsiedler2::Archiv CreateArchiv();

    /**
     * Checks if the specified triangle contains a texture holding the specified predicate.
     * @param triangle triangle to check
     * @param predicate condition the texture has to fulfill
     * @returns true if the predicate is fulfilled, false otherwise.
     */
    template<class T_Predicate>
    bool CheckTexture(const Triangle& triangle, T_Predicate predicate) const
    {
        if (triangle.rsu)
        {
            return mapping_.CheckTexture(textures[triangle.position].rsu, predicate);
        }
        else
        {
            return mapping_.CheckTexture(textures[triangle.position].lsd, predicate);
        }
    }
    
    /**
     * Checks if any of the textures around the specified point fulfill the  specified predicate.
     * @param point point on the map to check
     * @param predicate condition the texture has to fulfill
     * @returns true if the predicate is fulfilled, false otherwise.
     */
    template<class T_Predicate>
    bool AnyTexture(const MapPoint& point, T_Predicate predicate) const
    {
        auto condition = [this, &predicate](auto triangle) {
            return this->CheckTexture(triangle, predicate);
        };
        
        return helpers::contains_if(GetTriangles(point, size), condition);
    }
    
    /**
     * Checks if all of the textures around the specified point fulfill the  specified predicate.
     * @param point point on the map to check
     * @param predicate condition the texture has to fulfill
     * @returns true if the predicate is fulfilled, false otherwise.
     */
    template<class T_Predicate>
    bool AllTextures(const MapPoint& point, T_Predicate predicate) const
    {
        auto condition = [this, &predicate](auto triangle) {
            return !this->CheckTexture(triangle, predicate);
        };
        
        return !helpers::contains_if(GetTriangles(point, size), condition);
    }
    
    /**
     * Finds the first texture fulfilling the specified predicate.
     * @param predicate condition the texture has to fulfill
     * @returns a texture which fulfills the predicate or INVALID.
     */
    template<class T_Predicate>
    inline Texture FindTexture(T_Predicate predicate) const
    {
        for (auto texture : terrains_)
        {
            if (mapping_.CheckTexture(texture, predicate))
            {
                return texture;
            }
        }
        return Texture(Texture::INVALID);
    }
};

// OLD WORLD

class Map_
{
private:
    
    WorldDescription& worldDesc_;
    
public:

    Map_(WorldDescription& worldDesc, const MapExtent& size, int minHeight = 0, int maxHeight = 44);
    
    Height sea;
    Height mountains;
    
    Range height;
    DescIdx<LandscapeDesc> landscape;
    MapExtent size;
    std::string name;
    std::string author;
    uint8_t numPlayers;
    std::vector<MapPoint> hqPositions;
    std::set<int> harborsRsu;
    std::set<int> harborsLsd;
    std::vector<uint8_t> z;
    std::vector<uint8_t> road;
    std::vector<uint8_t> animal;
    std::vector<uint8_t> unknown1;
    std::vector<uint8_t> build;
    std::vector<uint8_t> unknown2;
    std::vector<uint8_t> unknown3;
    std::vector<uint8_t> resource;
    std::vector<uint8_t> shading;
    std::vector<uint8_t> unknown5;
    Textures textureRsu;
    Textures textureLsd;
    std::vector<uint8_t> objectType;
    std::vector<uint8_t> objectInfo;

    /**
     * Creates a new archiv for this map.
     * @return a new archiv containing the information of this map
     */
    libsiedler2::Archiv CreateArchiv();
};

}}

#endif // Map_h__
