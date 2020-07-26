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

#ifndef Resources_h__
#define Resources_h__

#include "mapGenerator/Map.h"
#include "mapGenerator/MapSettings.h"
#include "mapGenerator/RandomUtility.h"

namespace rttr { namespace mapGenerator {

    /**
     * Custom representation of a tree specifically for tree placement on random maps.
     * Each instance of a tree represents a different type of tree and can vary in animation state,
     * represented by various object indices per tree.
     */
    class Tree
    {
    public:
        uint8_t type;
        uint8_t index;
        Tree(uint8_t objectType, uint8_t objectIndex) : type(objectType), index(objectIndex) {}
    };

    std::vector<Tree> CreateTrees(const TextureMap& textures);

    void AddObjects(Map& map, RandomUtility& rnd);

    void AddResources(Map& map, RandomUtility& rnd, const MapSettings& settings);

}} // namespace rttr::mapGenerator

#endif // Resources_h__
