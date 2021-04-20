// Copyright (c) 2016 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#define BOOST_TEST_MODULE RTTR_Notifications

#include "notifications/NotificationManager.h"
#include "notifications/notifications.h"
#include "testNoteFunctions.h"
#include <boost/test/unit_test.hpp>
#include <string>
#include <utility>
#include <vector>

#if RTTR_HAS_VLD
#    include <vld.h>
#endif

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
    const Subscription sub1 = mgr.subscribe<StringNote>([&notes1](const StringNote& note) { notes1.push_back(note); });

    mgr.publish(StringNote{"Hello"});

    std::vector<StringNote> notes2;
    const Subscription sub2 = mgr.subscribe<StringNote>([&notes2](const StringNote& note) { notes2.push_back(note); });
    std::vector<IntNote> notes3;
    const Subscription sub3 = mgr.subscribe<IntNote>([&notes3](const IntNote& note) { notes3.push_back(note); });

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

BOOST_AUTO_TEST_CASE(PublishBeforeSubscribe)
{
    NotificationManager mgr;
    // No-Ops
    mgr.publish(StringNote{"Test"});
    mgr.publish(IntNote{42});

    std::string lastString;
    const auto sub1 = mgr.subscribe<StringNote>([&lastString](const StringNote& note) { lastString = note.value; });
    mgr.publish(IntNote{42}); // No-Op
    BOOST_TEST(lastString.empty());

    int lastValue = 0;
    const auto sub2 = mgr.subscribe<IntNote>([&lastValue](const IntNote& note) { lastValue = note.value; });
    mgr.publish(StringNote{"Test"});
    mgr.publish(IntNote{42});
    BOOST_TEST(lastString == "Test");
    BOOST_TEST(lastValue == 42);
}

BOOST_AUTO_TEST_CASE(Unsubscribe)
{
    NotificationManager mgr;
    std::vector<StringNote> notes1, notes2;
    Subscription subscription1 = mgr.subscribe<StringNote>([&notes1](const auto& note) { notes1.push_back(note); });

    {
        const Subscription sub2 = mgr.subscribe<StringNote>([&notes2](const auto& note) { notes2.push_back(note); });
        mgr.publish(StringNote{"Test"});
        BOOST_TEST(notes1.size() == 1u);
        BOOST_TEST(notes2.size() == 1u);
        // sub2 goes out of scope and should be unregistred...
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

BOOST_AUTO_TEST_CASE(SubscribeDuringPublish)
{
    NotificationManager mgr;
    {
        // Subscribe next callback
        Subscription sub2, sub3;
        int called1 = 0, called2 = 0;
        std::string called3;
        const auto sub = mgr.subscribe<IntNote>([&](const IntNote&) {
            if(called1++ == 0)
            {
                BOOST_TEST_CHECKPOINT("Triggered 1st callback, subscribing 2nd");
                sub2 = mgr.subscribe<IntNote>([&](const IntNote&) {
                    if(called2++ == 0)
                    {
                        BOOST_TEST_CHECKPOINT("Triggered 2nd callback, subscribing 3rd");
                        sub3 = mgr.subscribe<StringNote>([&](const StringNote&) { called3 = "1"; });
                    }
                });
            }
        });
        mgr.publish(IntNote{});
        BOOST_TEST(sub2);
        BOOST_TEST(sub3);
        BOOST_TEST(called1 == 1);
        BOOST_TEST(called2 == 1);
        mgr.publish(IntNote{});
        BOOST_TEST(called1 == 2);
        BOOST_TEST(called2 == 2);
        BOOST_TEST(called3.empty());
        mgr.publish(StringNote{});
        BOOST_TEST(called3 == "1");
    }
}
BOOST_AUTO_TEST_CASE(UnsubscribeDuringPublish)
{
    NotificationManager mgr;
    {
        Subscription sub;
        int called = 0;
        sub = mgr.subscribe<IntNote>([&](const IntNote&) {
            called++;
            // Unsubscribe self
            mgr.unsubscribe(sub);
        });
        mgr.publish(IntNote{});
        BOOST_TEST(!sub);
        BOOST_TEST(called == 1);
        mgr.publish(IntNote{});
        BOOST_TEST(called == 1);
    }
    {
        // Unsubscribe later callback on 2nd call
        Subscription sub1, sub2;
        int called1 = 0, called2 = 0;
        sub1 = mgr.subscribe<IntNote>([&](const IntNote&) {
            if(called1++)
                mgr.unsubscribe(sub2);
        });
        sub2 = mgr.subscribe<IntNote>([&](const IntNote&) { called2++; });
        mgr.publish(IntNote{});
        BOOST_TEST(called1 == 1);
        BOOST_TEST(called2 == 1);
        mgr.publish(IntNote{});
        BOOST_TEST(!sub2);
        BOOST_TEST(called1 == 2);
        BOOST_TEST(called2 == 1);
        mgr.publish(IntNote{});
        BOOST_TEST(called1 == 3);
        BOOST_TEST(called2 == 1);
    }
}

BOOST_AUTO_TEST_CASE(PublishDuringPublish)
{
    NotificationManager mgr;
    std::string lastString;
    int lastValue = 0;

    const auto sub1 = mgr.subscribe<StringNote>([&](const StringNote& note) {
        lastString = note.value;
        mgr.publish(IntNote{42});
    });
    const auto sub2 = mgr.subscribe<IntNote>([&lastValue](const IntNote& note) { lastValue = note.value; });

    mgr.publish(StringNote{"Test"});
    BOOST_TEST(lastString == "Test");
    BOOST_TEST(lastValue == 42);
}

BOOST_AUTO_TEST_CASE(SendAndReceiveAcrossTranslationUnits)
{
    NotificationManager mgr;
    std::string lastString;
    int lastValue = 0;
    const auto sub1 = mgr.subscribe<StringNote>([&lastString](const StringNote& note) { lastString = note.value; });
    const auto sub2 = mgr.subscribe<IntNote>([&lastValue](const IntNote& note) { lastValue = note.value; });
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

BOOST_AUTO_TEST_CASE(ThrowDuringPublish)
{
    NotificationManager mgr;
    int called1 = 0, called2 = 0;
    Subscription sub1 = mgr.subscribe<IntNote>([&](const IntNote&) {
        if(called1++ == 0)
            throw std::runtime_error("Something went wrong");
    });
    Subscription sub2 = mgr.subscribe<IntNote>([&](const IntNote&) { called2++; });
    // First publish throws, 2nd note is not called
    BOOST_CHECK_THROW(mgr.publish(IntNote{}), std::runtime_error);
    BOOST_TEST(called1 == 1);
    BOOST_TEST(called2 == 0);
    // 2nd publish does not and hence both counters are increased
    mgr.publish(IntNote{});
    BOOST_TEST(called1 == 2);
    BOOST_TEST(called2 == 1);
}

BOOST_AUTO_TEST_SUITE_END()
