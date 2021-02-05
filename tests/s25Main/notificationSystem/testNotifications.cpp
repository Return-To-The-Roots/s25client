// Copyright (c) 2016 - 2021 Settlers Freaks (sf-team at siedler25.org)
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

#define BOOST_TEST_MODULE RTTR_Notifications

#include "notifications/NotificationManager.h"
#include "notifications/notifications.h"
#include "testNoteFunctions.h"
#include <boost/test/unit_test.hpp>
#include <string>
#include <utility>
#include <vector>

BOOST_AUTO_TEST_SUITE(Notifications)

struct StringNote
{
    ENABLE_NOTIFICATION(StringNote)

    std::string value;
};
struct IntNote
{
    ENABLE_NOTIFICATION(IntNote)

    int value = 0;
};

BOOST_AUTO_TEST_CASE(NoteIdsAreUnique)
{
    BOOST_TEST(StringNote::getNoteId() != IntNote::getNoteId());
    BOOST_TEST(StringNote::getNoteId() != getTestNote1IdRecv());
    BOOST_TEST(StringNote::getNoteId() != getTestNote2IdRecv());

    BOOST_TEST(IntNote::getNoteId() != getTestNote1IdRecv());
    BOOST_TEST(IntNote::getNoteId() != getTestNote2IdRecv());

    BOOST_TEST(getTestNote1IdRecv() != getTestNote2IdRecv());

    BOOST_TEST(getTestNote1IdRecv() == getTestNote1IdSend());
    BOOST_TEST(getTestNote2IdRecv() == getTestNote2IdSend());
}

BOOST_AUTO_TEST_CASE(SubscribeAndNotify)
{
    NotificationManager mgr;
    std::vector<StringNote> notes1;
    Subscription subscription1 =
      mgr.subscribe<StringNote>([&notes1](const StringNote& note) { notes1.push_back(note); });

    mgr.publish(StringNote{"Hello"});

    std::vector<StringNote> notes2;
    Subscription subscription2 =
      mgr.subscribe<StringNote>([&notes2](const StringNote& note) { notes2.push_back(note); });
    std::vector<IntNote> notes3;
    Subscription subscription3 = mgr.subscribe<IntNote>([&notes3](const IntNote& note) { notes3.push_back(note); });

    mgr.publish(StringNote{"World"});
    mgr.publish(IntNote{42});

    BOOST_TEST(notes1.size() == 2u);
    BOOST_TEST(notes1[0].value == "Hello");
    BOOST_TEST(notes1[1].value == "World");

    BOOST_TEST(notes2.size() == 1u);
    BOOST_TEST(notes2[0].value == "World");

    BOOST_TEST(notes3.size() == 1u);
    BOOST_TEST(notes3[0].value == 42);
}

BOOST_AUTO_TEST_CASE(Unsubscribe)
{
    NotificationManager mgr;
    std::vector<StringNote> notes1, notes2;
    Subscription subscription1 = mgr.subscribe<StringNote>([&notes1](const auto& note) { notes1.push_back(note); });

    {
        Subscription subscription2 = mgr.subscribe<StringNote>([&notes2](const auto& note) { notes2.push_back(note); });
        mgr.publish(StringNote{"Test"});
        BOOST_TEST(notes1.size() == 1u);
        BOOST_TEST(notes2.size() == 1u);
        // subscription2 goes out of scope and should be unregistred...
    }
    // ... but subscription1 should still be active
    mgr.publish(StringNote{"Test"});
    BOOST_TEST(notes1.size() == 2u);
    BOOST_TEST(notes2.size() == 1u);
    mgr.unsubscribe(subscription1);
    // Nothing active anymore
    mgr.publish(StringNote{"Test"});
    BOOST_TEST(notes1.size() == 2u);
    BOOST_TEST(notes2.size() == 1u);
}

BOOST_AUTO_TEST_CASE(DestroyManager)
{
    Subscription subscription;
    std::vector<StringNote> notes;
    {
        NotificationManager mgr;
        subscription = mgr.subscribe<StringNote>([&notes](const auto& note) { notes.push_back(note); });
        mgr.publish(StringNote{"Test"});
        BOOST_TEST(notes.size() == 1u);
        // Manager goes out of scope
    }
    // But we shall not crash when subscription goes out of scope
}

BOOST_AUTO_TEST_CASE(SendAndReceiveAcrossTranslationUnits)
{
    NotificationManager mgr;
    std::string lastString;
    int lastValue = 0;
    auto sub1 = mgr.subscribe<StringNote>([&lastString](const StringNote& note) { lastString = note.value; });
    auto sub2 = mgr.subscribe<IntNote>([&lastValue](const IntNote& note) { lastValue = note.value; });
    subscribeTestNote1(mgr);
    subscribeTestNote2(mgr);

    BOOST_TEST_REQUIRE(getLastTestNote1() == 0);
    BOOST_TEST_REQUIRE(getLastTestNote2() == 0);

    mgr.publish(StringNote{"Test"});
    mgr.publish(IntNote{42});
    sendTestNote1(mgr, 1337);
    sendTestNote2(mgr, 7331);
    BOOST_TEST(lastString == "Test");
    BOOST_TEST(lastValue == 42);
    BOOST_TEST(getLastTestNote1() == 1337);
    BOOST_TEST(getLastTestNote2() == 7331);
}

BOOST_AUTO_TEST_SUITE_END()
