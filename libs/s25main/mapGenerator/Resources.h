// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "mapGenerator/Map.h"
#include "mapGenerator/MapSettings.h"
#include "mapGenerator/RandomUtility.h"

namespace rttr::mapGenerator {

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

void AddObjects(Map& map, RandomUtility& rnd, const MapSettings& settings);

void AddResources(Map& map, RandomUtility& rnd, const MapSettings& settings);

void AddAnimals(Map& map, RandomUtility& rnd);

} // namespace rttr::mapGenerator
