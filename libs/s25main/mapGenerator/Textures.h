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

#ifndef Textures_h__
#define Textures_h__

#include "helpers/containerUtils.h"
#include "mapGenerator/Algorithms.h"
#include "mapGenerator/Triangles.h"
#include "gameData/WorldDescription.h"

namespace rttr { namespace mapGenerator {

    struct TexturePair
    {
        DescIdx<TerrainDesc> rsu;
        DescIdx<TerrainDesc> lsd;

        TexturePair() : rsu(DescIdx<TerrainDesc>::INVALID), lsd(DescIdx<TerrainDesc>::INVALID) {}
        TexturePair(DescIdx<TerrainDesc> texture) : rsu(texture), lsd(texture) {}
    };

    class TextureOperator
    {
    private:
        const WorldDescription& worldDesc_;
        const DescIdx<LandscapeDesc>& landscape_;

        std::vector<DescIdx<TerrainDesc>> terrains_;

    public:
        TextureOperator(const WorldDescription& worldDesc, const DescIdx<LandscapeDesc>& landscape)
            : worldDesc_(worldDesc), landscape_(landscape)
        {
            for(DescIdx<TerrainDesc> t(0); t.value < worldDesc_.terrain.size(); t.value++)
            {
                if(worldDesc_.get(t).landscape == landscape)
                {
                    terrains_.push_back(t);
                }
            }
        }

        inline uint8_t GetTextureId(const DescIdx<TerrainDesc>& texture) const { return worldDesc_.get(texture).s2Id; }

        inline uint8_t GetLandscapeId() const { return worldDesc_.get(landscape_).s2Id; }

        template<class T_Predicate>
        inline DescIdx<TerrainDesc> Find(T_Predicate predicate) const
        {
            for(auto texture : terrains_)
            {
                if(predicate(worldDesc_.get(texture)))
                {
                    return texture;
                }
            }

            throw std::invalid_argument("no texture found for specified predicate");
        }

        template<class T_Predicate>
        inline std::vector<DescIdx<TerrainDesc>> FindAll(T_Predicate predicate) const
        {
            auto condition = [&predicate, this](const auto& texture) { return predicate(worldDesc_.get(texture)); };

            std::vector<DescIdx<TerrainDesc>> textures;
            std::copy_if(terrains_.begin(), terrains_.end(), std::back_inserter(textures), condition);

            return textures;
        }

        template<class T_SortBy>
        inline void Sort(std::vector<DescIdx<TerrainDesc>>& textures, T_SortBy sortBy) const
        {
            auto lessThan = [this, &sortBy](const auto& t1, const auto& t2) {
                return sortBy(worldDesc_.get(t1)) < sortBy(worldDesc_.get(t2));
            };

            std::sort(textures.begin(), textures.end(), lessThan);
        }

        template<class T_Predicate>
        inline bool Check(const DescIdx<TerrainDesc>& texture, T_Predicate predicate) const
        {
            return predicate(worldDesc_.get(texture));
        }
    };

    class TextureMap : public NodeMapBase<TexturePair>, public TextureOperator
    {
    public:
        TextureMap(const WorldDescription& worldDesc, const DescIdx<LandscapeDesc>& landscape) : TextureOperator(worldDesc, landscape) {}

        template<class T_Predicate>
        bool Check(const Triangle& triangle, T_Predicate predicate) const
        {
            if(triangle.rsu)
            {
                return TextureOperator::Check(nodes[GetIdx(triangle.position)].rsu, predicate);
            } else
            {
                return TextureOperator::Check(nodes[GetIdx(triangle.position)].lsd, predicate);
            }
        }

        template<class T_Predicate>
        bool Any(const MapPoint& point, T_Predicate predicate) const
        {
            auto condition = [this, &predicate](auto triangle) { return this->Check(triangle, predicate); };

            return helpers::contains_if(GetTriangles(point, this->GetSize()), condition);
        }

