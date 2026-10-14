// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GameObject.h"
#include "GamePlayer.h"
#include "worldFixtures/CreateEmptyWorld.h"
#include "worldFixtures/TestEventManager.h"
#include "worldFixtures/WorldFixture.h"
#include "world/GameWorld.h"
#include "nodeObjs/noDisappearingMapEnvObject.h"
#include "nodeObjs/noEnvObject.h"
#include "nodeObjs/noFire.h"
#include "nodeObjs/noShipBuildingSite.h"
#include "nodeObjs/noSign.h"
#include "nodeObjs/noSkeleton.h"
#include "rttr/test/testHelpers.hpp"
#include <boost/mpl/list.hpp>
#include <boost/test/unit_test.hpp>
#include <memory>
#include <type_traits>

namespace {
/// Stands in for a worker that places an object on a event,
/// e.g. a geologist digging, then placing a sign.
/// The crucial part is that it runs *after* the object's death event but *before* the kill
/// list is processed, which is exactly the window in which the double free used to happen.
struct DummyWorker : GameObject
{
    GameWorld& world;
    MapPoint pos;
    bool nodeWasEmptyWhenReplaced = false;
    static constexpr unsigned staticObjId = 42;

    explicit DummyWorker(GameWorld& world, MapPoint pos) : world(world), pos(pos) {}

    void HandleEvent(unsigned) override
    {
        nodeWasEmptyWhenReplaced = (world.GetNode(pos).obj == nullptr);
        world.DestroyNO(pos, false);
        world.SetNO(pos, new noStaticObject(pos, staticObjId));
    }

    // LCOV_EXCL_START
    void Destroy() override {}
    void Serialize(SerializedGameData&) const override {}
    GO_Type GetGOT() const final { return GO_Type::NofGeologist; }
    // LCOV_EXCL_STOP
};

struct Fixture : WorldFixture<CreateEmptyWorld, 1>
{
    const MapPoint testPos; /// Position not blocked by HQ for use
    const unsigned startNumObjs;
    Fixture() : testPos(world.GetPlayer(0).GetHQPos() + MapPoint(4, 4)), startNumObjs(GameObject::GetNumObjs()) {}
    unsigned getNumObjs() const { return GameObject::GetNumObjs() - startNumObjs; }
};

template<class T>
noBase* makeEnvObject(MapPoint pos)
{
    if constexpr(std::is_same_v<T, noSign>)
        return new noSign(pos, Resource(ResourceType::Iron, 4));
    else if constexpr(std::is_same_v<T, noSkeleton>)
        return new noSkeleton(pos);
    else
        return new T(pos, 0); // Others can be constructed with a position and int or bool
}
} // namespace

BOOST_FIXTURE_TEST_SUITE(DisappearingObjects, Fixture)

using DisappearingEnvObjs = boost::mpl::list<noSign, noDisappearingMapEnvObject, noSkeleton, noFire>;

