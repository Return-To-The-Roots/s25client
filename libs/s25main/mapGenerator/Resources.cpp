// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "mapGenerator/Resources.h"
#include "RttrForeachPt.h"
#include "helpers/containerUtils.h"
#include "helpers/make_array.h"
#include "mapGenerator/Algorithms.h"
#include "mapGenerator/Terrain.h"
#include "mapGenerator/TextureHelper.h"

namespace rttr::mapGenerator {

std::vector<Tree> createTrees(const TextureMap& textures)
{
    const auto landscapeId = textures.getLandscapeId();

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

void addObjects(Map& map, RandomUtility& rnd, const MapSettings& settings)
{
    std::set<MapPoint, MapPointLess> excludedArea;
    NodeMapBase<unsigned> probabilities;
    probabilities.resize(map.size, 0u);

    // Do not allow to place trees/stone piles nearby head quarters or
    // harbor positions to avoid inaccessible harbors or invalid player positions.

    const auto harborOrHeadquarter = [&map](const MapPoint& pt) {
        return helpers::contains(map.hqPositions, pt)
               || helpers::containsIf(map.harbors, [pt](const Triangle& tr) { return tr.position == pt; });
    };

    RTTR_FOREACH_PT(MapPoint, map.size)
    {
        if(harborOrHeadquarter(pt))
        {
            auto suroundingArea = map.getTextures().getPointsInRadiusWithCenter(pt, 5);
            excludedArea.insert(suroundingArea.begin(), suroundingArea.end());
        } else if(map.textureMap.any(pt, isSnowOrLava))
        {
            excludedArea.insert(pt);
        }
    }

    const auto distanceToExcludedArea = distancesTo(excludedArea, map.size);

    // 1) compute maximum water distance until mountain area
    // 2) probabilities
    // 2a) non-mountains: distance to water
    // 2b) mountains: max prob - distance to non-mountain
    // 2c) forbidden area: 0
    // 3) tree type
    // 3a) non-mountains: distance to water
    // 3b) mountains: last element of trees

    auto& textureMap = map.textureMap;
    const auto mountain = [&textureMap](const MapPoint& pt) { return textureMap.any(pt, isMountainOrSnowOrLava); };
    const auto mountainDistance = distancesTo(map.size, mountain);

    const auto water = [&textureMap](const MapPoint& pt) { return textureMap.any(pt, isWater); };
    auto waterDistance = distancesTo(map.size, water);

    RTTR_FOREACH_PT(MapPoint, map.size)
    {
        // work around to avoid mountains to be considered for the total range of
        // distance-to-water values
        if(waterDistance[pt] > 0 && mountainDistance[pt] == 0)
        {
            waterDistance[pt] = 1;
        }
    }

    const auto range = getRange(waterDistance);
    const auto probRange = ValueRange<unsigned>(settings.trees / 2, settings.trees);
    const unsigned probDiff = probRange.getDifference();

    const auto mountainFoot = [&map](const MapPoint& pt) { return !map.textureMap.all(pt, isMountainOrSnowOrLava); };
    const auto mountainDepth = distancesTo(map.size, mountainFoot);
    const auto mountainRange = getRange(mountainDepth);

    RTTR_FOREACH_PT(MapPoint, map.size)
    {
        if(waterDistance[pt] > 0 && distanceToExcludedArea[pt] > 0)
        {
            if(mountainDistance[pt] > 0)
            {
                const unsigned prob = probDiff ? mapValueToIndex(waterDistance[pt], range, probDiff) : 0;
                probabilities[pt] = prob + probRange.minimum;
            } else
            {
                const unsigned diff = mountainRange.maximum - mountainDepth[pt];
                const unsigned prob = probDiff ? mapValueToIndex(diff, mountainRange, probDiff) : 0;
                probabilities[pt] = prob + probRange.minimum;
            }
        } else
        {
            probabilities[pt] = 0;
        }
    }

    const auto trees = createTrees(map.textureMap);
    const auto treeForPoint = [&mountainDistance, &waterDistance, &range, &trees](const MapPoint& pt) {
        if(mountainDistance[pt] == 0)
        {
            return trees.back();
        }
        auto index = mapValueToIndex(waterDistance[pt], range, trees.size());
        return trees[index];
    };

    RTTR_FOREACH_PT(MapPoint, map.size)
    {
        const bool canPlaceStonePile = waterDistance[pt] > 0 && distanceToExcludedArea[pt] > 0;
        if(canPlaceStonePile && rnd.byChance(settings.stonePiles))
        {
            map.objectTypes[pt] = rnd.byChance(50) ? 0xCC : 0xCD;
            map.objectInfos[pt] = rnd.randomValue(1, 6);
            continue;
        }

        const auto treeProb = probabilities[pt];
        if(treeProb > 0 && rnd.byChance(treeProb))
        {
            auto tree = treeForPoint(pt);

            map.objectTypes[pt] = tree.type;
            map.objectInfos[pt] = tree.index + rnd.randomValue(0, 7);
        }
    }
}

void addResources(Map& map, RandomUtility& rnd, const MapSettings& settings)
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
    auto mRIs = helpers::makeArray(MineableResourceInfo(settings.ratioCoal, libsiedler2::R_Coal),
                                   MineableResourceInfo(settings.ratioGold, libsiedler2::R_Gold),
                                   MineableResourceInfo(settings.ratioIron, libsiedler2::R_Iron),
                                   MineableResourceInfo(settings.ratioGranite, libsiedler2::R_Granite));
    const int total = settings.ratioCoal + settings.ratioGold + settings.ratioIron + settings.ratioGranite;

    // Helper to pick an index [0,4) into the mRIs array that identifies a randomly selected resource we should
    // place. Note that we do not perform budget adjustments and checks here.
    auto resourcePicker = [&]() {
        int ratio = 0;
        int randomNumber = rnd.randomValue(1, std::max(1, total));
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
        if(textures.all(pt, isMinableMountain))
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
                    const MapPoint pt_d = resources.makeMapPoint(pt + Position(xd, yd));
                    // Only place it on mines that have no resource yet, adjust the budget for each placed
                    // resource.
                    if(!resources[pt_d] && textures.all(pt_d, isMinableMountain))
                    {
                        --mRI.budget;
                        resources[pt_d] = mRI.resource + rnd.randomValue(1, 8);
                    }
                }
            }
        } else if(textures.all(pt, isWater))
        {
            resources[pt] = libsiedler2::R_Fish;
        } else
        {
            resources[pt] = libsiedler2::R_Water;
        }
    }
}

void addAnimals(Map& map, RandomUtility& rnd)
{
    std::vector<libsiedler2::Animal> landAnimals{libsiedler2::Animal::Rabbit, libsiedler2::Animal::Fox,
                                                 libsiedler2::Animal::Stag, libsiedler2::Animal::Deer,
                                                 libsiedler2::Animal::Sheep};
    const auto& textures = map.textureMap;
    RTTR_FOREACH_PT(MapPoint, map.size)
    {
        if(rnd.byChance(3))
        {
            if(textures.all(pt, isWater))
            {
                map.animals[pt] = libsiedler2::Animal::Duck;
            } else if(textures.all(pt, isLand))
            {
                map.animals[pt] = rnd.randomItem(landAnimals);
            }
        }
    }
}

} // namespace rttr::mapGenerator
