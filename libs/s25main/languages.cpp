// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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
