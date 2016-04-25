// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#include "defines.h" // IWYU pragma: keep
#include "postSystem/PostBox.h"
#include "postSystem/PostMsg.h"
#include <boost/test/unit_test.hpp>
#include <boost/bind.hpp>
#include <vector>

BOOST_AUTO_TEST_SUITE(PostBoxTestSuite)

BOOST_AUTO_TEST_CASE(AddMsg)
{
    PostBox box;
    std::vector<PostMsg*> msgs;
    BOOST_REQUIRE_EQUAL(box.GetNumMsgs(), 0u);
    BOOST_REQUIRE_EQUAL(box.GetMsg(0u), (PostMsg*) NULL);
    for(unsigned i = 0; i < box.GetMaxMsgs(); i++)
    {
        PostMsg* msg = new PostMsg(0, "Test", PMC_GENERAL);
        box.AddMsg(msg);
        BOOST_REQUIRE_EQUAL(box.GetNumMsgs(), i + 1);
        BOOST_REQUIRE_EQUAL(box.GetMsg(i), msg);
        BOOST_REQUIRE_EQUAL(box.GetMsg(i + 1), (PostMsg*) NULL);
        msgs.push_back(msg);
    }
    // Check that messages are still in their correct positions
    for(unsigned i = 0; i < box.GetMaxMsgs(); i++)
        BOOST_REQUIRE_EQUAL(box.GetMsg(i), msgs[i]);
    // Overfill
    PostMsg* msg = new PostMsg(0, "Test2", PMC_GENERAL);
    box.AddMsg(msg);
    BOOST_REQUIRE_EQUAL(box.GetNumMsgs(), box.GetMaxMsgs());
    BOOST_REQUIRE_EQUAL(box.GetMsg(box.GetMaxMsgs() - 1), msg);
    // Check that messages have been shifted
    for(unsigned i = 0; i + 1 < box.GetMaxMsgs(); i++)
        BOOST_REQUIRE_EQUAL(box.GetMsg(i), msgs[i + 1]);
}

BOOST_AUTO_TEST_CASE(DeleteMsg)
{
    PostBox box;
    BOOST_REQUIRE_EQUAL(box.DeleteMsg(0u), false);
    BOOST_REQUIRE_EQUAL(box.DeleteMsg(box.GetMaxMsgs()), false);
    // Add 1 msg
    box.AddMsg(new PostMsg(0, "", PMC_OTHER));
    // Deleting only this msg should succeed
    for(unsigned i = 1; i < box.GetMaxMsgs(); i++)
        BOOST_REQUIRE_EQUAL(box.DeleteMsg(i), false);
    BOOST_REQUIRE_EQUAL(box.GetNumMsgs(), 1u);
    BOOST_REQUIRE_EQUAL(box.DeleteMsg(0u), true);
    BOOST_REQUIRE_EQUAL(box.GetNumMsgs(), 0u);
    PostMsg* msg = new PostMsg(0, "", PMC_GENERAL);
    box.AddMsg(msg);
    BOOST_REQUIRE_EQUAL(box.DeleteMsg(msg), true);
    BOOST_REQUIRE_EQUAL(box.GetNumMsgs(), 0u);
    // Now fill it
    for(unsigned i = 0; i < box.GetMaxMsgs(); i++)
        box.AddMsg(new PostMsg(i, "Test", PMC_GENERAL));
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
    BOOST_REQUIRE_EQUAL(box.GetMsg(box.GetMaxMsgs() - 3), (PostMsg*) NULL);
}

struct CallbackChecker
{
    PostBox& box;
    unsigned newCalls, delCalls;
    CallbackChecker(PostBox& box): box(box), newCalls(0), delCalls(0){}
    void OnNew(unsigned ct)
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
    box.ObserveNewMsg(boost::bind(&CallbackChecker::OnNew, &cb, _1));
    box.ObserveDeletedMsg(boost::bind(&CallbackChecker::OnDel, &cb, _1));
    for(unsigned i = 0; i < box.GetMaxMsgs(); i++)
        box.AddMsg(new PostMsg(i, "Test", PMC_GENERAL));
    BOOST_REQUIRE_EQUAL(cb.newCalls, box.GetMaxMsgs());
    BOOST_REQUIRE_EQUAL(cb.delCalls, 0u);
    for(unsigned i = 0; i < box.GetMaxMsgs(); i++)
        box.DeleteMsg(0u);
    BOOST_REQUIRE_EQUAL(cb.newCalls, box.GetMaxMsgs());
    BOOST_REQUIRE_EQUAL(cb.delCalls, box.GetMaxMsgs());
}

BOOST_AUTO_TEST_SUITE_END()
