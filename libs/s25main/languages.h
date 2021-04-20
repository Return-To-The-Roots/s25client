// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "s25util/Singleton.h"
#include <string>
#include <utility>
#include <vector>

struct Language
{
    Language(std::string name, std::string code) : name(std::move(name)), code(std::move(code)) {}

    std::string name;
    std::string code; // "normaler" locale-code
};

class Languages : public Singleton<Languages>
{
public:
    Languages();

    static void setLanguage(const std::string& lang_code);
    std::string setLanguage(unsigned i);

    unsigned size();
    const Language& getLanguage(unsigned i);

private:
    void loadLanguages();

    std::vector<Language> languages;
    bool loaded;
};

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#define LANGUAGES Languages::inst()
