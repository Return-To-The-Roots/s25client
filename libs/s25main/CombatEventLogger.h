// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameData/BuildingConsts.h"
#include "gameData/MilitaryConsts.h"
#include <array>

namespace CombatEventLogger {

void LogAttackOrder(unsigned gf, unsigned char attackerPlayer, unsigned char defenderPlayer, BuildingType targetType,
                    unsigned targetObjId, bool strongSoldiers, unsigned desiredCount, unsigned actualCount,
                    const std::array<unsigned, NUM_SOLDIER_RANKS>& actualByRank);

void LogAggressiveDefenderOrder(unsigned gf, unsigned char attackerPlayer, BuildingType targetType, unsigned targetObjId,
                                unsigned char defenderPlayer, BuildingType defenderHomeType, unsigned defenderHomeObjId,
                                unsigned char defenderRank);

void LogFightResult(unsigned gf, unsigned char attackerPlayer, BuildingType targetType, unsigned targetObjId,
                    unsigned char defenderPlayer, unsigned attackerRank, unsigned attackerStartHp, unsigned defenderRank,
                    unsigned defenderStartHp, const char* winnerRole, unsigned winnerRemainingHp);

void RecordCaptureDestroyed(unsigned capturingObjId, BuildingType type);
void LogCapture(unsigned gf, unsigned char attackerPlayer, unsigned char defenderPlayer, BuildingType buildingType,
                unsigned buildingObjId);

} // namespace CombatEventLogger
