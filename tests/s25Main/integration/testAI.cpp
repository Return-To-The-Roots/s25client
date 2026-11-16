// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "PointOutput.h"
#include "RttrForeachPt.h"
#include "ai/AIPlayer.h"
#include "ai/aijh/AIPlayerJH.h"
#include "buildings/noBuilding.h"
#include "buildings/noBuildingSite.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobMilitary.h"
#include "factories/AIFactory.h"
#include "factories/BuildingFactory.h"
#include "helpers/EnumRange.h"
#include "helpers/containerUtils.h"
#include "network/GameMessage_Chat.h"
#include "notifications/NodeNote.h"
#include "worldFixtures/WorldWithGCExecution.h"
#include "worldFixtures/terrainHelpers.h"
#include "nodeObjs/noFlag.h"
#include "nodeObjs/noTree.h"
#include "gameTypes/GameTypesOutput.h"
#include "gameTypes/MineResourceBehavior.h"
#include "gameTypes/Resource.h"
#include "gameData/BuildingProperties.h"
#include "gameData/MilitaryConsts.h"
#include "gameData/WorldDescription.h"
#include "rttr/test/random.hpp"
#include <boost/test/unit_test.hpp>
#include <memory>
#include <set>

namespace {
// We need border land
using BiggerWorldWithGCExecution = WorldWithGCExecution<1, 24, 22>;
using EmptyWorldFixture1P = WorldFixture<CreateEmptyWorld, 1>;
using EmptyWorldFixture2P = WorldFixture<CreateEmptyWorld, 2>;

template<class T_Col>
inline bool containsBldType(const T_Col& collection, BuildingType type)
{
    return helpers::contains_if(collection,
                                [type](const noBaseBuilding* bld) { return bld->getBuildingType() == type; });
}

inline bool playerHasBld(const GamePlayer& player, BuildingType type)
{
    const BuildingRegister& blds = player.getBuildingRegister();
    if(containsBldType(blds.getBuildingSites(), type))
        return true;
    if(BuildingProperties::isMilitary(type))
        return containsBldType(blds.getMilitaryBuildings(), type);
    if(BuildingProperties::isWareHouse(type)) // Includes harbors
        return containsBldType(blds.getStorehouses(), type);
    return !blds.getBuildings(type).empty();
}

DescIdx<TerrainDesc> GetMineableTerrain(const WorldDescription& desc)
{
    const auto terrain = desc.terrain.find([](const TerrainDesc& t) { return t.Is(ETerrain::Mineable); });
    BOOST_TEST_REQUIRE(terrain);
    return terrain;
}

void makeWorldMineable(GameWorld& world)
{
    const DescIdx<TerrainDesc> mineableTerrain = GetMineableTerrain(world.getDescription());
    RTTR_FOREACH_PT(MapPoint, world.getSize())
    {
        MapNode& node = world.getNodeWriteable(pt);
        node.t1 = node.t2 = mineableTerrain;
        node.resources = Resource();
    }
    world.initAfterLoad();
}

// Mark every node as a usable, owned, reachable build spot so the AI's position search only depends on the
// mine resource behavior under test and not on unrelated terrain/ownership state.
void initAIJhNodes(AIJH::AIPlayerJH& aijh, const GameWorld& world, const unsigned player)
{
    RTTR_FOREACH_PT(MapPoint, world.getSize())
    {
        AIJH::Node& node = aijh.getAINode(pt);
        node.bq = world.GetBQ(pt, player);
        node.owned = true;
        node.reachable = true;
        node.farmed = false;
    }
}

struct MockAI final : public AIPlayer
{
    MockAI(unsigned char playerId, const GameWorldBase& gwb, const AI::Level level) : AIPlayer(playerId, gwb, level) {}
    // LCOV_EXCL_START
    void runGF(unsigned /*gf*/, bool /*gfisnwf*/) override {}
    void onChatMessage(unsigned /*sendPlayerId*/, ChatDestination, const std::string& /*msg*/) override {}
    // LCOV_EXCL_STOP
};
} // namespace

// Note game command execution is emulated to be like the ones send via network:
// Run "Network Frame" then execute GCs from last NWF
// Also use "HARD" AI for faster execution
BOOST_AUTO_TEST_SUITE(AI)

