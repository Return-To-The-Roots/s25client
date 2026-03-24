// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AICombatController.h"
#include "ai/aijh/runtime/AIPlayerJH.h"

#include "buildings/nobMilitary.h"

#include <algorithm>
#include <random>
#include <vector>

namespace AIJH {

const nobBaseMilitary* AICombatController::SelectAttackTargetRandom() const
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

    const bool inDefenseMode = IsInDefenseMode();
    for(const nobBaseMilitary* target : potentialTargets)
    {
        const unsigned potentialAttackers = CalcPotentialAttackers(*target);
        if(potentialAttackers == 0)
            continue;

        unsigned attackersStrength = 0;
        sortedMilitaryBlds myBuildings = owner_.gwb.LookForMilitaryBuildings(target->GetPos(), 2);
        for(const nobBaseMilitary* otherMilBld : myBuildings)
        {
            if(otherMilBld->GetPlayer() == owner_.playerId)
            {
                const auto* myMil = dynamic_cast<const nobMilitary*>(otherMilBld);
                if(!myMil || myMil->IsUnderAttack())
                    continue;

                unsigned newAttackers = 0;
                attackersStrength += myMil->GetSoldiersStrengthForAttack(target->GetPos(), newAttackers);
            }
        }

        if(owner_.GetLevel() == AI::Level::Hard && target->GetGOT() == GO_Type::NobMilitary)
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

const nobBaseMilitary* AIPlayerJH::SelectAttackTargetRandom() const
{
    return combatController_->SelectAttackTargetRandom();
}

} // namespace AIJH
