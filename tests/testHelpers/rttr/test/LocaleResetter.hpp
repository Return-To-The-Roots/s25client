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

#ifndef LocaleResetter_h__
#define LocaleResetter_h__

#include <mygettext/mygettext.h>

namespace rttr { namespace test {
    struct LocaleResetter
    {
        const std::string oldLoc;
        LocaleResetter(const char* newLoc) : oldLoc(mygettext::setlocale(LC_ALL, nullptr)) { mygettext::setlocale(LC_ALL, newLoc); }
        ~LocaleResetter() { mygettext::setlocale(LC_ALL, oldLoc.c_str()); }
    };
}} // namespace rttr::test

#endif // LocaleResetter_h__
