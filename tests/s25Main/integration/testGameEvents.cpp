// Copyright (c) 2016 - 2017 Settlers Freaks (sf-team at siedler25.org)
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
    BOOST_TEST_REQUIRE(evMgr.GetCurrentGF() == 42u);
    for(unsigned gf = 43; gf < 50; gf++)
    {
        evMgr.ExecuteNextGF();
        BOOST_TEST_REQUIRE(evMgr.GetCurrentGF() == gf);
    }
}

class TestEventHandler : public GameObject
{
public:
    std::vector<unsigned> handledEventIds;

    void HandleEvent(unsigned evId) override { handledEventIds.push_back(evId); }
    // LCOV_EXCL_START
    void Destroy() override {}
    void Serialize(SerializedGameData&) const override {}
    GO_Type GetGOT() const final { return GO_Type::Staticobject; }
    // LCOV_EXCL_STOP
};

BOOST_AUTO_TEST_CASE(AddAndExecuteEvent)
{
    const unsigned startGF = 1;
    TestEventManager evMgr(startGF);
    TestEventHandler obj;
    // Check that adding an event works and data is correctly set
    const GameEvent* ev = evMgr.AddEvent(&obj, 5, 42);
    BOOST_TEST_REQUIRE(ev->obj == &obj);
    BOOST_TEST_REQUIRE(ev->startGF == startGF);
    BOOST_TEST_REQUIRE(ev->length == 5u);
    BOOST_TEST_REQUIRE(ev->id == 42u);
    BOOST_TEST_REQUIRE(ev->GetTargetGF() == startGF + ev->length);
    BOOST_CHECK(evMgr.IsEventActive(obj, 42));
    BOOST_CHECK(!evMgr.IsEventActive(obj, 43));
    BOOST_CHECK(evMgr.ObjectHasEvents(obj));
    // Check that event is not executed before it is due
    for(unsigned i = startGF + 1; i < ev->GetTargetGF(); i++)
    {
        evMgr.ExecuteNextGF();
        BOOST_TEST_REQUIRE(evMgr.GetCurrentGF() == i); // Rechecked for clarity
        BOOST_TEST_REQUIRE(obj.handledEventIds.size() == 0u);
    }
    BOOST_TEST_REQUIRE(evMgr.GetCurrentGF() + 1u == ev->GetTargetGF());
    // Now the event should be executed
    evMgr.ExecuteNextGF();
    BOOST_TEST_REQUIRE(obj.handledEventIds.size() == 1u);
    BOOST_TEST_REQUIRE(obj.handledEventIds.front() == 42u);
    // And nothing should be left
    BOOST_CHECK(!evMgr.IsEventActive(obj, 42));
    BOOST_CHECK(!evMgr.ObjectHasEvents(obj));
}

BOOST_AUTO_TEST_CASE(AddSuspendedEvent)
{
    const unsigned startGF = 100;
    EventManager evMgr(startGF);
    TestEventHandler obj;
    // Add an event with length 50, where 48 GFs have already elapsed (2 left)
    const GameEvent* ev = evMgr.AddEvent(&obj, 50, 42, 48);
    BOOST_TEST_REQUIRE(ev->obj == &obj);
    BOOST_TEST_REQUIRE(ev->startGF == startGF - 48u); // "Started" 48 GFs ago
    BOOST_TEST_REQUIRE(ev->length == 50u);
    BOOST_TEST_REQUIRE(ev->id == 42u);
    BOOST_TEST_REQUIRE(ev->GetTargetGF() == startGF + 2);
    // Check that event is not executed before it is due
    evMgr.ExecuteNextGF();
    BOOST_TEST_REQUIRE(obj.handledEventIds.size() == 0u);
    // Now the event should be executed
    evMgr.ExecuteNextGF();
    BOOST_TEST_REQUIRE(obj.handledEventIds.size() == 1u);
    BOOST_TEST_REQUIRE(obj.handledEventIds.front() == 42u);
}

BOOST_AUTO_TEST_CASE(MultipleEvents)
{
    EventManager evMgr(0);
    TestEventHandler obj;
    evMgr.AddEvent(&obj, 5, 42); //-V525
    evMgr.AddEvent(&obj, 5, 43);
    evMgr.AddEvent(&obj, 6, 44);
    // Execute first 2 events
    for(unsigned i = 1; i <= 5; i++)
        evMgr.ExecuteNextGF();
    BOOST_TEST_REQUIRE(obj.handledEventIds.size() == 2u);
    BOOST_TEST_REQUIRE(obj.handledEventIds[0] == 42u);
    BOOST_TEST_REQUIRE(obj.handledEventIds[1] == 43u);
    // And 3rd
    evMgr.ExecuteNextGF();
    BOOST_TEST_REQUIRE(obj.handledEventIds.size() == 3u);
    BOOST_TEST_REQUIRE(obj.handledEventIds[2] == 44u);
}

