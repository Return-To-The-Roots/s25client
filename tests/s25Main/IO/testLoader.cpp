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

#include "Loader.h"
#include "RttrConfig.h"
#include "test/testConfig.h"
#include "libsiedler2/ArchivItem_Bitmap_Raw.h"
#include "libsiedler2/ArchivItem_Text.h"
#include "libsiedler2/PixelBufferBGRA.h"
#include "libsiedler2/libsiedler2.h"
#include "rttr/test/TmpFolder.hpp"
#include "s25util/Log.h"
#include "s25util/Tokenizer.h"
#include <rttr/test/LogAccessor.hpp>
#include <boost/filesystem.hpp>
#include <boost/nowide/fstream.hpp>
#include <boost/test/unit_test.hpp>

namespace bfs = boost::filesystem;

class LoaderFixture
{
public:
    std::unique_ptr<Loader> loader;
    LoaderFixture() : loader(std::make_unique<Loader>(LOG, RTTRCONFIG)) {}
};

BOOST_AUTO_TEST_SUITE(LoaderTests)

static boost::test_tools::predicate_result compareTxts(const libsiedler2::Archiv& archive, const std::string& expectedContents)
{
    std::vector<std::string> txts = Tokenizer(expectedContents, "|").explode();
    if(txts.size() != archive.size())
    {
        boost::test_tools::predicate_result res(false);
        res.message() << "Item count mismatch [" << archive.size() << " != " << txts.size() << "]";
        return res;
    }
    for(unsigned i = 0; i < txts.size(); i++)
    {
        if(txts[i].empty())
        {
            if(archive[i])
            {
                boost::test_tools::predicate_result res(false);
                res.message() << "Unexpected item at " << i;
                return res;
            }
        } else if(!archive[i] || !dynamic_cast<const libsiedler2::ArchivItem_Text*>(archive[i]))
        {
            boost::test_tools::predicate_result res(false);
            res.message() << "Item " << i << " not found";
            return res;
        } else
        {
            const auto* arTxt = static_cast<const libsiedler2::ArchivItem_Text*>(archive[i]);
            if(arTxt->getText() != txts[i])
            {
                boost::test_tools::predicate_result res(false);
                res.message() << "Mismatch at " << i << " [" << arTxt->getText() << " != " << txts[i] << "]";
                return res;
            }
        }
    }

    return true;
}

const std::string mainFile = RTTR_BASE_DIR "/tests/testData/test.GER";
const std::string overrideFolder1 = RTTR_BASE_DIR "/tests/testData/override1";
const std::string overrideFolder2 = RTTR_BASE_DIR "/tests/testData/override2";
const std::string overrideFolder3 = RTTR_BASE_DIR "/tests/testData/override3";

BOOST_FIXTURE_TEST_CASE(TestPredicate, LoaderFixture)
{
    BOOST_REQUIRE(loader->Load(overrideFolder1 + "/test.GER"));
    const auto& txt = loader->GetArchive("test");
    BOOST_REQUIRE(compareTxts(txt, "1||20"));
    BOOST_TEST(compareTxts(txt, "1|").message().str() == "Item count mismatch [3 != 2]");
    BOOST_TEST(compareTxts(txt, "1||20|2").message().str() == "Item count mismatch [3 != 4]");
    BOOST_TEST(compareTxts(txt, "1||").message().str() == "Unexpected item at 2");
    BOOST_TEST(compareTxts(txt, "1|2|20").message().str() == "Item 1 not found");
    BOOST_TEST(compareTxts(txt, "4||20").message().str() == "Mismatch at 0 [1 != 4]");
}

