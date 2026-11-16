// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "postSystem/PostBox.h"
#include "postSystem/PostMsg.h"
#include <boost/test/unit_test.hpp>
#include <vector>

BOOST_AUTO_TEST_SUITE(PostBoxTestSuite)

BOOST_AUTO_TEST_CASE(addMsg)
{
    PostBox box;
    std::vector<const PostMsg*> msgs;
    BOOST_TEST_REQUIRE(box.getNumMsgs() == 0u);
    BOOST_TEST_REQUIRE(!box.getMsg(0u));
    for(unsigned i = 0; i < PostBox::getMaxMsgs(); i++)
    {
        auto msg = std::make_unique<PostMsg>(0, "Test", PostCategory::General);
        const auto* msgPtr = msg.get();
        box.addMsg(std::move(msg));
        BOOST_TEST_REQUIRE(box.getNumMsgs() == i + 1);
        BOOST_TEST_REQUIRE(box.getMsg(i) == msgPtr);
        BOOST_TEST_REQUIRE(!box.getMsg(i + 1));
        msgs.push_back(msgPtr);
    }
    // Check that messages are still in their correct positions
    for(unsigned i = 0; i < PostBox::getMaxMsgs(); i++)
        BOOST_TEST_REQUIRE(box.getMsg(i) == msgs[i]);
    // Overfill
    auto msg = std::make_unique<PostMsg>(0, "Test2", PostCategory::General);
    const auto* msgPtr = msg.get();
    box.addMsg(std::move(msg));
    BOOST_TEST_REQUIRE(box.getNumMsgs() == box.getMaxMsgs());
    BOOST_TEST_REQUIRE(box.getMsg(box.getMaxMsgs() - 1) == msgPtr);
    // Check that messages have been shifted
    for(unsigned i = 0; i + 1 < PostBox::getMaxMsgs(); i++)
        BOOST_TEST_REQUIRE(box.getMsg(i) == msgs[i + 1]);
}

BOOST_AUTO_TEST_CASE(deleteMsg)
{
    PostBox box;
    BOOST_TEST_REQUIRE(box.deleteMsg(0u) == false);
    BOOST_TEST_REQUIRE(box.deleteMsg(box.getMaxMsgs()) == false);
    // Add 1 msg
    box.addMsg(std::make_unique<PostMsg>(0, "", PostCategory::General));
    // Deleting only this msg should succeed
    for(unsigned i = 1; i < PostBox::getMaxMsgs(); i++)
        BOOST_TEST_REQUIRE(box.deleteMsg(i) == false);
    BOOST_TEST_REQUIRE(box.getNumMsgs() == 1u);
    BOOST_TEST_REQUIRE(box.deleteMsg(0u) == true);
    BOOST_TEST_REQUIRE(box.getNumMsgs() == 0u);
    auto msg = std::make_unique<PostMsg>(0, "", PostCategory::General);
    const auto* msgPtr = msg.get();
    box.addMsg(std::move(msg));
    BOOST_TEST_REQUIRE(box.deleteMsg(msgPtr) == true);
    BOOST_TEST_REQUIRE(box.getNumMsgs() == 0u);
    // Now fill it
    for(unsigned i = 0; i < PostBox::getMaxMsgs(); i++)
        box.addMsg(std::make_unique<PostMsg>(i, "Test", PostCategory::General));
    BOOST_TEST_REQUIRE(box.deleteMsg(0u) == true);
    BOOST_TEST_REQUIRE(box.deleteMsg(5u) == true);
    BOOST_TEST_REQUIRE(box.deleteMsg(box.getMaxMsgs() - 3) == true);
    BOOST_TEST_REQUIRE(box.getNumMsgs() == box.getMaxMsgs() - 3);
    unsigned expected = 1;
    for(unsigned i = 0; i < box.getNumMsgs(); i++, expected++)
    {
        if(i == 5)
            expected++; // This one was also deleted
        BOOST_TEST_REQUIRE(box.getMsg(i)->getSendFrame() == expected);
    }
    BOOST_TEST_REQUIRE(!box.getMsg(box.getMaxMsgs() - 3));
}

struct CallbackChecker
{
    PostBox& box;
    unsigned newCalls, delCalls;
    CallbackChecker(PostBox& box) : box(box), newCalls(0), delCalls(0) {}
    void onNew(const PostMsg& /*msg*/, unsigned ct)
    {
        newCalls++;
        BOOST_TEST_REQUIRE(box.getNumMsgs() == ct);
    }
    void onDel(unsigned ct)
    {
        delCalls++;
        BOOST_TEST_REQUIRE(box.getNumMsgs() == ct);
    }
};

BOOST_AUTO_TEST_CASE(MsgCallbacks)
{
    PostBox box;
    CallbackChecker cb(box);
    box.observeNewMsg([&cb](const auto& msg, auto ct) { cb.onNew(msg, ct); });
    box.observeDeletedMsg([&cb](auto ct) { cb.onDel(ct); });
    for(unsigned i = 0; i < PostBox::getMaxMsgs(); i++)
        box.addMsg(std::make_unique<PostMsg>(i, "Test", PostCategory::General));
    BOOST_TEST_REQUIRE(cb.newCalls == box.getMaxMsgs());
    BOOST_TEST_REQUIRE(cb.delCalls == 0u);
    for(unsigned i = 0; i < PostBox::getMaxMsgs(); i++)
        box.deleteMsg(0u);
    BOOST_TEST_REQUIRE(cb.newCalls == box.getMaxMsgs());
    BOOST_TEST_REQUIRE(cb.delCalls == box.getMaxMsgs());
}

BOOST_AUTO_TEST_CASE(ClearMsgs)
{
    PostBox box;
    CallbackChecker cb(box);
    box.observeDeletedMsg([&cb](auto ct) { cb.onDel(ct); });
    for(unsigned i = 0; i < PostBox::getMaxMsgs(); i++)
        box.addMsg(std::make_unique<PostMsg>(i, "Test", PostCategory::General));
    // Deleting should delete all messages and call delete callback for each one
    box.clear();
    BOOST_TEST_REQUIRE(box.getNumMsgs() == 0u);
    BOOST_TEST_REQUIRE(cb.delCalls == box.getMaxMsgs());
}

BOOST_AUTO_TEST_SUITE_END()
