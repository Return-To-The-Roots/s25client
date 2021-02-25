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

#include "FrameCounter.h"
#include "Timer.h"
#include <rttr/test/MockClock.hpp>
#include <boost/test/unit_test.hpp>
#include <helpers/chronoIO.h>
#include <sstream>

// LCOV_EXCL_START
namespace boost { namespace test_tools { namespace tt_detail {
    template<class T, class R>
    struct print_log_value<std::chrono::duration<T, R>>
    {
        void operator()(std::ostream& out, const std::chrono::duration<T, R>& value)
        {
            out << helpers::withUnit(value);
        }
    };
}}} // namespace boost::test_tools::tt_detail
// LCOV_EXCL_STOP

BOOST_AUTO_TEST_SUITE(Timers)

BOOST_AUTO_TEST_CASE(ChronoIO)
{
    {
        std::ostringstream s;
        s << helpers::withUnit(std::chrono::milliseconds(42));
        BOOST_TEST(s.str() == "42ms");
    }
    {
        std::ostringstream s;
        s << helpers::withUnit(std::chrono::seconds(31));
        BOOST_TEST(s.str() == "31s");
    }
    {
        std::ostringstream s;
        s << helpers::withUnit(std::chrono::minutes(12));
        BOOST_TEST(s.str() == "12min");
    }
}

BOOST_FIXTURE_TEST_CASE(TimerClass, rttr::test::MockClockFixture)
{
    using namespace std::chrono;
    Timer timer;
    BOOST_TEST_REQUIRE(!timer.isRunning());
    // getElapsed on non-running timer throws
    BOOST_REQUIRE_THROW(timer.getElapsed(), std::runtime_error);
    currentTime += seconds(5);
    timer.start();
    BOOST_TEST_REQUIRE(timer.isRunning());
    BOOST_TEST_REQUIRE(timer.getElapsed() == milliseconds(0));
    currentTime += milliseconds(3);
    BOOST_TEST_REQUIRE(timer.getElapsed() == milliseconds(3));
    // Multiple requests allowed
    BOOST_TEST_REQUIRE(timer.getElapsed() == milliseconds(3));
    currentTime += milliseconds(10);
    BOOST_TEST_REQUIRE(timer.getElapsed() == milliseconds(13));
    // start on running timer throws and does not change instance
    BOOST_REQUIRE_THROW(timer.start(), std::runtime_error);
    BOOST_TEST_REQUIRE(timer.getElapsed() == milliseconds(13));
    timer.stop();
    BOOST_TEST_REQUIRE(!timer.isRunning());
    // Restart
    timer.start();
    BOOST_TEST_REQUIRE(timer.getElapsed() == milliseconds(0));
    currentTime += milliseconds(5);
    BOOST_TEST_REQUIRE(timer.getElapsed() == milliseconds(5));
    timer.restart();
    BOOST_TEST_REQUIRE(timer.getElapsed() == milliseconds(0));
    timer.stop();
    // Works on stopped
    timer.restart();
    currentTime += milliseconds(15);
    BOOST_TEST_REQUIRE(timer.getElapsed() == milliseconds(15));
}

BOOST_AUTO_TEST_CASE(FrameCounterBasic)
{
    // Let numFPS frames pass and then check after updateInterval that frame rate matches
    for(unsigned numFPS = 10; numFPS < 420; numFPS += 7)
    {
        FrameCounter ctr;
        FrameCounter::clock::time_point time, startTime;
        for(unsigned t = 0; t < numFPS; t++)
        {
            ctr.update(time);
            time += std::chrono::milliseconds(1);
            BOOST_TEST_REQUIRE(ctr.getFrameRate() == 0u);
        }
        time = startTime + std::chrono::seconds(1);
        // And again but with twice the rate. Framerate should be calculated after next update
        startTime = time;
        for(unsigned t = 0; t < numFPS; t++)
        {
            ctr.update(time);
            time += std::chrono::milliseconds(1);
            ctr.update(time);
            time += std::chrono::milliseconds(1);
            BOOST_TEST_REQUIRE(ctr.getFrameRate() == numFPS);
        }
        time = startTime + std::chrono::seconds(1);
        ctr.update(time);
        BOOST_TEST_REQUIRE(ctr.getFrameRate() == numFPS * 2u);
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
            ctr.update(time += std::chrono::microseconds(1));
        time = startTime + std::chrono::seconds(10);
        ctr.update(time); // 66 Frames in 10s -> 6.6 FPS = 7
        BOOST_TEST_REQUIRE(ctr.getFrameRate() == 7u);
        startTime = time;
    }
    for(int i = 0; i < 20; i++)
    {
        for(int t = 1; t < 64; t++)
            ctr.update(time += std::chrono::microseconds(1));
        time = startTime + std::chrono::seconds(10);
        ctr.update(time); // 64 Frames in 10s -> 6.4 FPS = 6
        BOOST_TEST_REQUIRE(ctr.getFrameRate() == 6u);
        startTime = time;
    }
}

BOOST_AUTO_TEST_CASE(FrameTimerBasic)
{
    using namespace std::chrono;

    FrameTimer::clock::time_point time = FrameTimer::clock::now();
    FrameTimer timer_(10, 5, time); // 10 FPS, max 5 frames behind
    milliseconds frameTime(1000 / 10);
    FrameTimer::clock::duration zero(0);
    // one full frameTime to next
    BOOST_TEST_REQUIRE(timer_.calcTimeToNextFrame(time) == frameTime);
    // Time passes exactly 1 frame and recheck
    time += frameTime;
    BOOST_TEST_REQUIRE(timer_.calcTimeToNextFrame(time) == zero);
    timer_.update(time);
    BOOST_TEST_REQUIRE(timer_.calcTimeToNextFrame(time) == frameTime);

    // Time passes a bit, but next frame not reached yet
    FrameTimer::clock::duration diff = milliseconds(5);
    time += frameTime - diff;
    BOOST_TEST_REQUIRE(timer_.calcTimeToNextFrame(time) == diff);
    // Time passes to after next frame
    time += 2 * diff;
    BOOST_TEST_REQUIRE(timer_.calcTimeToNextFrame(time) == zero);
    // After this the next frame will be a bit early
    timer_.update(time);
    BOOST_TEST_REQUIRE(timer_.calcTimeToNextFrame(time) == frameTime - diff);
}

BOOST_AUTO_TEST_SUITE_END()
