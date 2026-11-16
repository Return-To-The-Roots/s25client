// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "uiHelpers.hpp"
#include "Loader.h"
#include "Settings.h"
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
    if(!dynamic_cast<MockupVideoDriver*>(VIDEODRIVER.getDriver()))
    {
        rttr::test::LogAccessor logAcc;
        VIDEODRIVER.loadDriver(new MockupVideoDriver(&WINDOWMANAGER));
        RTTR_REQUIRE_LOG_CONTAINS("Mockup Video Driver", false);
        VIDEODRIVER.createScreen(VideoMode(800, 600), DisplayMode::Windowed);
        BOOST_TEST_CHECKPOINT("Load dummy files");
        LOADER.loadDummyGUIFiles();
        LOADER.loadDummySoundFiles();
        BOOST_TEST_CHECKPOINT("Switch to Desktop");
        WINDOWMANAGER.Switch(std::make_unique<DummyDesktop>());
        BOOST_TEST_CHECKPOINT("Dummy Draw");
        WINDOWMANAGER.draw();
        logAcc.clearLog();
    }
    // Don't try to play sounds
    SETTINGS.sound.effectsEnabled = false;
    BOOST_TEST_CHECKPOINT("GUI test initialized");
}

MockupVideoDriver* getVideoDriver()
{
    auto* video = dynamic_cast<MockupVideoDriver*>(VIDEODRIVER.getDriver());
    if(!video)
    {
        initGUITests();
        video = dynamic_cast<MockupVideoDriver*>(VIDEODRIVER.getDriver());
    }
    return video;
}

Fixture::~Fixture()
{
    if(!dynamic_cast<DummyDesktop*>(WINDOWMANAGER.getCurrentDesktop()) || WINDOWMANAGER.getTopMostWindow())
    {
        // Switch back to new, empty desktop to clean up active windows
        WINDOWMANAGER.Switch(std::make_unique<DummyDesktop>());
        WINDOWMANAGER.draw();
    }
}

} // namespace uiHelper
