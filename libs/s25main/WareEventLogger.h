// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameTypes/GoodTypes.h"

namespace WareEventLogger {

void LogInventoryChange(unsigned gf, unsigned char playerId, GoodType good, int count);

} // namespace WareEventLogger
