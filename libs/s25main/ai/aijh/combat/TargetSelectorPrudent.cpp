// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AICombatController.h"
#include "ai/aijh/runtime/AIPlayerJH.h"

#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobMilitary.h"
#include "gameData/MilitaryConsts.h"

#include <algorithm>
#include <limits>
#include <random>
#include <vector>

namespace AIJH {

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

const nobBaseMilitary* AICombatController::SelectAttackTargetPrudent() const
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
          owner_.gwb.LookForMilitaryBuildings(target->GetPos(), static_cast<unsigned short>(BASE_ATTACKING_DISTANCE));

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

const nobBaseMilitary* AIPlayerJH::SelectAttackTargetPrudent() const
{
    return combatController_->SelectAttackTargetPrudent();
}

} // namespace AIJH
