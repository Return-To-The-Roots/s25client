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
unsigned GetMaxMountainSize(const MapExtent& size);

/**
 * Computes the amount of free land to keep around water and mountains.
 *
 * @param mountainDistance desired distance of headquarters to mountains
 * @param mountains desired amount of mountains, deserts and swamps in percentage (between 0 and 100) - an amount of
 * 50 keeps the desired distance available while higher values reduce the free land down to the space required by
 * headquarters
 *
 * @returns the distance to water and mountains up to which land stays free.
 */
unsigned GetMinFreeLand(unsigned mountainDistance, unsigned mountains);

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
    void FillFreeLand(unsigned mountainLevel);
    void AddTextures(unsigned mountainLevel);
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