BOOST_FIXTURE_TEST_CASE(Overrides, LoaderFixture)
{
    rttr::test::LogAccessor logAcc;

    // No override folders
    BOOST_REQUIRE(loader->Load(mainFile));
    BOOST_REQUIRE(compareTxts(loader->GetArchive("test"), "0|10"));

    // 1 override folder
    loader->AddOverrideFolder(overrideFolder1);
    // LoadOverrideFiles loads simply the override file itself
    BOOST_REQUIRE(loader->LoadOverrideFiles());
    BOOST_REQUIRE(compareTxts(loader->GetArchive("test"), "1||20"));
    // Explicitly loading a file overwrites this and override file is used
    BOOST_REQUIRE(loader->Load(mainFile));
    BOOST_REQUIRE(compareTxts(loader->GetArchive("test"), "1|10|20"));
    // LoadOverrideFiles has no effect (already loaded)
    BOOST_REQUIRE(loader->LoadOverrideFiles());
    BOOST_REQUIRE(compareTxts(loader->GetArchive("test"), "1|10|20"));

    // 2 override folders
    loader->ClearOverrideFolders();
    loader->AddOverrideFolder(overrideFolder1);
    loader->AddOverrideFolder(overrideFolder2);
    // LoadOverrideFiles loads override file from1 with override from 2
    BOOST_REQUIRE(loader->LoadOverrideFiles());
    BOOST_REQUIRE(compareTxts(loader->GetArchive("test"), "2||20|30"));
    // Explicitly loading a file overwrites this and override file is used
    BOOST_REQUIRE(loader->Load(mainFile));
    BOOST_REQUIRE(compareTxts(loader->GetArchive("test"), "2|10|20|30"));
    // LoadOverrideFiles has no effect (already loaded)
    BOOST_REQUIRE(loader->LoadOverrideFiles());
    BOOST_REQUIRE(compareTxts(loader->GetArchive("test"), "2|10|20|30"));

    // Load an override with a double extension
    loader->ClearOverrideFolders();
    loader->AddOverrideFolder(overrideFolder3);
    BOOST_REQUIRE(loader->Load(mainFile));
    BOOST_REQUIRE(compareTxts(loader->GetArchive("test"), "Hello World Override|10"));

    // Avoid log cluttering
    logAcc.clearLog();
}

BOOST_FIXTURE_TEST_CASE(BobOverrides, LoaderFixture)
{
    rttr::test::LogAccessor logAcc;
    rttr::test::TmpFolder tmpFolder;

    const bfs::path bobFile = tmpFolder.get() / "foo.bob";
    bfs::create_directory(bobFile);

    auto bmpRaw = std::make_unique<libsiedler2::ArchivItem_Bitmap_Raw>();
    libsiedler2::PixelBufferBGRA buffer(3, 7);
    bmpRaw->create(buffer);
    libsiedler2::Archiv bmp;
    bmp.push(std::move(bmpRaw));
    BOOST_TEST_REQUIRE(libsiedler2::Write((bobFile / "0.bmp").string(), bmp) == 0);
    BOOST_TEST_REQUIRE(libsiedler2::Write((bobFile / "1.bmp").string(), bmp) == 0);

    libsiedler2::ArchivItem_Text txt;
    txt.setText("10 332\n11 334\n"); // mapping file
    {
        boost::nowide::ofstream f(bobFile / "mapping.links");
        BOOST_TEST_REQUIRE(txt.write(f, false) == 0);
    }

    BOOST_REQUIRE(loader->Load(bobFile));
    const auto& bob = loader->GetArchive("foo");
    // Bob override folders have special handling: They are converted into an archive containing the elements of the folder
    // as this is how bob files are after loading
    BOOST_TEST_REQUIRE(bob.size() == 1u);
    const auto* nested = dynamic_cast<const libsiedler2::Archiv*>(bob[0]);
    BOOST_TEST_REQUIRE(nested);
    BOOST_TEST_REQUIRE(nested->size() == 3u);
    BOOST_TEST(!nested->get(2)); // Text item removed
    for(size_t i = 0; i < 2; ++i)
    {
        auto* curBmp = dynamic_cast<const libsiedler2::ArchivItem_Bitmap*>(nested->get(i));
        BOOST_TEST_REQUIRE(curBmp);
        BOOST_TEST(curBmp->getWidth() == 3);
        BOOST_TEST(curBmp->getHeight() == 7);
    }
    // TODO: Test mapping merge. Can't be done ATM as it requires an actual bob file to override

    // Avoid log cluttering
    logAcc.clearLog();
}

BOOST_AUTO_TEST_SUITE_END()
