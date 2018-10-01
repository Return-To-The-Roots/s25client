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

#include "rttrDefines.h" // IWYU pragma: keep
#include "notifications/NotificationManager.h"
#include "notifications/notifications.h"
#include <boost/bind.hpp>
#include <boost/test/unit_test.hpp>
#include <string>
#include <vector>

BOOST_AUTO_TEST_SUITE(NotificationsTestSuite)

struct TestNote
{
    ENABLE_NOTIFICATION(TestNote)

    TestNote(const std::string& text) : text(text) {}
    std::string text;
};

struct PushToVec
{
    static void push(std::vector<TestNote>& vec, const TestNote& note) { vec.push_back(note); }
};

BOOST_AUTO_TEST_CASE(SubscribeAndNotify)
{
    NotificationManager mgr;
    std::vector<TestNote> notes1;
    Subscribtion subscribtion1 = mgr.subscribe<TestNote>(boost::bind(&PushToVec::push, boost::ref(notes1), _1));

    mgr.publish(TestNote("Hello"));

    std::vector<TestNote> notes2;
    Subscribtion subscribtion2 = mgr.subscribe<TestNote>(boost::bind(&PushToVec::push, boost::ref(notes2), _1));

    mgr.publish(TestNote("World"));

    BOOST_CHECK_EQUAL(notes1.size(), 2u);
    BOOST_CHECK_EQUAL("Hello", notes1[0].text);
    BOOST_CHECK_EQUAL("World", notes1[1].text);

    BOOST_CHECK_EQUAL(notes2.size(), 1u);
    BOOST_CHECK_EQUAL("World", notes2[0].text);
}

BOOST_AUTO_TEST_CASE(Unsubscribe)
{
    NotificationManager mgr;
    std::vector<TestNote> notes1, notes2;
    Subscribtion subscribtion1 = mgr.subscribe<TestNote>(boost::bind(&PushToVec::push, boost::ref(notes1), _1));

    {
        Subscribtion subscribtion2 = mgr.subscribe<TestNote>(boost::bind(&PushToVec::push, boost::ref(notes2), _1));
        mgr.publish(TestNote("Test"));
        BOOST_REQUIRE_EQUAL(notes1.size(), 1u);
        BOOST_REQUIRE_EQUAL(notes2.size(), 1u);
        // subscribtion2 goes out of scope and should be unregistred...
    }
    // ... but subscription1 should still be active
    mgr.publish(TestNote("Test"));
    BOOST_CHECK_EQUAL(notes1.size(), 2u);
    BOOST_CHECK_EQUAL(notes2.size(), 1u);
    mgr.unsubscribe(subscribtion1);
    // Nothing active anymore
    mgr.publish(TestNote("Test"));
    BOOST_CHECK_EQUAL(notes1.size(), 2u);
    BOOST_CHECK_EQUAL(notes2.size(), 1u);
}

BOOST_AUTO_TEST_CASE(DestroyManager)
{
    Subscribtion subscribtion;
    std::vector<TestNote> notes;
    {
        NotificationManager mgr;
        subscribtion = mgr.subscribe<TestNote>(boost::bind(&PushToVec::push, boost::ref(notes), _1));
        mgr.publish(TestNote("Test"));
        BOOST_REQUIRE_EQUAL(notes.size(), 1u);
        // Manager goes out of scope
    }
    // But we shall not crash when subscribtion goes out of scope
}

BOOST_AUTO_TEST_SUITE_END()
