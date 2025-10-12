// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "iwWares.h"
#include "GamePlayer.h"
#include "nodeObjs/noFlag.h"

class iwRoadStats : public iwWares
{
public:
    iwRoadStats(const GamePlayer& player, const noFlag* flag);
};
