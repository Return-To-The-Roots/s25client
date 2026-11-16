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

    void handleEvent(unsigned) override
    {
        nodeWasEmptyWhenReplaced = (world.getNode(pos).obj == nullptr);
        world.destroyNO(pos, false);
        world.setNO(pos, new noStaticObject(pos, staticObjId));
    }

    // LCOV_EXCL_START
    void destroy() override {}
    void serialize(SerializedGameData&) const override {}
    GO_Type getGOT() const final { return GO_Type::NofGeologist; }
    // LCOV_EXCL_STOP
};

struct Fixture : WorldFixture<CreateEmptyWorld, 1>
{
    const MapPoint testPos; /// Position not blocked by HQ for use
    const unsigned startNumObjs;
    Fixture() : testPos(world.getPlayer(0).getHQPos() + MapPoint(4, 4)), startNumObjs(GameObject::getNumObjs()) {}
    unsigned getNumObjs() const { return GameObject::getNumObjs() - startNumObjs; }
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
    world.setNO(testPos, &obj);
    const MapPoint testPos2 = testPos + MapPoint(1, 0);
    noBase& obj2 = *makeEnvObject<T>(testPos2);
    world.setNO(testPos2, &obj2);
    BOOST_TEST(getNumObjs() == 2u);

    // Should have an event scheduled for removal of the obj
    BOOST_TEST_REQUIRE(em.getObjEvents(obj).size() == 1u);

    if constexpr(!std::is_same_v<T, noFire>)
    {
        // Except for fire, which burns until it gets removed,
        // objects start fading after some time and only then get removed, so trigger the first event.
        em.rescheduleEvent(em.getObjEvents(obj).at(0), em.getCurrentGF() + 1);
        em.rescheduleEvent(em.getObjEvents(obj2).at(0), em.getCurrentGF() + 1);
        em.executeNextGF();
    }
    DummyWorker worker(world, testPos);
    DummyWorker worker2(world, testPos2);
    // Add worker event before and after object event, but both in the same GF
    em.addEvent(&worker, 1);
    em.rescheduleEvent(em.getObjEvents(obj).at(0), em.getCurrentGF() + 1);
    em.rescheduleEvent(em.getObjEvents(obj2).at(0), em.getCurrentGF() + 1);
    em.addEvent(&worker2, 1);

    BOOST_TEST(getNumObjs() == 4u);
    em.executeNextGF();
    // Just replaced the objects, so count should be the same.
    // A double free would decrease this multiple times per object.
    BOOST_TEST(getNumObjs() == 4u);
    // Object directly destroyed before moving to itself kill list
    BOOST_TEST(!worker.nodeWasEmptyWhenReplaced);
    // Object moved itself to kill list
    BOOST_TEST(worker2.nodeWasEmptyWhenReplaced);
    // Objects placed by "workers" are present on the map
    BOOST_TEST(ensureNonNull(world.getSpecObj<noStaticObject>(testPos)).getItemID() == DummyWorker::staticObjId);
    BOOST_TEST(ensureNonNull(world.getSpecObj<noStaticObject>(testPos2)).getItemID() == DummyWorker::staticObjId);
}

// noShipBuildingSite has no timed death event; its removal happens synchronously in MakeBuildStep.
// Completing the build needs a sea (noShip asserts on it), so only the Destroy() guard is
// exercised here: the object sits in the kill list while another object already occupies the node.
BOOST_AUTO_TEST_CASE(ShipBuildingSiteDestroyDoesNotClobberNode)
{
    // Similar for ship building sites: Replacing it by a new object must not hurt that new obj
    auto* site = new noShipBuildingSite(testPos, 0);
    world.setNO(testPos, site);
    auto* staticObj = new noStaticObject(testPos, DummyWorker::staticObjId);
    BOOST_TEST(getNumObjs() == 2u);
    world.destroyNO(testPos);
    world.setNO(testPos, staticObj);
    BOOST_TEST(getNumObjs() == 1u); // Catch possible double-free
    BOOST_TEST(ensureNonNull(world.getSpecObj<noStaticObject>(testPos)).getItemID() == DummyWorker::staticObjId);
    world.destroyNO(testPos);

    // Fully build ship, then build new object there
    // Fake a nearby sea
    world.getNodeWriteable(world.getNeighbour(testPos, Direction::SouthWest)).seaId = SeaId(1);
    site = new noShipBuildingSite(testPos, 0);
    world.setNO(testPos, site);
    BOOST_TEST(getNumObjs() == 1u);
    do
    {
        site->makeBuildStep();
    } while(world.getNO(testPos)->getGOT() == GO_Type::Shipbuildingsite);
    // Replaced by ship
    BOOST_TEST(world.getPlayer(0).getNumShips() == 1u);
    // Building site will be replaced at end of GF
    BOOST_TEST(getNumObjs() == 2u);
    BOOST_TEST(em.isObjectInKillList(*site));
    // Now place new object at the free spot
    staticObj = new noStaticObject(testPos, DummyWorker::staticObjId);
    BOOST_TEST(getNumObjs() == 3u);
    world.setNO(testPos, staticObj);

    // Should only destroy the building site
    em.executeNextGF();
    BOOST_TEST(getNumObjs() == 2u);
    BOOST_TEST(ensureNonNull(world.getSpecObj<noStaticObject>(testPos)).getItemID() == DummyWorker::staticObjId);
}

BOOST_AUTO_TEST_SUITE_END()
