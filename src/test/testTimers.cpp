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
#include "FrameCounter.h"
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_SUITE(Timers)

BOOST_AUTO_TEST_CASE(FrameCounterBasic)
{
    // Let numFPS frames pass and then check after updateInterval that frame rate matches
    for(unsigned numFPS = 10; numFPS < 420; ++numFPS)
    {
        FrameCounter ctr;
        FrameCounter::clock::time_point time, startTime;
        for(unsigned t = 0; t < numFPS; t++)
        {
            ctr.update(time);
            time += boost::chrono::milliseconds(1);
            BOOST_REQUIRE_EQUAL(ctr.getFrameRate(), 0u);
        }
        time = startTime + boost::chrono::seconds(1);
        // And again but with twice the rate. Framerate should be calculated after next update
        startTime = time;
        for(unsigned t = 0; t < numFPS; t++)
        {
            ctr.update(time);
            time += boost::chrono::milliseconds(1);
            ctr.update(time);
            time += boost::chrono::milliseconds(1);
            BOOST_REQUIRE_EQUAL(ctr.getFrameRate(), numFPS);
        }
        time = startTime + boost::chrono::seconds(1);
        ctr.update(time);
        BOOST_REQUIRE_EQUAL(ctr.getFrameRate(), numFPS * 2u);
    }
}

BOOST_AUTO_TEST_CASE(FrameCounterRounding)
{
    FrameCounter ctr;
    FrameCounter::clock::time_point time, startTime;
    // Repeat a couple of times to make sure it does not change
    for(int i = 0; i < 20; i++)
    {
        for(int t = 1; t < 66; t++)
            ctr.update(time += boost::chrono::microseconds(1));
        time = startTime + boost::chrono::seconds(10);
        ctr.update(time); // 66 Frames in 10s -> 6.6 FPS = 7
        BOOST_REQUIRE_EQUAL(ctr.getFrameRate(), 7u);
        startTime = time;
    }
    for(int i = 0; i < 20; i++)
    {
        for(int t = 1; t < 64; t++)
            ctr.update(time += boost::chrono::microseconds(1));
        time = startTime + boost::chrono::seconds(10);
        ctr.update(time); // 64 Frames in 10s -> 6.4 FPS = 6
        BOOST_REQUIRE_EQUAL(ctr.getFrameRate(), 6u);
        startTime = time;
    }
}

BOOST_AUTO_TEST_CASE(FrameTimerBasic)
{
    using namespace boost::chrono;
    FrameTimer::clock::time_point time = FrameTimer::clock::now();
    FrameTimer timer_(10, 5, time); // 10 FPS, max 5 frames behind
    milliseconds frameTime(1000 / 10);
    FrameTimer::clock::duration zero(0);
    // one full frameTime to next
    BOOST_REQUIRE_EQUAL(timer_.calcTimeToNextFrame(time), frameTime);
    // Time passes exactly 1 frame and recheck
    time += frameTime;
    BOOST_REQUIRE_EQUAL(timer_.calcTimeToNextFrame(time), zero);
    timer_.update(time);
    BOOST_REQUIRE_EQUAL(timer_.calcTimeToNextFrame(time), frameTime);

    // Time passes a bit, but next frame not reached yet
    FrameTimer::clock::duration diff = milliseconds(5);
    time += frameTime - diff;
    BOOST_REQUIRE_EQUAL(timer_.calcTimeToNextFrame(time), diff);
    // Time passes to after next frame
    time += 2 * diff;
    BOOST_REQUIRE_EQUAL(timer_.calcTimeToNextFrame(time), zero);
    // After this the next frame will be a bit early
    timer_.update(time);
    BOOST_REQUIRE_EQUAL(timer_.calcTimeToNextFrame(time), frameTime - diff);
}

BOOST_AUTO_TEST_SUITE_END()
