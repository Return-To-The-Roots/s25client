// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AIPlayerJH.h"

#include "buildings/nobMilitary.h"
#include "gameTypes/BuildingType.h"

#include <vector>

namespace AIJH {

const nobBaseMilitary* AIPlayerJH::SelectAttackTargetBiting() const
{
    unsigned unused_special_targets = 0;
    std::vector<const nobBaseMilitary*> potentialTargets = GetPotentialTargets(unused_special_targets);
    if(potentialTargets.empty())
        return nullptr;

    // const bool inDefenseMode = (attackMode == CombatMode::DefenseMode);
    const nobBaseMilitary* bestTarget = nullptr;
    unsigned bestLossCount = 0;

    for(const nobBaseMilitary* target : potentialTargets)
    {
        const unsigned potentialAttackers = CalcPotentialAttackers(*target);
        if(potentialAttackers == 0)
            continue;

        unsigned attackersStrength = 0;
        sortedMilitaryBlds myBuildings = gwb.LookForMilitaryBuildings(target->GetPos(), 2);
        for(const nobBaseMilitary* otherMilBld : myBuildings)
        {
            if(otherMilBld->GetPlayer() != playerId)
                continue;
            const auto* myMil = dynamic_cast<const nobMilitary*>(otherMilBld);
            if(!myMil || myMil->IsUnderAttack())
                continue;

            unsigned newAttackers = 0;
            attackersStrength += myMil->GetSoldiersStrengthForAttack(target->GetPos(), newAttackers);
        }

        if(level == AI::Level::Hard && target->GetGOT() == GO_Type::NobMilitary)
        {
            const auto* enemyTarget = static_cast<const nobMilitary*>(target);
            if(attackersStrength <= enemyTarget->GetSoldiersStrength() + 1 || enemyTarget->GetNumTroops() == 0)
                continue;
        }

        // if(inDefenseMode && !CanAttackInDefenseMode(*target, potentialAttackers))
        //     continue;

        if(target->GetBuildingType() == BuildingType::Headquarters)
            return target;

        unsigned lossCount = 0;
        if(const auto* enemyTarget = dynamic_cast<const nobMilitary*>(target))
            lossCount = enemyTarget->EstimateCaptureLossCount();

        if(!bestTarget || lossCount > bestLossCount)
        {
            bestTarget = target;
            bestLossCount = lossCount;
        }
    }

    return bestTarget;
}

} // namespace AIJH

