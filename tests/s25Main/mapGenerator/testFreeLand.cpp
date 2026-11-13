// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "PointOutput.h"
#include "mapGenFixtures.h"
#include "mapGenerator/Algorithms.h"
#include "mapGenerator/FreeLand.h"
#include "mapGenerator/Resources.h"
#include "mapGenerator/TextureHelper.h"
#include "libsiedler2/enumTypes.h"
#include <boost/test/unit_test.hpp>
#include <algorithm>

using namespace rttr::mapGenerator;

namespace {
constexpr uint8_t landHeight = 10;
constexpr unsigned mountainLevel = 30;
constexpr MountainSettings settings{3, 100, 25};
constexpr unsigned numLandscapes = 3;

unsigned idx(LandFill type)
{
    return static_cast<unsigned>(type);
}

struct FreeLandFixture : MapGenFixture
{
    /// Creates a map covered by land except for a single row of water
    Map createLandMap(const MapExtent size) const
    {
        Map map = createMap(size);
        map.z.Resize(size, landHeight);
        RTTR_FOREACH_PT(MapPoint, size)
        {
            if(pt.y == 0)
                map.z[pt] = map.height.minimum;
        }
        return map;
    }

    /// Creates a map with a slope from the west to the center of the map
    Map createSlopedMap(const MapExtent size) const
    {
        Map map = createMap(size);
        map.z.Resize(size, landHeight);
        RTTR_FOREACH_PT(MapPoint, size)
        {
            map.z[pt] = static_cast<uint8_t>(landHeight + std::min<unsigned>(pt.x, size.x - pt.x));
        }
        return map;
    }

    static Mountain createArea(const Map& map, const MapPoint& center, unsigned radius)
    {
        const auto points = map.z.GetPointsInRadiusWithCenter(center, radius);
        return Mountain(points.begin(), points.end());
    }
};

template<class T_Predicate>
unsigned countObjects(const Map& map, T_Predicate&& predicate)
{
    unsigned count = 0;
    RTTR_FOREACH_PT(MapPoint, map.size)
    {
        if(map.objectTypes[pt] != libsiedler2::OT_Empty && predicate(pt))
            count++;
    }
    return count;
}
} // namespace

BOOST_FIXTURE_TEST_SUITE(FreeLandTests, FreeLandFixture)

BOOST_AUTO_TEST_CASE(SelectLandFill_returns_mountain_without_any_amounts)
{
    RandomUtility rnd(0);

    BOOST_TEST((SelectLandFill(rnd, {0, 0, 0}, {0, 0, 0}) == LandFill::Mountain));
}

BOOST_AUTO_TEST_CASE(SelectLandFill_starts_with_mountain_if_desired)
{
    for(unsigned seed = 0; seed < 20; seed++)
    {
        RandomUtility rnd(seed);
        BOOST_TEST_REQUIRE((SelectLandFill(rnd, {1, 100, 100}, {0, 0, 0}) == LandFill::Mountain));
    }
}

BOOST_AUTO_TEST_CASE(SelectLandFill_returns_only_desired_types)
{
    RandomUtility rnd(0);
    LandFillAmounts assigned{};

    for(unsigned i = 0; i < 50; i++)
    {
        const LandFill type = SelectLandFill(rnd, {0, 30, 0}, assigned);
        BOOST_TEST_REQUIRE((type == LandFill::Desert));
        assigned[idx(type)] += 10;
    }
}

BOOST_AUTO_TEST_CASE(SelectLandFill_keeps_proportions_of_amounts)
{
    const LandFillAmounts amounts{50, 30, 20};

    for(unsigned seed = 0; seed < 10; seed++)
    {
        RandomUtility rnd(seed);
        LandFillAmounts assigned{};

        // Even a low number of areas of decreasing size has to match the proportions roughly
        unsigned total = 0;
        for(unsigned size = 100; size > 40; size -= 5)
        {
            assigned[idx(SelectLandFill(rnd, amounts, assigned))] += size;
            total += size;
        }

        for(unsigned i = 0; i < numLandFills; i++)
        {
            const int share = static_cast<int>(assigned[i] * 100 / total);
            BOOST_TEST_REQUIRE(std::abs(share - static_cast<int>(amounts[i])) <= 15, "Type " << i << ": " << share);
        }
    }
}

