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

unsigned GetMaximumHeight(const MapExtent& size);
unsigned GetCoastline(const MapExtent& size);
unsigned GetIslandRadius(const MapExtent& size);
unsigned GetIslandSize(const MapExtent& size);
unsigned getMaxMountainSize(const MapExtent& size);

/**
 * Computes the distance to water and mountains up to which land stays free.
 *
 * @param mountains desired amount of mountains, deserts and swamps in percent. 50 keeps the mountain distance while
 * higher values reduce it down to the space required by headquarters.
 */
unsigned getMinFreeLand(unsigned mountainDistance, unsigned mountains);

unsigned GetSmoothRadius(const MapExtent& size);
unsigned GetSmoothIterations(const MapExtent& size);

void SmoothHeightMap(NodeMapBase<uint8_t>& z, const ValueRange<uint8_t>& range);

class RandomMap
{
private:
    RandomUtility& rnd_;
    Map& map_;
    Texturizer texturizer_;
    MapSettings settings_;

    std::vector<River> CreateRivers(MapPoint source = MapPoint::Invalid());
    void CreateFreeIslands(unsigned waterNodes);
    void fillFreeLand(unsigned mountainLevel);
    void addTextures(unsigned mountainLevel);
    void CreateMixedMap();
    void CreateLandMap();
    void CreateWaterMap();

public:
    RandomMap(RandomUtility& rnd, Map& map);
    void Create(const MapSettings& settings);
};

Map GenerateRandomMap(RandomUtility& rnd, const WorldDescription& worldDesc, const MapSettings& settings);
void CreateRandomMap(const boost::filesystem::path& filePath, const MapSettings& settings);

} // namespace rttr::mapGenerator
