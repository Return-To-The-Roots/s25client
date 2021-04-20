// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GamePlayerInfo.h"

GamePlayerInfo::GamePlayerInfo(unsigned playerId, const PlayerInfo& playerInfo)
    : PlayerInfo(playerInfo), id(playerId), isDefeated(false)
{}
