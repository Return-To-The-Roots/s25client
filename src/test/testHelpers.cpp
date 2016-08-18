// Copyright (c) 2016 Settlers Freaks (sf-team at siedler25.org)
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

#include "test/testHelpers.h"
#include "Random.h"
#include "desktops/Desktop.h"
#include "WindowManager.h"
#include "Loader.h"
#include "drivers/VideoDriverWrapper.h"
#include "test/DummyVideoDriver.h"
#include <boost/test/unit_test.hpp>
#include <iostream>
#include <cstdlib>

void doInitGameRNG(unsigned defaultValue /*= 1337*/, const char* fileName /*= ""*/, unsigned line /*= 0*/)
{
#ifdef RTTR_RAND_TEST
    RANDOM.Init(rand() + defaultValue);
#else
    RANDOM.Init(defaultValue);
#endif
    if(fileName && fileName[0])
        std::cout << "Ingame RNG (" << fileName << "#" << line << ")= " << RANDOM.GetCurrentRandomValue() << std::endl;
}

class DummyDesktop: public Desktop
{
public: DummyDesktop(): Desktop(NULL){}
};

void initGUITests()
{
    BOOST_TEST_CHECKPOINT("Load video driver");
    VIDEODRIVER.LoadDriver(new DummyVideoDriver());
    BOOST_TEST_CHECKPOINT("Load dummy files");
    LOADER.LoadDummyGUIFiles();
    BOOST_TEST_CHECKPOINT("Switch to Desktop");
    WINDOWMANAGER.Switch(new DummyDesktop);
    BOOST_TEST_CHECKPOINT("Dummy Draw");
    WINDOWMANAGER.Draw();
    BOOST_TEST_CHECKPOINT("GUI test initialized");
}
