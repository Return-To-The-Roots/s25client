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
#include "helpers/make_array.h"
#include "mapGenerator/Algorithms.h"
#include "mapGenerator/Terrain.h"
#include "mapGenerator/TextureHelper.h"

namespace rttr { namespace mapGenerator {

    std::vector<Tree> CreateTrees(const TextureMap& textures)
    {
        const auto landscapeId = textures.GetLandscapeId();

        Tree pineApple(libsiedler2::OT_Palm, libsiedler2::OI_Tree1_Begin);
        Tree palm1(libsiedler2::OT_TreeOrPalm, libsiedler2::OI_Palm_Begin);
        Tree palm2(libsiedler2::OT_Palm, libsiedler2::OI_TreeOrPalm_Begin);
        Tree fir(libsiedler2::OT_Palm + 1, libsiedler2::OI_TreeOrPalm_Begin);
        Tree oak(libsiedler2::OT_TreeOrPalm, libsiedler2::OI_Tree2_Begin);
        Tree birch(libsiedler2::OT_TreeOrPalm, libsiedler2::OI_Tree1_Begin);
        Tree cherry(libsiedler2::OT_Palm, libsiedler2::OI_Palm_Begin);
        Tree pine(libsiedler2::OT_TreeOrPalm, libsiedler2::OI_TreeOrPalm_Begin);
        Tree cypress(libsiedler2::OT_Palm, libsiedler2::OI_Tree2_Begin);

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

    void AddObjects(Map& map, RandomUtility& rnd, const MapSettings& settings)
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
                auto suroundingArea = map.getTextures().GetPointsInRadiusWithCenter(pt, 5);
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

        const auto range = GetRange(waterDistance);
        const auto probRange = ValueRange<unsigned>(settings.trees / 2, settings.trees);
        const unsigned probDiff = probRange.GetDifference();

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
                    const unsigned prob = probDiff ? MapValueToIndex(waterDistance[pt], range, probDiff) : 0;
                    probabilities[pt] = prob + probRange.minimum;
                } else
                {
                    const unsigned diff = mountainRange.maximum - mountainDepth[pt];
                    const unsigned prob = probDiff ? MapValueToIndex(diff, mountainRange, probDiff) : 0;
                    probabilities[pt] = prob + probRange.minimum;
                }
            } else
            {
                probabilities[pt] = 0;
            }
        }

        const auto trees = CreateTrees(map.textureMap);
        const auto treeForPoint = [&mountainDistance, &waterDistance, &range, &trees](const MapPoint& pt) {
            if(mountainDistance[pt] == 0)
            {
                return trees.back();
            }
            auto index = MapValueToIndex(waterDistance[pt], range, trees.size());
            return trees[index];
        };

        RTTR_FOREACH_PT(MapPoint, map.size)
        {
            const bool canPlaceStonePile = waterDistance[pt] > 0 && distanceToExcludedArea[pt] > 0;
            if(canPlaceStonePile && rnd.ByChance(settings.stonePiles))
            {
                map.objectTypes[pt] = rnd.ByChance(50) ? 0xCC : 0xCD;
                map.objectInfos[pt] = rnd.RandomValue(1, 6);
                continue;
            }

            const auto treeProb = probabilities[pt];
            if(treeProb > 0 && rnd.ByChance(treeProb))
            {
                auto tree = treeForPoint(pt);

                map.objectTypes[pt] = tree.type;
                map.objectInfos[pt] = tree.index + rnd.RandomValue(0, 7);
            }
        }
    }

    void AddResources(Map& map, RandomUtility& rnd, const MapSettings& settings)
    {
        const auto& textures = map.textureMap;
        auto& resources = map.resources;
        struct MineableResourceInfo
        {
            // The current "budget", that is, how many many are we over, or under, compared to the desired distribution.
            int budget = 0;
            // The ratio is the user defined ratio for this resource.
            int ratio;
            // The resource encoding
            libsiedler2::Resource resource;
            MineableResourceInfo(int ratio, libsiedler2::Resource resource) : ratio(ratio), resource(resource) {}
        };
        auto mRIs = helpers::make_array(MineableResourceInfo(settings.ratioCoal, libsiedler2::R_Coal),
                                        MineableResourceInfo(settings.ratioGold, libsiedler2::R_Gold),
                                        MineableResourceInfo(settings.ratioIron, libsiedler2::R_Iron),
                                        MineableResourceInfo(settings.ratioGranite, libsiedler2::R_Granite));
        const int total = settings.ratioCoal + settings.ratioGold + settings.ratioIron + settings.ratioGranite;

        // Helper to pick an index [0,4) into the mRIs array that identifies a randomly selected resource we should
        // place. Note that we do not perform budget adjustments and checks here.
        auto resourcePicker = [&]() {
            int ratio = 0;
            int randomNumber = rnd.RandomValue(1, std::max(1, total));
            for(unsigned i = 0; i < mRIs.size(); ++i)
            {
                ratio += mRIs[i].ratio;
                if(randomNumber <= ratio)
                    return i;
            }
            RTTR_Assert_Msg(0, "Expected to pick a resource!");
            return 0u;
        };

        RTTR_FOREACH_PT(MapPoint, map.size)
        {
            if(textures.All(pt, IsMinableMountain))
            {
                // No mineable resources wanted.
                if(total == 0)
                    continue;

                // Pick a random resource
                unsigned randomMRIindex = resourcePicker();
                auto& mRI = mRIs[randomMRIindex];

                // Adjust and check the budget, if we are over we will not place the resource because we have already in
                // the past.
                ++mRI.budget;
                if(mRI.budget <= 0)
                    continue;

                // Budget is positive so we want to place resources, however, we avoid overwriting existing clusters as
                // this results in sprinkles. If we have a resource on this field we keep the budget and place a cluster
                // later.
                if(resources[pt])
                    continue;

                // Cluster the resource around the map point.
                for(int xd = -4; xd <= 4; ++xd)
                {
                    for(int yd = -4; yd <= 4; ++yd)
                    {
                        const MapPoint pt_d = resources.MakeMapPoint(pt + Position(xd, yd));
                        // Only place it on mines that have no resource yet, adjust the budget for each placed
                        // resource.
                        if(!resources[pt_d] && textures.All(pt_d, IsMinableMountain))
                        {
                            --mRI.budget;
                            resources[pt_d] = mRI.resource + rnd.RandomValue(1, 8);
                        }
                    }
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
