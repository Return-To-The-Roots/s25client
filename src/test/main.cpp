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

#include "Random.h"
#include "ogl/glAllocator.h"
#include "ProgramInitHelpers.h"
#include "test/testHelpers.h"
#include "libutil/src/Log.h"
#include "libutil/src/StringStreamWriter.h"
#include "libsiedler2.h"

#define BOOST_TEST_MODULE RTTR_Test
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <vector>
#include <string>
#include <iostream>
#include <cstdlib>
#include <ctime>

namespace bfs = boost::filesystem;

struct TestSetup
{
    TestSetup()
    {
        InitLocale();
        // Write to string stream only to avoid file output on the test server
        LOG.open(new StringStreamWriter);

        // Make sure we have the RTTR folder in our current working directory
        std::vector<std::string> possiblePaths;
        possiblePaths.push_back(".");
        // Might be test folder
        possiblePaths.push_back("..");
        // Linux cmake style build setup
        possiblePaths.push_back("../../../build");
        // VS style build setup (additional Debug sub folder)
        possiblePaths.push_back("../../../../build");
        for(std::vector<std::string>::const_iterator it = possiblePaths.begin(); it != possiblePaths.end(); ++it)
        {
            if(bfs::is_directory(*it + "/RTTR"))
            {
                std::cout << "Changing to " << *it << std::endl;
                bfs::current_path(*it);
                break;
            }
        }
        srand(static_cast<unsigned>(time(NULL)));
        libsiedler2::setAllocator(new GlAllocator());
    }
    ~TestSetup()
    {
        libsiedler2::setAllocator(NULL);
    }
};

#if BOOST_VERSION >= 105900
    BOOST_GLOBAL_FIXTURE(TestSetup);
#else
	// Boost < 1.59 got the semicolon inside the macro causing an "extra ;" warning
	BOOST_GLOBAL_FIXTURE(TestSetup)
#endif

BOOST_AUTO_TEST_CASE(Basic)
{
    // Check if tests work
    BOOST_CHECK(true);
}

BOOST_AUTO_TEST_CASE(RandomTest)
{
    initGameRNG();
    std::vector<unsigned> resultCt6(6);
    std::vector<unsigned> resultCt13(13);
    std::vector<unsigned> resultCt28(28);
    const unsigned numSamples = 3000;
    for(unsigned i = 0; i < numSamples; i++)
    {
        ++resultCt6.at(RANDOM.Rand(__FILE__, __LINE__, 0, resultCt6.size()));
        ++resultCt13.at(RANDOM.Rand(__FILE__, __LINE__, 0, resultCt13.size()));
        ++resultCt28.at(RANDOM.Rand(__FILE__, __LINE__, 0, resultCt28.size()));
    }
    // Result must be at least 70% of the average
    for(unsigned i = 0; i < resultCt6.size(); i++)
        BOOST_REQUIRE_GT(resultCt6[i], numSamples / resultCt6.size() * 70u / 100u);
    for(unsigned i = 0; i < resultCt13.size(); i++)
        BOOST_REQUIRE_GT(resultCt13[i], numSamples / resultCt13.size() * 70u / 100u);
    for(unsigned i = 0; i < resultCt28.size(); i++)
        BOOST_REQUIRE_GT(resultCt28[i], numSamples / resultCt28.size() * 70u / 100u);
}
