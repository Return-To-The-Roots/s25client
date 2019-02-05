// Copyright (c) 2016 - 2018 Settlers Freaks (sf-team at siedler25.org)
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

#include "uiHelpers.hpp"
#include "Loader.h"
#include "WindowManager.h"
#include "desktops/Desktop.h"
#include "drivers/VideoDriverWrapper.h"
#include "ogl/glAllocator.h"
#include <libsiedler2/libsiedler2.h>
#include <boost/test/unit_test_log.hpp>
#include <mockupDrivers/MockupVideoDriver.h>
#include <rttr/test/LogAccessor.hpp>

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
    rttr::test::LogAccessor logAcc;
    VIDEODRIVER.LoadDriver(new MockupVideoDriver(&WINDOWMANAGER));
    RTTR_REQUIRE_LOG_CONTAINS("Loaded", false);
    VIDEODRIVER.CreateScreen(VideoMode(800, 600), false);
    BOOST_TEST_CHECKPOINT("Load dummy files");
    LOADER.LoadDummyGUIFiles();
    BOOST_TEST_CHECKPOINT("Switch to Desktop");
    WINDOWMANAGER.Switch(new DummyDesktop);
    BOOST_TEST_CHECKPOINT("Dummy Draw");
    WINDOWMANAGER.Draw();
    BOOST_TEST_CHECKPOINT("GUI test initialized");
}

MockupVideoDriver* GetVideoDriver()
{
    MockupVideoDriver* video = dynamic_cast<MockupVideoDriver*>(VIDEODRIVER.GetDriver());
    if(!video)
    {
        initGUITests();
        video = dynamic_cast<MockupVideoDriver*>(VIDEODRIVER.GetDriver());
    }
    return video;
}

} // namespace uiHelper
