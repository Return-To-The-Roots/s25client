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

#include "helpers/MaxEnumValue.h"
#include <array>
#include <cstdint>

/// Types of pacts
enum PactType : uint8_t
{
    TREATY_OF_ALLIANCE,
    NON_AGGRESSION_PACT
};
constexpr auto maxEnumValue(PactType)
{
    return NON_AGGRESSION_PACT;
}

/// Number of the various pacts
constexpr unsigned NUM_PACTS = helpers::NumEnumValues_v<PactType>;

constexpr unsigned DURATION_INFINITE = 0xFFFFFFFF;

/// Names of the possible pacts
extern const std::array<const char*, NUM_PACTS> PACT_NAMES;