BOOST_AUTO_TEST_CASE(SelectLandFill_varies_with_seed)
{
    const LandFillAmounts amounts{0, 50, 50};
    unsigned deserts = 0;

    for(unsigned seed = 0; seed < 50; seed++)
    {
        RandomUtility rnd(seed);
        if(SelectLandFill(rnd, amounts, {0, 0, 0}) == LandFill::Desert)
            deserts++;
    }

    BOOST_TEST(deserts > 0u);
    BOOST_TEST(deserts < 50u);
}

BOOST_AUTO_TEST_CASE(Swamp_and_desert_textures_exist_for_all_landscapes)
{
    for(unsigned i = 0; i < numLandscapes; i++)
    {
        landscape = DescIdx<LandscapeDesc>(i);
        const Map map = createMap(MapExtent(8, 8));

        const auto swamp = map.textureMap.Find(IsImpassable);
        const auto desert = map.textureMap.Find(IsCoastTerrain);

        // The swamp and its equivalents share the same index in all landscapes
        BOOST_TEST(map.textureMap.GetTextureId(swamp) == 3u);
        BOOST_TEST(!worldDesc.get(swamp).Is(ETerrain::Walkable));
        BOOST_TEST(!worldDesc.get(swamp).Is(ETerrain::Shippable));
        BOOST_TEST(worldDesc.get(desert).Is(ETerrain::Walkable));
        BOOST_TEST(!worldDesc.get(desert).Is(ETerrain::Buildable));
        BOOST_TEST(map.textureMap.FindAll(IsBuildableCoast).size() >= 2u);
    }
}

BOOST_AUTO_TEST_CASE(RaiseDesertHill_is_lower_than_mountain_and_keeps_surroundings)
{
    Map hill = createLandMap(MapExtent(48, 48));
    Map mountain = createLandMap(MapExtent(48, 48));
    const MapPoint center(24, 24);
    const Mountain area = createArea(hill, center, 10);
    RandomUtility rnd(0);

    RaiseDesertHill(hill, rnd, area);
    RaiseMountain(mountain, rnd, area);

    BOOST_TEST(hill.z[center] > landHeight);
    BOOST_TEST(hill.z[center] < mountain.z[center]);
    RTTR_FOREACH_PT(MapPoint, hill.size)
    {
        if(!helpers::contains(area, pt))
            BOOST_TEST_REQUIRE(hill.z[pt] == (pt.y == 0 ? hill.height.minimum : landHeight), "At point " << pt);
    }
}

BOOST_AUTO_TEST_CASE(RaiseDesertHill_rises_smoothly)
{
    Map map = createLandMap(MapExtent(48, 48));
    const Mountain area = createArea(map, MapPoint(24, 24), 10);
    RandomUtility rnd(0);

    RaiseDesertHill(map, rnd, area);

    RTTR_FOREACH_PT(MapPoint, map.size)
    {
        if(pt.y <= 1 || pt.y + 1 >= map.size.y) // skip the water
            continue;
        for(const MapPoint& neighbor : map.z.GetNeighbours(pt))
        {
            const int difference = static_cast<int>(map.z[pt]) - static_cast<int>(map.z[neighbor]);
            BOOST_TEST_REQUIRE(std::abs(difference) <= 1, "At point " << pt);
        }
    }
}

BOOST_AUTO_TEST_CASE(RaiseDesertHill_applies_transitions_from_buildable_land_to_desert)
{
    for(unsigned i = 0; i < numLandscapes; i++)
    {
        landscape = DescIdx<LandscapeDesc>(i);
        Map map = createLandMap(MapExtent(48, 48));
        const MapPoint center(24, 24);
        const Mountain area = createArea(map, center, 8);
        const auto footDistance = GetFootDistances(map.size, area);
        RandomUtility rnd(0);

        RaiseDesertHill(map, rnd, area);

        auto transitions = map.textureMap.FindAll(IsBuildableCoast);
        map.textureMap.Sort(transitions, ByHumidity);
        const auto count = [&map](DescIdx<TerrainDesc> texture) {
            return helpers::count_if(map.getTextures(), [texture](const TexturePair& pair) {
                return pair.rsu == texture || pair.lsd == texture;
            });
        };

        for(const MapPoint& pt : area)
        {
            if(footDistance[pt] < 2)
            {
                BOOST_TEST_REQUIRE(map.textureMap.Any(pt, IsBuildableLand), "At point " << pt);
                BOOST_TEST_REQUIRE(!helpers::contains(map.barren, pt), "At point " << pt);
            } else
            {
                BOOST_TEST_REQUIRE(map.textureMap.All(pt, IsCoastTerrain), "At point " << pt);
                BOOST_TEST_REQUIRE(helpers::contains(map.barren, pt), "At point " << pt);
            }
        }
        // The two driest buildable textures surround the desert
        BOOST_TEST(count(transitions[0]) > 0);
        BOOST_TEST(count(transitions[1]) > 0);
        BOOST_TEST(count(transitions[2]) == 0);
        BOOST_TEST(map.textureMap.All(center, IsCoastTerrain));
    }
}

