// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "Inventory.h"
#include <algorithm>

void Inventory::clear()
{
    std::fill(goods.begin(), goods.end(), 0);
    std::fill(people.begin(), people.end(), 0);
    std::fill(armoredSoldiers.begin(), armoredSoldiers.end(), 0);
}