BOOST_FIXTURE_TEST_CASE(PlayerHasBld_IsCorrect, EmptyWorldFixture1P)
{
    const GamePlayer& player = world.getPlayer(0);
    BOOST_TEST(playerHasBld(player, BuildingType::Headquarters));
    MapPoint pos = player.getHQPos();
    for(const auto bld : {BuildingType::Woodcutter, BuildingType::Barracks, BuildingType::Storehouse})
    {
        pos = world.makeMapPoint(pos + Position(2, 0));
        BOOST_TEST_INFO(bld);
        BOOST_TEST(!playerHasBld(player, bld));
        BuildingFactory::createBuilding(world, bld, pos, player.getPlayerId(), Nation::Romans);
        BOOST_TEST_INFO(bld);
        BOOST_TEST(playerHasBld(player, bld));
    }
}

BOOST_FIXTURE_TEST_CASE(AIChat, EmptyWorldFixture2P)
{
    MockAI ai(1, world, AI::Level::Easy);
    ai.getAIInterface().chat("Hello players!");
    ai.getAIInterface().chat("2nd Message!", ChatDestination::Allies);
    const auto msgs = ai.getAIInterface().fetchChatMessages();
    BOOST_TEST_REQUIRE(msgs.size() == 2u);
    BOOST_TEST(msgs[0]->player == 1u);
    BOOST_TEST(msgs[0]->destination == ChatDestination::All);
    BOOST_TEST(msgs[0]->text == "Hello players!");
    BOOST_TEST(msgs[1]->player == 1u);
    BOOST_TEST(msgs[1]->destination == ChatDestination::Allies);
    BOOST_TEST(msgs[1]->text == "2nd Message!");
    // Messages cleared by first call
    BOOST_TEST(ai.getAIInterface().fetchChatMessages().empty());
    // Can readd
    const auto dest = rttr::test::randomEnum<ChatDestination>();
    ai.getAIInterface().chat("Hello again!", dest);
    // Iterate just like in ExecuteNWF function
    for(auto& msg : ai.getAIInterface().fetchChatMessages())
    {
        BOOST_TEST(msg->player == 1u);
        BOOST_TEST(msg->destination == dest);
        BOOST_TEST(msg->text == "Hello again!");
    }
}

BOOST_FIXTURE_TEST_CASE(MineResourceRatingAccountsForS4LikeExhaustion, EmptyWorldFixture1P)
{
    const MapPoint resourcePos = world.makeMapPoint(world.getPlayer(0).getHQPos() + Position(2, 0));
    world.getNodeWriteable(resourcePos).resources = Resource(ResourceType::Coal, 1);

    MockAI ai(0, world, AI::Level::Easy);
    const auto ratingWith = [&](const MineResourceBehavior behavior) {
        ggs.setSelection(AddonId::COALMINE_RESOURCE_BEHAVIOR, static_cast<unsigned>(behavior));
        return ai.getAIInterface().getResourceRating(resourcePos, AIResource::Coal);
    };

    // A spot with a coal deposit gets the full radius rating for all behaviors that mine the deposit as-is...
    const int defaultRating = ratingWith(MineResourceBehavior::Default);
    BOOST_TEST(defaultRating == static_cast<int>(RES_RADIUS[AIResource::Coal]));
    BOOST_TEST(ratingWith(MineResourceBehavior::Inexhaustible) == defaultRating);
    BOOST_TEST(ratingWith(MineResourceBehavior::WorkEverywhere) == defaultRating);

    // ...only S4-like exhaustion lowers the rating because a nearly depleted deposit produces less over its lifetime.
    const int s4LikeRating = ratingWith(MineResourceBehavior::S4LikeExhaustion);
    BOOST_TEST(s4LikeRating > 0);
    BOOST_TEST(s4LikeRating < defaultRating);
}

