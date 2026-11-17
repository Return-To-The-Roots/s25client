// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "mapGenerator/FreeLand.h"
#include "helpers/EnumRange.h"
#include "helpers/containerUtils.h"
#include "mapGenerator/Algorithms.h"
#include "mapGenerator/TextureHelper.h"
#include "libsiedler2/enumTypes.h"
#include <algorithm>
#include <cstdint>
#include <numeric>
#include <utility>

namespace rttr::mapGenerator {

namespace {
    /// Maximum elevation of the center of a desert hill
    constexpr unsigned maxDesertElevation = 4;
    /// Number of rings of transition textures around the desert
    constexpr unsigned desertTransitions = 2;
    /// Chance in percentage for bones on a node around the center of a desert hill
    constexpr unsigned boneChance = 12;
    /// Chance in percentage for a dead tree on a node next to a swamp
    constexpr unsigned deadTreeChance = 33;
    /// Number of passes to close gaps of an area
    constexpr unsigned gapIterations = 2;
    /// Number of neighbors within an area required for a node to be considered a gap of the area
    constexpr unsigned minGapNeighbors = 4;

    void placeDecoration(Map& map, const MapPoint& pt, uint8_t index)
    {
        map.objectTypes[pt] = OT_Decoration;
        map.objectInfos[pt] = index;
    }
} // namespace

Mountain closeGaps(const MapExtent& size, const Mountain& area)
{
    NodeMapBase<uint8_t> neighbors;
    neighbors.resize(size);
    Mountain closedArea = area;

    for(unsigned i = 0; i < gapIterations; i++)
    {
        std::set<MapPoint, MapPointLess> candidates;
        for(const MapPoint& pt : closedArea)
        {
            for(const MapPoint& neighbor : neighbors.getNeighbours(pt))
            {
                if(!helpers::contains(closedArea, neighbor))
                    candidates.insert(neighbor);
            }
        }

        std::vector<MapPoint> gaps;
        for(const MapPoint& candidate : candidates)
        {
            const auto inArea = [&closedArea](const MapPoint& pt) { return helpers::contains(closedArea, pt); };
            if(helpers::countIf(neighbors.getNeighbours(candidate), inArea) >= minGapNeighbors)
                gaps.push_back(candidate);
        }

        if(gaps.empty())
            break;
        closedArea.insert(gaps.begin(), gaps.end());
    }

    return closedArea;
}

LandFill selectLandFill(RandomUtility& rnd, const LandFillAmounts& amounts, const LandFillAmounts& assigned)
{
    const uint64_t totalAmount = std::accumulate(amounts.begin(), amounts.end(), uint64_t(0));
    const uint64_t totalAssigned = std::accumulate(assigned.begin(), assigned.end(), uint64_t(0));

    if(totalAmount == 0 || (totalAssigned == 0 && amounts[LandFill::Mountain] > 0))
        return LandFill::Mountain;

    // A type is behind if its share of assigned nodes doesn't exceed its share of the amounts.
    // This always applies to at least one of the types.
    std::vector<LandFill> candidates;
    unsigned candidateAmount = 0;
    for(const LandFill type : helpers::EnumRange<LandFill>())
    {
        if(amounts[type] > 0 && assigned[type] * totalAmount <= totalAssigned * amounts[type])
        {
            candidates.push_back(type);
            candidateAmount += amounts[type];
        }
    }

    if(candidates.size() == 1)
        return candidates.front();

    unsigned value = rnd.randomValue(1u, candidateAmount);
    for(const LandFill candidate : candidates)
    {
        if(value <= amounts[candidate])
            return candidate;
        value -= amounts[candidate];
    }

    return candidates.back();
}

void raiseDesertHill(Map& map, RandomUtility& rnd, const Mountain& area)
{
    if(area.empty())
        return;

    const auto footDistance = getFootDistances(map.size, area);
    const unsigned maxDistance = getMaximum(footDistance, area);
    // Small hills get less transitions to still contain some desert
    const unsigned transitions = std::min(desertTransitions, maxDistance);

    const auto desert = map.textureMap.find(isCoastTerrain);
    auto transitionTextures = map.textureMap.findAll(isBuildableCoast);
    map.textureMap.sort(transitionTextures, byHumidity);

    // Neighboring nodes share triangles. Texturing from the foot to the center lets the inner texture win which results
    // in one row of triangles per transition and nodes of the desert which are entirely covered by desert.
    std::vector<MapPoint> nodes(area.begin(), area.end());
    std::stable_sort(nodes.begin(), nodes.end(), [&footDistance](const MapPoint& lhs, const MapPoint& rhs) {
        return footDistance[lhs] < footDistance[rhs];
    });

    for(const MapPoint& pt : nodes)
    {
        const unsigned distance = footDistance[pt];
        const unsigned elevation = std::min((distance + 1) / 2, maxDesertElevation);
        map.z[pt] = static_cast<uint8_t>(std::min<unsigned>(map.z[pt] + elevation, map.height.maximum));

        if(distance >= transitions)
        {
            map.textureMap.set(pt, desert);
            map.barren.insert(pt);
        } else
        {
            // The driest texture is next to the desert
            const auto maxIndex = static_cast<unsigned>(transitionTextures.size() - 1);
            const unsigned index = std::min(transitions - 1 - distance, maxIndex);
            map.textureMap.set(pt, transitionTextures[index]);
        }
    }

    bool hasBones = false;
    for(const MapPoint& pt : area)
    {
        const unsigned distance = footDistance[pt];
        const bool isCenter = distance == maxDistance;
        const bool isAroundCenter = distance >= transitions && distance * 3 >= maxDistance * 2;

        if((isCenter && !hasBones) || (isAroundCenter && rnd.byChance(boneChance)))
        {
            placeDecoration(map, pt, rnd.byChance(50) ? OI_Bone1 : OI_Bone2);
            hasBones = true;
        }
    }
}

void createSwamp(Map& map, RandomUtility& rnd, const Mountain& area)
{
    if(area.empty())
        return;

    const auto swamp = map.textureMap.find(isImpassable);
    const uint8_t level = map.z[*std::min_element(
      area.begin(), area.end(), [&map](const MapPoint& lhs, const MapPoint& rhs) { return map.z[lhs] < map.z[rhs]; })];

    // Other landscapes (rivers, mountains, deserts, water) are textured already or located at sea level. Rivers follow
    // the height of the land. Lowering their banks would leave the water on top of a ridge.
    const auto obstacleDistances = distancesTo(map.size, [&map](const MapPoint& pt) {
        if(map.z[pt] == map.height.minimum)
            return true;
        const auto triangles = getTriangles(pt, map.size);
        return helpers::containsIf(triangles,
                                   [&map](const Triangle& t) { return static_cast<bool>(map.textureMap.get(t)); });
    });

    for(const MapPoint& pt : area)
    {
        map.z[pt] = level;
        map.textureMap.set(pt, swamp);
        map.barren.insert(pt);
    }

    const auto distances = distancesTo(area, map.size);

    RTTR_FOREACH_PT(MapPoint, map.size)
    {
        const unsigned distance = distances[pt];
        if(distance == 0)
            continue;

        // The land descends to the swamp by one level per node at most. Other landscapes and the land next to them
        // keep their height. From there the land may be lowered by one more level with each node.
        const unsigned maxLowering = obstacleDistances[pt] > 0 ? obstacleDistances[pt] - 1 : 0;
        const unsigned target = std::min<unsigned>(map.z[pt], level + distance);
        map.z[pt] = static_cast<uint8_t>(std::max(target, map.z[pt] > maxLowering ? map.z[pt] - maxLowering : 0u));

        if(distance == 1 && map.objectTypes[pt] == libsiedler2::OT_Empty && rnd.byChance(deadTreeChance))
        {
            placeDecoration(map, pt, rnd.byChance(50) ? OI_DeadTree : OI_DeadTreeTrunk);
        }
    }
}

void smoothDesertTransitions(Map& map)
{
    auto& textures = map.textureMap;
    const auto desert = textures.find(isCoastTerrain);
    std::vector<MapPoint> desertNodes;
    for(const MapPoint& pt : map.barren)
    {
        if(textures.all(pt, isCoastTerrain))
            desertNodes.push_back(pt);
    }
    if(desertNodes.empty())
        return;

    // Land from dry to lush: each ring around the desert may be one step more lush than the previous one
    auto ramp = textures.findAll(isBuildableLand);
    textures.sort(ramp, [](const TerrainDesc& terrain) { return std::make_pair(terrain.humidity, terrain.s2Id); });
    const auto exceeds = [&textures, &ramp](const MapPoint& pt, unsigned limit) {
        for(const Triangle& triangle : getTriangles(pt, textures.textures_.getSize()))
        {
            const auto it = std::find(ramp.begin(), ramp.end(), textures.get(triangle));
            if(it != ramp.end() && static_cast<unsigned>(it - ramp.begin()) > limit)
                return true;
        }
        return false;
    };
    const auto isWalkableLand = [](const TerrainDesc& terrain) {
        return terrain.kind == TerrainKind::Land && terrain.is(ETerrain::Walkable);
    };

    const auto distances = distancesTo(desertNodes, map.size);
    const unsigned rings = std::min<unsigned>(3, static_cast<unsigned>(ramp.size()));

    // Neighboring nodes share triangles: painting from the outside to the inside keeps one row for each transition
    for(unsigned ring = rings; ring > 0; ring--)
    {
        RTTR_FOREACH_PT(MapPoint, map.size)
        {
            if(distances[pt] == ring && !helpers::contains(map.barren, pt) && textures.all(pt, isWalkableLand)
               && exceeds(pt, ring - 1))
                textures.set(pt, ramp[ring - 1]);
        }
    }
    for(const MapPoint& pt : desertNodes)
        textures.set(pt, desert);
}

std::vector<FilledArea> fillFreeLand(Map& map, RandomUtility& rnd, unsigned mountainLevel,
                                     const MountainSettings& settings, const LandFillAmounts& amounts)
{
    std::vector<LandFill> types;
    LandFillAmounts assigned{};

    // The type is required before the area is known as it has an impact on the size
    const auto getSize = [&](unsigned index) {
        const LandFill type = selectLandFill(rnd, amounts, assigned);
        unsigned size = getMountainSize(settings, index);
        if(type == LandFill::Swamp)
            size = std::max(size / 2, std::min(size, settings.minSize));

        types.push_back(type);
        assigned[type] += size;
        return size;
    };

    const auto areas = findMountains(map, rnd, mountainLevel, settings.minFreeLand, getSize);

    std::vector<FilledArea> filledAreas;
    for(unsigned i = 0; i < areas.size(); i++)
    {
        // Gaps would end up as pits within desert hills and as tiny islands within swamps
        const Mountain area = types[i] == LandFill::Mountain ? areas[i] : closeGaps(map.size, areas[i]);

        switch(types[i])
        {
            case LandFill::Mountain: raiseMountain(map, rnd, area); break;
            case LandFill::Desert: raiseDesertHill(map, rnd, area); break;
            case LandFill::Swamp: createSwamp(map, rnd, area); break;
        }
        filledAreas.push_back(FilledArea{types[i], area});
    }

    return filledAreas;
}

} // namespace rttr::mapGenerator
