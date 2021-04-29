// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "resources/ArchiveLoader.h"
#include "resources/ResolvedFile.h"
#include "test/testConfig.h"
#include "libsiedler2/Archiv.h"
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

namespace fs = boost::filesystem;

namespace {
libsiedler2::Archiv createTxtArchive(const std::initializer_list<const char*>& values)
{
    libsiedler2::Archiv txt;
    for(const char* s : values)
    {
        if(!s)
            txt.push(nullptr);
        else
        {
            auto txtItem = std::make_unique<libsiedler2::ArchivItem_Text>();
            txtItem->setText(s);
            txt.push(std::move(txtItem));
        }
    }
    return txt;
}
struct CreateTestData
{
    const rttr::test::TmpFolder resourceFolder;
    const fs::path mainFile, overrideFolder1, overrideFolder2;
    CreateTestData()
        : mainFile(resourceFolder / fs::path("test.GER")), overrideFolder1(resourceFolder / fs::path("override1")),
          overrideFolder2(resourceFolder / fs::path("override2"))
    {
        createTxtArchiveFile(mainFile, {"0", "10"});
        fs::create_directory(overrideFolder1);
        createTxtArchiveFile(overrideFolder1 / mainFile.filename(), {"1", nullptr, "20"});
        fs::create_directory(overrideFolder2);
        createTxtArchiveFile(overrideFolder2 / mainFile.filename(), {"2", nullptr, nullptr, "30"});
    }

private:
    void createTxtArchiveFile(const fs::path& path, const std::initializer_list<const char*>& values)
    {
        libsiedler2::Archiv txt = createTxtArchive(values);
        BOOST_TEST_REQUIRE(libsiedler2::Write(path, txt) == 0);
    }
};
} // namespace

BOOST_AUTO_TEST_SUITE(LoaderTests)

static boost::test_tools::predicate_result compareTxts(const libsiedler2::Archiv& archive,
                                                       const std::string& expectedContents)
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

BOOST_AUTO_TEST_CASE(TestPredicate)
{
    // Create archive of size 3 where first item is "1", second is empty and third is "20"
    libsiedler2::Archiv txt = createTxtArchive({"1", nullptr, "20"});

    BOOST_TEST_REQUIRE(compareTxts(txt, "1||20"));
    BOOST_TEST(compareTxts(txt, "1|").message().str() == "Item count mismatch [3 != 2]");
    BOOST_TEST(compareTxts(txt, "1||20|2").message().str() == "Item count mismatch [3 != 4]");
    BOOST_TEST(compareTxts(txt, "1||").message().str() == "Unexpected item at 2");
    BOOST_TEST(compareTxts(txt, "1|2|20").message().str() == "Item 1 not found");
    BOOST_TEST(compareTxts(txt, "4||20").message().str() == "Mismatch at 0 [1 != 4]");
}

BOOST_FIXTURE_TEST_CASE(Overrides, CreateTestData)
{
    rttr::test::LogAccessor logAcc;
    ArchiveLoader loader(LOG);

    { // No override
        const auto archive = loader.load(ResolvedFile{mainFile});
        BOOST_TEST_REQUIRE(compareTxts(archive, "0|10"));
    }

    { // 1 override
        // Explicitly loading a file overwrites this and override file is used
        const auto archive = loader.load(ResolvedFile{mainFile, overrideFolder1 / mainFile.filename()});
        BOOST_TEST_REQUIRE(compareTxts(archive, "1|10|20"));
    }

    { // 2 overrides
        const auto archive = loader.load(
          ResolvedFile{mainFile, overrideFolder1 / mainFile.filename(), overrideFolder2 / mainFile.filename()});
        BOOST_TEST_REQUIRE(compareTxts(archive, "2|10|20|30"));
    }

    // Avoid log cluttering
    logAcc.clearLog();
}

BOOST_AUTO_TEST_CASE(BobOverrides)
{
    rttr::test::LogAccessor logAcc;
    rttr::test::TmpFolder tmpFolder;

    const fs::path bobFile = tmpFolder.get() / "foo.bob";
    fs::create_directory(bobFile);

    auto bmpRaw = std::make_unique<libsiedler2::ArchivItem_Bitmap_Raw>();
    libsiedler2::PixelBufferBGRA buffer(3, 7);
    bmpRaw->create(buffer);
    libsiedler2::Archiv bmp;
    bmp.push(std::move(bmpRaw));
    BOOST_TEST_REQUIRE(libsiedler2::Write((bobFile / "0.bmp"), bmp) == 0);
    BOOST_TEST_REQUIRE(libsiedler2::Write((bobFile / "1.bmp"), bmp) == 0);

    libsiedler2::ArchivItem_Text txt;
    txt.setText("10 332\n11 334\n"); // mapping file
    {
        boost::nowide::ofstream f(bobFile / "mapping.links");
        BOOST_TEST_REQUIRE(txt.write(f, false) == 0);
    }

    ArchiveLoader loader(LOG);
    const auto bob = loader.load(ResolvedFile{bobFile});
    // Bob override folders have special handling: They are converted into an archive containing the elements of the
    // folder as this is how bob files are after loading
    BOOST_TEST_REQUIRE(bob.size() == 1u);
    const auto* nested = dynamic_cast<const libsiedler2::Archiv*>(bob[0]);
    BOOST_TEST_REQUIRE(nested);
    BOOST_TEST_REQUIRE(nested->size() == 3u);
    BOOST_TEST(!nested->get(2)); // Text item removed
    for(size_t i = 0; i < 2; ++i)
    {
        const auto* curBmp = dynamic_cast<const libsiedler2::ArchivItem_Bitmap*>(nested->get(i));
        BOOST_TEST_REQUIRE(curBmp);
        BOOST_TEST(curBmp->getWidth() == 3);
        BOOST_TEST(curBmp->getHeight() == 7);
    }
    // TODO: Test mapping merge. Can't be done ATM as it requires an actual bob file to override

    // Avoid log cluttering
    logAcc.clearLog();
}

BOOST_AUTO_TEST_SUITE_END()