BOOST_FIXTURE_TEST_CASE(MineWorkEverywhereAffectsMatchingResourceOnly, EmptyWorldFixture1P)
{
    // Whole world is mineable terrain but has no mineral deposits at all.
    makeWorldMineable(world);
    ggs.setSelection(AddonId::COALMINE_RESOURCE_BEHAVIOR, static_cast<unsigned>(MineResourceBehavior::WorkEverywhere));

    AIJH::AIPlayerJH ai(0, world, AI::Level::Hard);
    initAIJhNodes(ai, world, 0);

    // The HQ is the only guaranteed reachable/owned starting spot, so searches are centered on it.
    const MapPoint searchCenter = world.getPlayer(0).getHQPos();
    // The only iron deposit sits well outside the search radius below, so default iron mining has no reachable spot.
    const MapPoint ironPos = world.makeMapPoint(searchCenter + Position(8, 0));
    world.getNodeWriteable(ironPos).resources = Resource(ResourceType::Iron, 4);

    // WorkEverywhere coal treats any mineable node as a coal spot, even one that actually holds an iron deposit.
    BOOST_TEST(ai.getAIInterface().getResourceRating(ironPos, AIResource::Coal)
               == static_cast<int>(RES_RADIUS[AIResource::Coal]));
    // So coal finds a spot right around the HQ, while default iron finds none because its only deposit is out of range.
    BOOST_TEST(ai.findBestPosition(searchCenter, AIResource::Coal, BuildingQuality::Mine, 5).isValid());
    BOOST_TEST(!ai.findBestPosition(searchCenter, AIResource::Ironore, BuildingQuality::Mine, 5).isValid());
}

BOOST_FIXTURE_TEST_CASE(GraniteMineResourceBehaviorAffectsAIMineSearch, EmptyWorldFixture1P)
{
    // Mineable terrain everywhere but no deposits, so only the granite behavior decides what the AI can place.
    makeWorldMineable(world);
    const MapPoint searchCenter = world.getPlayer(0).getHQPos();
    // The AI rates all nodes once during setup, so each behavior needs an AI of its own.
    const auto findsMineSpotFor = [&](const MineResourceBehavior behavior, const AIResource res) {
        ggs.setSelection(AddonId::GRANITEMINE_RESOURCE_BEHAVIOR, static_cast<unsigned>(behavior));
        AIJH::AIPlayerJH ai(0, world, AI::Level::Hard);
        initAIJhNodes(ai, world, 0);
        return ai.findBestPosition(searchCenter, res, BuildingQuality::Mine, 5).isValid();
    };

    // Default: No resources, no spot.
    BOOST_TEST(!findsMineSpotFor(MineResourceBehavior::Default, AIResource::Granite));
    BOOST_TEST(!findsMineSpotFor(MineResourceBehavior::Default, AIResource::Coal));
    // Same for S4-like exhaustion
    BOOST_TEST(!findsMineSpotFor(MineResourceBehavior::S4LikeExhaustion, AIResource::Granite));
    BOOST_TEST(!findsMineSpotFor(MineResourceBehavior::S4LikeExhaustion, AIResource::Coal));
    // WorkEverywhere lets the AI place a granite mine on any mineable node, but only for granite, not coal.
    BOOST_TEST(findsMineSpotFor(MineResourceBehavior::WorkEverywhere, AIResource::Granite));
    BOOST_TEST(!findsMineSpotFor(MineResourceBehavior::WorkEverywhere, AIResource::Coal));
    // Inexhaustible does not imply "work everywhere": without an actual deposit there is still no spot.
    BOOST_TEST(!findsMineSpotFor(MineResourceBehavior::Inexhaustible, AIResource::Granite));
    BOOST_TEST(!findsMineSpotFor(MineResourceBehavior::Inexhaustible, AIResource::Coal));
    // But if we place a granite deposit, the AI can find it
    world.getNodeWriteable(searchCenter).resources = Resource(ResourceType::Granite, 1);
    for(const auto behavior : helpers::EnumRange<MineResourceBehavior>{})
        BOOST_TEST_CONTEXT("Behavior: " << rttr::enum_cast(behavior))
        {
            BOOST_TEST(findsMineSpotFor(behavior, AIResource::Granite));
            BOOST_TEST(!findsMineSpotFor(behavior, AIResource::Coal));
        }
}

