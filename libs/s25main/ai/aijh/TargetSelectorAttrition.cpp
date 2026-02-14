// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AIPlayerJH.h"

#include "AIConfig.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobMilitary.h"
#include "EventManager.h"
#include "gameTypes/StatisticTypes.h"
#include "world/GameWorldBase.h"

#include <algorithm>
#include <cstdint>
#include <vector>

namespace AIJH {

namespace {

constexpr unsigned RECENT_RECAPTURE_WINDOW_GFS = 2000;
constexpr unsigned RECENT_TARGET_WINDOW_GFS = 1000;

unsigned GetSoldierCount(const nobBaseMilitary& building)
{
    if(const auto* military = dynamic_cast<const nobMilitary*>(&building))
        return military->GetNumTroops();

    if(const auto* warehouse = dynamic_cast<const nobBaseWarehouse*>(&building))
        return warehouse->GetNumSoldiers();

    return building.DefendersAvailable() ? 1u : 0u;
}

bool HasForceAdvantage(const GameWorldBase& gwb, const AIInterface& aii, unsigned char player_id,
                       double forceAdvantageRatio)
{
    const auto& player = gwb.GetPlayer(player_id);
    const unsigned own_strength = player.GetStatisticCurrentValue(StatisticType::Military);

    unsigned strongest_enemy = 0;
    for(unsigned pid = 0; pid < gwb.GetNumPlayers(); ++pid)
    {
        if(pid == player_id || !aii.IsPlayerAttackable(pid))
            continue;

        const unsigned enemy_strength = gwb.GetPlayer(pid).GetStatisticCurrentValue(StatisticType::Military);
        strongest_enemy = std::max(strongest_enemy, enemy_strength);
    }

    if(strongest_enemy == 0)
        return true;

    return static_cast<double>(own_strength) >= static_cast<double>(strongest_enemy) * forceAdvantageRatio;
}

bool IsRecentCapture(const nobBaseMilitary& target, unsigned currentGF, unsigned captureWindow)
{
    const unsigned capturedGF = target.GetCapturedGF();
    if(capturedGF == 0)
        return false;

    return currentGF >= capturedGF && currentGF - capturedGF <= captureWindow;
}

const nobBaseMilitary*
  PickBestTarget(const std::vector<const nobBaseMilitary*>& candidates, unsigned currentGF, unsigned captureWindow)
{
    if(candidates.empty())
        return nullptr;

    std::vector<const nobBaseMilitary*> recent;
    recent.reserve(candidates.size());
    for(const nobBaseMilitary* target : candidates)
    {
        if(IsRecentCapture(*target, currentGF, captureWindow))
            recent.push_back(target);
    }

    const auto& prioritized = recent.empty() ? candidates : recent;
    return *std::min_element(prioritized.begin(), prioritized.end(), [](const nobBaseMilitary* lhs,
                                                                       const nobBaseMilitary* rhs) {
        const unsigned lhs_count = GetSoldierCount(*lhs);
        const unsigned rhs_count = GetSoldierCount(*rhs);
        if(lhs_count == rhs_count)
            return lhs < rhs;
        return lhs_count < rhs_count;
    });
}

} // namespace

const nobBaseMilitary* AIPlayerJH::SelectAttackTargetAttrition() const
{
    unsigned unused_special_targets = 0;
    std::vector<const nobBaseMilitary*> potentialTargets = GetPotentialTargets(unused_special_targets);
    if(potentialTargets.empty())
        return nullptr;

    const unsigned currentGF = gwb.GetEvMgr().GetCurrentGF();
    std::vector<const nobBaseMilitary*> recaptureCandidates;
    recaptureCandidates.reserve(potentialTargets.size());

    for(const nobBaseMilitary* target : potentialTargets)
    {
        if(target->GetGOT() != GO_Type::NobMilitary)
            continue;
        if(target->GetOriginOwner() != playerId)
            continue;
        recaptureCandidates.push_back(target);
    }

    if(const nobBaseMilitary* recapture = PickBestTarget(recaptureCandidates, currentGF, RECENT_RECAPTURE_WINDOW_GFS))
        return recapture;

    if(HasForceAdvantage(gwb, aii, playerId, config_.combat.forceAdvantageRatio))
        return SelectAttackTargetBiting();

    return nullptr;
}

} // namespace AIJH
