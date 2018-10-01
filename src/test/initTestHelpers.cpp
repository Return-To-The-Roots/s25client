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
#include "test/initTestHelpers.h"
#include "Loader.h"
#include "WindowManager.h"
#include "desktops/Desktop.h"
#include "drivers/VideoDriverWrapper.h"
#include "lua/GameDataLoader.h"
#include "mockupDrivers/MockupVideoDriver.h"
#include "random/Random.h"
#include "test/helperFuncs.h"
#include <boost/test/unit_test.hpp>
#include <cstdlib>
#include <iostream>

// Unix only, but cygwin does not have execinfo.h
#if !defined(_WIN32) && !defined(__CYGWIN__)
#include <csignal>
#include <execinfo.h>

void SegFaultHandler(int /*sig*/)
{
    const unsigned maxTrace = 256;
    void* stacktrace[maxTrace];
    unsigned num_frames = backtrace(stacktrace, maxTrace);
    char** stacktraceNames = backtrace_symbols(stacktrace, num_frames);
    for(unsigned i = 0; i < num_frames; i++)
    {
        std::cerr << std::hex << stacktrace[i];
        if(stacktraceNames)
            std::cerr << ": " << stacktraceNames[i];
        std::cerr << std::endl;
    }
    free(stacktraceNames);

    abort();
}

void installSegFaultHandler()
{
    signal(SIGSEGV, SegFaultHandler);
    struct sigaction newAction;
    newAction.sa_handler = SegFaultHandler;
    sigemptyset(&newAction.sa_mask);
    newAction.sa_flags = 0;
    sigaction(SIGSEGV, &newAction, NULL);
}
#else
void installSegFaultHandler() {}
#endif

void doInitGameRNG(unsigned defaultValue /*= 1337*/, const char* fileName /*= ""*/, unsigned line /*= 0*/)
{
#ifdef RTTR_RAND_TEST
    defaultValue += rand();
#endif
    RANDOM.Init(defaultValue);
    // Reduce log clutter
    bool print = defaultValue != 1337;
#ifdef RTTR_RAND_TEST
    print = true;
#endif
    if(print && fileName && fileName[0])
        std::cout << "Ingame RNG (" << fileName << "#" << line << ")= " << RANDOM.GetCurrentState() << "(" << defaultValue << ")"
                  << std::endl;
}

class DummyDesktop : public Desktop
{
public:
    DummyDesktop() : Desktop(NULL) {}
};

void initGUITests()
{
    installSegFaultHandler();
    BOOST_TEST_CHECKPOINT("Load video driver");
    LogAccessor logAcc;
    VIDEODRIVER.LoadDriver(new MockupVideoDriver(&WINDOWMANAGER));
    RTTR_REQUIRE_LOG_CONTAINS("Loaded", false);
    VIDEODRIVER.CreateScreen(800, 600, false);
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

void loadGameData(WorldDescription& worldDesc)
{
    GameDataLoader gdLoader(worldDesc);
    BOOST_REQUIRE(gdLoader.Load());
}