BOOST_FIXTURE_TEST_CASE(KeepBQUpdated, BiggerWorldWithGCExecution)
{
    addStartResources();
    // Place some trees to reduce BQ at some points
    RTTR_FOREACH_PT(MapPoint, world.getSize())
    {
        if(pt.x % 4 == 0 && pt.y % 2 == 0 && world.getNode(pt).bq == BuildingQuality::Castle
           && world.calcDistance(pt, hqPos) > 6)
            world.setNO(pt, new noTree(pt, 0, 3));
    }
    world.initAfterLoad();

    auto ai = AIFactory::create(AI::Info(AI::Type::Default, AI::Level::Hard), curPlayer, world);
    const AIJH::AIPlayerJH& aijh = static_cast<AIJH::AIPlayerJH&>(*ai);

    const auto assertBqEqualOnWholeMap = [this, &aijh](const unsigned lineNr) {
        BOOST_TEST_CONTEXT("Line #" << lineNr)
        RTTR_FOREACH_PT(MapPoint, world.getSize())
        {
            BOOST_TEST_INFO(pt);
            BOOST_TEST(this->world.GetBQ(pt, curPlayer) == aijh.getAINode(pt).bq);
        }
    };
    const auto assertBqEqualAround = [this, &aijh](const unsigned lineNr, MapPoint pt, unsigned radius) {
        BOOST_TEST_CONTEXT("Line #" << lineNr)
        world.checkPointsInRadius(
          pt, radius,
          [&](const MapPoint curPt, unsigned) {
              BOOST_TEST_INFO(curPt);
              BOOST_TEST(this->world.GetBQ(curPt, curPlayer) == aijh.getAINode(curPt).bq);
              return false;
          },
          true);
    };

    // 100GFs for initialization
    for(unsigned gf = 0; gf < 100; ++gf)
    {
        em.executeNextGF();
        ai->runGF(em.getCurrentGF(), true);
    }
    assertBqEqualOnWholeMap(__LINE__);

    // Set and destroy flag everywhere possible
    std::vector<MapPoint> possibleFlagNodes;
    RTTR_FOREACH_PT(MapPoint, world.getSize())
    {
        if(world.GetBQ(pt, curPlayer) != BuildingQuality::Nothing && !world.isFlagAround(pt))
            possibleFlagNodes.push_back(pt);
    }
    for(const MapPoint flagPos : possibleFlagNodes)
    {
        this->SetFlag(flagPos);
        BOOST_TEST_REQUIRE(world.getSpecObj<noFlag>(flagPos));
        em.executeNextGF();
        ai->runGF(em.getCurrentGF(), true);
        assertBqEqualAround(__LINE__, flagPos, 3);

        this->DestroyFlag(flagPos);
        BOOST_TEST_REQUIRE(!world.getSpecObj<noFlag>(flagPos));
        em.executeNextGF();
        ai->runGF(em.getCurrentGF(), true);
        assertBqEqualAround(__LINE__, flagPos, 3);
    }

    // Build road
    const MapPoint flagPos = world.makeMapPoint(world.getNeighbour(hqPos, Direction::SouthEast) + Position(4, 0));
    this->BuildRoad(world.getNeighbour(hqPos, Direction::SouthEast), false, std::vector<Direction>(4, Direction::East));
    BOOST_TEST_REQUIRE(world.getSpecObj<noFlag>(flagPos)->getRoute(Direction::West));
    em.executeNextGF();
    ai->runGF(em.getCurrentGF(), true);
    assertBqEqualAround(__LINE__, flagPos, 6);

    // Destroy road and flag
    this->DestroyFlag(flagPos);
    em.executeNextGF();
    ai->runGF(em.getCurrentGF(), true);
    assertBqEqualAround(__LINE__, flagPos, 6);

    // Build building
    const MapPoint bldPos = world.makeMapPoint(hqPos + Position(5, 0));
    this->SetBuildingSite(bldPos, BuildingType::Barracks);
    BOOST_TEST_REQUIRE(world.getSpecObj<noBuildingSite>(bldPos));
    em.executeNextGF();
    ai->runGF(em.getCurrentGF(), true);
    assertBqEqualAround(__LINE__, bldPos, 6);

    this->BuildRoad(world.getNeighbour(bldPos, Direction::SouthEast), false,
                    std::vector<Direction>(5, Direction::West));
    em.executeNextGF();
    ai->runGF(em.getCurrentGF(), true);
    RTTR_EXEC_TILL(2000, world.getSpecObj<noBuilding>(bldPos));
    em.executeNextGF();
    ai->runGF(em.getCurrentGF(), false);
    assertBqEqualOnWholeMap(__LINE__);

    // Gain land
    const nobMilitary* bld = world.getSpecObj<nobMilitary>(bldPos);
    for(unsigned i = 0; i < 500; i++)
    {
        em.executeNextGF();
        ai->runGF(em.getCurrentGF(), false);
        if(bld->getNumTroops() > 0u)
            break;
    }
    BOOST_TEST_REQUIRE(bld->getNumTroops() > 0u);
    assertBqEqualOnWholeMap(__LINE__);

    // Move the boundary by one node
    std::set<MapPoint, MapPointLess> outerBoundaryNodes;
    std::vector<MapPoint> borderNodes;
    RTTR_FOREACH_PT(MapPoint, world.getSize())
    {
        if(world.isBorderNode(pt, curPlayer + 1))
        {
            borderNodes.push_back(pt);
            world.checkPointsInRadius(
              pt, 1,
              [&outerBoundaryNodes, &world = this->world, curPlayer = this->curPlayer](const MapPoint curPt, unsigned) {
                  if(world.getNode(curPt).owner != curPlayer + 1)
                      outerBoundaryNodes.insert(curPt);
                  return false;
              },
              false);
        }
    }
    // Once to outside
    for(const MapPoint pt : outerBoundaryNodes)
        world.setOwner(pt, curPlayer + 1);
    world.recalcBorderStones(Position(0, 0), Extent(world.getSize()));
    for(const MapPoint pt : outerBoundaryNodes)
        world.getNotifications().publish(NodeNote(NodeNote::Owner, pt));
    em.executeNextGF();
    ai->runGF(em.getCurrentGF(), false);
    assertBqEqualOnWholeMap(__LINE__);

    // And back
    for(const MapPoint pt : outerBoundaryNodes)
        world.setOwner(pt, 0);
    world.recalcBorderStones(Position(0, 0), Extent(world.getSize()));
    for(const MapPoint pt : outerBoundaryNodes)
        world.getNotifications().publish(NodeNote(NodeNote::Owner, pt));
    em.executeNextGF();
    ai->runGF(em.getCurrentGF(), false);
    assertBqEqualOnWholeMap(__LINE__);

    // And once to inside
    for(const MapPoint pt : borderNodes)
        world.setOwner(pt, 0);
    world.recalcBorderStones(Position(0, 0), Extent(world.getSize()));
    for(const MapPoint pt : borderNodes)
        world.getNotifications().publish(NodeNote(NodeNote::Owner, pt));
    em.executeNextGF();
    ai->runGF(em.getCurrentGF(), false);
    assertBqEqualOnWholeMap(__LINE__);
}

