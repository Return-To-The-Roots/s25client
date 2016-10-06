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
#include "ListDir.h"
#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/foreach.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/iostreams/stream.hpp>

BOOST_AUTO_TEST_SUITE(FileIOSuite)

struct FileOpenFixture
{
    FileOpenFixture()
    {
        tmpPath = bfs::absolute(bfs::unique_path());
        BOOST_TEST_CHECKPOINT("Creating special filenames");
#ifdef _WIN32
        // Widestring UCS2 path
        fileNormal = bfs::path(L"1Normal.txt");
        fileUmlaut = bfs::path(L"2Um\u00E4\u00F6\u00FCLaut.txt");
        fileSpecial = bfs::path(L"3Spe\u0139\u00D4cial.txt");
#else
        // Use UTF8 (widestring not portable, either 16 or 32 bit)
        fileNormal = bfs::path("1Normal.txt");
        fileUmlaut = bfs::path("2Um\xC3\xA4\xC3\xB6\xC3\xBC""Laut.txt");
        fileSpecial = bfs::path("3Spe\xC4\xB9\xC3\x94""cial.txt");
#endif
        BOOST_TEST_CHECKPOINT("Creating tmp path" << tmpPath);
        bfs::create_directories(tmpPath);
        BOOST_TEST_CHECKPOINT("Creating files");
        bfs::ofstream fNormal(tmpPath / fileNormal);
        bfs::ofstream fUmlaut(tmpPath / fileUmlaut);
        bfs::ofstream fSpecial(tmpPath / fileSpecial);
        BOOST_TEST_CHECKPOINT("Filling files");
        fNormal << "OK";
        fUmlaut << "OK";
        fSpecial << "OK";
    }
    virtual ~FileOpenFixture()
    {
        bfs::remove_all(tmpPath);
    }
    bfs::path tmpPath, fileNormal, fileUmlaut, fileSpecial;
};

BOOST_FIXTURE_TEST_CASE(TestListDir, FileOpenFixture)
{
    std::string parentPath = tmpPath.string();
    std::vector<std::string> files = ListDir(parentPath, "txt");
    BOOST_REQUIRE_EQUAL(files.size(), 3u);
    BOOST_FOREACH(const std::string& file, files)
    {
        BOOST_REQUIRE(bfs::exists(file));
        BOOST_REQUIRE(bfs::path(file).is_absolute());

        // Scopes for auto-close
        {
            // path input
            bfs::path filePath(file);
            bfs::ifstream sFile(filePath);
            BOOST_REQUIRE(sFile);
            std::string content;
            BOOST_REQUIRE(sFile >> content);
            BOOST_REQUIRE_EQUAL(content, "OK");
        }

        {
            // string input
            bfs::ifstream sFile(file);
            BOOST_REQUIRE(sFile);
            std::string content;
            BOOST_REQUIRE(sFile >> content);
            BOOST_REQUIRE_EQUAL(content, "OK");
        }

        {
            // Memory mapped file
            boost::iostreams::mapped_file_source mmapFile;
            try
            {
                mmapFile.open(bfs::path(file));
            } catch (std::exception& e)
            {
                BOOST_FAIL(e.what());
            }
            typedef boost::iostreams::stream<boost::iostreams::mapped_file_source> MMStream;
            MMStream map(mmapFile);
        }

    }
}

BOOST_AUTO_TEST_SUITE_END()
