// Copyright (c) 2016 - 2020 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ListDir.h"
#include <s25util/utf8.h>
#include <boost/filesystem.hpp>
#include <boost/iostreams/device/mapped_file.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/nowide/fstream.hpp>
#include <boost/test/unit_test.hpp>

namespace bfs = boost::filesystem;
namespace bnw = boost::nowide;

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
        fileUmlaut = bfs::path("2Um\xC3\xA4\xC3\xB6\xC3\xBC"
                               "Laut.txt");
        fileSpecial = bfs::path("3Spe\xC4\xB9\xC3\x94"
                                "cial.txt");
#endif
        BOOST_TEST_CHECKPOINT("Creating tmp path" << tmpPath);
        bfs::create_directories(tmpPath);
        BOOST_TEST_CHECKPOINT("Creating files");
        bnw::ofstream fNormal(tmpPath / fileNormal);
        bnw::ofstream fUmlaut(tmpPath / fileUmlaut);
        bnw::ofstream fSpecial(tmpPath / fileSpecial);
        BOOST_TEST_CHECKPOINT("Filling files");
        fNormal << "OK";
        fUmlaut << "OK";
        fSpecial << "OK";
    }
    virtual ~FileOpenFixture() { bfs::remove_all(tmpPath); }
    bfs::path tmpPath, fileNormal, fileUmlaut, fileSpecial;
};

BOOST_FIXTURE_TEST_CASE(TestListDir, FileOpenFixture)
{
    const std::vector<bfs::path> files = ListDir(tmpPath, "txt");
    BOOST_TEST_REQUIRE(files.size() == 3u);
    for(const bfs::path& file : files)
    {
        BOOST_TEST_REQUIRE(bfs::exists(file));
        BOOST_TEST_REQUIRE(file.is_absolute());

        // String result must still be utf8
        BOOST_TEST_REQUIRE(s25util::isValidUTF8(file.string()));

        // Scopes for auto-close
        {
            // path input
            bnw::ifstream sFile(file);
            BOOST_TEST_REQUIRE(!!sFile);
            std::string content;
            BOOST_TEST_REQUIRE(!!(sFile >> content));
            BOOST_TEST_REQUIRE(content == "OK");
        }

        {
            // Memory mapped file
            boost::iostreams::mapped_file_source mmapFile;
            mmapFile.open(bfs::path(file));
            BOOST_TEST_REQUIRE(mmapFile.is_open());
            using MMStream = boost::iostreams::stream<boost::iostreams::mapped_file_source>;
            MMStream map(mmapFile);
            std::string content;
            BOOST_TEST_REQUIRE(!!(map >> content));
            BOOST_TEST_REQUIRE(content == "OK");
        }
    }
}

BOOST_AUTO_TEST_SUITE_END()
