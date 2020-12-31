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

#include "helpers/EnumArray.h"
#include "gameTypes/GoodTypes.h"
#include "gameTypes/Nation.h"
#include "s25util/warningSuppression.h"

/// Umgekehrte Konvertierung: Gibt den Schildtyp für jede Nation an
const helpers::EnumArray<GoodType, Nation> SUPPRESS_UNUSED SHIELD_TYPES = {
  GoodType::ShieldAfricans, GoodType::ShieldJapanese, GoodType::ShieldRomans, GoodType::ShieldVikings,
  GoodType::ShieldJapanese};

/// Macht ggf. aus den verschiedenen Schilden der Nationen jeweils immer das römische normale Schild für
/// die Warensysteme usw
inline constexpr GoodType ConvertShields(const GoodType& good)
{
    return (good == GoodType::ShieldVikings || good == GoodType::ShieldAfricans || good == GoodType::ShieldJapanese) ?
             GoodType::ShieldRomans :
             good;
}

inline constexpr GoodType convertShieldToNation(const GoodType good, const Nation nation)
{
    return (good == GoodType::ShieldRomans) ? SHIELD_TYPES[nation] : good;
}
