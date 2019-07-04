// Copyright (c) 2016 - 2019 Settlers Freaks (sf-team at siedler25.org)
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

#include "mockupDrivers/MockupVideoDriver.h"
#include "uiHelper/uiHelpers.hpp"
#include <boost/test/unit_test.hpp>

// LCOV_EXCL_START
static std::ostream& operator<<(std::ostream& os, const VideoMode& mode)
{
    return os << mode.width << "x" << mode.height;
}
// LCOV_EXCL_STOP

BOOST_AUTO_TEST_CASE(FindClosestVideoMode)
{
    auto* driver = uiHelper::GetVideoDriver();
    driver->video_modes_ = {VideoMode(800, 600), VideoMode(1000, 600), VideoMode(1500, 800)};
    // Exact match
    for(const auto& mode : driver->video_modes_)
        BOOST_TEST(driver->FindClosestVideoMode(mode) == mode);
    // Close match
    BOOST_TEST(driver->FindClosestVideoMode(VideoMode(850, 622)) == VideoMode(800, 600));
    BOOST_TEST(driver->FindClosestVideoMode(VideoMode(950, 570)) == VideoMode(1000, 600));
    BOOST_TEST(driver->FindClosestVideoMode(VideoMode(2000, 622)) == VideoMode(1500, 800));
    BOOST_TEST(driver->FindClosestVideoMode(VideoMode(1200, 900)) == VideoMode(1500, 800));
}
