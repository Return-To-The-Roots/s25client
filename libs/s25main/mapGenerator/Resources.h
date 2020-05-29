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

    /*
    bool IsHeadQuarterOrHarborPosition(const Map_& map, int index);

    std::vector<int> GetTreeTypes(const Texture& texture, const TextureMapping_& mapping);

    std::vector<int> GetTreeInfos(const Texture& texture, const TextureMapping_& mapping);

    std::vector<libsiedler2::Animal> GetAnimals(const Texture& texture, const TextureMapping_& mapping);

    int GetTreeProbability(const Texture& texture, const TextureMapping_& mapping, int freeZoneDistance);

    int GetStoneProbability(const Texture& texture, const TextureMapping_& mapping, int freeZoneDistance);

    int GetAnimalProbability(const Texture& texture, const TextureMapping_& mapping);

    void PlaceTrees(Map_& map,
                    RandomUtility& rnd,
                    const TextureMapping_& mapping,
                    const std::vector<int>& freeZone);

    void PlaceStones(Map_& map,
                     RandomUtility& rnd,
                     const TextureMapping_& mapping,
                     const std::vector<int>& freeZone);

    void PlaceMinesAndFish(Map_& map,
                           RandomUtility& rnd,
                           const TextureMapping_& mapping,
                           const MapSettings& settings);

    void PlaceAnimals(Map_& map, RandomUtility& rnd, const TextureMapping_& mapping);

    void PlaceResources(Map_& map,
                        RandomUtility& rnd,
                        const TextureMapping_& mapping,
                        const MapSettings& settings);
    */

}} // namespace rttr::mapGenerator

#endif // Resources_h__
