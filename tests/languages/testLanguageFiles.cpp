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

#define BOOST_TEST_MODULE RTTR_LanguageFiles

#include "languageFiles.h"
#include "mygettext/readCatalog.h"
#include "s25util/utf8.h"
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/test/unit_test.hpp>
#include <map>

struct FormatProperties
{
    int numParameters;
};

static std::map<std::string, FormatProperties> getGoldMapping()
{
    const std::map<std::string, std::string> translations =
      mygettext::readCatalog(std::string(RTTR_TRANSLATION_DIR) + "/rttr-en_GB.mo", "UTF-8");
    std::map<std::string, FormatProperties> result;
    for(const auto& entry : translations)
    {
        if(entry.first.empty())
            continue;
        try
        {
            const boost::format fmt(entry.first);
            result.emplace(std::make_pair(entry.first, FormatProperties{fmt.expected_args()}));
        } catch(const std::exception&)
        {
            result.emplace(std::make_pair(entry.first, FormatProperties{0}));
        }
    }
    return result;
}

static std::string replaceLF(std::string s)
{
    size_t index = 0;
    while((index = s.find('\n')) != std::string::npos)
    {
        s.replace(index, 1, "\\n");
    }
    return s;
}

BOOST_AUTO_TEST_CASE(AllFilesHaveValidFormat)
{
    const auto goldMapping = getGoldMapping();
    for(const auto& it : boost::filesystem::directory_iterator(RTTR_TRANSLATION_DIR))
    {
        if(!is_regular_file(it.status()) || it.path().extension() != ".mo")
            continue;
        const auto translatedStrings = mygettext::readCatalog(it.path().string(), "UTF-8");

        BOOST_TEST_CONTEXT("Locale: " << it.path().stem())
        for(const auto& entry : goldMapping)
        {
            const auto it = translatedStrings.find(entry.first);
            if(it == translatedStrings.end())
                continue; // Not translated
            BOOST_TEST_CONTEXT("Entry '" << replaceLF(it->first) << "' => '" << replaceLF(it->second) << "'")
            {
                BOOST_TEST(s25util::isValidUTF8(it->second));
                // Note: "Check 50%" is invalid (ends in %) but "50% checked" is not and translations might move the % around
                // Hence rely on the number of format args which should be consistent
                try
                {
                    const boost::format fmt(it->second);
                    BOOST_TEST(fmt.expected_args() == entry.second.numParameters);
                } catch(const std::exception&)
                {
                    if(entry.second.numParameters > 0) // Should have been a format string
                        BOOST_TEST_ERROR("Invalid format string");
                }
            }
        }
    }
}
