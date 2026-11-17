// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "mapGenerator/Map.h"
#include "mapGenerator/MapSettings.h"
#include "mapGenerator/RandomUtility.h"
#include "mapGenerator/Rivers.h"

#include <boost/filesystem/path.hpp>

namespace rttr::mapGenerator {

unsigned getMaximumHeight(const MapExtent& size);
unsigned getCoastline(const MapExtent& size);
unsigned getIslandRadius(const MapExtent& size);
unsigned getIslandSize(const MapExtent& size);
unsigned getMaxMountainSize(const MapExtent& size);

/**
 * Computes the distance to water and mountains up to which land stays free.
 *
 * @param mountains desired amount of mountains, deserts and swamps in percent. 50 keeps the mountain distance while
 * higher values reduce it down to the space required by headquarters.
 */
unsigned getMinFreeLand(unsigned mountainDistance, unsigned mountains);

unsigned getSmoothRadius(const MapExtent& size);
unsigned getSmoothIterations(const MapExtent& size);

void smoothHeightMap(NodeMapBase<uint8_t>& z, const ValueRange<uint8_t>& range);

class RandomMap
{
private:
    RandomUtility& rnd_;
    Map& map_;
    Texturizer texturizer_;
    MapSettings settings_;

    std::vector<River> createRivers(MapPoint source = MapPoint::invalid());
    void createFreeIslands(unsigned waterNodes);
    void fillFreeLand(unsigned mountainLevel);
    void addTextures(unsigned mountainLevel);
    void createMixedMap();
    void createLandMap();
    void createWaterMap();

public:
    RandomMap(RandomUtility& rnd, Map& map);
    void create(const MapSettings& settings);
};

Map generateRandomMap(RandomUtility& rnd, const WorldDescription& worldDesc, const MapSettings& settings);
void createRandomMap(const boost::filesystem::path& filePath, const MapSettings& settings);

} // namespace rttr::mapGenerator
