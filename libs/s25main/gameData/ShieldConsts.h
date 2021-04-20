// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
