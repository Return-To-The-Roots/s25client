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

#include "notifications/NotificationManager.h"
#include "notifications/notifications.h"
#include <boost/test/unit_test.hpp>
#include <string>
#include <utility>
#include <vector>

BOOST_AUTO_TEST_SUITE(NotificationsTestSuite)

struct TestNote
{
    ENABLE_NOTIFICATION(TestNote)

    TestNote(std::string text) : text(std::move(text)) {}
    std::string text;
};

BOOST_AUTO_TEST_CASE(SubscribeAndNotify)
{
    NotificationManager mgr;
    std::vector<TestNote> notes1;
    Subscription subscription1 = mgr.subscribe<TestNote>([&notes1](const auto& note) { notes1.push_back(note); });

    mgr.publish(TestNote("Hello"));

    std::vector<TestNote> notes2;
    Subscription subscription2 = mgr.subscribe<TestNote>([&notes2](const auto& note) { notes2.push_back(note); });

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
    Subscription subscription1 = mgr.subscribe<TestNote>([&notes1](const auto& note) { notes1.push_back(note); });

    {
        Subscription subscription2 = mgr.subscribe<TestNote>([&notes2](const auto& note) { notes2.push_back(note); });
        mgr.publish(TestNote("Test"));
        BOOST_REQUIRE_EQUAL(notes1.size(), 1u);
        BOOST_REQUIRE_EQUAL(notes2.size(), 1u);
        // subscription2 goes out of scope and should be unregistred...
    }
    // ... but subscription1 should still be active
    mgr.publish(TestNote("Test"));
    BOOST_CHECK_EQUAL(notes1.size(), 2u);
    BOOST_CHECK_EQUAL(notes2.size(), 1u);
    mgr.unsubscribe(subscription1);
    // Nothing active anymore
    mgr.publish(TestNote("Test"));
    BOOST_CHECK_EQUAL(notes1.size(), 2u);
    BOOST_CHECK_EQUAL(notes2.size(), 1u);
}

BOOST_AUTO_TEST_CASE(DestroyManager)
{
    Subscription subscription;
    std::vector<TestNote> notes;
    {
        NotificationManager mgr;
        subscription = mgr.subscribe<TestNote>([&notes](const auto& note) { notes.push_back(note); });
        mgr.publish(TestNote("Test"));
        BOOST_REQUIRE_EQUAL(notes.size(), 1u);
        // Manager goes out of scope
    }
    // But we shall not crash when subscription goes out of scope
}

BOOST_AUTO_TEST_SUITE_END()
