// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "helpers/EnumArray.h"
#include "gameTypes/GoodTypes.h"

/// Mapping of tools to goods
constexpr helpers::EnumArray<GoodType, Tool> TOOL_TO_GOOD = {
  GoodType::Tongs,    GoodType::Hammer,     GoodType::Axe,    GoodType::Saw,     GoodType::PickAxe,    GoodType::Shovel,
  GoodType::Crucible, GoodType::RodAndLine, GoodType::Scythe, GoodType::Cleaver, GoodType::Rollingpin, GoodType::Bow};

constexpr helpers::EnumArray<signed, Tool> SUPPRESS_UNUSED TOOL_PRIORITY {
    {
        2, //Tongs
        3, //Hammer
        4, //Axe
        3, //Saw
        8, //PickAxe
        1, //Shovel
        5, //Crucible
        3, //RodAndLine
        8, //Scythe
        2, //Cleaver
        4, //Rollingpin
        1, //Bow
    }
};