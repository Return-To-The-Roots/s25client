// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GameEvent.h"
#include "GameObject.h"
#include "RTTR_AssertError.h"
#include "worldFixtures/TestEventManager.h"
#include <rttr/test/LogAccessor.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(GameEventsTestSuite)

BOOST_AUTO_TEST_CASE(AdvanceGFs)
{
    EventManager evMgr(42);
    BOOST_TEST_REQUIRE(evMgr.getCurrentGF() == 42u);
    for(unsigned gf = 43; gf < 50; gf++)
    {
        evMgr.executeNextGF();
        BOOST_TEST_REQUIRE(evMgr.getCurrentGF() == gf);
    }
}

class TestEventHandler : public GameObject
{
public:
    std::vector<unsigned> handledEventIds;

    void handleEvent(unsigned evId) override { handledEventIds.push_back(evId); }
    // LCOV_EXCL_START
    void destroy() override {}
    void Serialize(SerializedGameData&) const override {}
    GO_Type getGOT() const final { return GO_Type::Staticobject; }
    // LCOV_EXCL_STOP
};

BOOST_AUTO_TEST_CASE(AddAndExecuteEvent)
{
    const unsigned startGF = 1;
    TestEventManager evMgr(startGF);
    TestEventHandler obj;
    // Check that adding an event works and data is correctly set
    const GameEvent* ev = evMgr.addEvent(&obj, 5, 42);
    BOOST_TEST_REQUIRE(ev->obj == &obj);
    BOOST_TEST_REQUIRE(ev->startGF == startGF);
    BOOST_TEST_REQUIRE(ev->length == 5u);
    BOOST_TEST_REQUIRE(ev->id == 42u);
    BOOST_TEST_REQUIRE(ev->getTargetGF() == startGF + ev->length);
    BOOST_TEST(evMgr.isEventActive(obj, 42));
    BOOST_TEST(!evMgr.isEventActive(obj, 43));
    BOOST_TEST(evMgr.objectHasEvents(obj));
    // Check that event is not executed before it is due
    for(unsigned i = startGF + 1; i < ev->getTargetGF(); i++)
    {
        evMgr.executeNextGF();
        BOOST_TEST_REQUIRE(evMgr.getCurrentGF() == i); // Rechecked for clarity
        BOOST_TEST_REQUIRE(obj.handledEventIds.size() == 0u);
    }
    BOOST_TEST_REQUIRE(evMgr.getCurrentGF() + 1u == ev->getTargetGF());
    // Now the event should be executed
    evMgr.executeNextGF();
    BOOST_TEST_REQUIRE(obj.handledEventIds.size() == 1u);
    BOOST_TEST_REQUIRE(obj.handledEventIds.front() == 42u);
    // And nothing should be left
    BOOST_TEST(!evMgr.isEventActive(obj, 42));
    BOOST_TEST(!evMgr.objectHasEvents(obj));
}

BOOST_AUTO_TEST_CASE(AddSuspendedEvent)
{
    const unsigned startGF = 100;
    EventManager evMgr(startGF);
    TestEventHandler obj;
    // Add an event with length 50, where 48 GFs have already elapsed (2 left)
    const GameEvent* ev = evMgr.addEvent(&obj, 50, 42, 48);
    BOOST_TEST_REQUIRE(ev->obj == &obj);
    BOOST_TEST_REQUIRE(ev->startGF == startGF - 48u); // "Started" 48 GFs ago
    BOOST_TEST_REQUIRE(ev->length == 50u);
    BOOST_TEST_REQUIRE(ev->id == 42u);
    BOOST_TEST_REQUIRE(ev->getTargetGF() == startGF + 2);
    // Check that event is not executed before it is due
    evMgr.executeNextGF();
    BOOST_TEST_REQUIRE(obj.handledEventIds.size() == 0u);
    // Now the event should be executed
    evMgr.executeNextGF();
    BOOST_TEST_REQUIRE(obj.handledEventIds.size() == 1u);
    BOOST_TEST_REQUIRE(obj.handledEventIds.front() == 42u);
}

BOOST_AUTO_TEST_CASE(MultipleEvents)
{
    EventManager evMgr(0);
    TestEventHandler obj;
    evMgr.addEvent(&obj, 5, 42); //-V525
    evMgr.addEvent(&obj, 5, 43);
    evMgr.addEvent(&obj, 6, 44);
    // Execute first 2 events
    for(unsigned i = 1; i <= 5; i++)
        evMgr.executeNextGF();
    BOOST_TEST_REQUIRE(obj.handledEventIds.size() == 2u);
    BOOST_TEST_REQUIRE(obj.handledEventIds[0] == 42u);
    BOOST_TEST_REQUIRE(obj.handledEventIds[1] == 43u);
    // And 3rd
    evMgr.executeNextGF();
    BOOST_TEST_REQUIRE(obj.handledEventIds.size() == 3u);
    BOOST_TEST_REQUIRE(obj.handledEventIds[2] == 44u);
}

