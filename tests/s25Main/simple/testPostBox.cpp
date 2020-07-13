// Copyright (c) 2016 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "postSystem/PostBox.h"
#include "postSystem/PostMsg.h"
#include <boost/test/unit_test.hpp>
#include <vector>

BOOST_AUTO_TEST_SUITE(PostBoxTestSuite)

BOOST_AUTO_TEST_CASE(AddMsg)
{
    PostBox box;
    std::vector<const PostMsg*> msgs;
    BOOST_REQUIRE_EQUAL(box.GetNumMsgs(), 0u);
    BOOST_REQUIRE_EQUAL(box.GetMsg(0u), nullptr);
    for(unsigned i = 0; i < PostBox::GetMaxMsgs(); i++)
    {
        auto msg = std::make_unique<PostMsg>(0, "Test", PostCategory::General);
        const auto* msgPtr = msg.get();
        box.AddMsg(std::move(msg));
        BOOST_REQUIRE_EQUAL(box.GetNumMsgs(), i + 1);
        BOOST_REQUIRE_EQUAL(box.GetMsg(i), msgPtr);
        BOOST_REQUIRE_EQUAL(box.GetMsg(i + 1), nullptr);
        msgs.push_back(msgPtr);
    }
    // Check that messages are still in their correct positions
    for(unsigned i = 0; i < PostBox::GetMaxMsgs(); i++)
        BOOST_REQUIRE_EQUAL(box.GetMsg(i), msgs[i]);
    // Overfill
    auto msg = std::make_unique<PostMsg>(0, "Test2", PostCategory::General);
    const auto* msgPtr = msg.get();
    box.AddMsg(std::move(msg));
    BOOST_REQUIRE_EQUAL(box.GetNumMsgs(), box.GetMaxMsgs());
    BOOST_REQUIRE_EQUAL(box.GetMsg(box.GetMaxMsgs() - 1), msgPtr);
    // Check that messages have been shifted
    for(unsigned i = 0; i + 1 < PostBox::GetMaxMsgs(); i++)
        BOOST_REQUIRE_EQUAL(box.GetMsg(i), msgs[i + 1]);
}

BOOST_AUTO_TEST_CASE(DeleteMsg)
{
    PostBox box;
    BOOST_REQUIRE_EQUAL(box.DeleteMsg(0u), false);
    BOOST_REQUIRE_EQUAL(box.DeleteMsg(box.GetMaxMsgs()), false);
    // Add 1 msg
    box.AddMsg(std::make_unique<PostMsg>(0, "", PostCategory::General));
    // Deleting only this msg should succeed
    for(unsigned i = 1; i < PostBox::GetMaxMsgs(); i++)
        BOOST_REQUIRE_EQUAL(box.DeleteMsg(i), false);
    BOOST_REQUIRE_EQUAL(box.GetNumMsgs(), 1u);
    BOOST_REQUIRE_EQUAL(box.DeleteMsg(0u), true);
    BOOST_REQUIRE_EQUAL(box.GetNumMsgs(), 0u);
    auto msg = std::make_unique<PostMsg>(0, "", PostCategory::General);
    const auto* msgPtr = msg.get();
    box.AddMsg(std::move(msg));
    BOOST_REQUIRE_EQUAL(box.DeleteMsg(msgPtr), true);
    BOOST_REQUIRE_EQUAL(box.GetNumMsgs(), 0u);
    // Now fill it
    for(unsigned i = 0; i < PostBox::GetMaxMsgs(); i++)
        box.AddMsg(std::make_unique<PostMsg>(i, "Test", PostCategory::General));
    BOOST_REQUIRE_EQUAL(box.DeleteMsg(0u), true);
    BOOST_REQUIRE_EQUAL(box.DeleteMsg(5u), true);
    BOOST_REQUIRE_EQUAL(box.DeleteMsg(box.GetMaxMsgs() - 3), true);
    BOOST_REQUIRE_EQUAL(box.GetNumMsgs(), box.GetMaxMsgs() - 3);
    unsigned expected = 1;
    for(unsigned i = 0; i < box.GetNumMsgs(); i++, expected++)
    {
        if(i == 5)
            expected++; // This one was also deleted
        BOOST_REQUIRE_EQUAL(box.GetMsg(i)->GetSendFrame(), expected);
    }
    BOOST_REQUIRE_EQUAL(box.GetMsg(box.GetMaxMsgs() - 3), nullptr);
}

struct CallbackChecker
{
    PostBox& box;
    unsigned newCalls, delCalls;
    CallbackChecker(PostBox& box) : box(box), newCalls(0), delCalls(0) {}
    void OnNew(const PostMsg& /*msg*/, unsigned ct)
    {
        newCalls++;
        BOOST_REQUIRE_EQUAL(box.GetNumMsgs(), ct);
    }
    void OnDel(unsigned ct)
    {
        delCalls++;
        BOOST_REQUIRE_EQUAL(box.GetNumMsgs(), ct);
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
    BOOST_REQUIRE_EQUAL(cb.newCalls, box.GetMaxMsgs());
    BOOST_REQUIRE_EQUAL(cb.delCalls, 0u);
    for(unsigned i = 0; i < PostBox::GetMaxMsgs(); i++)
        box.DeleteMsg(0u);
    BOOST_REQUIRE_EQUAL(cb.newCalls, box.GetMaxMsgs());
    BOOST_REQUIRE_EQUAL(cb.delCalls, box.GetMaxMsgs());
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
    BOOST_REQUIRE_EQUAL(box.GetNumMsgs(), 0u);
    BOOST_REQUIRE_EQUAL(cb.delCalls, box.GetMaxMsgs());
}

BOOST_AUTO_TEST_SUITE_END()
