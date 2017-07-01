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
#include "libutil/src/System.h"
#include "libutil/src/ucString.h"
#include <boost/test/unit_test.hpp>
#include <boost/filesystem/operations.hpp>

BOOST_AUTO_TEST_SUITE(SystemTestSuite)

std::string nonExistingVarName = "NonExistantVar";
#ifdef _WIN32
std::string existingVarName = "WINDIR";
#else
std::string existingVarName = "HOME";
#endif // _WIN32

BOOST_AUTO_TEST_CASE(GetEnvVar)
{
    BOOST_REQUIRE(!System::envVarExists(nonExistingVarName));
    BOOST_REQUIRE_EQUAL(System::getEnvVar(nonExistingVarName), std::string());
    BOOST_REQUIRE(System::envVarExists(existingVarName));
    BOOST_REQUIRE_NE(System::getEnvVar(existingVarName), std::string());
    BOOST_REQUIRE(isValidUTF8(System::getEnvVar(existingVarName)));
}

BOOST_AUTO_TEST_CASE(GetPathFromEnv)
{
    BOOST_REQUIRE(!System::envVarExists(nonExistingVarName));
    BOOST_REQUIRE(System::getPathFromEnvVar(nonExistingVarName).empty());
    BOOST_REQUIRE(System::envVarExists(existingVarName));
    BOOST_REQUIRE(!System::getPathFromEnvVar(existingVarName).empty());
}

BOOST_AUTO_TEST_CASE(GetHome)
{
    bfs::path homePath = System::getHomePath();
    BOOST_REQUIRE(!homePath.empty());
    BOOST_REQUIRE(bfs::exists(homePath));
}

BOOST_AUTO_TEST_CASE(GetUsername)
{
    BOOST_REQUIRE(!System::getUserName().empty());
    BOOST_REQUIRE(isValidUTF8(System::getUserName()));
}

BOOST_AUTO_TEST_SUITE_END()
