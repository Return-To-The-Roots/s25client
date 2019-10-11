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

#include "drivers/VideoDriverWrapper.h"
#include "helpers/containerUtils.h"
#include "mockupDrivers/MockupVideoDriver.h"
#include "uiHelper/uiHelpers.hpp"
#include <libutil/warningSuppression.h>
#include <glad/glad.h>
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

namespace rttrOglMock2 {
RTTR_IGNORE_DIAGNOSTIC("-Wmissing-declarations")

std::vector<GLuint> activeTextures;

void APIENTRY glGenTextures(GLsizei n, GLuint* textures)
{
    static GLuint cur = 0;
    for(; n > 0; --n)
    {
        *(textures++) = ++cur;
        activeTextures.push_back(cur);
    }
}
void APIENTRY glDeleteTextures(GLsizei n, const GLuint* textures)
{
    for(; n > 0; --n)
    {
        BOOST_TEST(*textures != 0u);
        BOOST_TEST(helpers::contains(activeTextures, *textures));
        helpers::remove(activeTextures, *(textures++));
    }
}

RTTR_POP_DIAGNOSTIC
} // namespace rttrOglMock2

BOOST_FIXTURE_TEST_CASE(CreateAndDestroyTextures, uiHelper::Fixture)
{
    uiHelper::initGUITests();
    // Fresh start
    VIDEODRIVER.DestroyScreen();
    VIDEODRIVER.CreateScreen(VideoMode(800, 600), false);

    glGenTextures = rttrOglMock2::glGenTextures;
    glDeleteTextures = rttrOglMock2::glDeleteTextures;
    for(unsigned i = 1u; i <= 5u; ++i)
        BOOST_TEST(VIDEODRIVER.GenerateTexture() == i);
    BOOST_TEST_REQUIRE(rttrOglMock2::activeTextures.size() == 5u);
    VIDEODRIVER.DestroyScreen();
    BOOST_TEST_REQUIRE(rttrOglMock2::activeTextures.empty());
    // Next cleanup call is a no-op (validated inside glDeleteTextures)
    VIDEODRIVER.CleanUp();

    VIDEODRIVER.CreateScreen(VideoMode(800, 600), false);
    glGenTextures = rttrOglMock2::glGenTextures;
    glDeleteTextures = rttrOglMock2::glDeleteTextures;
    for(unsigned i = 1u; i <= 5u; ++i)
        VIDEODRIVER.GenerateTexture();
    BOOST_TEST_REQUIRE(rttrOglMock2::activeTextures.size() == 5u);
    VIDEODRIVER.CleanUp();
    BOOST_TEST_REQUIRE(rttrOglMock2::activeTextures.empty());
    // Next cleanup call is a no-op (validated inside glDeleteTextures)
    VIDEODRIVER.CleanUp();
}
