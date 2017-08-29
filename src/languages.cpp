// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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
#include "languages.h"

#include "Loader.h"

#include "Settings.h"
#include "files.h"
#include "mygettext/src/mygettext.h"
#include "libsiedler2/src/ArchivItem_Ini.h"
#include "libsiedler2/src/ArchivItem_Text.h"

#include <algorithm>

bool operator<(const Language& o1, const Language& o2)
{
    if(o1.name < o2.name)
        return true;

    if(o1.name == o2.name)
        return o1.code < o2.code;
    return false;
}

void Languages::loadLanguages()
{
    const libsiedler2::Archiv& langInfo =
      dynamic_cast<const libsiedler2::ArchivItem_Ini&>(*LOADER.GetInfoN("languages")->find("Languages"));
    unsigned count = langInfo.size();

    for(unsigned i = 0; i < count; i++)
    {
        const libsiedler2::ArchivItem_Text& langEntry = dynamic_cast<const libsiedler2::ArchivItem_Text&>(*langInfo[i]);
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

    if(i < languages.size())
        return languages[i];

    return languages.at(0);
}

unsigned Languages::getCount()
{
    if(!loaded)
        loadLanguages();

    return unsigned(languages.size());
}

void Languages::setLanguage(const std::string& lang_code)
{
    SETTINGS.language.language = lang_code; //-V807

    std::string locale = mysetlocale(LC_ALL, lang_code.c_str());
    if(SETTINGS.language.language.empty())
        SETTINGS.language.language = locale;

    const char* domain = "rttr";
    bind_textdomain_codeset(domain, "UTF-8");
    bindtextdomain(domain, FILE_PATHS[15]);
    textdomain(domain);
}

const std::string Languages::setLanguage(unsigned i)
{
    const Language& l = getLanguage(i);

    setLanguage(l.code);

    return l.code;
}
