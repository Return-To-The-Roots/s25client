// Copyright (c) 2016 -2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "rttrDefines.h" // IWYU pragma: keep
#include "Loader.h"
#include "RttrConfig.h"
#include "files.h"
#include "languages.h"
#include "mygettext/mygettext.h"
#include "libsiedler2/ArchivItem_Ini.h"
#include "libutil/StringConversion.h"
#include <boost/test/unit_test.hpp>
#include <string>

struct LocaleFixture
{
    LocaleFixture() { LOADER.LoadFile(RTTRCONFIG.ExpandPath(FILE_PATHS[95]) + "/languages.ini", NULL, true); }
};

BOOST_FIXTURE_TEST_SUITE(Locales, LocaleFixture)

BOOST_AUTO_TEST_CASE(LocaleFormatTest)
{
    libsiedler2::ArchivItem_Ini ini;
    BOOST_CHECK_EQUAL(s25util::toStringClassic(123456), "123456");
    BOOST_CHECK_EQUAL(s25util::toStringClassic(123456.5), "123456.5");
    BOOST_CHECK_EQUAL(s25util::fromStringClassic<float>("123456.5"), 123456.5);
    BOOST_CHECK_EQUAL(s25util::fromStringClassic<int>("123456"), 123456);
    ini.setValue("int", 123456);
    ini.setValue("string", "123456");
    BOOST_REQUIRE_EQUAL(ini.getValue("int"), "123456");
    BOOST_REQUIRE_EQUAL(ini.getValueI("string"), 123456);

    std::string oldLang = mysetlocale(LC_ALL, NULL);
    // Should work on all languages
    const unsigned numLanguages = LANGUAGES.getCount();
    for(unsigned i = 0; i < numLanguages; i++)
    {
        LANGUAGES.setLanguage(i);
        BOOST_CHECK_EQUAL(s25util::toStringClassic(123456), "123456");
        BOOST_CHECK_EQUAL(s25util::toStringClassic(123456.5), "123456.5");
        BOOST_CHECK_EQUAL(s25util::fromStringClassic<float>("123456.5"), 123456.5);
        BOOST_CHECK_EQUAL(s25util::fromStringClassic<int>("123456"), 123456);
        ini.setValue("int", 123456);
        ini.setValue("string", "123456");
        BOOST_REQUIRE_EQUAL(ini.getValue("int"), "123456");
        BOOST_REQUIRE_EQUAL(ini.getValueI("string"), 123456);
    }
    mysetlocale(LC_ALL, oldLang.c_str());
}

BOOST_AUTO_TEST_SUITE_END()