BOOST_AUTO_TEST_CASE(RaiseDesertHill_places_bones_around_center_only)
{
    for(unsigned seed = 0; seed < 10; seed++)
    {
        Map map = createLandMap(MapExtent(48, 48));
        const Mountain area = createArea(map, MapPoint(24, 24), 9);
        const auto footDistance = GetFootDistances(map.size, area);
        const unsigned maxDistance = GetMaximum(footDistance, area);
        RandomUtility rnd(seed);

        RaiseDesertHill(map, rnd, area);

        BOOST_TEST_REQUIRE(countObjects(map, [](const MapPoint&) { return true; }) > 0u);
        RTTR_FOREACH_PT(MapPoint, map.size)
        {
            if(map.objectTypes[pt] == libsiedler2::OT_Empty)
                continue;
            BOOST_TEST_REQUIRE(map.objectTypes[pt] == OT_Decoration, "At point " << pt);
            BOOST_TEST_REQUIRE((map.objectInfos[pt] == OI_Bone1 || map.objectInfos[pt] == OI_Bone2), "At point " << pt);
            BOOST_TEST_REQUIRE(helpers::contains(area, pt), "At point " << pt);
            BOOST_TEST_REQUIRE(footDistance[pt] * 3 >= maxDistance * 2, "At point " << pt);
        }
    }
}

BOOST_AUTO_TEST_CASE(RaiseDesertHill_supports_tiny_areas)
{
    Map map = createLandMap(MapExtent(16, 16));
    const MapPoint center(8, 8);
    RandomUtility rnd(0);

    RaiseDesertHill(map, rnd, Mountain{center});
    RaiseDesertHill(map, rnd, Mountain{});

    BOOST_TEST(map.textureMap.All(center, IsCoastTerrain));
    BOOST_TEST(map.objectTypes[center] == OT_Decoration);
}

BOOST_AUTO_TEST_CASE(RaiseDesertHill_does_not_exceed_maximum_height)
{
    Map map = createMap(MapExtent(32, 32));
    map.z.Resize(map.size, map.height.maximum - 1);
    RandomUtility rnd(0);

    RaiseDesertHill(map, rnd, createArea(map, MapPoint(16, 16), 8));

    RTTR_FOREACH_PT(MapPoint, map.size)
    {
        BOOST_TEST_REQUIRE(map.z[pt] <= map.height.maximum, "At point " << pt);
    }
}

BOOST_AUTO_TEST_CASE(CreateSwamp_is_entirely_flat_and_impassable)
{
    for(unsigned i = 0; i < numLandscapes; i++)
    {
        landscape = DescIdx<LandscapeDesc>(i);
        Map map = createSlopedMap(MapExtent(48, 48));
        const Mountain area = createArea(map, MapPoint(18, 24), 6);
        const uint8_t lowest = map.z[MapPoint(12, 24)];
        RandomUtility rnd(0);

        CreateSwamp(map, rnd, area);

        for(const MapPoint& pt : area)
        {
            BOOST_TEST_REQUIRE(map.z[pt] == lowest, "At point " << pt);
            BOOST_TEST_REQUIRE(map.textureMap.All(pt, IsImpassable), "At point " << pt);
            BOOST_TEST_REQUIRE(helpers::contains(map.barren, pt), "At point " << pt);
        }
        BOOST_TEST(map.barren.size() == area.size());
    }
}

