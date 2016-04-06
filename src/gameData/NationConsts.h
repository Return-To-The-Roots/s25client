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

#pragma once

#ifndef NationConsts_h__
#define NationConsts_h__

#include "mygettext/src/mygettext.h"
#include <boost/array.hpp>

/// Völker (dont forget to change shield-count in iwWares too ...)
enum Nation
{
	NAT_AFRICANS = 0,
	NAT_JAPANESE,
	NAT_ROMANS,
	NAT_VIKINGS,
	NAT_BABYLONIANS,
	NAT_COUNT,
	NAT_INVALID = 0xFFFFFFFF
};

const boost::array<const char*, NAT_COUNT> SUPPRESS_UNUSED NationNames = {{
        gettext_noop("Africans"),
        gettext_noop("Japanese"),
        gettext_noop("Romans"),
        gettext_noop("Vikings"),
        gettext_noop("Babylonians")
}};

#define NATIVE_NAT_COUNT 4

/// Konvertierungstabelle von RttR-Nation-Indizes in Original-S2-Nation-Indizes
const boost::array<unsigned char, NAT_COUNT> SUPPRESS_UNUSED NATION_RTTR_TO_S2 =
{{
    3,
    2,
    0,
    1,
    0 /* Babylonians get the roman figures where no others are used */
}};

#endif // NationConsts_h__
