// Copyright (C) 2005 - 2025 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later
#include <gameTypes/JobTypes.h>

ArmoredSoldier jobEnumToAmoredSoldierEnum(const Job job)
{
    return static_cast<ArmoredSoldier>(getSoldierRank(job));
}