BOOST_AUTO_TEST_CASE(CreateSwamp_lowers_shore_towards_swamp)
{
    Map map = createSlopedMap(MapExtent(48, 48));
    const Map original = createSlopedMap(MapExtent(48, 48));
    const Mountain area = createArea(map, MapPoint(18, 24), 6);
    const uint8_t level = original.z[MapPoint(12, 24)];
    const auto distances = DistancesTo(area, map.size);
    RandomUtility rnd(0);

    CreateSwamp(map, rnd, area);

    unsigned lowered = 0;
    RTTR_FOREACH_PT(MapPoint, map.size)
    {
        if(distances[pt] == 0)
            continue;
        // The land never rises by more than one level per node - no matter how far away from the swamp
        BOOST_TEST_REQUIRE(map.z[pt] == std::min<unsigned>(original.z[pt], level + distances[pt]), "At point " << pt);
        if(map.z[pt] < original.z[pt])
            lowered++;
    }
    BOOST_TEST(lowered > 0u);
}

BOOST_AUTO_TEST_CASE(CreateSwamp_avoids_steep_slopes_on_steep_land)
{
    Map map = createMap(MapExtent(48, 48));
    map.z.Resize(map.size, landHeight);
    RTTR_FOREACH_PT(MapPoint, map.size)
    {
        // Ridge in the middle of the map which is three times as steep as the shore of a swamp
        map.z[pt] = static_cast<uint8_t>(landHeight + 3 * std::min<unsigned>(pt.x, map.size.x - pt.x));
    }
    const Mountain area = createArea(map, MapPoint(16, 24), 5);
    RandomUtility rnd(0);

    CreateSwamp(map, rnd, area);

    for(const MapPoint& pt : map.z.GetPointsInRadiusWithCenter(MapPoint(16, 24), 10))
    {
        // The land above the swamp is lowered while the land below keeps its slope
        if(pt.x < 16)
            continue;
        for(const MapPoint& neighbor : map.z.GetNeighbours(pt))
        {
            if(neighbor.x <= pt.x) // look uphill only
                continue;
            const int difference = static_cast<int>(map.z[neighbor]) - static_cast<int>(map.z[pt]);
            BOOST_TEST_REQUIRE(difference <= 1, "At point " << pt);
        }
    }
}

BOOST_AUTO_TEST_CASE(CreateSwamp_never_lowers_banks_below_rivers)
{
    // Rivers follow the height of the land instead of the sea level
    Map map = createSlopedMap(MapExtent(48, 48));
    const auto water = map.textureMap.Find(IsWater);
    std::vector<MapPoint> river;
    for(unsigned y = 0; y < map.size.y; y++)
    {
        river.push_back(MapPoint(27, y));
        map.textureMap.Set(river.back(), water);
    }
    const Map original = map;
    const auto riverDistances = DistancesTo(river, map.size);
    RandomUtility rnd(0);

    // The river passes the upper side of the swamp at a distance of three nodes
    const Mountain area = createArea(map, MapPoint(18, 24), 6);
    CreateSwamp(map, rnd, area);

    unsigned lowered = 0;
    RTTR_FOREACH_PT(MapPoint, map.size)
    {
        if(helpers::contains(area, pt))
            continue;
        if(riverDistances[pt] <= 1)
            BOOST_TEST_REQUIRE(map.z[pt] == original.z[pt], "At point " << pt);
        else
        {
            // The land is lowered by one more level with each node further away from the river
            BOOST_TEST_REQUIRE(original.z[pt] - map.z[pt] < static_cast<int>(riverDistances[pt]), "At point " << pt);
            if(map.z[pt] < original.z[pt])
                lowered++;
        }
    }
    BOOST_TEST(lowered > 0u);
}

BOOST_AUTO_TEST_CASE(CreateSwamp_keeps_shape_of_other_landscapes)
{
    Map map = createSlopedMap(MapExtent(48, 48));
    const Mountain area = createArea(map, MapPoint(18, 24), 6);
    const MapPoint mountain(26, 24);
    const uint8_t height = map.z[mountain];
    map.textureMap.Set(mountain, map.textureMap.Find(IsMinableMountain));
    RandomUtility rnd(0);

    CreateSwamp(map, rnd, area);

    BOOST_TEST(map.z[mountain] == height);
}

