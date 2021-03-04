// Copyright (c) 2016 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#define BOOST_TEST_MODULE RTTR_Config

#include "RttrConfig.h"
#include "s25util/System.h"
#include <rttr/test/BaseFixture.hpp>
#include <rttr/test/LogAccessor.hpp>
#include <boost/filesystem.hpp>
#include <boost/nowide/args.hpp>
#include <boost/test/unit_test.hpp>

#if RTTR_HAS_VLD
#    include <vld.h>
#endif

namespace fs = boost::filesystem;

class ResetWorkDir
{
    fs::path oldWorkDir;

public:
    ResetWorkDir() : oldWorkDir(fs::current_path()) {}
    ~ResetWorkDir() { fs::current_path(oldWorkDir); }
};

BOOST_FIXTURE_TEST_CASE(PrefixPath, rttr::test::BaseFixture)
{
    rttr::test::LogAccessor logAcc;
    fs::path prefixPath = RTTRCONFIG.GetPrefixPath();
    BOOST_TEST_REQUIRE(!prefixPath.empty());
    BOOST_TEST(fs::exists(prefixPath));
    BOOST_TEST(fs::is_directory(prefixPath));
    // No entry of the path should be the NULL terminator
    for(const char c : prefixPath.string())
    {
        BOOST_TEST(c != '\0');
    }
    // If the env var is not set (usually should not be) then set it to check if that is used as the prefix path
    if(!System::envVarExists("RTTR_PREFIX_DIR"))
    {
        fs::path fakePrefixPath = fs::current_path() / "testPrefixPath";
        BOOST_TEST_REQUIRE(System::setEnvVar("RTTR_PREFIX_DIR", fakePrefixPath.string()));
        BOOST_TEST(RTTRCONFIG.GetPrefixPath() == fakePrefixPath);
        RTTR_REQUIRE_LOG_CONTAINS("manually set", false);
        BOOST_TEST_REQUIRE(System::removeEnvVar("RTTR_PREFIX_DIR"));
    }
    {
        ResetWorkDir resetWorkDir;
        // Just change it in case we would not change it back
        fs::current_path(fs::current_path().parent_path());
        BOOST_TEST_REQUIRE(RTTRCONFIG.Init());
        BOOST_TEST(prefixPath == fs::current_path());
    }
}
