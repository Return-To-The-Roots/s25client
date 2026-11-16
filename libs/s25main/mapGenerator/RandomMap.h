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
unsigned getSmoothRadius(const MapExtent& size);
unsigned getSmoothIterations(const MapExtent& size);

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
