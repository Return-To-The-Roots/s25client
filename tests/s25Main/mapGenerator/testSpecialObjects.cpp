// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "PointOutput.h"
#include "helpers/EnumRange.h"
#include "mapGenFixtures.h"
#include "mapGenerator/FreeLand.h"
#include "mapGenerator/Resources.h"
#include "mapGenerator/SpecialObjects.h"
#include "mapGenerator/TextureHelper.h"
#include "gameTypes/Direction.h"
#include "libsiedler2/enumTypes.h"
#include <boost/test/unit_test.hpp>
#include <algorithm>
#include <cstdlib>
#include <set>

using namespace rttr::mapGenerator;

namespace {
constexpr uint8_t landHeight = 10;
constexpr unsigned numLandscapes = 3;

struct SpecialObjectsFixture : MapGenFixture
{
    static bool isMeadow(const TerrainDesc& terrain) { return IsBuildableLand(terrain) && terrain.s2Id == 0x08; }

    static bool isLushMeadow(const TerrainDesc& terrain) { return IsBuildableLand(terrain) && terrain.s2Id == 0x0A; }

    /// Creates a flat map which is entirely covered by meadow
    Map createLandMap(const MapExtent size, bool lush = false) const
    {
        Map map = createMap(size);
        map.z.Resize(size, landHeight);
        const auto land = map.textureMap.Find(lush ? isLushMeadow : isMeadow);
        RTTR_FOREACH_PT(MapPoint, size)
        {
            map.textureMap.Set(pt, land);
        }
        return map;
    }
};

unsigned countObjects(const Map& map)
{
    unsigned count = 0;
    RTTR_FOREACH_PT(MapPoint, map.size)
    {
        if(map.objectTypes[pt] != libsiedler2::OT_Empty)
            count++;
    }
    return count;
}
} // namespace

BOOST_FIXTURE_TEST_SUITE(SpecialObjectsTests, SpecialObjectsFixture)

BOOST_AUTO_TEST_CASE(GetNumSpecialScenes_returns_a_few_scenes_depending_on_map_size)
{
    BOOST_TEST(GetNumSpecialScenes(MapExtent(32, 32)) == 1u);
    BOOST_TEST(GetNumSpecialScenes(MapExtent(64, 64)) < GetNumSpecialScenes(MapExtent(128, 128)));
    BOOST_TEST(GetNumSpecialScenes(MapExtent(128, 128)) < GetNumSpecialScenes(MapExtent(256, 256)));
    BOOST_TEST(GetNumSpecialScenes(MapExtent(1024, 1024)) <= 6u);
}

BOOST_AUTO_TEST_CASE(FindSceneSites_returns_requested_number_of_distant_sites)
{
    const Map map = createLandMap(MapExtent(96, 96));
    RandomUtility rnd(0);

    const auto sites = FindSceneSites(map, rnd, 4);

    BOOST_TEST_REQUIRE(sites.size() == 4u);
    for(unsigned i = 0; i < sites.size(); i++)
    {
        for(unsigned j = i + 1; j < sites.size(); j++)
            BOOST_TEST(map.z.CalcDistance(sites[i], sites[j]) >= 4 * sceneRadius);
    }
}

BOOST_AUTO_TEST_CASE(FindSceneSites_finds_nothing_without_buildable_land)
{
    Map map = createMap(MapExtent(32, 32));
    const auto water = map.textureMap.Find(IsWater);
    RTTR_FOREACH_PT(MapPoint, map.size)
    {
        map.textureMap.Set(pt, water);
    }
    RandomUtility rnd(0);

    BOOST_TEST(FindSceneSites(map, rnd, 3).empty());
}

BOOST_AUTO_TEST_CASE(FindSceneSites_keeps_distance_to_obstacles_and_players)
{
    Map map = createLandMap(MapExtent(64, 64));
    const MapPoint hq(10, 10);
    const MapPoint tree(40, 40);
    const MapPoint swamp(20, 45);
    map.hqPositions.push_back(hq);
    map.harbors.push_back(Triangle(true, MapPoint(50, 12)));
    map.objectTypes[tree] = libsiedler2::OT_TreeOrPalm;
    map.textureMap.Set(swamp, map.textureMap.Find(IsImpassable));

    for(unsigned seed = 0; seed < 30; seed++)
    {
        RandomUtility rnd(seed);
        for(const MapPoint& site : FindSceneSites(map, rnd, 3))
        {
            // Objects around headquarters and harbors are removed within a radius of 5
            BOOST_TEST_REQUIRE(map.z.CalcDistance(site, hq) > sceneRadius + 5, "At point " << site);
            BOOST_TEST_REQUIRE(map.z.CalcDistance(site, MapPoint(50, 12)) > sceneRadius + 5, "At point " << site);
            BOOST_TEST_REQUIRE(map.z.CalcDistance(site, tree) > sceneCoreRadius + 1, "At point " << site);
            BOOST_TEST_REQUIRE(map.z.CalcDistance(site, swamp) > sceneCoreRadius + 1, "At point " << site);
        }
    }
}