BOOST_AUTO_TEST_CASE(SmoothDesertTransitions_separates_desert_from_lush_land)
{
    for(unsigned i = 0; i < numLandscapes; i++)
    {
        landscape = DescIdx<LandscapeDesc>(i);
        Map map = createLandMap(MapExtent(48, 48));
        auto ramp = map.textureMap.FindAll(IsBuildableLand);
        map.textureMap.Sort(ramp, ByHumidity);
        const auto lush = ramp.back();
        RTTR_FOREACH_PT(MapPoint, map.size)
        {
            map.textureMap.Set(pt, lush);
        }
        const MapPoint center(24, 24);
        const Mountain area = createArea(map, center, 6);
        RandomUtility rnd(0);
        RaiseDesertHill(map, rnd, area);
        // Texturing coasts may replace the transitions of the hill
        for(const MapPoint& pt : area)
        {
            if(!helpers::contains(map.barren, pt))
                map.textureMap.Set(pt, lush);
        }
        for(const MapPoint& pt : map.barren)
            map.textureMap.Set(pt, map.textureMap.Find(IsCoastTerrain));

        SmoothDesertTransitions(map);

        const auto humidityOf = [this, &map](const Triangle& triangle) {
            return worldDesc.get(map.textureMap.Get(triangle)).humidity;
        };
        RTTR_FOREACH_PT(MapPoint, map.size)
        {
            uint8_t driest = 100;
            uint8_t wettest = 0;
            for(const Triangle& triangle : GetTriangles(pt, map.size))
            {
                driest = std::min(driest, humidityOf(triangle));
                wettest = std::max(wettest, humidityOf(triangle));
            }
            // Desert (0) meets steppe (30) only, steppe meets savannah (60) and savannah meets meadow (95)
            BOOST_TEST_REQUIRE((driest > 0 || wettest <= 30), "At point " << pt);
            BOOST_TEST_REQUIRE((driest > 30 || wettest <= 60), "At point " << pt);
            BOOST_TEST_REQUIRE((driest > 60 || wettest <= 95), "At point " << pt);
        }
        BOOST_TEST(map.textureMap.All(center, IsCoastTerrain));
        for(const MapPoint& pt : map.barren)
            BOOST_TEST_REQUIRE(map.textureMap.All(pt, IsCoastTerrain), "At point " << pt);
    }
}

BOOST_AUTO_TEST_CASE(SmoothDesertTransitions_keeps_maps_without_deserts)
{
    Map map = createLandMap(MapExtent(32, 32));
    const auto land = map.textureMap.Find(IsBuildableLand);
    RTTR_FOREACH_PT(MapPoint, map.size)
    {
        map.textureMap.Set(pt, land);
    }
    RandomUtility rnd(0);
    CreateSwamp(map, rnd, createArea(map, MapPoint(16, 16), 4));
    const auto before = map.getTextures();

    SmoothDesertTransitions(map);

    RTTR_FOREACH_PT(MapPoint, map.size)
    {
        BOOST_TEST_REQUIRE((map.getTextures()[pt].rsu == before[pt].rsu), "At point " << pt);
        BOOST_TEST_REQUIRE((map.getTextures()[pt].lsd == before[pt].lsd), "At point " << pt);
    }
}

BOOST_AUTO_TEST_CASE(CreateSwamp_places_dead_trees_on_border_to_free_land_only)
{
    Map map = createLandMap(MapExtent(48, 48));
    const Mountain area = createArea(map, MapPoint(24, 24), 6);
    const auto distances = DistancesTo(area, map.size);
    RandomUtility rnd(0);

    CreateSwamp(map, rnd, area);

    BOOST_TEST_REQUIRE(countObjects(map, [](const MapPoint&) { return true; }) > 0u);
    RTTR_FOREACH_PT(MapPoint, map.size)
    {
        if(map.objectTypes[pt] == libsiedler2::OT_Empty)
            continue;
        BOOST_TEST_REQUIRE(map.objectTypes[pt] == OT_Decoration, "At point " << pt);
        BOOST_TEST_REQUIRE((map.objectInfos[pt] == OI_DeadTree || map.objectInfos[pt] == OI_DeadTreeTrunk),
                           "At point " << pt);
        BOOST_TEST_REQUIRE(distances[pt] == 1u, "At point " << pt);
    }
}

