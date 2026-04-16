// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AICombatController.h"

#include "ai/aijh/config/AIConfig.h"
#include "buildings/nobMilitary.h"
#include "gameTypes/BuildingType.h"
#include "world/GameWorldBase.h"

#include <vector>

namespace AIJH {

const nobBaseMilitary* AICombatController::SelectAttackTargetBiting() const
{
    unsigned unused_special_targets = 0;
    std::vector<const nobBaseMilitary*> potentialTargets = GetPotentialTargets(unused_special_targets);
    if(potentialTargets.empty())
        return nullptr;

    const auto& buildingScores = owner_.GetConfig().combat.buildingScores;

    // const bool inDefenseMode = (attackMode == CombatMode::DefenseMode);
    const nobBaseMilitary* bestTarget = nullptr;
    unsigned bestLossScore = 0;

    for(const nobBaseMilitary* target : potentialTargets)
    {
        const unsigned potentialAttackers = CalcPotentialAttackers(*target);
        if(potentialAttackers == 0)
            continue;

        unsigned attackersStrength = 0;
        sortedMilitaryBlds myBuildings = owner_.GetWorld().LookForMilitaryBuildings(target->GetPos(), 2);
        for(const nobBaseMilitary* otherMilBld : myBuildings)
        {
            if(otherMilBld->GetPlayer() != owner_.GetPlayerId())
                continue;
            const auto* myMil = dynamic_cast<const nobMilitary*>(otherMilBld);
            if(!myMil || myMil->IsUnderAttack())
                continue;

            unsigned newAttackers = 0;
            attackersStrength += myMil->GetSoldiersStrengthForAttack(target->GetPos(), newAttackers);
        }

        if(owner_.GetLevel() == AI::Level::Hard && target->GetGOT() == GO_Type::NobMilitary)
        {
            const auto* enemyTarget = static_cast<const nobMilitary*>(target);
            if(attackersStrength <= enemyTarget->GetSoldiersStrength() + 1)
                continue;
        }

        // if(inDefenseMode && !CanAttackInDefenseMode(*target, potentialAttackers))
        //     continue;

        if(target->GetBuildingType() == BuildingType::Headquarters)
            return target;

        unsigned lossScore = 0;
        if(const auto* enemyTarget = dynamic_cast<const nobMilitary*>(target))
        {
            const std::vector<BuildingType> lostBuildings = enemyTarget->GetBuildingsLostOnCapture();
            for(const BuildingType buildingType : lostBuildings)
                lossScore += buildingScores[buildingType];
        }

        if(!bestTarget || lossScore > bestLossScore)
        {
            bestTarget = target;
            bestLossScore = lossScore;
        }
    }

    return bestTarget;
}

} // namespace AIJH
