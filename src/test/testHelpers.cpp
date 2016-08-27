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

#ifndef _WIN32
#   include <execinfo.h>
#   include <csignal>

void SegFaultHandler(int /*sig*/)
{
    const unsigned int maxTrace = 256;
    void* stacktrace[maxTrace];
    unsigned num_frames = backtrace(stacktrace, maxTrace);
    char** stacktraceNames = backtrace_symbols(stacktrace, num_frames);
    for(unsigned i = 0; i < num_frames; i++){
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
void installSegFaultHandler(){}
#endif

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
    installSegFaultHandler();
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
