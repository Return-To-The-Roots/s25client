// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "helpers/MaxEnumValue.h"

enum class MineNoOutputFallback
{
    ProduceNothing,
    ProduceGranite25,
    ProduceGranite50,
    ProduceGranite100,
    ProduceLowerGradeResource
};

constexpr auto maxEnumValue(MineNoOutputFallback)
{
    return MineNoOutputFallback::ProduceLowerGradeResource;
}
