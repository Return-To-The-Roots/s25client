// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AIPlayerJH.h"

#include "helpers/containerUtils.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobMilitary.h"
#include "gameData/MilitaryConsts.h"

#include <algorithm>
#include <cstdlib>
#include <list>
#include <random>
#include <vector>
#include <limits>

namespace AIJH {

using ::BASE_ATTACKING_DISTANCE;

namespace {

unsigned GetSoldierCount(const nobBaseMilitary& building)
{
    if(const auto* military = dynamic_cast<const nobMilitary*>(&building))
        return military->GetNumTroops();

    if(const auto* warehouse = dynamic_cast<const nobBaseWarehouse*>(&building))
        return warehouse->GetNumSoldiers();

    return building.DefendersAvailable() ? 1u : 0u;
}

} // namespace

const nobBaseMilitary* AIPlayerJH::SelectAttackTarget(TargetSelectionMode mode) const
{
    switch(mode)
    {
        case TargetSelectionMode::Random: return SelectAttackTargetRandom();
        case TargetSelectionMode::Prudent: return SelectAttackTargetPrudent();
    }
    return nullptr;
}

std::vector<const nobBaseMilitary*>
  AIPlayerJH::GetPotentialTargets(unsigned& hq_or_harbor_without_soldiers) const
{
    hq_or_harbor_without_soldiers = 0;
    std::vector<const nobBaseMilitary*> potentialTargets;

    const std::list<nobMilitary*>& militaryBuildings = aii.GetMilitaryBuildings();
    const unsigned numMilBlds = militaryBuildings.size();
    if(numMilBlds == 0)
        return potentialTargets;

    constexpr unsigned limit = 40;
    for(const nobMilitary* milBld : militaryBuildings)
    {
        if(rand() % numMilBlds > limit)
            continue;

        if(milBld->GetFrontierDistance() == FrontierDistance::Far)
            continue;

        const MapPoint src = milBld->GetPos();
        sortedMilitaryBlds buildings = gwb.LookForMilitaryBuildings(src, 2);
        for(const nobBaseMilitary* target : buildings)
        {
            if(helpers::contains(potentialTargets, target))
                continue;
            if(target->GetGOT() == GO_Type::NobMilitary && static_cast<const nobMilitary*>(target)->IsNewBuilt())
                continue;
            const MapPoint dest = target->GetPos();
            if(gwb.CalcDistance(src, dest) < BASE_ATTACKING_DISTANCE && aii.IsPlayerAttackable(target->GetPlayer())
               && aii.IsVisible(dest))
            {
                if(target->GetGOT() != GO_Type::NobMilitary && !target->DefendersAvailable())
                {
                    ++hq_or_harbor_without_soldiers;
                    potentialTargets.insert(potentialTargets.begin(), target);
                }
                else
                    potentialTargets.push_back(target);
            }
        }
    }

    return potentialTargets;
}

const nobBaseMilitary* AIPlayerJH::SelectAttackTargetRandom() const
{
    unsigned hq_or_harbor_without_soldiers = 0;
    std::vector<const nobBaseMilitary*> potentialTargets = GetPotentialTargets(hq_or_harbor_without_soldiers);
    if(potentialTargets.empty())
        return nullptr;

    if(potentialTargets.size() > hq_or_harbor_without_soldiers)
    {
        auto beginShuffle = potentialTargets.begin() + hq_or_harbor_without_soldiers;
        std::shuffle(beginShuffle, potentialTargets.end(), std::mt19937(std::random_device()()));
    }

    const bool inDefenseMode = (attackMode == CombatMode::DefenseMode);
    for(const nobBaseMilitary* target : potentialTargets)
    {
        const unsigned potentialAttackers = CalcPotentialAttackers(*target);
        if(potentialAttackers == 0)
            continue;

        unsigned attackersStrength = 0;
        sortedMilitaryBlds myBuildings = gwb.LookForMilitaryBuildings(target->GetPos(), 2);
        for(const nobBaseMilitary* otherMilBld : myBuildings)
        {
            if(otherMilBld->GetPlayer() == playerId)
            {
                const auto* myMil = dynamic_cast<const nobMilitary*>(otherMilBld);
                if(!myMil || myMil->IsUnderAttack())
                    continue;

                unsigned newAttackers = 0;
                attackersStrength += myMil->GetSoldiersStrengthForAttack(target->GetPos(), newAttackers);
            }
        }

        if(level == AI::Level::Hard && target->GetGOT() == GO_Type::NobMilitary)
        {
            const auto* enemyTarget = static_cast<const nobMilitary*>(target);
            if(attackersStrength <= enemyTarget->GetSoldiersStrength() + 2 || enemyTarget->GetNumTroops() == 0)
                continue;
        }

        if(inDefenseMode && !CanAttackInDefenseMode(*target, potentialAttackers))
            continue;

        return target;
    }

    return nullptr;
}

const nobBaseMilitary* AIPlayerJH::SelectAttackTargetPrudent() const
{
    unsigned unused_special_targets = 0;
    std::vector<const nobBaseMilitary*> potentialTargets = GetPotentialTargets(unused_special_targets);
    if(potentialTargets.empty())
        return nullptr;

    unsigned minDefenders = std::numeric_limits<unsigned>::max();
    std::vector<const nobBaseMilitary*> defenderFiltered;

    for(const nobBaseMilitary* target : potentialTargets)
    {
        if(target->IsUnderAttack())
            continue;

        const unsigned defenders = GetSoldierCount(*target);
        const unsigned attackers = CalcPotentialAttackers(*target);

        if(attackers < defenders + 2)
            continue;

        if(defenders < minDefenders)
        {
            minDefenders = defenders;
            defenderFiltered.clear();
        }

        if(defenders == minDefenders)
            defenderFiltered.push_back(target);
    }

    if(defenderFiltered.empty())
        return nullptr;
    if(defenderFiltered.size() == 1)
        return defenderFiltered.front();

    unsigned minCounterStrength = std::numeric_limits<unsigned>::max();
    std::vector<const nobBaseMilitary*> counterFiltered;

    for(const nobBaseMilitary* target : defenderFiltered)
    {
        unsigned counterStrength = 0;
        sortedMilitaryBlds enemyBuildings =
          gwb.LookForMilitaryBuildings(target->GetPos(), static_cast<unsigned short>(BASE_ATTACKING_DISTANCE));

        for(const nobBaseMilitary* enemyBld : enemyBuildings)
        {
            if(enemyBld->GetPlayer() != target->GetPlayer())
                continue;

            const unsigned soldiers = GetSoldierCount(*enemyBld);
            if(soldiers > 1)
                counterStrength += soldiers - 1;
        }

        if(counterStrength < minCounterStrength)
        {
            minCounterStrength = counterStrength;
            counterFiltered.clear();
        }

        if(counterStrength == minCounterStrength)
            counterFiltered.push_back(target);
    }

    if(counterFiltered.empty())
        return nullptr;
    if(counterFiltered.size() == 1)
        return counterFiltered.front();

    std::shuffle(counterFiltered.begin(), counterFiltered.end(), std::mt19937(std::random_device()()));
    return counterFiltered.front();
}

} // namespace AIJH
