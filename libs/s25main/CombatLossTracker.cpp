// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "CombatLossTracker.h"

#include "buildings/nobBaseMilitary.h"
#include "figures/nofActiveSoldier.h"
#include "figures/nofAggressiveDefender.h"
#include "figures/nofAttacker.h"
#include "figures/nofDefender.h"
#include <unordered_map>

namespace {
std::unordered_map<unsigned, CombatStats> gCombatStats;

void AddLoss(const unsigned targetObjId, const unsigned char rank, const bool attacker)
{
    auto it = gCombatStats.find(targetObjId);
    if(it == gCombatStats.end())
        return;
    auto& arr = attacker ? it->second.attackerLosses : it->second.defenderLosses;
    const std::size_t idx = std::min<std::size_t>(rank, arr.size() - 1);
    arr[idx]++;
}

const nobBaseMilitary* GetTargetBuilding(const nofAggressiveDefender& defender)
{
    if(const nofAttacker* attacker = defender.GetAttacker())
        return attacker->GetAttackedGoal();
    return defender.GetHome();
}

const nobBaseMilitary* GetTargetBuilding(const nofDefender& defender)
{
    if(const nofAttacker* attacker = defender.GetAttacker())
        return attacker->GetAttackedGoal();
    return defender.GetHome();
}
} // namespace

namespace CombatLossTracker {

void RegisterCombat(const unsigned targetObjId)
{
    gCombatStats.try_emplace(targetObjId, CombatStats{});
}

CombatStats TakeStats(const unsigned targetObjId)
{
    const auto it = gCombatStats.find(targetObjId);
    if(it == gCombatStats.end())
        return CombatStats{};
    CombatStats result = it->second;
    gCombatStats.erase(it);
    return result;
}

void ReportLoss(const nofActiveSoldier& soldier)
{
    if(const auto* attacker = dynamic_cast<const nofAttacker*>(&soldier))
    {
        if(const nobBaseMilitary* goal = attacker->GetAttackedGoal())
            AddLoss(goal->GetObjId(), attacker->GetRank(), true);
        return;
    }
    if(const auto* defender = dynamic_cast<const nofDefender*>(&soldier))
    {
        if(const nobBaseMilitary* goal = GetTargetBuilding(*defender))
            AddLoss(goal->GetObjId(), defender->GetRank(), false);
        return;
    }
    if(const auto* aggDef = dynamic_cast<const nofAggressiveDefender*>(&soldier))
    {
        if(const nobBaseMilitary* goal = GetTargetBuilding(*aggDef))
            AddLoss(goal->GetObjId(), aggDef->GetRank(), false);
        return;
    }
}

void ReportParticipant(const nofActiveSoldier& soldier)
{
    const nofAttacker* attacker = dynamic_cast<const nofAttacker*>(&soldier);
    const nofAggressiveDefender* aggDef = attacker ? nullptr : dynamic_cast<const nofAggressiveDefender*>(&soldier);

    const nobBaseMilitary* goal = nullptr;
    bool isAttacker = false;
    if(attacker)
    {
        goal = attacker->GetAttackedGoal();
        isAttacker = true;
    } else if(const auto* defender = dynamic_cast<const nofDefender*>(&soldier))
        goal = GetTargetBuilding(*defender);
    else if(aggDef)
        goal = GetTargetBuilding(*aggDef);

    if(!goal)
        return;

    auto it = gCombatStats.find(goal->GetObjId());
    if(it == gCombatStats.end())
        return;

    auto& arr = isAttacker ? it->second.attackerForces : it->second.defenderForces;
    const std::size_t idx = std::min<std::size_t>(soldier.GetRank(), arr.size() - 1);
    arr[idx]++;
}

void ReportDestroyedBuilding(const unsigned targetObjId, const BuildingType type)
{
    auto it = gCombatStats.find(targetObjId);
    if(it == gCombatStats.end())
        return;

    ++it->second.destroyedBuildings[type];
}

} // namespace CombatLossTracker
