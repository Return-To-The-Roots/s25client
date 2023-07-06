// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "PointOutput.h"
#include "drivers/VideoDriverWrapper.h"
#include "helpers/containerUtils.h"
#include "mockupDrivers/MockupVideoDriver.h"
#include "uiHelper/uiHelpers.hpp"
#include "rttr/test/LogAccessor.hpp"
#include <rttr/test/stubFunction.hpp>
#include <s25util/warningSuppression.h>
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
        helpers::erase(activeTextures, *(textures++));
    }
}

RTTR_POP_DIAGNOSTIC
} // namespace rttrOglMock2

BOOST_FIXTURE_TEST_CASE(CreateAndDestroyTextures, uiHelper::Fixture)
{
    // Fresh start
    {
        rttr::test::LogAccessor logAcc;
        VIDEODRIVER.DestroyScreen();
        VIDEODRIVER.CreateScreen(VideoMode(800, 600), false);
        logAcc.clearLog();
    }

    RTTR_STUB_FUNCTION(glGenTextures, rttrOglMock2::glGenTextures);
    RTTR_STUB_FUNCTION(glDeleteTextures, rttrOglMock2::glDeleteTextures);

    for(unsigned i = 1u; i <= 5u; ++i)
        BOOST_TEST(VIDEODRIVER.GenerateTexture() == i);
    BOOST_TEST_REQUIRE(rttrOglMock2::activeTextures.size() == 5u);
    {
        rttr::test::LogAccessor logAcc;
        VIDEODRIVER.DestroyScreen();
        logAcc.clearLog();
    }
    BOOST_TEST_REQUIRE(rttrOglMock2::activeTextures.empty());
    // Next cleanup call is a no-op (validated inside glDeleteTextures)
    VIDEODRIVER.CleanUp();

    {
        rttr::test::LogAccessor logAcc;
        VIDEODRIVER.CreateScreen(VideoMode(800, 600), false);
        logAcc.clearLog();
    }
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

BOOST_AUTO_TEST_CASE(TranslateDimensionsBetweenScreenAndViewSpace)
{
    auto* driver = uiHelper::GetVideoDriver();
    driver->CreateScreen("", VideoMode(800 * 2, 600 * 2), false);

    {
        // GUI scale 100%; translations are no-ops
        const auto& guiScale = driver->getGuiScale();
        BOOST_TEST(guiScale.percent() == 100u);
        BOOST_TEST(guiScale.scaleFactor() == 1.f);
        BOOST_TEST(guiScale.invScaleFactor() == 1.f);
        BOOST_TEST(guiScale.screenToView(800.f) == 800.f);
        BOOST_TEST(guiScale.viewToScreen(800.f) == 800.f);
        BOOST_TEST(Position(guiScale.screenToView(Position(800, 600))) == Position(800, 600));
        BOOST_TEST(Position(guiScale.viewToScreen(Position(800, 600))) == Position(800, 600));
    }

    {
        // GUI scale 50%; view coords > screen coords
        driver->setGuiScalePercent(50);
        const auto& guiScale = driver->getGuiScale();
        BOOST_TEST(guiScale.percent() == 50u);
        BOOST_TEST(guiScale.scaleFactor() == .5f);
        BOOST_TEST(guiScale.invScaleFactor() == 2.f);
        BOOST_TEST(guiScale.screenToView(400.f) == 800.f);
        BOOST_TEST(guiScale.viewToScreen(800.f) == 400.f);
        BOOST_TEST(Position(guiScale.screenToView(Position(400, 300))) == Position(800, 600));
        BOOST_TEST(Position(guiScale.viewToScreen(Position(800, 600))) == Position(400, 300));
    }

    {
        // GUI scale 200%; screen coords > view coords
        driver->setGuiScalePercent(200);
        const auto& guiScale = driver->getGuiScale();
        BOOST_TEST(guiScale.percent() == 200u);
        BOOST_TEST(guiScale.scaleFactor() == 2.f);
        BOOST_TEST(guiScale.invScaleFactor() == .5f);
        BOOST_TEST(guiScale.screenToView(800.f) == 400.f);
        BOOST_TEST(guiScale.viewToScreen(400.f) == 800.f);
        BOOST_TEST(Position(guiScale.screenToView(Position(800, 600))) == Position(400, 300));
        BOOST_TEST(Position(guiScale.viewToScreen(Position(400, 300))) == Position(800, 600));
    }
}

BOOST_AUTO_TEST_CASE(GuiScaleRangeCalculation)
{
    GuiScaleRange range;
    auto* driver = uiHelper::GetVideoDriver();
    driver->CreateScreen("", VideoMode(800 * 3, 600 * 3), false);

    // Regular DPI configuration
    range = driver->getGuiScaleRange();
    BOOST_TEST(range.minPercent == 100u);
    BOOST_TEST(range.maxPercent == 300u);
    BOOST_TEST(range.recommendedPercent == 100u);

    // Maximum is constrained by width
    driver->ResizeScreen(VideoMode(800 * 2, 600 * 3), false);
    BOOST_TEST(driver->getGuiScaleRange().maxPercent == 200u);

    // Maximum is constrained by height
    driver->ResizeScreen(VideoMode(800 * 3, 600 * 2), false);
    BOOST_TEST(driver->getGuiScaleRange().maxPercent == 200u);

    // HighDPI configuration (50% larger render size than window size)
    driver->ResizeScreen(VideoMode(800 * 2, 600 * 2), false);
    driver->SetNewSize(driver->GetWindowSize(), Extent(800 * 3, 600 * 3));
    range = driver->getGuiScaleRange();
    BOOST_TEST(range.minPercent == 100u);
    BOOST_TEST(range.maxPercent == (200 * 1.5));
    BOOST_TEST(range.recommendedPercent == (100 * 1.5));
}