/// Reproducer for #1967: An object replacing an existing object might be placed in the same GF as when that object
/// removed it self by a timed event. This must not lead to a Destroy/free call from both the kill list and the replace
/// action.
BOOST_AUTO_TEST_CASE_TEMPLATE(RemoveEnvObjectFromNodeBeforeKill, T, DisappearingEnvObjs)
{
    noBase& obj = *makeEnvObject<T>(testPos);
    world.SetNO(testPos, &obj);
    const MapPoint testPos2 = testPos + MapPoint(1, 0);
    noBase& obj2 = *makeEnvObject<T>(testPos2);
    world.SetNO(testPos2, &obj2);
    BOOST_TEST(getNumObjs() == 2u);

    // Should have an event scheduled for removal of the obj
    BOOST_TEST_REQUIRE(em.GetObjEvents(obj).size() == 1u);

    if constexpr(!std::is_same_v<T, noFire>)
    {
        // Except for fire, which burns until it gets removed,
        // objects start fading after some time and only then get removed, so trigger the first event.
        em.RescheduleEvent(em.GetObjEvents(obj).at(0), em.GetCurrentGF() + 1);
        em.RescheduleEvent(em.GetObjEvents(obj2).at(0), em.GetCurrentGF() + 1);
        em.ExecuteNextGF();
    }
    DummyWorker worker(world, testPos);
    DummyWorker worker2(world, testPos2);
    // Add worker event before and after object event, but both in the same GF
    em.AddEvent(&worker, 1);
    em.RescheduleEvent(em.GetObjEvents(obj).at(0), em.GetCurrentGF() + 1);
    em.RescheduleEvent(em.GetObjEvents(obj2).at(0), em.GetCurrentGF() + 1);
    em.AddEvent(&worker2, 1);

    BOOST_TEST(getNumObjs() == 4u);
    em.ExecuteNextGF();
    // Just replaced the objects, so count should be the same.
    // A double free would decrease this multiple times per object.
    BOOST_TEST(getNumObjs() == 4u);
    // Object directly destroyed before moving to itself kill list
    BOOST_TEST(!worker.nodeWasEmptyWhenReplaced);
    // Object moved itself to kill list
    BOOST_TEST(worker2.nodeWasEmptyWhenReplaced);
    // Objects placed by "workers" are present on the map
    BOOST_TEST(ensureNonNull(world.GetSpecObj<noStaticObject>(testPos)).GetItemID() == DummyWorker::staticObjId);
    BOOST_TEST(ensureNonNull(world.GetSpecObj<noStaticObject>(testPos2)).GetItemID() == DummyWorker::staticObjId);
}

// noShipBuildingSite has no timed death event; its removal happens synchronously in MakeBuildStep.
// Completing the build needs a sea (noShip asserts on it), so only the Destroy() guard is
// exercised here: the object sits in the kill list while another object already occupies the node.
BOOST_AUTO_TEST_CASE(ShipBuildingSiteDestroyDoesNotClobberNode)
{
    // Similar for ship building sites: Replacing it by a new object must not hurt that new obj
    auto* site = new noShipBuildingSite(testPos, 0);
    world.SetNO(testPos, site);
    auto* staticObj = new noStaticObject(testPos, DummyWorker::staticObjId);
    BOOST_TEST(getNumObjs() == 2u);
    world.DestroyNO(testPos);
    world.SetNO(testPos, staticObj);
    BOOST_TEST(getNumObjs() == 1u); // Catch possible double-free
    BOOST_TEST(ensureNonNull(world.GetSpecObj<noStaticObject>(testPos)).GetItemID() == DummyWorker::staticObjId);
    world.DestroyNO(testPos);

    // Fully build ship, then build new object there
    // Fake a nearby sea
    world.GetNodeWriteable(world.GetNeighbour(testPos, Direction::SouthWest)).seaId = SeaId(1);
    site = new noShipBuildingSite(testPos, 0);
    world.SetNO(testPos, site);
    BOOST_TEST(getNumObjs() == 1u);
    do
    {
        site->MakeBuildStep();
    } while(world.GetNO(testPos)->GetGOT() == GO_Type::Shipbuildingsite);
    // Replaced by ship
    BOOST_TEST(world.GetPlayer(0).GetNumShips() == 1u);
    // Building site will be replaced at end of GF
    BOOST_TEST(getNumObjs() == 2u);
    BOOST_TEST(em.IsObjectInKillList(*site));
    // Now place new object at the free spot
    staticObj = new noStaticObject(testPos, DummyWorker::staticObjId);
    BOOST_TEST(getNumObjs() == 3u);
    world.SetNO(testPos, staticObj);

    // Should only destroy the building site
    em.ExecuteNextGF();
    BOOST_TEST(getNumObjs() == 2u);
    BOOST_TEST(ensureNonNull(world.GetSpecObj<noStaticObject>(testPos)).GetItemID() == DummyWorker::staticObjId);
}

BOOST_AUTO_TEST_SUITE_END()
