// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameData/BuildingConsts.h"
#include "gameData/MilitaryConsts.h"
#include <array>
#include <map>

class nofActiveSoldier;

struct CombatStats
{
    std::array<unsigned, NUM_SOLDIER_RANKS> attackerLosses{};
    std::array<unsigned, NUM_SOLDIER_RANKS> defenderLosses{};
    std::array<unsigned, NUM_SOLDIER_RANKS> attackerForces{};
    std::array<unsigned, NUM_SOLDIER_RANKS> defenderForces{};
    std::map<BuildingType, unsigned> destroyedBuildings;
    bool hadEngagement = false;
};

namespace CombatLossTracker {

void RegisterCombat(unsigned targetObjId);
CombatStats TakeStats(unsigned targetObjId);
void ReportLoss(const nofActiveSoldier& soldier);
void ReportParticipant(const nofActiveSoldier& soldier);
void ReportDestroyedBuilding(unsigned targetObjId, BuildingType type);

} // namespace CombatLossTracker
