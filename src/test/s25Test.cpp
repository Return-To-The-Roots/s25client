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

#include "defines.h" // IWYU pragma: keep
#include "ProgramInitHelpers.h"
#include "files.h"
#include "ogl/glAllocator.h"
#include "libsiedler2/libsiedler2.h"
#include "libutil/LocaleHelper.h"
#include "libutil/Log.h"
#include "libutil/StringStreamWriter.h"

#define BOOST_TEST_MODULE RTTR_Test
#include <boost/filesystem.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/test/unit_test.hpp>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

// Test helpers. Header only
#include "helpers/helperTests.hpp" // IWYU pragma: keep

namespace bfs = boost::filesystem;

struct TestSetup
{
    TestSetup()
    {
        if(!LocaleHelper::init())
            throw std::runtime_error("Could not init locale");
        // Write to string stream only to avoid file output on the test server
        LOG.open(new StringStreamWriter);
        if(!InitWorkingDirectory())
            throw std::runtime_error("Could not init working directory. Misplaced binary?");
        if(!bfs::is_directory(RTTRDIR))
            throw std::runtime_error(std::string(RTTRDIR) + " not found. Binary misplaced or RTTR folder not copied?");
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