BOOST_FIXTURE_TEST_CASE(BuildWoodIndustry, WorldWithGCExecution<1>)
{
    // Place a few trees
    for(const MapPoint& pt : world.getPointsInRadius(hqPos + MapPoint(4, 0), 2))
    {
        if(!world.getNode(pt).obj)
            world.setNO(pt, new noTree(pt, 0, 3));
    }
    world.initAfterLoad();

    const GamePlayer& player = world.getPlayer(curPlayer);
    auto ai = AIFactory::create(AI::Info(AI::Type::Default, AI::Level::Hard), curPlayer, world);
    // Build a woodcutter, sawmill and forester at some point
    for(unsigned gf = 0; gf < 2000;)
    {
        std::vector<gc::GameCommandPtr> aiGcs = ai->FetchGameCommands();
        for(unsigned i = 0; i < 5; i++, gf++)
        {
            em.executeNextGF();
            ai->runGF(em.getCurrentGF(), i == 0);
        }
        for(gc::GameCommandPtr& gc : aiGcs)
        {
            gc->execute(world, curPlayer);
        }
        if(playerHasBld(player, BuildingType::Sawmill) && playerHasBld(player, BuildingType::Woodcutter)
           && playerHasBld(player, BuildingType::Forester))
            break;
    }
    BOOST_TEST(playerHasBld(player, BuildingType::Sawmill));
    BOOST_TEST(playerHasBld(player, BuildingType::Woodcutter));
    BOOST_TEST(playerHasBld(player, BuildingType::Forester));
}

