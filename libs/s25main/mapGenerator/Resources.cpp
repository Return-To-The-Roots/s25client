// Copyright (c) 2017 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "mapGenerator/Resources.h"
#include "RttrForeachPt.h"
#include "helpers/containerUtils.h"
#include "mapGenerator/Algorithms.h"
#include "mapGenerator/Terrain.h"
#include "mapGenerator/TextureHelper.h"

namespace rttr { namespace mapGenerator {

    std::vector<Tree> CreateTrees(const TextureMap& textures)
    {
        const auto landscapeId = textures.GetLandscapeId();

        Tree pineApple(197, 0x70);
        Tree palm1(196, 0xF0);
        Tree palm2(197, 0x30);
        Tree fir(198, 0x30);
        Tree oak(196, 0xB0);
        Tree birch(196, 0x70);
        Tree cherry(197, 0xF0);
        Tree pine(196, 0x30);
        Tree cypress(197, 0xB0);

        if(landscapeId == 0x0) // greenland
        {
            return {pineApple, palm1, palm2, cypress, oak, birch, oak, cherry, oak, birch, cherry, oak, birch, pine, birch, pine, fir};
        }

        if(landscapeId == 0x1) // wasteland
        {
            return {pineApple, palm1, palm2, cypress, oak, birch, oak, cherry, oak, birch, cherry, oak, birch, pine, birch, pine, fir};
        }

        if(landscapeId == 0x2) // winter
        {
            return {fir, oak, birch, cherry, pine, cypress, oak, birch, cherry, pine, cypress};
        }

        throw std::invalid_argument("invalid landscape type");
    }

    void AddObjects(Map& map, RandomUtility& rnd)
    {
        ValueMap<unsigned> probabilities(map.size, 0u);

        // Do not allow to place trees on head quarters or harbor positions
        // to avoid inaccessible harbors or invalid player positions.

        auto isForbidden = [&map](const MapPoint& pt) {
            return map.objectInfos[pt] == libsiedler2::OI_HeadquarterMask
                   || helpers::contains_if(map.harbors, [pt](const Triangle& tr) { return tr.position == pt; });
        };

        auto isForbiddenArea = [&isForbidden, &map](const MapPoint& pt) {
            return helpers::contains_if(map.textures.GetPointsInRadiusWithCenter(pt, 5), isForbidden);
        };

        auto distanceToForbiddenArea = Distances(map.size, isForbiddenArea);

        // 1) compute maximum water distance until mountain area
        // 2) probabilities
        // 2a) non-mountains: distance to water
        // 2b) mountains: max prob - distance to non-mountain
        // 3c) forbidden area: 0
        // 3) tree type
        // 3a) non-mountains: distance to water
        // 3b) mountains: last element of trees

        auto mountainDistance = Distances(map.size, [&map](const MapPoint& pt) { return map.textures.Any(pt, IsMountainOrSnowOrLava); });

        auto waterDistance = Distances(map.size, [&map](const MapPoint& pt) { return map.textures.Any(pt, IsWater); });

        RTTR_FOREACH_PT(MapPoint, map.size)
        {
            if(waterDistance[pt] > 0 && mountainDistance[pt] == 0)
            {
                waterDistance[pt] = 1;
            }
        }

        auto range = waterDistance.GetRange();
        auto probRange = ValueRange<unsigned>(15, 40);
        auto probDiff = probRange.GetDifference();

        auto mountainDepth = Distances(map.size, [&map](const MapPoint& pt) { return !map.textures.All(pt, IsMountainOrSnowOrLava); });

        auto maximumMountainDepth = mountainDepth.GetMaximum();
        auto mountainRange = mountainDepth.GetRange();

        RTTR_FOREACH_PT(MapPoint, map.size)
        {
            const unsigned waterDist = waterDistance[pt];
            const unsigned mountainDist = mountainDistance[pt];

            if(waterDist > 0 && distanceToForbiddenArea[pt] > 0)
            {
                if(mountainDist > 0)
                {
                    auto prob = MapValueToIndex(waterDist, range, probDiff);
                    probabilities[pt] = prob + probRange.minimum;
                } else
                {
                    auto diff = maximumMountainDepth - mountainDepth[pt];
                    auto prob = MapValueToIndex(diff, mountainRange, probDiff);
                    probabilities[pt] = prob + probRange.minimum;
                }
            } else
            {
                probabilities[pt] = 0;
            }
        }

        auto trees = CreateTrees(map.textures);
        auto treeForPoint = [&mountainDistance, &waterDistance, &range, &trees](const MapPoint& pt) {
            if(mountainDistance[pt] == 0)
            {
                return trees.back();
            }
            auto index = MapValueToIndex(waterDistance[pt], range, trees.size());
            return trees[index];
        };

        RTTR_FOREACH_PT(MapPoint, map.size)
        {
            auto prob = probabilities[pt];

            if(prob > 0 && rnd.ByChance(prob))
            {
                auto tree = treeForPoint(pt);

                map.objectInfos[pt] = tree.type;
                map.objectTypes[pt] = tree.index + rnd.Rand(0, 7);
            }
        }
    }

