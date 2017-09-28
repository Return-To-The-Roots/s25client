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
#include "LocaleHelper.h"
#include "files.h"
#include "ogl/glAllocator.h"
// Test helpers. Header only
#include "helpers/helperTests.hpp" // IWYU pragma: keep
#include "libsiedler2/libsiedler2.h"
#include "libutil/Log.h"
#include "libutil/StringStreamWriter.h"

#define BOOST_TEST_MODULE RTTR_Test
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/test/unit_test.hpp>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <string>
#include <vector>

namespace bfs = boost::filesystem;

struct TestSetup
{
    TestSetup()
    {
        if(!LocaleHelper::init())
            throw std::runtime_error("Could not init locale");
        // Write to string stream only to avoid file output on the test server
        LOG.open(new StringStreamWriter);

        // Make sure we have the RTTR folder in our current working directory
        std::vector<bfs::path> possiblePaths;
        possiblePaths.push_back(".");
        // Might be test folder
        possiblePaths.push_back("..");
        // Linux cmake style build setup
        possiblePaths.push_back("../../../build");
        // VS style build setup (additional Debug sub folder)
        possiblePaths.push_back("../../../../build");
        for(std::vector<bfs::path>::const_iterator it = possiblePaths.begin(); it != possiblePaths.end(); ++it)
        {
            if(bfs::is_directory(*it / RTTRDIR))
            {
                std::cout << "Changing to " << *it << std::endl;
                bfs::current_path(*it);
                break;
            }
        }
        srand(static_cast<unsigned>(time(NULL)));
        libsiedler2::setAllocator(new GlAllocator());
    }
    ~TestSetup() { libsiedler2::setAllocator(NULL); }
};

#if BOOST_VERSION >= 105900
BOOST_GLOBAL_FIXTURE(TestSetup);
#else
// Boost < 1.59 got the semicolon inside the macro causing an "extra ;" warning
BOOST_GLOBAL_FIXTURE(TestSetup)
#endif

BOOST_AUTO_TEST_CASE(LocaleFormatTest)
{
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(1234), "1234");
    BOOST_CHECK_EQUAL(boost::lexical_cast<std::string>(1234.5), "1234.5");
    BOOST_CHECK_EQUAL(boost::lexical_cast<float>("1234.5"), 1234.5);
    BOOST_CHECK_EQUAL(boost::lexical_cast<int>("1234"), 1234);
}
