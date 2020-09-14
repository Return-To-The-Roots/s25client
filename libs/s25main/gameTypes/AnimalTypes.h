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

#pragma once

#include "helpers/MaxEnumValue.h"
#include <cstdint>

/// Tierarten
enum Species : uint8_t
{
    SPEC_POLARBEAR = 0,
    SPEC_RABBITWHITE,
    SPEC_RABBITGREY,
    SPEC_FOX,
    SPEC_STAG,
    SPEC_DEER,
    SPEC_DUCK,
    SPEC_SHEEP
};
DEFINE_MAX_ENUM_VALUE(Species, Species::SPEC_SHEEP)
