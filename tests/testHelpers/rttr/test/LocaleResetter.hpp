// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <mygettext/mygettext.h>

namespace rttr::test {
    struct LocaleResetter
    {
        const std::string oldLoc;
        LocaleResetter(const char* newLoc) : oldLoc(mygettext::setlocale(LC_ALL, nullptr))
        {
            mygettext::setlocale(LC_ALL, newLoc);
        }
        ~LocaleResetter() { mygettext::setlocale(LC_ALL, oldLoc.c_str()); }
    };
} // namespace rttr::test