        template<class T_Predicate>
        bool All(const MapPoint& point, T_Predicate predicate) const
        {
            auto condition = [this, &predicate](auto triangle) { return !this->Check(triangle, predicate); };

            return !helpers::contains_if(GetTriangles(point, this->GetSize()), condition);
        }

        /**
         * Updates the texture for the specified triangle.
         *
         * @param triangle triangle to update
         * @param texture texture to place on the triangle
         */
        void Set(const Triangle& triangle, const DescIdx<TerrainDesc>& texture);

        /**
         * Updates all textures around the specified node.
         *
         * @param pt position of the node
         * @param texture texture to place
         */
        void Set(const MapPoint& pt, const DescIdx<TerrainDesc>& texture);

        /**
         * Gets the texture of the specified triangle.
         *
         * @param triangle reference to the triangle to find the texture for
         *
         * @returns the texture of the triangle.
         */
        DescIdx<TerrainDesc> Get(const Triangle& triangle) const;
    };

    class Texturizer
    {
    private:
        ValueMap<uint8_t>& z_;
        TextureMap& textures_;
        unsigned seaLevel_;

        /**
         * Creates a mapping from z-value to texture based on sea- and mountain level.
         *
         * @param mountainLevel minimum height mountains start from
         *
         * @returns a vector of textures with each index representing a z-value.
         */
        std::vector<DescIdx<TerrainDesc>> CreateTextureMapping(unsigned mountainLevel);

        /**
         * Adds textures based on z-values, sea- and mountain levels.
         *
         * @param mountainLevel minimum height mountains start from
         */
        void ApplyTexturingByHeightMap(unsigned mountainLevel);

        /**
         * Sets textures near the specified coast to ensure a smooth looking transition from water to buildable land.
         *
         * @param coast nodes to consider for re-texturing
         * @param width width (in node edges) for each coast transition texture
         */
        void ApplyCoastTexturing(const std::vector<MapPoint>& coast, unsigned width);

        /**
         * Creates smooth transitions between grass and mountain textures.
         *
         * @param mountainFoot all points on the map which are surounded by mountain and grass
         */
        void ApplyMountainTransitions(const std::vector<MapPoint>& mountainFoot);

    public:
        Texturizer(ValueMap<uint8_t>& z, TextureMap& textures) : z_(z), textures_(textures) { seaLevel_ = z.GetRange().minimum; }

        /**
         * Adds missing textures based on the height map and the specified sea- and mountain levels.
         *
         * @param mountainLevel minimum z-value defining the starting height for mountains
         * @param coastline size of the coastline
         */
        void AddTextures(unsigned mountainLevel, unsigned coastline);
    };

    /**
     * Replaces textures around the specified map point with the specified texture if the current texture is not part of the excluded
     * textures.
     *
     * @param map reference to the map to replace textures for
     * @param point all triangles around this point are checked and textures being replaced
     * @param texture new texture to replace current textures with
     * @param excluded textures which are excluded from replacement
     */
    void ReplaceTextureForPoint(NodeMapBase<TexturePair>& textures, const MapPoint& point, const DescIdx<TerrainDesc>& texture,
                                const std::set<DescIdx<TerrainDesc>>& excluded);

    /**
     * Replaces textures for all nodes of the map and all neighboring nodes within the specified radius by the specified texture.
     * Triangles of nodes which contain exlcuded textures are being skipped.
     *
     * @param map reference to the map to apply texture replacement to
     * @param radius radius around specified nodes to also consider for texture replacement
     * @param nodes initial nodes to consider for texture replacement. Other nodes which are considered for texture replacement are added to
     * this vector.
     * @param texture texture to apply to nodes' triangles
     * @param excluded set of texture which shouldn't get replaced
     */
    void ReplaceTextures(NodeMapBase<TexturePair>& textures, unsigned radius, std::set<MapPoint, MapPointLess>& nodes,
                         const DescIdx<TerrainDesc>& texture, const std::set<DescIdx<TerrainDesc>>& excluded);

}} // namespace rttr::mapGenerator

#endif // Textures_h__
