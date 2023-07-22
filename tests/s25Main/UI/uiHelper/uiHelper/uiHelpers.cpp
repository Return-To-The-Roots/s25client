// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "uiHelpers.hpp"
#include "Loader.h"
#include "WindowManager.h"
#include "desktops/Desktop.h"
#include "drivers/VideoDriverWrapper.h"
#include "ogl/glAllocator.h"
#include <libsiedler2/libsiedler2.h>
#include <rttr/test/LogAccessor.hpp>
#include <boost/test/unit_test_log.hpp>
#include <mockupDrivers/MockupVideoDriver.h>

namespace uiHelper {
class DummyDesktop : public Desktop
{
public:
    DummyDesktop() : Desktop(nullptr) {}
};
void initGUITests()
{
    libsiedler2::setAllocator(new GlAllocator);
    BOOST_TEST_CHECKPOINT("Load video driver");
    if(!dynamic_cast<MockupVideoDriver*>(VIDEODRIVER.GetDriver()))
    {
        rttr::test::LogAccessor logAcc;
        VIDEODRIVER.LoadDriver(new MockupVideoDriver(&WINDOWMANAGER));
        RTTR_REQUIRE_LOG_CONTAINS("Mockup Video Driver", false);
        VIDEODRIVER.CreateScreen(VideoMode(800, 600), DisplayMode::None);
        BOOST_TEST_CHECKPOINT("Load dummy files");
        LOADER.LoadDummyGUIFiles();
        BOOST_TEST_CHECKPOINT("Switch to Desktop");
        WINDOWMANAGER.Switch(std::make_unique<DummyDesktop>());
        BOOST_TEST_CHECKPOINT("Dummy Draw");
        WINDOWMANAGER.Draw();
        logAcc.clearLog();
    }
    BOOST_TEST_CHECKPOINT("GUI test initialized");
}

MockupVideoDriver* GetVideoDriver()
{
    auto* video = dynamic_cast<MockupVideoDriver*>(VIDEODRIVER.GetDriver());
    if(!video)
    {
        initGUITests();
        video = dynamic_cast<MockupVideoDriver*>(VIDEODRIVER.GetDriver());
    }
    return video;
}

Fixture::~Fixture()
{
    if(!dynamic_cast<DummyDesktop*>(WINDOWMANAGER.GetCurrentDesktop()) || WINDOWMANAGER.GetTopMostWindow())
    {
        // Switch back to new, empty desktop to clean up active windows
        WINDOWMANAGER.Switch(std::make_unique<DummyDesktop>());
        WINDOWMANAGER.Draw();
    }
}

} // namespace uiHelper
