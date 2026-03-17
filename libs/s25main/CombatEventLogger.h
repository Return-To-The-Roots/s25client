// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameData/BuildingConsts.h"
#include "gameData/MilitaryConsts.h"
#include <array>
#include <vector>

namespace CombatEventLogger {

struct AttackSource
{
    BuildingType buildingType;
    unsigned buildingObjId;
    unsigned count;
    std::array<unsigned, NUM_SOLDIER_RANKS> byRank{};
};

void LogAttackOrder(unsigned gf, unsigned char attackerPlayer, unsigned char defenderPlayer, BuildingType targetType,
                    unsigned targetObjId, bool strongSoldiers, unsigned desiredCount, unsigned actualCount,
                    const std::array<unsigned, NUM_SOLDIER_RANKS>& actualByRank,
                    const std::vector<AttackSource>& sources);

void LogAggressiveDefenderOrder(unsigned gf, unsigned char attackerPlayer, BuildingType targetType, unsigned targetObjId,
                                unsigned char defenderPlayer, const std::vector<AttackSource>& sources,
                                unsigned char defenderRank);

void LogFightResult(unsigned gf, unsigned char attackerPlayer, BuildingType targetType, unsigned targetObjId,
                    unsigned char defenderPlayer, unsigned attackerRank, unsigned attackerStartHp, unsigned defenderRank,
                    unsigned defenderStartHp, const char* winnerRole, unsigned winnerRemainingHp);

void RecordCaptureDestroyed(unsigned capturingObjId, BuildingType type, unsigned destroyedObjId);
void LogCapture(unsigned gf, unsigned char attackerPlayer, unsigned char defenderPlayer, BuildingType buildingType,
                unsigned buildingObjId);

} // namespace CombatEventLogger