BOOST_AUTO_TEST_CASE(CreateScene_surrounds_special_object_by_decoration_within_scene)
{
    for(unsigned i = 0; i < numLandscapes; i++)
    {
        landscape = DescIdx<LandscapeDesc>(i);
        for(const SceneType type : helpers::EnumRange<SceneType>())
        {
            for(const SceneStyle style : helpers::EnumRange<SceneStyle>())
            {
                for(unsigned seed = 0; seed < 5; seed++)
                {
                    Map map = createLandMap(MapExtent(40, 40));
                    const MapPoint center(20, 20);
                    RandomUtility rnd(seed);

                    CreateScene(map, rnd, center, type, style);

                    BOOST_TEST_CONTEXT("Landscape " << i << " scene " << static_cast<unsigned>(type) << " style "
                                                    << static_cast<unsigned>(style) << " seed " << seed)
                    {
                        BOOST_TEST(IsSpecialObject(map.objectTypes[center], map.objectInfos[center]));
                        BOOST_TEST(countObjects(map) >= 15u);
                        BOOST_TEST(map.reserved.size() == 169u);
                        RTTR_FOREACH_PT(MapPoint, map.size)
                        {
                            const bool inScene = map.z.CalcDistance(center, pt) <= sceneRadius;
                            BOOST_TEST_REQUIRE(helpers::contains(map.reserved, pt) == inScene, "At point " << pt);
                            if(!inScene)
                            {
                                // The surrounding land is left untouched
                                BOOST_TEST_REQUIRE(map.objectTypes[pt] == libsiedler2::OT_Empty, "At point " << pt);
                                BOOST_TEST_REQUIRE(map.z[pt] == landHeight, "At point " << pt);
                            }
                            // Transitions may reach the triangles right next to the scene
                            if(map.z.CalcDistance(center, pt) > sceneRadius + 1)
                                BOOST_TEST_REQUIRE(map.textureMap.All(pt, isMeadow), "At point " << pt);
                            if(map.textureMap.All(pt, IsWater))
                                BOOST_TEST_REQUIRE(map.objectTypes[pt] == libsiedler2::OT_Empty, "At point " << pt);
                            // Hills and hollows are smooth
                            for(const MapPoint& neighbor : map.z.GetNeighbours(pt))
                            {
                                const int difference = static_cast<int>(map.z[pt]) - static_cast<int>(map.z[neighbor]);
                                BOOST_TEST_REQUIRE(std::abs(difference) <= 2, "At point " << pt);
                            }
                        }
                    }
                }
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(CreateScene_keeps_footprint_of_castle_ruin_free_and_even)
{
    for(const SceneStyle style : helpers::EnumRange<SceneStyle>())
    {
        for(unsigned seed = 0; seed < 10; seed++)
        {
            Map map = createLandMap(MapExtent(40, 40));
            const MapPoint center(20, 20);
            RandomUtility rnd(seed);

            CreateScene(map, rnd, center, SceneType::CastleRuin, style);

            BOOST_TEST(map.objectInfos[center] == OI_CastleRuin);
            for(const Direction dir : {Direction::West, Direction::NorthWest, Direction::NorthEast})
            {
                // The ruin covers some of its neighbors
                BOOST_TEST_REQUIRE(map.objectTypes[map.z.GetNeighbour(center, dir)] == libsiedler2::OT_Empty);
            }
            for(const MapPoint& pt : map.z.GetNeighbours(center))
                BOOST_TEST_REQUIRE(map.z[pt] == map.z[center], "At point " << pt);
        }
    }
}

BOOST_AUTO_TEST_CASE(CreateScene_blends_ground_into_surrounding_land)
{
    Map map = createLandMap(MapExtent(40, 40));
    const MapPoint center(20, 20);
    RandomUtility rnd(0);
    const auto isFlowers = [](const TerrainDesc& terrain) { return terrain.s2Id == 0x0F; };

    CreateScene(map, rnd, center, SceneType::RuinGrove, SceneStyle::Overgrown);

    // Flowers in the middle fade out via the textures between flowers and meadow
    BOOST_TEST(map.textureMap.All(center, isFlowers));
    std::set<uint8_t> textures;
    RTTR_FOREACH_PT(MapPoint, map.size)
    {
        textures.insert(map.textureMap.GetTextureId(map.getTextures()[pt].rsu));
        // Flowers never touch the meadow of the surrounding land directly
        if(map.textureMap.Any(pt, isFlowers))
            BOOST_TEST_REQUIRE(!map.textureMap.Any(pt, isMeadow), "At point " << pt);
    }
    BOOST_TEST(textures.count(0x09) == 1u);
    BOOST_TEST(textures.count(0x0A) == 1u);
}

BOOST_AUTO_TEST_CASE(CreateScene_uses_all_transition_textures_between_dry_ground_and_meadow)
{
    const auto isSteppe = [](const TerrainDesc& terrain) { return terrain.s2Id == 0x0E; };
    const auto isSavannah = [](const TerrainDesc& terrain) { return terrain.s2Id == 0x00; };

    for(const SceneStyle style : helpers::EnumRange<SceneStyle>())
    {
        for(unsigned seed = 0; seed < 5; seed++)
        {
            Map map = createLandMap(MapExtent(40, 40));
            const MapPoint center(20, 20);
            RandomUtility rnd(seed);

            CreateScene(map, rnd, center, SceneType::Camp, style);

            BOOST_TEST(map.textureMap.All(center, isSteppe));
            unsigned savannah = 0;
            RTTR_FOREACH_PT(MapPoint, map.size)
            {
                if(map.textureMap.Any(pt, isSavannah))
                    savannah++;
                // Steppe is always separated from meadow by savannah
                if(map.textureMap.Any(pt, isSteppe))
                    BOOST_TEST_REQUIRE(!map.textureMap.Any(pt, isMeadow), "At point " << pt);
            }
            BOOST_TEST(savannah > 6u);
        }
    }
}

BOOST_AUTO_TEST_CASE(CreateScene_surrounds_pond_by_transitions_from_sand_to_meadow)
{
    const auto isSteppe = [](const TerrainDesc& terrain) { return terrain.s2Id == 0x0E; };
    const auto isSavannah = [](const TerrainDesc& terrain) { return terrain.s2Id == 0x00; };

    for(const SceneType type : helpers::EnumRange<SceneType>())
    {
        for(unsigned seed = 0; seed < 5; seed++)
        {
            Map map = createLandMap(MapExtent(40, 40));
            const MapPoint center(20, 20);
            RandomUtility rnd(seed);

            CreateScene(map, rnd, center, type, SceneStyle::Crag);

            BOOST_TEST_CONTEXT("Scene " << static_cast<unsigned>(type) << " seed " << seed)
            {
                unsigned water = 0;
                RTTR_FOREACH_PT(MapPoint, map.size)
                {
                    if(map.textureMap.All(pt, IsWater))
                        water++;
                    // Water is enclosed by sand, sand by steppe and steppe by savannah
                    if(map.textureMap.Any(pt, IsWater))
                        BOOST_TEST_REQUIRE(
                          map.textureMap.All(pt, [](const TerrainDesc& t) { return IsWater(t) || IsCoastTerrain(t); }),
                          "At point " << pt);
                    if(map.textureMap.Any(pt, IsCoastTerrain))
                        BOOST_TEST_REQUIRE(!map.textureMap.Any(pt, isMeadow), "At point " << pt);
                    if(map.textureMap.Any(pt, isSteppe))
                        BOOST_TEST_REQUIRE(!map.textureMap.Any(pt, isMeadow), "At point " << pt);
                    if(map.textureMap.Any(pt, IsCoastTerrain))
                        BOOST_TEST_REQUIRE(!map.textureMap.Any(pt, isSavannah), "At point " << pt);
                }
                BOOST_TEST(water >= 2u);
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(CreateScene_adds_transitions_within_lush_meadow)
{
    // The most lush meadow is several textures away from sand but looks like any other meadow
    const auto isGreen = [](const TerrainDesc& terrain) { return IsBuildableLand(terrain) && terrain.humidity > 90; };
    const auto isSteppe = [](const TerrainDesc& terrain) { return terrain.s2Id == 0x0E; };
    const auto isSavannah = [](const TerrainDesc& terrain) { return terrain.s2Id == 0x00; };

    for(const SceneType type : helpers::EnumRange<SceneType>())
    {
        for(const SceneStyle style : helpers::EnumRange<SceneStyle>())
        {
            for(unsigned seed = 0; seed < 5; seed++)
            {
                Map map = createLandMap(MapExtent(40, 40), true);
                const MapPoint center(20, 20);
                RandomUtility rnd(seed);

                CreateScene(map, rnd, center, type, style);

                BOOST_TEST_CONTEXT("Scene " << static_cast<unsigned>(type) << " style " << static_cast<unsigned>(style)
                                            << " seed " << seed)
                {
                    RTTR_FOREACH_PT(MapPoint, map.size)
                    {
                        // Sand meets steppe, steppe meets savannah and only savannah meets meadow
                        if(map.textureMap.Any(pt, IsCoastTerrain))
                        {
                            BOOST_TEST_REQUIRE(!map.textureMap.Any(pt, isGreen), "At point " << pt);
                            BOOST_TEST_REQUIRE(!map.textureMap.Any(pt, isSavannah), "At point " << pt);
                        }
                        if(map.textureMap.Any(pt, isSteppe))
                            BOOST_TEST_REQUIRE(!map.textureMap.Any(pt, isGreen), "At point " << pt);
                    }
                }
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(CreateScene_leaves_other_landscapes_untouched)
{
    for(const SceneType type : helpers::EnumRange<SceneType>())
    {
        for(const SceneStyle style : helpers::EnumRange<SceneStyle>())
        {
            for(unsigned seed = 0; seed < 5; seed++)
            {
                // Water, a mountain and a tree are located in the outer part of the scene
                Map map = createLandMap(MapExtent(40, 40));
                const MapPoint center(20, 20);
                const auto water = map.textureMap.Find(IsWater);
                RTTR_FOREACH_PT(MapPoint, map.size)
                {
                    if(pt.x >= 26)
                    {
                        map.textureMap.Set(pt, water);
                        map.z[pt] = map.height.minimum;
                    }
                }
                const MapPoint mountain(15, 15);
                const MapPoint tree(17, 26);
                map.textureMap.Set(mountain, map.textureMap.Find(IsMinableMountain));
                map.z[mountain] = landHeight + 8;
                map.objectTypes[tree] = libsiedler2::OT_TreeOrPalm;
                map.objectInfos[tree] = libsiedler2::OI_TreeOrPalm_Begin;
                const Map original = map;
                RandomUtility rnd(seed);

                CreateScene(map, rnd, center, type, style);

                BOOST_TEST_CONTEXT("Scene " << static_cast<unsigned>(type) << " style " << static_cast<unsigned>(style)
                                            << " seed " << seed)
                {
                    BOOST_TEST(IsSpecialObject(map.objectTypes[center], map.objectInfos[center]));
                    BOOST_TEST(map.objectTypes[tree] == original.objectTypes[tree]);
                    BOOST_TEST(map.objectInfos[tree] == original.objectInfos[tree]);
                    const auto isLand = [this](DescIdx<TerrainDesc> texture) {
                        return IsBuildableLand(worldDesc.get(texture));
                    };
                    RTTR_FOREACH_PT(MapPoint, map.size)
                    {
                        // Only triangles of buildable land are painted
                        const TexturePair& before = original.getTextures()[pt];
                        const TexturePair& after = map.getTextures()[pt];
                        BOOST_TEST_REQUIRE((isLand(before.rsu) || after.rsu == before.rsu), "At point " << pt);
                        BOOST_TEST_REQUIRE((isLand(before.lsd) || after.lsd == before.lsd), "At point " << pt);
                        // Nodes which are part of other landscapes keep their height and get no objects
                        if(original.textureMap.All(pt, IsBuildableLand))
                            continue;
                        BOOST_TEST_REQUIRE(map.z[pt] == original.z[pt], "At point " << pt);
                        BOOST_TEST_REQUIRE(map.objectTypes[pt] == original.objectTypes[pt], "At point " << pt);
                    }
                }
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(CreateScene_adds_rim_around_rock)
{
    Map map = createLandMap(MapExtent(40, 40));
    const MapPoint center(20, 20);
    RandomUtility rnd(0);
    const auto isRock = [](const TerrainDesc& terrain) { return terrain.s2Id == 0x22; };

    CreateScene(map, rnd, center, SceneType::CastleRuin, SceneStyle::Overgrown);

    BOOST_TEST(map.textureMap.All(center, isRock));
    BOOST_TEST(map.z[center] > landHeight);
    RTTR_FOREACH_PT(MapPoint, map.size)
    {
        if(map.textureMap.Any(pt, isRock))
            BOOST_TEST_REQUIRE(map.textureMap.All(pt, IsBuildableMountain), "At point " << pt);
    }
}

BOOST_AUTO_TEST_CASE(CreateScene_raises_crag_next_to_pond)
{
    for(unsigned seed = 0; seed < 10; seed++)
    {
        Map map = createLandMap(MapExtent(40, 40));
        const MapPoint center(20, 20);
        RandomUtility rnd(seed);

        CreateScene(map, rnd, center, SceneType::RuinGrove, SceneStyle::Crag);

        unsigned pond = 0;
        unsigned rocks = 0;
        uint8_t top = 0;
        uint8_t waterLevel = 0;
        RTTR_FOREACH_PT(MapPoint, map.size)
        {
            top = std::max(top, map.z[pt]);
            if(map.objectTypes[pt] == libsiedler2::OT_Stone1 || map.objectTypes[pt] == libsiedler2::OT_Stone2)
                rocks++;
            if(map.textureMap.All(pt, IsWater))
            {
                BOOST_TEST_REQUIRE((pond == 0 || map.z[pt] == waterLevel), "At point " << pt);
                waterLevel = map.z[pt];
                pond++;
            }
        }
        BOOST_TEST(pond >= 2u);
        BOOST_TEST(rocks >= 2u);
        BOOST_TEST(top >= landHeight + 3);
        BOOST_TEST(waterLevel < landHeight);
    }
}

BOOST_AUTO_TEST_CASE(CreateScene_lines_road_of_avenue_with_trees)
{
    for(unsigned seed = 0; seed < 10; seed++)
    {
        Map map = createLandMap(MapExtent(40, 40));
        const MapPoint center(20, 20);
        RandomUtility rnd(seed);

        CreateScene(map, rnd, center, SceneType::CastleRuin, SceneStyle::Avenue);

        // One of the six directions is a free road with trees on both sides
        unsigned roads = 0;
        for(const Direction dir : helpers::EnumRange<Direction>())
        {
            MapPoint pt = center;
            bool isRoad = true;
            for(unsigned i = 1; i <= sceneRadius; i++)
            {
                pt = map.z.GetNeighbour(pt, dir);
                isRoad &= map.objectTypes[pt] == libsiedler2::OT_Empty;
            }
            if(isRoad)
                roads++;
        }
        BOOST_TEST(roads >= 1u);
        unsigned trees = 0;
        RTTR_FOREACH_PT(MapPoint, map.size)
        {
            if(map.objectTypes[pt] == libsiedler2::OT_TreeOrPalm || map.objectTypes[pt] == libsiedler2::OT_Palm
               || map.objectTypes[pt] == libsiedler2::OT_Palm + 1)
                trees++;
        }
        BOOST_TEST(trees == 8u);
    }
}

BOOST_AUTO_TEST_CASE(CreateScene_lines_trail_with_remains)
{
    Map map = createLandMap(MapExtent(40, 40));
    const MapPoint center(20, 20);
    RandomUtility rnd(0);

    CreateScene(map, rnd, center, SceneType::StoneCircle, SceneStyle::Trail);

    unsigned stones = 0;
    RTTR_FOREACH_PT(MapPoint, map.size)
    {
        if(map.objectTypes[pt] == OT_Decoration && map.objectInfos[pt] >= 0x18 && map.objectInfos[pt] <= 0x1E)
            stones++;
    }
    // Circle around the gate and the stones along the trail
    BOOST_TEST(stones >= 6u + 5u);
    BOOST_TEST((map.objectInfos[center] == OI_Gate || map.objectInfos[center] == OI_OpenGate));
}

BOOST_AUTO_TEST_CASE(CreateScene_places_single_tent_and_rows_of_graves_in_any_style)
{
    for(const SceneStyle style : helpers::EnumRange<SceneStyle>())
    {
        for(unsigned seed = 0; seed < 5; seed++)
        {
            Map camp = createLandMap(MapExtent(40, 40));
            Map graveyard = createLandMap(MapExtent(40, 40));
            const MapPoint center(20, 20);
            RandomUtility rnd(seed);

            CreateScene(camp, rnd, center, SceneType::Camp, style);
            CreateScene(graveyard, rnd, center, SceneType::Graveyard, style);

            unsigned tents = 0;
            unsigned graves = 0;
            RTTR_FOREACH_PT(MapPoint, camp.size)
            {
                if(camp.objectTypes[pt] == OT_Decoration && camp.objectInfos[pt] == OI_Tent)
                    tents++;
                if(graveyard.objectTypes[pt] == OT_Decoration && graveyard.objectInfos[pt] == OI_Cross)
                    graves++;
            }
            BOOST_TEST(tents == 1u);
            BOOST_TEST(graves >= 7u);
        }
    }
}

BOOST_AUTO_TEST_CASE(CreateScene_levels_ground_and_smooths_transition)
{
    Map map = createLandMap(MapExtent(48, 48));
    RTTR_FOREACH_PT(MapPoint, map.size)
    {
        map.z[pt] = static_cast<uint8_t>(landHeight + pt.x / 2);
    }
    const MapPoint center(24, 24);
    const uint8_t level = map.z[center];
    RandomUtility rnd(0);

    CreateScene(map, rnd, center, SceneType::Camp, SceneStyle::Trail);

    for(const MapPoint& pt : map.z.GetPointsInRadiusWithCenter(center, sceneRadius))
    {
        const unsigned distance = map.z.CalcDistance(center, pt);
        const int difference = static_cast<int>(map.z[pt]) - static_cast<int>(level);
        const int tolerance = distance <= 3 ? 0 : static_cast<int>(distance) - 3;
        BOOST_TEST_REQUIRE(std::abs(difference) <= tolerance, "At point " << pt);
    }
}

BOOST_AUTO_TEST_CASE(AddSpecialScenes_mixes_up_types_of_scenes)
{
    Map map = createLandMap(MapExtent(256, 256));
    RandomUtility rnd(0);

    const auto scenes = AddSpecialScenes(map, rnd);

    BOOST_TEST_REQUIRE(scenes.size() == GetNumSpecialScenes(map.size));
    std::set<SceneType> types;
    for(const Scene& scene : scenes)
    {
        types.insert(scene.type);
        BOOST_TEST(IsSpecialObject(map.objectTypes[scene.center], map.objectInfos[scene.center]));
    }
    BOOST_TEST(types.size() == 5u);
    BOOST_TEST(map.reserved.size() == 169 * scenes.size());
}

BOOST_AUTO_TEST_CASE(AddSpecialScenes_varies_with_seed)
{
    Map map1 = createLandMap(MapExtent(64, 64));
    Map map2 = createLandMap(MapExtent(64, 64));
    RandomUtility rnd1(1);
    RandomUtility rnd2(2);

    const auto scenes1 = AddSpecialScenes(map1, rnd1);
    const auto scenes2 = AddSpecialScenes(map2, rnd2);

    BOOST_TEST_REQUIRE(scenes1.size() == 1u);
    BOOST_TEST_REQUIRE(scenes2.size() == 1u);
    BOOST_TEST(scenes1.front().center != scenes2.front().center);
}

BOOST_AUTO_TEST_CASE(AddObjects_keeps_scenes_free_from_trees_and_stone_piles)
{
    Map map = createLandMap(MapExtent(48, 48));
    RandomUtility rnd(0);
    const auto scenes = AddSpecialScenes(map, rnd);
    BOOST_TEST_REQUIRE(!scenes.empty());
    const auto typesBefore = map.objectTypes;
    const auto infosBefore = map.objectInfos;

    MapSettings settings;
    settings.trees = 100;
    settings.stonePiles = 20;
    AddObjects(map, rnd, settings);

    BOOST_TEST(countObjects(map) > 500u);
    for(const MapPoint& pt : map.reserved)
    {
        BOOST_TEST_REQUIRE(map.objectTypes[pt] == typesBefore[pt], "At point " << pt);
        BOOST_TEST_REQUIRE(map.objectInfos[pt] == infosBefore[pt], "At point " << pt);
    }
}

BOOST_AUTO_TEST_SUITE_END()
