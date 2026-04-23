// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "RttrForeachPt.h"
#include "ai/AIQueryService.h"
#include "ai/AIResource.h"
#include "helpers/EnumRange.h"
#include "worldFixtures/CreateEmptyWorld.h"
#include "worldFixtures/WorldFixture.h"
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(AIQueryServiceCache)

namespace {
using SmallWorldFixture = WorldFixture<CreateEmptyWorld, 1>;
// 24x22 = 528 nodes — large enough for most tests; used for invalidation radius tests
using LargerWorldFixture = WorldFixture<CreateEmptyWorld, 1, 24, 22>;

constexpr unsigned kDefaultTTL = 1000; // kDefaultTTL used for Wood, Plantspace
} // namespace

BOOST_FIXTURE_TEST_CASE(SameFrameHit, SmallWorldFixture)
{
    AIQueryService queries(world, 0);
    const MapPoint pt = world.GetPlayer(0).GetHQPos();

    queries.ResetResourceValueCacheStats();
    const int v1 = queries.CalcResourceValue(pt, AIResource::Wood);
    BOOST_TEST(queries.GetResourceValueCacheHits() == 0u);
    BOOST_TEST(queries.GetResourceValueCacheMisses() == 1u);

    const int v2 = queries.CalcResourceValue(pt, AIResource::Wood);
    BOOST_TEST(queries.GetResourceValueCacheHits() == 1u);
    BOOST_TEST(queries.GetResourceValueCacheMisses() == 1u);
    BOOST_TEST(v1 == v2);
}

BOOST_FIXTURE_TEST_CASE(DirectionalFormBypassesCache, SmallWorldFixture)
{
    AIQueryService queries(world, 0);
    const MapPoint pt = world.GetPlayer(0).GetHQPos();

    queries.ResetResourceValueCacheStats();
    // Warm the non-directional cache entry first
    queries.CalcResourceValue(pt, AIResource::Wood);
    BOOST_TEST(queries.GetResourceValueCacheMisses() == 1u);

    // Directional calls should never touch the cache
    queries.CalcResourceValue(pt, AIResource::Wood, Direction::East, 0xffff);
    queries.CalcResourceValue(pt, AIResource::Wood, Direction::East, 0xffff);
    BOOST_TEST(queries.GetResourceValueCacheHits() == 0u);
    BOOST_TEST(queries.GetResourceValueCacheMisses() == 1u);
}

BOOST_FIXTURE_TEST_CASE(TTLExpiryIsAMiss, SmallWorldFixture)
{
    AIQueryService queries(world, 0);
    const MapPoint pt = world.GetPlayer(0).GetHQPos();

    queries.ResetResourceValueCacheStats();
    queries.CalcResourceValue(pt, AIResource::Wood);
    BOOST_TEST(queries.GetResourceValueCacheMisses() == 1u);

    // Advance exactly kDefaultTTL frames so the Wood entry expires
    for(unsigned i = 0; i < kDefaultTTL; ++i)
        world.GetEvMgr().ExecuteNextGF();

    queries.CalcResourceValue(pt, AIResource::Wood);
    BOOST_TEST(queries.GetResourceValueCacheMisses() == 2u);
    BOOST_TEST(queries.GetResourceValueCacheHits() == 0u);
}

BOOST_FIXTURE_TEST_CASE(EvictionSweepsExpiredEntries, LargerWorldFixture)
{
    AIQueryService queries(world, 0);

    // First pass: fill cache with Wood entries — all are misses.
    // (Wood uses kDefaultTTL so entries expire predictably after kDefaultTTL frames.)
    unsigned long long totalMisses = 0;
    RTTR_FOREACH_PT(MapPoint, world.GetSize())
    {
        queries.CalcResourceValue(pt, AIResource::Wood);
        ++totalMisses;
    }
    BOOST_TEST(queries.GetResourceValueCacheMisses() == totalMisses);
    BOOST_TEST(queries.GetResourceValueCacheHits() == 0u);

    // Advance kDefaultTTL frames so all Wood entries expire
    for(unsigned i = 0; i < kDefaultTTL; ++i)
        world.GetEvMgr().ExecuteNextGF();

    // Second pass: all Wood entries expired, so all re-queries are misses
    RTTR_FOREACH_PT(MapPoint, world.GetSize())
        queries.CalcResourceValue(pt, AIResource::Wood);

    BOOST_TEST(queries.GetResourceValueCacheHits() == 0u);
    BOOST_TEST(queries.GetResourceValueCacheMisses() == totalMisses * 2);
}