BOOST_AUTO_TEST_CASE(Reschedule)
{
    TestEventManager evMgr(0);
    TestEventHandler obj;
    const GameEvent* ev = evMgr.addEvent(&obj, 5, 42);
    // Run 3 of the 5 GFs
    for(unsigned i = 1; i <= 3; i++)
        evMgr.executeNextGF();
    std::vector<const GameEvent*> evts = evMgr.getObjEvents(obj);
    BOOST_TEST_REQUIRE(evts.size() == 1u);
    BOOST_TEST_REQUIRE(evts.front() == ev);
    // Execute at GF 7 instead
    evMgr.rescheduleEvent(evts.front(), 7);
    for(unsigned i = 3; i <= 5; i++)
        evMgr.executeNextGF();
    BOOST_TEST_REQUIRE(obj.handledEventIds.size() == 0u);
    for(unsigned i = 5; i <= 7; i++)
        evMgr.executeNextGF();
    BOOST_TEST_REQUIRE(obj.handledEventIds.size() == 1u);
}

class TestLogKill final : public GameObject
{
public:
    EventManager& em;
    TestLogKill(EventManager& em) : em(em) {}

    static unsigned killNum, destroyNum;
    ~TestLogKill() override { killNum++; }
    void handleEvent(unsigned /*evId*/) override
    {
        BOOST_TEST_REQUIRE(!em.isObjectInKillList(*this));
        // Kill this on event
        em.addToKillList(this);
        BOOST_TEST_REQUIRE(em.isObjectInKillList(*this));
    }
    // LCOV_EXCL_START
    void destroy() override { destroyNum++; }
    void Serialize(SerializedGameData&) const override {}
    GO_Type getGOT() const final { return GO_Type::Staticobject; }
    // LCOV_EXCL_STOP
};

unsigned TestLogKill::killNum = 0;
unsigned TestLogKill::destroyNum = 0;

BOOST_AUTO_TEST_CASE(KillList)
{
    EventManager evMgr(0);
    auto* obj = new TestLogKill(evMgr);
    evMgr.addEvent(obj, 2);
    // Nothing should happened yet
    evMgr.executeNextGF();
    BOOST_TEST_REQUIRE(TestLogKill::killNum == 0u);
    BOOST_TEST_REQUIRE(TestLogKill::destroyNum == 0u);
    // And now it should be killed (destroy and delete)
    evMgr.executeNextGF();
    BOOST_TEST_REQUIRE(TestLogKill::killNum == 1u);
    BOOST_TEST_REQUIRE(TestLogKill::destroyNum == 1u);
    BOOST_TEST_REQUIRE(!evMgr.isObjectInKillList(*obj));
}

class TestRemoveEvent : public TestEventHandler
{
public:
    EventManager& em;
    const GameEvent* ev2Remove;
    TestRemoveEvent(EventManager& em) : em(em), ev2Remove(nullptr) {}

    void handleEvent(unsigned evId) override
    {
        TestEventHandler::handleEvent(evId);
        if(evId == 42 && ev2Remove)
        {
            em.removeEvent(ev2Remove);
            // Should be set to nullptr
            BOOST_TEST_REQUIRE(!ev2Remove);
        }
    }
};

BOOST_AUTO_TEST_CASE(removeEvent)
{
    EventManager evMgr(0);
    TestRemoveEvent obj(evMgr);
    evMgr.addEvent(&obj, 1, 42);
    obj.ev2Remove = evMgr.addEvent(&obj, 2, 43);
    // Execute time for both events. Only first should have fired
    evMgr.executeNextGF();
    evMgr.executeNextGF();
    BOOST_TEST_REQUIRE(obj.handledEventIds.size() == 1u);
    BOOST_TEST_REQUIRE(obj.handledEventIds.front() == 42u);

    // Try the same but with an event in the same GF
    evMgr.addEvent(&obj, 1, 42);
    obj.ev2Remove = evMgr.addEvent(&obj, 1, 43);
    evMgr.executeNextGF();
    BOOST_TEST_REQUIRE(obj.handledEventIds.size() == 2u);
    BOOST_TEST_REQUIRE(obj.handledEventIds[1] == 42u);
    BOOST_TEST(!evMgr.objectHasEvents(obj));
}

#if RTTR_ENABLE_ASSERTS
BOOST_AUTO_TEST_CASE(InvalidEvent)
{
    rttr::test::LogAccessor logAcc;
    EventManager evMgr(100);
    TestEventHandler obj;
    // Need object
    RTTR_REQUIRE_ASSERT(evMgr.addEvent(nullptr, 1));
    // Length must be > 0
    RTTR_REQUIRE_ASSERT(evMgr.addEvent(&obj, 0));
    // ... even for continued events
    RTTR_REQUIRE_ASSERT(evMgr.addEvent(nullptr, 50, 0, 50));
    // continued event cannot start before the game
    RTTR_REQUIRE_ASSERT(evMgr.addEvent(nullptr, 200, 0, 150));
}
#endif

BOOST_AUTO_TEST_SUITE_END()