    void AddResources(Map& map, RandomUtility& rnd, const MapSettings& settings)
    {
        const auto& textures = map.textures;
        auto& resources = map.resources;

        RTTR_FOREACH_PT(MapPoint, map.size)
        {
            if(textures.All(pt, IsMinableMountain))
            {
                int randomNumber = rnd.Rand(1, 100);
                int ratio = settings.ratioGold;

                if(randomNumber < ratio)
                {
                    resources[pt] = libsiedler2::R_Gold + rnd.Rand(0, 8);
                    continue;
                }

                ratio += settings.ratioCoal;
                if(randomNumber < ratio)
                {
                    resources[pt] = libsiedler2::R_Coal + rnd.Rand(0, 8);
                    continue;
                }

                ratio += settings.ratioIron;
                if(randomNumber < ratio)
                {
                    resources[pt] = libsiedler2::R_Iron + rnd.Rand(0, 8);
                    continue;
                }

                ratio += settings.ratioGranite;
                if(randomNumber < ratio)
                {
                    resources[pt] = libsiedler2::R_Granite + rnd.Rand(0, 8);
                }
            } else if(textures.All(pt, IsWater))
            {
                resources[pt] = libsiedler2::R_Fish;
            } else
            {
                resources[pt] = libsiedler2::R_Water;
            }
        }
    }

    /*
    Animals GetAnimals(const Texture& texture, const TextureMapping_& mapping)
    {
        if (mapping.water == texture)
        {
            return { A_Duck, A_Duck2 };
        }

        if (mapping.IsGrassland(texture))
        {
            return { A_Rabbit, A_Sheep, A_Deer, A_Fox };
        }

        if (mapping.IsMountain(texture))
        {
            return { A_Deer, A_Fox };
        }

        if (mapping.IsCoast(texture))
        {
            return { A_Fox };
        }

        if (mapping.IsCoast(texture, 1))
        {
            return { A_Fox, A_Rabbit };
        }

        if (mapping.IsCoast(texture, 2))
        {
            return { A_Rabbit, A_Sheep };
        }

        return {};
    }

    int GetStoneProbability(const Texture& texture, const TextureMapping_& mapping, int freeZoneDistance)
    {
        if (freeZoneDistance < 10 || texture == mapping.water)
        {
            return 0;
        }

        return 2;
    }

    int GetAnimalProbability(const Texture& texture, const TextureMapping_& mapping)
    {
        return mapping.GetHumidity(texture) / 10 + 2;
    }

    void PlaceStones(Map_& map,
                     RandomUtility& rnd,
                     const TextureMapping_& mapping,
                     const std::vector<int>& freeZone)
    {
        auto size = map.size.x * map.size.y;
        auto textures = map.textureRsu;
        auto stoneTypes = std::vector<int>{ 0xCC, 0xCD };

        for (int i = 0; i < size; ++i)
        {
            auto prob = GetStoneProbability(textures[i], mapping, freeZone[i]);

            if (rnd.ByChance(prob))
            {
                auto position = GridPosition(i, map.size);
                auto neighbors = GridCollect(position, map.size, rnd.DRand(0.0, 2.0));

                for (auto neighbor : neighbors)
                {
                    auto index = neighbor.x + neighbor.y * map.size.x;
                    if (mapping.IsBuildable(textures[index]))
                    {
                        auto type = rnd.Index(stoneTypes.size());

                        map.objectType[index] = rnd.Rand(1, 6);
                        map.objectInfo[index] = stoneTypes[type];
                    }
                }
            }
        }
    }

    void PlaceAnimals(Map_& map, RandomUtility& rnd, const TextureMapping_& mapping)
    {
        auto size = map.size.x * map.size.y;
        auto textures = map.textureRsu;
        auto& animalsOnMap = map.animal;

        Texture texture;

        for (int i = 0; i < size; ++i)
        {
            texture = textures[i];

            auto animals = GetAnimals(texture, mapping);
            auto prob = GetAnimalProbability(texture, mapping);

            if (rnd.ByChance(prob) && !animals.empty())
            {
                animalsOnMap[i] = animals[rnd.Index(animals.size())];
            }
        }
    }

    void PlaceResources(Map_& map,
                        RandomUtility& rnd,
                        const TextureMapping_& mapping,
                        const MapSettings& settings)
    {
        auto isHqOrHarbor = [map] (int index) { return IsHeadQuarterOrHarborPosition(map, index); };
        const auto& freeZone = DistanceByProperty(map.size, isHqOrHarbor);

        PlaceTrees(map, rnd, mapping, freeZone);
        PlaceStones(map, rnd, mapping, freeZone);
        PlaceMinesAndFish(map, rnd, mapping, settings);
        PlaceAnimals(map, rnd, mapping);
    }
    */

}} // namespace rttr::mapGenerator
