// $Id: languages.h 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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

#include "Singleton.h"

class Languages: public Singleton<Languages>
{
    public:
        struct Language
        {
            Language(std::string name, std::string code) : name(name), code(code) {}

            static bool compare(const Language& o1, const Language& o2)
            {
                if (o1.name < o2.name)
                    return true;

                if (o1.name == o2.name)
                {
                    if (o1.code < o2.code)
                        return true;
                }
                return false;
            }

            std::string name;
            std::string code;  // "normaler" locale-code
        };

    public:
        Languages() : loaded(false) {}

        void setLanguage(const std::string& lang_code);
        const std::string setLanguage(unsigned int i);

        unsigned int getCount(void);
        const Language& getLanguage(unsigned int i);

    protected:
        void loadLanguages(void);

    private:
        std::vector<Language> languages;
        bool loaded;
};

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#define LANGUAGES Languages::inst()

#endif // !LANGUAGES_H_INCLUDED
