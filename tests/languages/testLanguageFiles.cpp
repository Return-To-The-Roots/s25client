// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#define BOOST_TEST_MODULE RTTR_LanguageFiles

#include "helpers/OptionalIO.h"
#include "languageFiles.h"
#include "mygettext/readCatalog.h"
#include "s25util/utf8.h"
#include <boost/filesystem.hpp>
#include <boost/format.hpp>
#include <boost/test/unit_test.hpp>
#include <map>
#include <optional>

#if RTTR_HAS_VLD
#    include <vld.h>
#endif

namespace {
struct FormatProperties
{
    std::optional<int> numParameters;
};

FormatProperties getFormatProperties(const std::string& str)
{
    try
    {
        const boost::format fmt(str);
        return FormatProperties{fmt.expected_args()};
    } catch(const std::exception&)
    {
        return FormatProperties{std::nullopt};
    }
}

std::map<std::string, FormatProperties> getGoldMapping()
{
    const std::map<std::string, std::string> translations =
      mygettext::readCatalog(std::string(RTTR_TRANSLATION_DIR) + "/rttr-en_GB.mo", "UTF-8");
    std::map<std::string, FormatProperties> result;
    for(const auto& entry : translations)
    {
        if(entry.first.empty())
            continue;
        result.emplace(entry.first, getFormatProperties(entry.first));
    }
    return result;
}

std::string replaceLF(std::string s)
{
    size_t index = 0;
    while((index = s.find('\n')) != std::string::npos)
    {
        s.replace(index, 1, "\\n");
    }
    return s;
}
} // namespace

// GCC until 10 has issues comparing std::optional in BOOST_TEST
#if defined(__GNUC__) && __GNUC__ < 10
#    define BOOST_TEST_OPTIONAL BOOST_CHECK
#else
#    define BOOST_TEST_OPTIONAL BOOST_TEST
#endif

BOOST_AUTO_TEST_CASE(AllFilesHaveValidFormat)
{
    const auto goldMapping = getGoldMapping();
    for(const auto& itFile : boost::filesystem::directory_iterator(RTTR_TRANSLATION_DIR))
    {
        if(!is_regular_file(itFile.status()) || itFile.path().extension() != ".mo")
            continue; // LCOV_EXCL_LINE
        const auto translatedStrings = mygettext::readCatalog(itFile.path().string(), "UTF-8");

        BOOST_TEST_CONTEXT("Locale: " << itFile.path().stem())
        for(const auto& entry : translatedStrings)
        {
            const auto itGoldEntry = goldMapping.find(entry.first);
            const auto& [origTxt, translation] = entry;
            BOOST_TEST_CONTEXT("Entry '" << replaceLF(origTxt) << "' => '" << replaceLF(translation) << "'")
            {
                BOOST_TEST(s25util::isValidUTF8(translation));
                // Note: "Check 50%" is invalid (ends in %) but "50% checked" is not and translations might move the %
                // around Hence rely on the number of format args which should be consistent
                FormatProperties origProps = getFormatProperties(origTxt);
                FormatProperties transProps = getFormatProperties(translation);
                // Orig text replacements must match with gold version
                // Might not exist if orig text is not "translated" (already in English)
                if(itGoldEntry != goldMapping.end())
                    BOOST_TEST_OPTIONAL(origProps.numParameters == itGoldEntry->second.numParameters);
                if(origProps.numParameters.has_value() == transProps.numParameters.has_value())
                {
                    BOOST_TEST_OPTIONAL(origProps.numParameters == transProps.numParameters);
                } else if(origProps.numParameters > 0)
                    BOOST_TEST_ERROR("Invalid format string in translation"); // LCOV_EXCL_LINE
            }
        }
    }
}
