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

#pragma once

#include "mapGenerator/Map.h"
#include "mapGenerator/MapSettings.h"
#include "mapGenerator/RandomUtility.h"
#include "mapGenerator/Rivers.h"

#include <boost/filesystem/path.hpp>

namespace rttr { namespace mapGenerator {

    unsigned GetMaximumHeight(const MapExtent& size);
    unsigned GetCoastline(const MapExtent& size);
    unsigned GetIslandRadius(const MapExtent& size);
    unsigned GetIslandNodes(const MapExtent& size, unsigned waterNodes);
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

        std::vector<River> CreateRivers();
        void CreateMixedMap();
        void CreateLandMap();
        void CreateWaterMap();

    public:
        RandomMap(RandomUtility& rnd, Map& map);
        void Create(const MapSettings& settings);
    };

    Map GenerateRandomMap(RandomUtility& rnd, const WorldDescription& worldDesc, const MapSettings& settings);
    void CreateRandomMap(const boost::filesystem::path& filePath, const MapSettings& settings);

}} // namespace rttr::mapGenerator
