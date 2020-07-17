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

#include "RttrConfig.h"
#include "helperFuncs.h"
#include "rttrDefines.h" // IWYU pragma: keep
#include "s25util/System.h"
#include "s25util/ucString.h"
#include <boost/filesystem/operations.hpp>
#include <boost/nowide/args.hpp>
#include <boost/test/unit_test.hpp>

namespace std {
std::ostream& operator<<(std::ostream& out, const std::wstring& value)
{
    return out << cvWideStringToUTF8(value);
}
} // namespace std

BOOST_AUTO_TEST_SUITE(SystemTestSuite)

BOOST_AUTO_TEST_CASE(GetSetRemoveEnvVar)
{
#ifdef _WIN32
    std::string varName = cvWideStringToUTF8(L"_RTTR_TEST_VAR_WITH_UMLAUTS_\u00E4\u00F6\u00FC_END_");
    std::string varValue = cvWideStringToUTF8(L"ValueWithSpecialChars_\u0139\u00D4_END");
#else
    // Use UTF8 (wide string not portable, either 16 or 32 bit)
    std::string varName = "_RTTR_TEST_VAR_WITH_UMLAUTS_\xC3\xA4\xC3\xB6\xC3\xBC"
                          "_END_";
    std::string varValue = "ValueWithSpecialChars_\xC4\xB9\xC3\x94"
                           "_END";
#endif // _WIN32
    // Create wide string versions
    std::wstring varNameW = cvUTF8ToWideString(varName);
    std::wstring varValueW = cvUTF8ToWideString(varValue);

    // Var should not exist
    BOOST_REQUIRE(!System::envVarExists(varName));
    BOOST_REQUIRE_EQUAL(System::getEnvVar(varName), "");
    BOOST_REQUIRE(!System::envVarExists(varNameW));
    BOOST_REQUIRE_EQUAL(System::getEnvVar(varNameW), L"");

    // Set variable
    BOOST_REQUIRE(System::setEnvVar(varName, varValue));
    for(int i = 0; i < 2; i++)
    {
        // Now it should exist
        BOOST_REQUIRE(System::envVarExists(varName));
        std::string value = System::getEnvVar(varName);
        BOOST_REQUIRE(isValidUTF8(value));
        BOOST_REQUIRE_EQUAL(value, varValue);
        // Same with wide string
        BOOST_REQUIRE(System::envVarExists(varNameW));
        std::wstring valueW = System::getEnvVar(varNameW);
        BOOST_REQUIRE_EQUAL(valueW, varValueW);

        // Remove and set wide string version
        BOOST_REQUIRE(System::removeEnvVar(varName));
        BOOST_REQUIRE(!System::envVarExists(varName));
        BOOST_REQUIRE(System::setEnvVar(varNameW, varValueW));
    }
    // Remove also wide string version
    BOOST_REQUIRE(System::removeEnvVar(varNameW));
    BOOST_REQUIRE(!System::envVarExists(varNameW));
}

BOOST_AUTO_TEST_CASE(GetPathFromEnv)
{
    std::string nonExistingVarName = "NonExistantVar";
#ifdef _WIN32
    std::string existingVarName = "WINDIR";
#else
    std::string existingVarName = "HOME";
#endif // _WIN32
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

BOOST_AUTO_TEST_CASE(GetExePath)
{
    bfs::path exePath = System::getExecutablePath();
    BOOST_REQUIRE(!exePath.empty());
    BOOST_REQUIRE(bfs::exists(exePath));
    BOOST_REQUIRE(bfs::is_regular_file(exePath));
}

class ResetWorkDir
{
    bfs::path oldWorkDir;

public:
    ResetWorkDir() : oldWorkDir(bfs::current_path()) {}
    ~ResetWorkDir() { bfs::current_path(oldWorkDir); }
};

BOOST_AUTO_TEST_CASE(PrefixPath)
{
    LogAccessor logAcc;
    bfs::path prefixPath = RTTRCONFIG.GetPrefixPath();
    BOOST_REQUIRE(!prefixPath.empty());
    BOOST_REQUIRE(bfs::exists(prefixPath));
    BOOST_REQUIRE(bfs::is_directory(prefixPath));
    std::string strPrefixPath = prefixPath.string();
    // No entry of the path should be the NULL terminator
    for(unsigned i = 0; i < strPrefixPath.size(); i++)
    {
        BOOST_REQUIRE_NE(strPrefixPath[i], 0);
    }
    // If the env var is not set (usually should not be) then set it to check if that is used as the prefix path
    if(!System::envVarExists("RTTR_PREFIX_DIR"))
    {
        bfs::path fakePrefixPath = bfs::current_path() / "testPrefixPath";
        BOOST_REQUIRE(System::setEnvVar("RTTR_PREFIX_DIR", fakePrefixPath.string()));
        BOOST_REQUIRE_EQUAL(RTTRCONFIG.GetPrefixPath(), fakePrefixPath);
        RTTR_REQUIRE_LOG_CONTAINS("manually set", false);
        BOOST_REQUIRE(System::removeEnvVar("RTTR_PREFIX_DIR"));
    }
    {
        ResetWorkDir resetWorkDir;
        // Just change it in case we would not change it back
        bfs::current_path(bfs::current_path().parent_path());
        BOOST_REQUIRE(RTTRCONFIG.Init());
        BOOST_REQUIRE_EQUAL(prefixPath, bfs::current_path());
    }
}

BOOST_AUTO_TEST_SUITE_END()
