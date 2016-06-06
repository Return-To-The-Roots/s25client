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

#define BOOST_TEST_MODULE RTTR_Test
#include <boost/test/unit_test.hpp>
#include <boost/filesystem.hpp>
#include <vector>
#include <string>
#include <iostream>

namespace bfs = boost::filesystem;

struct TestSetup
{
    TestSetup()
    {
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