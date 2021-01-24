// Copyright (c) 2017 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

        Tree pineApple(libsiedler2::OI_Palm, libsiedler2::OT_Tree1_Begin);
        Tree palm1(libsiedler2::OI_TreeOrPalm, libsiedler2::OT_Palm_Begin);
        Tree palm2(libsiedler2::OI_Palm, libsiedler2::OT_TreeOrPalm_Begin);
        Tree fir(libsiedler2::OI_Palm + 1, libsiedler2::OT_TreeOrPalm_Begin);
        Tree oak(libsiedler2::OI_TreeOrPalm, libsiedler2::OT_Tree2_Begin);
        Tree birch(libsiedler2::OI_TreeOrPalm, libsiedler2::OT_Tree1_Begin);
        Tree cherry(libsiedler2::OI_Palm, libsiedler2::OT_Palm_Begin);
        Tree pine(libsiedler2::OI_TreeOrPalm, libsiedler2::OT_TreeOrPalm_Begin);
        Tree cypress(libsiedler2::OI_Palm, libsiedler2::OT_Tree2_Begin);

        if(landscapeId == 0x0) // greenland
        {
            return {pineApple, palm1,  palm2, cypress, oak,  birch, oak,  cherry, oak,
                    birch,     cherry, oak,   birch,   pine, birch, pine, fir};
        }

        if(landscapeId == 0x1) // wasteland
        {
            return {pineApple, palm1,  palm2, cypress, oak,  birch, oak,  cherry, oak,
                    birch,     cherry, oak,   birch,   pine, birch, pine, fir};
        }

        if(landscapeId == 0x2) // winter
        {
            return {fir, oak, birch, cherry, pine, oak, birch, cherry, pine};
        }

        throw std::invalid_argument("invalid landscape type");
    }

    void AddObjects(Map& map, RandomUtility& rnd)
    {
        std::set<MapPoint, MapPointLess> excludedArea;
        NodeMapBase<unsigned> probabilities;
        probabilities.Resize(map.size, 0u);

        // Do not allow to place trees/stone piles nearby head quarters or
        // harbor positions to avoid inaccessible harbors or invalid player positions.

        const auto harborOrHeadquarter = [&map](const MapPoint& pt) {
            return helpers::contains(map.hqPositions, pt)
                   || helpers::contains_if(map.harbors, [pt](const Triangle& tr) { return tr.position == pt; });
        };

        RTTR_FOREACH_PT(MapPoint, map.size)
        {
            if(harborOrHeadquarter(pt))
            {
                auto suroundingArea = map.textures.GetPointsInRadiusWithCenter(pt, 5);
                excludedArea.insert(suroundingArea.begin(), suroundingArea.end());
            } else if(map.textureMap.Any(pt, IsSnowOrLava))
            {
                excludedArea.insert(pt);
            }
        }

        const auto distanceToExcludedArea = DistancesTo(excludedArea, map.size);

        // 1) compute maximum water distance until mountain area
        // 2) probabilities
        // 2a) non-mountains: distance to water
        // 2b) mountains: max prob - distance to non-mountain
        // 2c) forbidden area: 0
        // 3) tree type
        // 3a) non-mountains: distance to water
        // 3b) mountains: last element of trees

        auto& textureMap = map.textureMap;
        const auto mountain = [&textureMap](const MapPoint& pt) { return textureMap.Any(pt, IsMountainOrSnowOrLava); };
        const auto mountainDistance = DistancesTo(map.size, mountain);

        const auto water = [&textureMap](const MapPoint& pt) { return textureMap.Any(pt, IsWater); };
        auto waterDistance = DistancesTo(map.size, water);

        RTTR_FOREACH_PT(MapPoint, map.size)
        {
            // work around to avoid mountains to be considered for the total range of
            // distance-to-water values
            if(waterDistance[pt] > 0 && mountainDistance[pt] == 0)
            {
                waterDistance[pt] = 1;
            }
        }

        auto range = GetRange(waterDistance);
        auto probRange = ValueRange<unsigned>(15, 40);
        auto probDiff = probRange.GetDifference();

        const auto mountainFoot = [&map](const MapPoint& pt) {
            return !map.textureMap.All(pt, IsMountainOrSnowOrLava);
        };
        const auto mountainDepth = DistancesTo(map.size, mountainFoot);
        const auto mountainRange = GetRange(mountainDepth);

        RTTR_FOREACH_PT(MapPoint, map.size)
        {
            if(waterDistance[pt] > 0 && distanceToExcludedArea[pt] > 0)
            {
                if(mountainDistance[pt] > 0)
                {
                    auto prob = MapValueToIndex(waterDistance[pt], range, probDiff);
                    probabilities[pt] = prob + probRange.minimum;
                } else
                {
                    auto diff = mountainRange.maximum - mountainDepth[pt];
                    auto prob = MapValueToIndex(diff, mountainRange, probDiff);
                    probabilities[pt] = prob + probRange.minimum;
                }
            } else
            {
                probabilities[pt] = 0;
            }
        }

        auto trees = CreateTrees(map.textureMap);
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
            auto treeProb = probabilities[pt];

            if(treeProb > 0 && rnd.ByChance(treeProb))
            {
                auto tree = treeForPoint(pt);

                map.objectInfos[pt] = tree.type;
                map.objectTypes[pt] = tree.index + rnd.RandomValue(0, 7);
            }

            auto graniteProb = probabilities[pt] / 4;

            if(graniteProb > 0 && rnd.ByChance(graniteProb))
            {
                map.objectInfos[pt] = rnd.ByChance(50) ? 0xCC : 0xCD;
                map.objectTypes[pt] = rnd.RandomValue(1, 6);
            }
        }
    }

    void AddResources(Map& map, RandomUtility& rnd, const MapSettings& settings)
    {
        const auto& textures = map.textureMap;
        auto& resources = map.resources;
        int total = settings.ratioCoal + settings.ratioGold + settings.ratioIron + settings.ratioGranite;

        RTTR_FOREACH_PT(MapPoint, map.size)
        {
            if(textures.All(pt, IsMinableMountain))
            {
                int randomNumber = rnd.RandomValue(1, total);
                int ratio = settings.ratioGold;

                if(randomNumber < ratio)
                {
                    resources[pt] = libsiedler2::R_Gold + rnd.RandomValue(0, 8);
                    continue;
                }

                ratio += settings.ratioCoal;
                if(randomNumber < ratio)
                {
                    resources[pt] = libsiedler2::R_Coal + rnd.RandomValue(0, 8);
                    continue;
                }

                ratio += settings.ratioIron;
                if(randomNumber < ratio)
                {
                    resources[pt] = libsiedler2::R_Iron + rnd.RandomValue(0, 8);
                    continue;
                }

                ratio += settings.ratioGranite;
                if(randomNumber < ratio)
                {
                    resources[pt] = libsiedler2::R_Granite + rnd.RandomValue(0, 8);
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

    void AddAnimals(Map& map, RandomUtility& rnd)
    {
        std::vector<libsiedler2::Animal> landAnimals{libsiedler2::Animal::Rabbit, libsiedler2::Animal::Fox,
                                                     libsiedler2::Animal::Stag, libsiedler2::Animal::Deer,
                                                     libsiedler2::Animal::Sheep};
        const auto& textures = map.textureMap;
        RTTR_FOREACH_PT(MapPoint, map.size)
        {
            if(rnd.ByChance(3))
            {
                if(textures.All(pt, IsWater))
                {
                    map.animals[pt] = libsiedler2::Animal::Duck;
                } else if(textures.All(pt, IsLand))
                {
                    map.animals[pt] = rnd.RandomItem(landAnimals);
                }
            }
        }
    }

}} // namespace rttr::mapGenerator
