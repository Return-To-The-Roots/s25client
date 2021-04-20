// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "languages.h"
#include "Loader.h"
#include "RttrConfig.h"
#include "files.h"
#include "mygettext/mygettext.h"
#include "libsiedler2/ArchivItem_Ini.h"
#include "libsiedler2/ArchivItem_Text.h"
#include <algorithm>

static bool operator<(const Language& o1, const Language& o2)
{
    if(o1.name < o2.name)
        return true;

    if(o1.name == o2.name)
        return o1.code < o2.code;
    return false;
}

Languages::Languages() : loaded(false)
{
    const char* domain = "rttr";
    mygettext::bind_textdomain_codeset(domain, "UTF-8");
    mygettext::bindtextdomain(domain, RTTRCONFIG.ExpandPath(s25::folders::languages).string().c_str());
    mygettext::textdomain(domain);
}

void Languages::loadLanguages()
{
    const libsiedler2::Archiv& langInfo =
      dynamic_cast<const libsiedler2::ArchivItem_Ini&>(*LOADER.GetArchive("languages").find("Languages"));
    unsigned count = langInfo.size();

    for(unsigned i = 0; i < count; i++)
    {
        const auto& langEntry = dynamic_cast<const libsiedler2::ArchivItem_Text&>(*langInfo[i]);
        Language lang(langEntry.getName(), langEntry.getText());
        RTTR_Assert(!lang.name.empty());
        languages.push_back(lang);
    }

    // Sprachen sortieren
    std::sort(languages.begin(), languages.end());

    // Systemsprache hinzufügen
    Language l(gettext_noop("System language"), "");
    languages.insert(languages.begin(), l);

    loaded = true;
}

const Language& Languages::getLanguage(unsigned i)
{
    if(!loaded)
        loadLanguages();

    if(i >= languages.size())
        i = 0;

    return languages.at(i);
}

unsigned Languages::size()
{
    if(!loaded)
        loadLanguages();

    return static_cast<unsigned>(languages.size());
}

void Languages::setLanguage(const std::string& lang_code)
{
    mygettext::setlocale(LC_ALL, lang_code.c_str());
}

std::string Languages::setLanguage(unsigned i)
{
    const Language& l = getLanguage(i);
    setLanguage(l.code);
    return l.code;
}
