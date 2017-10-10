// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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
#ifndef LANGUAGES_H_INCLUDED
#define LANGUAGES_H_INCLUDED

#pragma once

#include "libutil/Singleton.h"
#include <string>
#include <vector>

struct Language
{
    Language(const std::string& name, const std::string& code) : name(name), code(code) {}

    std::string name;
    std::string code; // "normaler" locale-code
};

class Languages : public Singleton<Languages>
{
public:
    Languages() : loaded(false) {}

    void setLanguage(const std::string& lang_code);
    const std::string setLanguage(unsigned i);

    unsigned getCount();
    const Language& getLanguage(unsigned i);

private:
    void loadLanguages();

    std::vector<Language> languages;
    bool loaded;
};

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#define LANGUAGES Languages::inst()

#endif // !LANGUAGES_H_INCLUDED