BOOST_FIXTURE_TEST_CASE(InvalidateResourceValueInRadius_EvictsMatchingBorderlandEntries, LargerWorldFixture)
{
    AIQueryService queries(world, 0);
    const MapPoint center = world.GetPlayer(0).GetHQPos();

    // Warm Borderland entries in the invalidation disc
    const unsigned radius = 6u;
    std::vector<MapPoint> disc = world.GetPointsInRadiusWithCenter(center, radius);
    for(const MapPoint& pt : disc)
        queries.CalcResourceValue(pt, AIResource::Borderland);

    // Warm a point strictly outside the invalidation radius.
    // Walk radius+1 = 7 steps East; on the 24x22 world the point remains distinct.
    MapPoint farPt = center;
    for(unsigned i = 0; i <= radius; ++i)
        farPt = world.GetNeighbour(farPt, Direction::East);
    // Confirm farPt is not in the disc
    BOOST_REQUIRE(world.CalcDistance(center, farPt) > radius);
    queries.CalcResourceValue(farPt, AIResource::Borderland);

    queries.ResetResourceValueCacheStats();
    queries.InvalidateResourceValueInRadius(center, AIResource::Borderland, radius);

    // All disc points must miss on re-query (evicted)
    for(const MapPoint& pt : disc)
        queries.CalcResourceValue(pt, AIResource::Borderland);
    BOOST_TEST(queries.GetResourceValueCacheMisses() == disc.size());

    // The far point outside the radius must still hit
    queries.CalcResourceValue(farPt, AIResource::Borderland);
    BOOST_TEST(queries.GetResourceValueCacheHits() == 1u);
}

BOOST_FIXTURE_TEST_CASE(InvalidateResourceValueInRadius_OnlyAffectsRequestedResource, SmallWorldFixture)
{
    AIQueryService queries(world, 0);
    const MapPoint pt = world.GetPlayer(0).GetHQPos();

    // Warm multiple resources at the same point
    queries.CalcResourceValue(pt, AIResource::Borderland);
    queries.CalcResourceValue(pt, AIResource::Wood);
    queries.CalcResourceValue(pt, AIResource::Stones);

    queries.ResetResourceValueCacheStats();
    queries.InvalidateResourceValueInRadius(pt, AIResource::Borderland, 6u);

    // Borderland must miss; Wood and Stones must hit
    queries.CalcResourceValue(pt, AIResource::Borderland);
    BOOST_TEST(queries.GetResourceValueCacheMisses() == 1u);
    BOOST_TEST(queries.GetResourceValueCacheHits() == 0u);

    queries.CalcResourceValue(pt, AIResource::Wood);
    queries.CalcResourceValue(pt, AIResource::Stones);
    BOOST_TEST(queries.GetResourceValueCacheHits() == 2u);
    BOOST_TEST(queries.GetResourceValueCacheMisses() == 1u);
}

BOOST_FIXTURE_TEST_CASE(InvalidateResourceValueInRadius_NoopOnEmptyCache, SmallWorldFixture)
{
    AIQueryService queries(world, 0);
    const MapPoint pt = world.GetPlayer(0).GetHQPos();

    queries.ResetResourceValueCacheStats();
    // Must not crash and stats must remain at zero
    queries.InvalidateResourceValueInRadius(pt, AIResource::Borderland, 6u);
    BOOST_TEST(queries.GetResourceValueCacheHits() == 0u);
    BOOST_TEST(queries.GetResourceValueCacheMisses() == 0u);
}

BOOST_AUTO_TEST_SUITE_END()