BOOST_AUTO_TEST_CASE(CloseGaps_adds_enclosed_nodes_only)
{
    const Map map = createLandMap(MapExtent(32, 32));
    const MapPoint center(16, 16);
    const Mountain closed = createArea(map, center, 5);
    Mountain area = closed;
    area.erase(center);
    area.erase(MapPoint(13, 15));

    BOOST_TEST((CloseGaps(map.size, area) == closed));
    BOOST_TEST((CloseGaps(map.size, closed) == closed));
}

BOOST_AUTO_TEST_CASE(FillFreeLand_creates_all_desired_types_on_disjoint_areas)
{
    Map map = createLandMap(MapExtent(64, 64));
    RandomUtility rnd(0);

    const auto areas = FillFreeLand(map, rnd, mountainLevel, settings, {50, 50, 50});

    BOOST_TEST_REQUIRE(areas.size() >= 3u);
    BOOST_TEST((areas.front().type == LandFill::Mountain));

    Mountain visited;
    std::array<unsigned, numLandFills> count{};
    for(const FilledArea& area : areas)
    {
        count[idx(area.type)]++;
        for(const MapPoint& pt : area.nodes)
        {
            BOOST_TEST_REQUIRE(visited.insert(pt).second, "At point " << pt);
        }
    }
    BOOST_TEST(count[idx(LandFill::Mountain)] > 0u);
    BOOST_TEST(count[idx(LandFill::Desert)] > 0u);
    BOOST_TEST(count[idx(LandFill::Swamp)] > 0u);
}

BOOST_AUTO_TEST_CASE(FillFreeLand_applies_textures_matching_the_type)
{
    Map map = createLandMap(MapExtent(64, 64));
    RandomUtility rnd(0);

    for(const FilledArea& area : FillFreeLand(map, rnd, mountainLevel, settings, {50, 50, 50}))
    {
        const auto all = [&map, &area](bool (*predicate)(const TerrainDesc&)) {
            return std::all_of(area.nodes.begin(), area.nodes.end(),
                               [&](const MapPoint& pt) { return map.textureMap.All(pt, predicate); });
        };
        const auto any = [&map, &area](bool (*predicate)(const TerrainDesc&)) {
            return std::any_of(area.nodes.begin(), area.nodes.end(),
                               [&](const MapPoint& pt) { return map.textureMap.All(pt, predicate); });
        };

        switch(area.type)
        {
            case LandFill::Mountain: BOOST_TEST(all(IsMinableMountain)); break;
            case LandFill::Desert: BOOST_TEST(any(IsCoastTerrain)); break;
            case LandFill::Swamp: BOOST_TEST(all(IsImpassable)); break;
        }
    }
}

BOOST_AUTO_TEST_CASE(FillFreeLand_creates_only_mountains_by_default)
{
    Map map = createLandMap(MapExtent(64, 64));
    Map expected = createLandMap(MapExtent(64, 64));
    RandomUtility rnd1(7);
    RandomUtility rnd2(7);

    const auto areas = FillFreeLand(map, rnd1, mountainLevel, settings, {50, 0, 0});
    const auto mountains = CreateMountains(expected, rnd2, mountainLevel, settings);

    BOOST_TEST_REQUIRE(areas.size() == mountains.size());
    for(unsigned i = 0; i < areas.size(); i++)
    {
        BOOST_TEST_REQUIRE((areas[i].type == LandFill::Mountain));
        BOOST_TEST_REQUIRE((areas[i].nodes == mountains[i]), "Area " << i);
    }
    RTTR_FOREACH_PT(MapPoint, map.size)
    {
        BOOST_TEST_REQUIRE(map.z[pt] == expected.z[pt], "At point " << pt);
    }
    BOOST_TEST(map.barren.empty());
}

BOOST_AUTO_TEST_CASE(FillFreeLand_limits_swamps_to_half_the_size)
{
    Map map = createLandMap(MapExtent(64, 64));
    RandomUtility rnd(0);

    const auto areas = FillFreeLand(map, rnd, mountainLevel, settings, {0, 0, 50});

    BOOST_TEST_REQUIRE(!areas.empty());
    for(const FilledArea& area : areas)
    {
        BOOST_TEST_REQUIRE((area.type == LandFill::Swamp));
    }
    // Closing gaps adds a few nodes to the desired size
    BOOST_TEST(areas.front().nodes.size() >= settings.maxSize / 2);
    BOOST_TEST(areas.front().nodes.size() < settings.maxSize * 3 / 4);
}

