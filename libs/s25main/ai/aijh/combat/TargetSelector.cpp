// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AICombatController.h"

#include "ai/AIInterface.h"
#include "ai/AIQueryService.h"
#include "helpers/containerUtils.h"
#include "buildings/nobMilitary.h"
#include "gameData/MilitaryConsts.h"
#include "world/GameWorldBase.h"

#include <algorithm>
#include <cstdlib>
#include <list>
#include <vector>

namespace AIJH {

using ::BASE_ATTACKING_DISTANCE;

namespace {

using TargetSelectorFn = const nobBaseMilitary* (AICombatController::*)() const;
using Mode = AICombatController::TargetSelectionMode;

TargetSelectorFn ResolveSelector(Mode mode)
{
    switch(mode)
    {
        case Mode::Random: return &AICombatController::SelectAttackTargetRandom;
        case Mode::Prudent: return &AICombatController::SelectAttackTargetPrudent;
        case Mode::Biting: return &AICombatController::SelectAttackTargetBiting;
        case Mode::Attrition: return &AICombatController::SelectAttackTargetAttrition;
    }
    return nullptr;
}

} // namespace

const nobBaseMilitary* AICombatController::SelectAttackTarget(TargetSelectionMode mode) const
{
    const TargetSelectorFn selector = ResolveSelector(mode);
    return selector ? (this->*selector)() : nullptr;
}

std::vector<const nobBaseMilitary*>
  AICombatController::GetPotentialTargets(unsigned& hq_or_harbor_without_soldiers) const
{
    hq_or_harbor_without_soldiers = 0;
    std::vector<const nobBaseMilitary*> potentialTargets;

    const AIQueryService& queries = owner_.GetInterface().Queries();
    const GameWorldBase& gwb = owner_.GetWorld();
    const std::list<nobMilitary*>& militaryBuildings = queries.GetMilitaryBuildings();
    const unsigned numMilBlds = militaryBuildings.size();
    if(numMilBlds == 0)
        return potentialTargets;

    constexpr unsigned limit = 40;
    std::vector<const nobMilitary*> sampledBuildings(militaryBuildings.begin(), militaryBuildings.end());
    if(sampledBuildings.size() > limit)
    {
        // Randomly sample up to `limit` military buildings so target expansion work stays bounded.
        for(unsigned i = 0; i < limit; ++i)
        {
            const unsigned j = i + rand() % (sampledBuildings.size() - i);
            std::swap(sampledBuildings[i], sampledBuildings[j]);
        }
        sampledBuildings.resize(limit);
    }

    for(const nobMilitary* milBld : sampledBuildings)
    {
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
            if(gwb.CalcDistance(src, dest) < BASE_ATTACKING_DISTANCE && queries.IsPlayerAttackable(target->GetPlayer())
               && queries.IsVisible(dest))
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
} // namespace AIJH
