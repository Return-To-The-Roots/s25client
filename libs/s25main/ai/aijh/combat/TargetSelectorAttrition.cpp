// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AICombatController.h"

#include "ai/AIInterface.h"
#include "ai/AIQueryService.h"
#include "ai/aijh/config/AIConfig.h"
#include "ai/aijh/debug/AIRuntimeProfiler.h"
#include "MilitaryStatsHolder.h"
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

bool HasForceAdvantage(const GameWorldBase& gwb, const AIQueryService& queries, unsigned char player_id,
                       double forceAdvantageRatio)
{
    const auto& player = gwb.GetPlayer(player_id);
    const unsigned own_strength = player.GetStatisticCurrentValue(StatisticType::Military);

    unsigned strongest_enemy = 0;
    for(unsigned pid = 0; pid < gwb.GetNumPlayers(); ++pid)
    {
        if(pid == player_id || !queries.IsPlayerAttackable(pid))
            continue;

        const unsigned enemy_strength = gwb.GetPlayer(pid).GetStatisticCurrentValue(StatisticType::Military);
        strongest_enemy = std::max(strongest_enemy, enemy_strength);
    }

    if(strongest_enemy == 0)
        return true;

    return static_cast<double>(own_strength) >= static_cast<double>(strongest_enemy) * forceAdvantageRatio;
}

bool HasNearTroopsDensityForBiting(const GameWorldBase& gwb, unsigned char player_id, double minNearTroopsDensity)
{
    if(minNearTroopsDensity <= 0.0)
        return true;

    const auto& player = gwb.GetPlayer(player_id);
    MilitaryStatsHolder::RefreshDensities(player);
    const auto& stats = MilitaryStatsHolder::GetPlayerStats(player_id);
    return stats.densityNear >= minNearTroopsDensity;
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

const nobBaseMilitary* AICombatController::SelectAttackTargetAttrition() const
{
    const ScopedAIRuntimeProfile attritionProfile(AIRuntimeProfileSection::SelectAttackTargetAttrition);
    unsigned unused_special_targets = 0;
    std::vector<const nobBaseMilitary*> potentialTargets = [&] {
        const ScopedAIRuntimeProfile potentialTargetsProfile(AIRuntimeProfileSection::AttritionGetPotentialTargets);
        return GetPotentialTargets(unused_special_targets);
    }();
    if(potentialTargets.empty())
        return nullptr;

    const unsigned currentGF = owner_.GetWorld().GetEvMgr().GetCurrentGF();
    std::vector<const nobBaseMilitary*> recaptureCandidates;
    recaptureCandidates.reserve(potentialTargets.size());

    {
        const ScopedAIRuntimeProfile recaptureScanProfile(AIRuntimeProfileSection::AttritionRecaptureScan,
                                                          potentialTargets.size());
        for(const nobBaseMilitary* target : potentialTargets)
        {
            if(target->GetGOT() != GO_Type::NobMilitary)
                continue;
            if(target->GetOriginOwner() != owner_.GetPlayerId())
                continue;
            recaptureCandidates.push_back(target);
        }
    }

    if(const nobBaseMilitary* recapture = [&] {
           const ScopedAIRuntimeProfile pickRecaptureProfile(AIRuntimeProfileSection::AttritionPickRecapture,
                                                             recaptureCandidates.size());
           return PickBestTarget(recaptureCandidates, currentGF, RECENT_RECAPTURE_WINDOW_GFS);
       }())
        return recapture;

    const bool hasForceAdvantage = [&] {
        const ScopedAIRuntimeProfile forceAdvantageProfile(AIRuntimeProfileSection::AttritionForceAdvantageCheck);
        return HasForceAdvantage(owner_.GetWorld(), owner_.GetInterface().Queries(), owner_.GetPlayerId(),
                                 owner_.GetConfig().combat.forceAdvantageRatio);
    }();
    const bool hasNearTroopsDensity = [&] {
        const ScopedAIRuntimeProfile nearTroopsDensityProfile(AIRuntimeProfileSection::AttritionNearTroopsDensityCheck);
        return HasNearTroopsDensityForBiting(owner_.GetWorld(), owner_.GetPlayerId(),
                                            owner_.GetConfig().combat.minNearTroopsDensity);
    }();
    if(hasForceAdvantage && hasNearTroopsDensity)
    {
        const ScopedAIRuntimeProfile fallbackBitingProfile(AIRuntimeProfileSection::AttritionFallbackBiting);
        return SelectAttackTargetBiting();
    }

    return nullptr;
}

} // namespace AIJH
