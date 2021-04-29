// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "postSystem/PostBox.h"
#include "postSystem/PostMsg.h"
#include <boost/test/unit_test.hpp>
#include <vector>

BOOST_AUTO_TEST_SUITE(PostBoxTestSuite)

BOOST_AUTO_TEST_CASE(AddMsg)
{
    PostBox box;
    std::vector<const PostMsg*> msgs;
    BOOST_TEST_REQUIRE(box.GetNumMsgs() == 0u);
    BOOST_TEST_REQUIRE(!box.GetMsg(0u));
    for(unsigned i = 0; i < PostBox::GetMaxMsgs(); i++)
    {
        auto msg = std::make_unique<PostMsg>(0, "Test", PostCategory::General);
        const auto* msgPtr = msg.get();
        box.AddMsg(std::move(msg));
        BOOST_TEST_REQUIRE(box.GetNumMsgs() == i + 1);
        BOOST_TEST_REQUIRE(box.GetMsg(i) == msgPtr);
        BOOST_TEST_REQUIRE(!box.GetMsg(i + 1));
        msgs.push_back(msgPtr);
    }
    // Check that messages are still in their correct positions
    for(unsigned i = 0; i < PostBox::GetMaxMsgs(); i++)
        BOOST_TEST_REQUIRE(box.GetMsg(i) == msgs[i]);
    // Overfill
    auto msg = std::make_unique<PostMsg>(0, "Test2", PostCategory::General);
    const auto* msgPtr = msg.get();
    box.AddMsg(std::move(msg));
    BOOST_TEST_REQUIRE(box.GetNumMsgs() == box.GetMaxMsgs());
    BOOST_TEST_REQUIRE(box.GetMsg(box.GetMaxMsgs() - 1) == msgPtr);
    // Check that messages have been shifted
    for(unsigned i = 0; i + 1 < PostBox::GetMaxMsgs(); i++)
        BOOST_TEST_REQUIRE(box.GetMsg(i) == msgs[i + 1]);
}

BOOST_AUTO_TEST_CASE(DeleteMsg)
{
    PostBox box;
    BOOST_TEST_REQUIRE(box.DeleteMsg(0u) == false);
    BOOST_TEST_REQUIRE(box.DeleteMsg(box.GetMaxMsgs()) == false);
    // Add 1 msg
    box.AddMsg(std::make_unique<PostMsg>(0, "", PostCategory::General));
    // Deleting only this msg should succeed
    for(unsigned i = 1; i < PostBox::GetMaxMsgs(); i++)
        BOOST_TEST_REQUIRE(box.DeleteMsg(i) == false);
    BOOST_TEST_REQUIRE(box.GetNumMsgs() == 1u);
    BOOST_TEST_REQUIRE(box.DeleteMsg(0u) == true);
    BOOST_TEST_REQUIRE(box.GetNumMsgs() == 0u);
    auto msg = std::make_unique<PostMsg>(0, "", PostCategory::General);
    const auto* msgPtr = msg.get();
    box.AddMsg(std::move(msg));
    BOOST_TEST_REQUIRE(box.DeleteMsg(msgPtr) == true);
    BOOST_TEST_REQUIRE(box.GetNumMsgs() == 0u);
    // Now fill it
    for(unsigned i = 0; i < PostBox::GetMaxMsgs(); i++)
        box.AddMsg(std::make_unique<PostMsg>(i, "Test", PostCategory::General));
    BOOST_TEST_REQUIRE(box.DeleteMsg(0u) == true);
    BOOST_TEST_REQUIRE(box.DeleteMsg(5u) == true);
    BOOST_TEST_REQUIRE(box.DeleteMsg(box.GetMaxMsgs() - 3) == true);
    BOOST_TEST_REQUIRE(box.GetNumMsgs() == box.GetMaxMsgs() - 3);
    unsigned expected = 1;
    for(unsigned i = 0; i < box.GetNumMsgs(); i++, expected++)
    {
        if(i == 5)
            expected++; // This one was also deleted
        BOOST_TEST_REQUIRE(box.GetMsg(i)->GetSendFrame() == expected);
    }
    BOOST_TEST_REQUIRE(!box.GetMsg(box.GetMaxMsgs() - 3));
}

struct CallbackChecker
{
    PostBox& box;
    unsigned newCalls, delCalls;
    CallbackChecker(PostBox& box) : box(box), newCalls(0), delCalls(0) {}
    void OnNew(const PostMsg& /*msg*/, unsigned ct)
    {
        newCalls++;
        BOOST_TEST_REQUIRE(box.GetNumMsgs() == ct);
    }
    void OnDel(unsigned ct)
    {
        delCalls++;
        BOOST_TEST_REQUIRE(box.GetNumMsgs() == ct);
    }
};

BOOST_AUTO_TEST_CASE(MsgCallbacks)
{
    PostBox box;
    CallbackChecker cb(box);
    box.ObserveNewMsg([&cb](const auto& msg, auto ct) { cb.OnNew(msg, ct); });
    box.ObserveDeletedMsg([&cb](auto ct) { cb.OnDel(ct); });
    for(unsigned i = 0; i < PostBox::GetMaxMsgs(); i++)
        box.AddMsg(std::make_unique<PostMsg>(i, "Test", PostCategory::General));
    BOOST_TEST_REQUIRE(cb.newCalls == box.GetMaxMsgs());
    BOOST_TEST_REQUIRE(cb.delCalls == 0u);
    for(unsigned i = 0; i < PostBox::GetMaxMsgs(); i++)
        box.DeleteMsg(0u);
    BOOST_TEST_REQUIRE(cb.newCalls == box.GetMaxMsgs());
    BOOST_TEST_REQUIRE(cb.delCalls == box.GetMaxMsgs());
}

BOOST_AUTO_TEST_CASE(ClearMsgs)
{
    PostBox box;
    CallbackChecker cb(box);
    box.ObserveDeletedMsg([&cb](auto ct) { cb.OnDel(ct); });
    for(unsigned i = 0; i < PostBox::GetMaxMsgs(); i++)
        box.AddMsg(std::make_unique<PostMsg>(i, "Test", PostCategory::General));
    // Deleting should delete all messages and call delete callback for each one
    box.Clear();
    BOOST_TEST_REQUIRE(box.GetNumMsgs() == 0u);
    BOOST_TEST_REQUIRE(cb.delCalls == box.GetMaxMsgs());
}

BOOST_AUTO_TEST_SUITE_END()
