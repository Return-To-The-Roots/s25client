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

#pragma once

#ifndef NationConsts_h__
#define NationConsts_h__

#include "mygettext/mygettext.h"
#include "gameTypes/Nation.h"
#include <s25util/warningSuppression.h>
#include <array>

const std::array<const char*, NUM_NATIONS> SUPPRESS_UNUSED NationNames = {
  gettext_noop("Africans"), gettext_noop("Japanese"), gettext_noop("Romans"), gettext_noop("Vikings"), gettext_noop("Babylonians")};

/// Konvertierungstabelle von RttR-Nation-Indizes in Original-S2-Nation-Indizes
const std::array<unsigned char, NUM_NATIONS> SUPPRESS_UNUSED NATION_RTTR_TO_S2 = {
  3, 2, 0, 1, 0 /* Babylonians get the roman figures where no others are used */
};

#endif // NationConsts_h__
