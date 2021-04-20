// Copyright (C) 2016 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "resources/ArchiveLocator.h"
#include "resources/ResourceId.h"
#include "test/testConfig.h"
#include "rttr/test/TmpFolder.hpp"
#include "s25util/Log.h"
#include "s25util/Tokenizer.h"
#include <rttr/test/LogAccessor.hpp>
#include <boost/filesystem.hpp>
#include <boost/nowide/fstream.hpp>
#include <boost/test/unit_test.hpp>

namespace fs = boost::filesystem;
using boost::test_tools::per_element;
using rttr::test::TmpFolder;

BOOST_AUTO_TEST_SUITE(ArchiveLocatorTests)

static fs::path createFile(const fs::path& filepath)
{
    fs::path folder = filepath.parent_path();
    if(!exists(folder))
        create_directories(folder);
    boost::nowide::ofstream of(filepath);
    BOOST_TEST_REQUIRE(!!(of << "Dummy data"));
    return filepath;
}

BOOST_AUTO_TEST_CASE(ResolveOverridesByPath)
{
    rttr::test::LogAccessor logAcc;
    ArchiveLocator locator(LOG);

    TmpFolder mainFolder;
    fs::path mainFile = createFile(mainFolder.get() / "test.lst");

    // No override folders
    BOOST_TEST(locator.resolve(mainFile) == ResolvedFile{mainFile}, per_element());

    // 1 override folder
    TmpFolder overrideFolder1;
    fs::path overrideFile1 = createFile(overrideFolder1.get() / "test.lst");
    locator.addOverrideFolder(overrideFolder1);
    BOOST_TEST(locator.resolve(mainFile) == (ResolvedFile{mainFile, overrideFile1}), per_element());

    // 2 override folders
    TmpFolder overrideFolder2;
    fs::path overrideFile2 = createFile(overrideFolder2.get() / "test.lst");
    locator.clear();
    locator.addOverrideFolder(overrideFolder1);
    locator.addOverrideFolder(overrideFolder2);
    BOOST_TEST(locator.resolve(mainFile) == (ResolvedFile{mainFile, overrideFile1, overrideFile2}), per_element());

    mainFile = createFile(mainFolder.get() / "test2.lst");
    // extension doesn't matter
    overrideFile1 = createFile(overrideFolder1.get() / "test2.ini");
    // can also be a folder w/o an extension
    overrideFile2 = createFile(overrideFolder2.get() / "test2" / "0.txt").parent_path();
    locator.clear();
    locator.addOverrideFolder(overrideFolder1);
    locator.addOverrideFolder(overrideFolder2);
    BOOST_TEST(locator.resolve(mainFile) == (ResolvedFile{mainFile, overrideFile1, overrideFile2}), per_element());

    // Special handling for bob files:
    mainFile = createFile(mainFolder.get() / "test3.bob");
    // Usually overwritten by bob.lst files
    overrideFile1 = createFile(overrideFolder1.get() / "test3.bob.lst");
    // But can also be a folder with .bob extension
    overrideFile2 = createFile(overrideFolder2.get() / "test3.bob" / "0.txt").parent_path();
    locator.clear();
    locator.addOverrideFolder(overrideFolder1);
    locator.addOverrideFolder(overrideFolder2);
    BOOST_TEST(locator.resolve(mainFile) == (ResolvedFile{mainFile, overrideFile1, overrideFile2}), per_element());

    // Avoid log cluttering
    logAcc.clearLog();
}

BOOST_AUTO_TEST_CASE(ResolveOverridesByResId)
{
    rttr::test::LogAccessor logAcc;
    ArchiveLocator locator(LOG);

    TmpFolder mainFolder;
    const fs::path mainFile = createFile(mainFolder.get() / "test.lst");
    TmpFolder overrideFolder;
    const fs::path overrideFile = createFile(overrideFolder.get() / "test.lst");
    locator.addAssetFolder(mainFolder);
    locator.addOverrideFolder(overrideFolder);

    // No override folders
    BOOST_TEST(locator.resolve(ResourceId("test")) == (ResolvedFile{mainFile, overrideFile}), per_element());

    // Avoid log cluttering
    logAcc.clearLog();
}

BOOST_AUTO_TEST_SUITE_END()
