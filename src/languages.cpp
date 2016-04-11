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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "defines.h" // IWYU pragma: keep
#include "languages.h"

#include "Loader.h"

#include "files.h"
#include "Settings.h"
#include "libsiedler2/src/ArchivItem_Ini.h"
#include "libsiedler2/src/ArchivItem_Text.h"
#include "mygettext/src/mygettext.h"

#include <algorithm>

// Include last!
#include "DebugNew.h" // IWYU pragma: keep

void Languages::loadLanguages()
{
    const libsiedler2::ArchivInfo& langInfo = dynamic_cast<const libsiedler2::ArchivItem_Ini&>(*LOADER.GetInfoN("languages")->find("Languages"));
    unsigned int count = langInfo.size();

    for(unsigned int i = 0; i < count; i++)
    {
        const libsiedler2::ArchivItem_Text& langEntry = dynamic_cast<const libsiedler2::ArchivItem_Text&>(*langInfo[i]);
        Language lang(langEntry.getName(), langEntry.getText());
        RTTR_Assert(!lang.name.empty());
        languages.push_back(lang);
    }

    // Sprachen sortieren
    std::sort(languages.begin(), languages.end(), Language::compare);

    // Systemsprache hinzufügen
    Language l(gettext_noop("System language"), "");
    languages.insert(languages.begin(), l);

    loaded = true;
}

const Languages::Language& Languages::getLanguage(unsigned int i)
{
    if(!loaded)
        loadLanguages();

    if(i < languages.size())
        return languages.at(i);

    return languages.at(0);
}

unsigned int Languages::getCount()
{
    if(!loaded)
        loadLanguages();

    return unsigned(languages.size());
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author FloSoft
 *  @author OLiver
 */
void Languages::setLanguage(const std::string& lang_code)
{
    SETTINGS.language.language = lang_code; //-V807

    std::string locale = mysetlocale(LC_ALL, lang_code.c_str());
    if(SETTINGS.language.language.length() == 0)
        SETTINGS.language.language = locale;

    const char* domain = "rttr";
    bind_textdomain_codeset(domain, "UTF-8");
    bindtextdomain(domain, FILE_PATHS[15]);
    textdomain(domain);
}

const std::string Languages::setLanguage(unsigned int i)
{
    const Language l = getLanguage(i);

    setLanguage(l.code);

    return l.code;
}