BOOST_AUTO_TEST_CASE(FillFreeLand_leaves_no_free_land_beyond_limit)
{
    Map map = createLandMap(MapExtent(64, 64));
    RandomUtility rnd(0);

    FillFreeLand(map, rnd, mountainLevel, settings, {20, 40, 40});

    const auto distances = GetFreeLandDistances(map, mountainLevel);
    BOOST_TEST(*std::max_element(distances.begin(), distances.end()) <= settings.minFreeLand);
}

BOOST_AUTO_TEST_CASE(AddObjects_keeps_decoration_and_places_nothing_on_barren_nodes)
{
    Map map = createMap(MapExtent(32, 32));
    map.z.Resize(map.size, landHeight);
    const auto land = map.textureMap.Find(IsBuildableLand);
    RTTR_FOREACH_PT(MapPoint, map.size)
    {
        map.textureMap.Set(pt, land);
    }
    const Mountain swamp = createArea(map, MapPoint(8, 16), 4);
    const Mountain hill = createArea(map, MapPoint(22, 16), 5);
    RandomUtility rnd(0);
    CreateSwamp(map, rnd, swamp);
    RaiseDesertHill(map, rnd, hill);
    const auto typesBefore = map.objectTypes;
    const auto infosBefore = map.objectInfos;

    MapSettings mapSettings;
    mapSettings.trees = 100;
    mapSettings.stonePiles = 20;
    AddObjects(map, rnd, mapSettings);

    BOOST_TEST_REQUIRE(countObjects(map, [](const MapPoint&) { return true; }) > 100u);
    RTTR_FOREACH_PT(MapPoint, map.size)
    {
        if(typesBefore[pt] != libsiedler2::OT_Empty)
        {
            BOOST_TEST_REQUIRE(map.objectTypes[pt] == typesBefore[pt], "At point " << pt);
            BOOST_TEST_REQUIRE(map.objectInfos[pt] == infosBefore[pt], "At point " << pt);
        } else if(helpers::contains(map.barren, pt))
        {
            BOOST_TEST_REQUIRE(map.objectTypes[pt] == libsiedler2::OT_Empty, "At point " << pt);
        }
    }
}

BOOST_AUTO_TEST_CASE(AddObjects_removes_decoration_around_headquarters)
{
    Map map = createMap(MapExtent(32, 32));
    map.z.Resize(map.size, landHeight);
    const auto land = map.textureMap.Find(IsBuildableLand);
    RTTR_FOREACH_PT(MapPoint, map.size)
    {
        map.textureMap.Set(pt, land);
    }
    const MapPoint hq(16, 16);
    RandomUtility rnd(0);
    CreateSwamp(map, rnd, createArea(map, MapPoint(9, 16), 4));
    map.hqPositions.push_back(hq);

    AddObjects(map, rnd, MapSettings());

    for(const MapPoint& pt : map.z.GetPointsInRadius(hq, 5))
    {
        BOOST_TEST_REQUIRE(map.objectTypes[pt] == libsiedler2::OT_Empty, "At point " << pt);
    }
}

BOOST_AUTO_TEST_CASE(AddResources_and_AddAnimals_skip_swamps_made_of_water)
{
    // Winter world uses ice floes (water) as equivalent for swamps
    for(unsigned i = 0; i < numLandscapes; i++)
    {
        landscape = DescIdx<LandscapeDesc>(i);
        Map map = createMap(MapExtent(32, 32));
        map.z.Resize(map.size, landHeight);
        const auto land = map.textureMap.Find(IsBuildableLand);
        RTTR_FOREACH_PT(MapPoint, map.size)
        {
            map.textureMap.Set(pt, land);
        }
        const Mountain swamp = createArea(map, MapPoint(16, 16), 8);
        RandomUtility rnd(0);
        CreateSwamp(map, rnd, swamp);

        AddResources(map, rnd, MapSettings());
        AddAnimals(map, rnd);

        for(const MapPoint& pt : swamp)
        {
            BOOST_TEST_REQUIRE(map.resources[pt] != libsiedler2::R_Fish, "At point " << pt);
            BOOST_TEST_REQUIRE((map.animals[pt] == libsiedler2::Animal::None), "At point " << pt);
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