namespace {
void forceExpansion(const GamePlayer& player, GameWorld& world)
{
    // No space for saw mill due to altitude diff of 3 in range 2 -> Huts only
    for(unsigned y = 0; y < world.getHeight(); y += 2)
    {
        for(unsigned x = 0; x < world.getWidth(); x += 2)
            world.changeAltitude(MapPoint(x, y), 13);
    }
    RTTR_FOREACH_PT(MapPoint, world.getSize())
    {
        BOOST_TEST_REQUIRE(world.GetBQ(pt, player.getPlayerId()) <= BuildingQuality::Hut);
    }
}

void runUntilMilitaryBuildingSiteFound(TestEventManager& em, unsigned curPlayer, GameWorld& world,
                                       const std::list<noBuildingSite*>& bldSites)
{
    auto ai = AIFactory::create(AI::Info(AI::Type::Default, AI::Level::Hard), curPlayer, world);
    for(unsigned gf = 0; gf < 2000;)
    {
        std::vector<gc::GameCommandPtr> aiGcs = ai->FetchGameCommands();
        for(unsigned i = 0; i < 5; i++, gf++)
        {
            em.executeNextGF();
            ai->runGF(em.getCurrentGF(), i == 0);
        }
        for(gc::GameCommandPtr& gc : aiGcs)
        {
            gc->execute(world, curPlayer);
        }
        if(containsBldType(bldSites, BuildingType::Barracks) || containsBldType(bldSites, BuildingType::Guardhouse))
            break;
    }
}
} // namespace

BOOST_FIXTURE_TEST_CASE(ExpandWhenNoSpace, BiggerWorldWithGCExecution)
{
    const auto& player = world.getPlayer(curPlayer);
    const auto& bldSites = player.getBuildingRegister().getBuildingSites();

    forceExpansion(player, world);
    runUntilMilitaryBuildingSiteFound(em, curPlayer, world, bldSites);

    BOOST_TEST_REQUIRE(
      (containsBldType(bldSites, BuildingType::Barracks) || containsBldType(bldSites, BuildingType::Guardhouse)));
}

BOOST_FIXTURE_TEST_CASE(DoNotBuildMilitaryBuildingsWithinComputerBarrier, BiggerWorldWithGCExecution)
{
    const auto& player = world.getPlayer(curPlayer);
    const auto& bldSites = player.getBuildingRegister().getBuildingSites();

    const auto& barrierPt = player.getHQPos();
    constexpr auto radius = HQ_RADIUS;

    world.setComputerBarrier(barrierPt, radius);
    forceExpansion(player, world);
    runUntilMilitaryBuildingSiteFound(em, curPlayer, world, bldSites);

    BOOST_TEST_REQUIRE(
      !(containsBldType(bldSites, BuildingType::Barracks) || containsBldType(bldSites, BuildingType::Guardhouse)));
}

BOOST_FIXTURE_TEST_CASE(DoBuildMilitaryBuildingsOutsideComputerBarrier, BiggerWorldWithGCExecution)
{
    const auto& player = world.getPlayer(curPlayer);
    const auto& bldSites = player.getBuildingRegister().getBuildingSites();

    auto barrierPt = player.getHQPos();
    // move barrier 2 tiles west of HQ, now military buildings should be buildable to the east
    barrierPt.x -= 2;
    constexpr auto radius = HQ_RADIUS;

    world.setComputerBarrier(barrierPt, radius);
    forceExpansion(player, world);
    runUntilMilitaryBuildingSiteFound(em, curPlayer, world, bldSites);

    BOOST_TEST_REQUIRE(
      (containsBldType(bldSites, BuildingType::Barracks) || containsBldType(bldSites, BuildingType::Guardhouse)));

    for(noBuildingSite* bldSite : bldSites)
        BOOST_TEST_REQUIRE(!world.checkPointsInRadius(
          barrierPt, radius,
          [bldSite](const MapPoint& pt, unsigned) {
              const auto type = bldSite->getBuildingType();
              return (type == BuildingType::Barracks || type == BuildingType::Guardhouse) && pt == bldSite->getPos();
          },
          true));
}

BOOST_AUTO_TEST_SUITE_END()