BOOST_AUTO_TEST_CASE(Reschedule)
{
    TestEventManager evMgr(0);
    TestEventHandler obj;
    const GameEvent* ev = evMgr.AddEvent(&obj, 5, 42);
    // Run 3 of the 5 GFs
    for(unsigned i = 1; i <= 3; i++)
        evMgr.ExecuteNextGF();
    std::vector<const GameEvent*> evts = evMgr.GetObjEvents(obj);
    BOOST_TEST_REQUIRE(evts.size() == 1u);
    BOOST_TEST_REQUIRE(evts.front() == ev);
    // Execute at GF 7 instead
    evMgr.RescheduleEvent(evts.front(), 7);
    for(unsigned i = 3; i <= 5; i++)
        evMgr.ExecuteNextGF();
    BOOST_TEST_REQUIRE(obj.handledEventIds.size() == 0u);
    for(unsigned i = 5; i <= 7; i++)
        evMgr.ExecuteNextGF();
    BOOST_TEST_REQUIRE(obj.handledEventIds.size() == 1u);
}

class TestLogKill final : public GameObject
{
public:
    EventManager& em;
    TestLogKill(EventManager& em) : em(em) {}

    static unsigned killNum, destroyNum;
    ~TestLogKill() override { killNum++; }
    void HandleEvent(unsigned /*evId*/) override
    {
        BOOST_TEST_REQUIRE(!em.IsObjectInKillList(*this));
        // Kill this on event
        em.AddToKillList(this);
        BOOST_TEST_REQUIRE(em.IsObjectInKillList(*this));
    }
    // LCOV_EXCL_START
    void Destroy() override { destroyNum++; }
    void Serialize(SerializedGameData&) const override {}
    GO_Type GetGOT() const final { return GO_Type::Staticobject; }
    // LCOV_EXCL_STOP
};

unsigned TestLogKill::killNum = 0;
unsigned TestLogKill::destroyNum = 0;

BOOST_AUTO_TEST_CASE(KillList)
{
    EventManager evMgr(0);
    auto* obj = new TestLogKill(evMgr);
    evMgr.AddEvent(obj, 2);
    // Nothing should happened yet
    evMgr.ExecuteNextGF();
    BOOST_TEST_REQUIRE(TestLogKill::killNum == 0u);
    BOOST_TEST_REQUIRE(TestLogKill::destroyNum == 0u);
    // And not it should be killed (destroy and delete)
    evMgr.ExecuteNextGF();
    BOOST_TEST_REQUIRE(TestLogKill::killNum == 1u);
    BOOST_TEST_REQUIRE(TestLogKill::destroyNum == 1u);
    BOOST_TEST_REQUIRE(!evMgr.IsObjectInKillList(*obj));
}

class TestRemoveEvent : public TestEventHandler
{
public:
    EventManager& em;
    const GameEvent* ev2Remove;
    TestRemoveEvent(EventManager& em) : em(em), ev2Remove(nullptr) {}

    void HandleEvent(unsigned evId) override
    {
        TestEventHandler::HandleEvent(evId);
        if(evId == 42 && ev2Remove)
        {
            em.RemoveEvent(ev2Remove);
            // Should be set to nullptr
            BOOST_TEST_REQUIRE(!ev2Remove);
        }
    }
};

BOOST_AUTO_TEST_CASE(RemoveEvent)
{
    EventManager evMgr(0);
    TestRemoveEvent obj(evMgr);
    evMgr.AddEvent(&obj, 1, 42);
    obj.ev2Remove = evMgr.AddEvent(&obj, 2, 43);
    // Execute time for both events. Only first should have fired
    evMgr.ExecuteNextGF();
    evMgr.ExecuteNextGF();
    BOOST_TEST_REQUIRE(obj.handledEventIds.size() == 1u);
    BOOST_TEST_REQUIRE(obj.handledEventIds.front() == 42u);

    // Try the same but with an event in the same GF
    evMgr.AddEvent(&obj, 1, 42);
    obj.ev2Remove = evMgr.AddEvent(&obj, 1, 43);
    evMgr.ExecuteNextGF();
    BOOST_TEST_REQUIRE(obj.handledEventIds.size() == 2u);
    BOOST_TEST_REQUIRE(obj.handledEventIds[1] == 42u);
    BOOST_CHECK(!evMgr.ObjectHasEvents(obj));
}

#if RTTR_ENABLE_ASSERTS
BOOST_AUTO_TEST_CASE(InvalidEvent)
{
    rttr::test::LogAccessor logAcc;
    EventManager evMgr(100);
    TestEventHandler obj;
    // Need object
    RTTR_REQUIRE_ASSERT(evMgr.AddEvent(nullptr, 1));
    // Length must be > 0
    RTTR_REQUIRE_ASSERT(evMgr.AddEvent(&obj, 0));
    // ... even for continued events
    RTTR_REQUIRE_ASSERT(evMgr.AddEvent(nullptr, 50, 0, 50));
    // continued event cannot start before the game
    RTTR_REQUIRE_ASSERT(evMgr.AddEvent(nullptr, 200, 0, 150));
}
#endif

BOOST_AUTO_TEST_SUITE_END()
