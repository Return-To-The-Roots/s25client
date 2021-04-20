// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "helpers/containerUtils.h"
#include "mapGenerator/Algorithms.h"
#include "mapGenerator/NodeMapUtilities.h"
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
        const DescIdx<LandscapeDesc> landscape_;

        std::vector<DescIdx<TerrainDesc>> terrains_;

    public:
        TextureOperator(const WorldDescription& worldDesc, const DescIdx<LandscapeDesc> landscape)
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

        inline uint8_t GetTextureId(DescIdx<TerrainDesc> texture) const { return worldDesc_.get(texture).s2Id; }

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
        inline bool Check(DescIdx<TerrainDesc> texture, T_Predicate predicate) const
        {
            return predicate(worldDesc_.get(texture));
        }
    };

    class TextureMap : public TextureOperator
    {
    public:
        NodeMapBase<TexturePair> textures_;

        TextureMap(const WorldDescription& worldDesc, DescIdx<LandscapeDesc> landscape, const MapExtent& size)
            : TextureOperator(worldDesc, landscape)
        {
            textures_.Resize(size);
        }

        template<class T_Predicate>
        bool Check(const Triangle& triangle, T_Predicate predicate) const
        {
            if(triangle.rsu)
            {
                return TextureOperator::Check(textures_[triangle.position].rsu, predicate);
            } else
            {
                return TextureOperator::Check(textures_[triangle.position].lsd, predicate);
            }
        }

        template<class T_Predicate>
        bool Any(const MapPoint& point, T_Predicate predicate) const
        {
            auto condition = [this, &predicate](auto triangle) { return this->Check(triangle, predicate); };
            return helpers::contains_if(GetTriangles(point, textures_.GetSize()), condition);
        }

        template<class T_Predicate>
        bool All(const MapPoint& point, T_Predicate predicate) const
        {
            auto condition = [this, &predicate](auto triangle) { return !this->Check(triangle, predicate); };
            return !helpers::contains_if(GetTriangles(point, textures_.GetSize()), condition);
        }

        /**
         * Updates the texture for the specified triangle.
         *
         * @param triangle triangle to update
         * @param texture texture to place on the triangle
         */
        void Set(const Triangle& triangle, DescIdx<TerrainDesc> texture);

        /**
         * Updates all textures around the specified node.
         *
         * @param pt position of the node
         * @param texture texture to place
         */
        void Set(const MapPoint& pt, DescIdx<TerrainDesc> texture);

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
        NodeMapBase<uint8_t>& z_;
        NodeMapBase<TexturePair>& textures_;
        TextureMap& textureMap_;
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

        /**
         * Creates swamp transitions between water and mountain textures.
         *
         * @param transitions all points surounded by mountain and water textures
         */
        void ApplyMountainWaterTransitions(const std::vector<MapPoint>& transitions);

    public:
        Texturizer(NodeMapBase<uint8_t>& z, NodeMapBase<TexturePair>& textures, TextureMap& textureMap)
            : z_(z), textures_(textures), textureMap_(textureMap)
        {
            seaLevel_ = GetRange(z).minimum;
        }

        /**
         * Adds missing textures based on the height map and the specified sea- and mountain levels.
         *
         * @param mountainLevel minimum z-value defining the starting height for mountains
         * @param coastline size of the coastline
         */
        void AddTextures(unsigned mountainLevel, unsigned coastline);
    };

    /**
     * Replaces textures around the specified map point with the specified texture if the current texture is not part
     * of the excluded textures.
     *
     * @param textures textures to replace
     * @param point all triangles around this point are checked and textures being replaced
     * @param texture new texture to replace current textures with
     * @param excluded textures which are excluded from replacement
     */
    void ReplaceTextureForPoint(NodeMapBase<TexturePair>& textures, const MapPoint& point, DescIdx<TerrainDesc> texture,
                                const std::set<DescIdx<TerrainDesc>>& excluded);

    /**
     * Replaces textures for all nodes of the map and all neighboring nodes within the specified radius by the
     * specified texture. Triangles of nodes which contain exlcuded textures are being skipped.
     *
     * @param textures textures to replace
     * @param radius radius around specified nodes to also consider for texture replacement
     * @param nodes initial nodes to consider for texture replacement. Other nodes which are considered for texture
     * replacement are added to
     * this vector.
     * @param texture texture to apply to nodes' triangles
     * @param excluded set of texture which shouldn't get replaced
     */
    void ReplaceTextures(NodeMapBase<TexturePair>& textures, unsigned radius, std::set<MapPoint, MapPointLess>& nodes,
                         DescIdx<TerrainDesc> texture, const std::set<DescIdx<TerrainDesc>>& excluded);

}} // namespace rttr::mapGenerator
