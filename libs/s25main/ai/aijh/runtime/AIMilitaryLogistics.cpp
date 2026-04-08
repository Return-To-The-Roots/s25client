// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AIPlayerJH.h"

#include "GlobalGameSettings.h"
#include "ai/aijh/combat/AICombatController.h"
#include "ai/aijh/planning/AIConstruction.h"
#include "ai/aijh/runtime/AIMilitaryLogistics.h"
#include "buildings/nobMilitary.h"
#include "gameData/MilitaryConsts.h"

#include <algorithm>
#include <vector>

namespace {

constexpr unsigned kRecentlyLostBuildingLifetime = 600;

} // namespace

namespace AIJH {

void AIMilitaryLogistics::RememberLostMilitaryBuilding(const MapPoint pt)
{
    PruneRecentlyLostBuildings();

    const auto it =
      std::find_if(recentlyLostBuildings_.begin(), recentlyLostBuildings_.end(),
                   [pt](const RecentlyLostBuilding& entry) { return entry.pos == pt; });
    if(it != recentlyLostBuildings_.end())
    {
        it->gf = owner_.currentGF_;
    } else
    {
        recentlyLostBuildings_.push_back({pt, owner_.currentGF_});
    }
}

void AIMilitaryLogistics::ForgetLostMilitaryBuilding(const MapPoint pt)
{
    recentlyLostBuildings_.erase(std::remove_if(recentlyLostBuildings_.begin(), recentlyLostBuildings_.end(),
                                                [pt](const RecentlyLostBuilding& entry) { return entry.pos == pt; }),
                                 recentlyLostBuildings_.end());
}

void AIMilitaryLogistics::PruneRecentlyLostBuildings()
{
    recentlyLostBuildings_.erase(
      std::remove_if(recentlyLostBuildings_.begin(), recentlyLostBuildings_.end(),
                     [this](const RecentlyLostBuilding& entry) {
                         return owner_.currentGF_ > entry.gf
                                && (owner_.currentGF_ - entry.gf) > kRecentlyLostBuildingLifetime;
                     }),
      recentlyLostBuildings_.end());
}

bool AIMilitaryLogistics::IsRecentlyLostMilitaryBuilding(const MapPoint pt) const
{
    return std::any_of(recentlyLostBuildings_.begin(), recentlyLostBuildings_.end(),
                       [pt](const RecentlyLostBuilding& entry) { return entry.pos == pt; });
}

void AIMilitaryLogistics::MilUpgradeOptim()
{
    const int upb = owner_.UpdateUpgradeBuilding();
    int count = 0;
    const std::list<nobMilitary*>& militaryBuildings = owner_.aii.GetMilitaryBuildings();
    for(const nobMilitary* milBld : militaryBuildings)
    {
        if(count != upb)
        {
            if(upb >= 0)
            {
                const FrontierDistance frontierDistance = milBld->GetFrontierDistance();
                if(frontierDistance == FrontierDistance::Far)
                {
                    if(!milBld->IsGoldDisabled())
                        owner_.aii.SetCoinsAllowed(milBld->GetPos(), false);

                    if(((unsigned)count + owner_.GetNumPlannedConnectedInlandMilitaryBlds()) < militaryBuildings.size())
                    {
                        if(milBld->GetNumTroops() > 1)
                        {
                            owner_.aii.SetTroopLimit(milBld->GetPos(), 0, 1);
                            for(unsigned rank = 1; rank < NUM_SOLDIER_RANKS; ++rank)
                                owner_.aii.SetTroopLimit(milBld->GetPos(), rank, 0);

                            for(unsigned rank = 0; rank < NUM_SOLDIER_RANKS; ++rank)
                                owner_.aii.SetTroopLimit(milBld->GetPos(), rank, milBld->GetMaxTroopsCt());
                        }
                    }
                } else
                {
                    if(milBld->IsGoldDisabled())
                        owner_.aii.SetCoinsAllowed(milBld->GetPos(), true);

                    owner_.construction->AddConnectFlagJob(milBld->GetFlag());
                }
            } else
            {
                if(milBld->IsGoldDisabled() && milBld->GetFrontierDistance() != FrontierDistance::Far)
                    owner_.aii.SetCoinsAllowed(milBld->GetPos(), true);
            }
        } else
        {
            if(!owner_.construction->IsConnectedToRoadSystem(milBld->GetFlag()))
            {
                owner_.construction->AddConnectFlagJob(milBld->GetFlag());
                continue;
            }
            if(milBld->IsGoldDisabled())
                owner_.aii.SetCoinsAllowed(milBld->GetPos(), true);
            owner_.aii.SetTroopLimit(milBld->GetPos(), 0, milBld->GetMaxTroopsCt());
            for(unsigned rank = 1; rank < owner_.ggs.GetMaxMilitaryRank(); ++rank)
                owner_.aii.SetTroopLimit(milBld->GetPos(), rank, 1);
            owner_.aii.SetTroopLimit(milBld->GetPos(), owner_.ggs.GetMaxMilitaryRank(), 0);
        }
        count++;
    }
}

bool AIMilitaryLogistics::HasFrontierBuildings()
{
    for(const nobMilitary* milBld : owner_.aii.GetMilitaryBuildings())
    {
        if(milBld->GetFrontierDistance() != FrontierDistance::Far)
            return true;
    }
    return false;
}

void AIMilitaryLogistics::UpdateTroopsLimit()
{
    std::vector<const nobMilitary*> eligibleBuildings;
    eligibleBuildings.reserve(owner_.aii.GetMilitaryBuildings().size());

    unsigned totalSoldiers = owner_.SoldierAvailable();
    unsigned totalCapacity = 0;

    for(const nobMilitary* milBld : owner_.aii.GetMilitaryBuildings())
    {
        if(!milBld || milBld->GetFrontierDistance() == FrontierDistance::Far)
            continue;

        eligibleBuildings.push_back(milBld);
        totalCapacity += milBld->GetMaxTroopsCt();
        totalSoldiers += milBld->GetNumTroops();
    }

    if(eligibleBuildings.empty())
        return;

    const unsigned distributableSoldiers = std::min(totalSoldiers, totalCapacity);
    std::vector<unsigned> newLimits(eligibleBuildings.size(), 1u);

    unsigned assigned = static_cast<unsigned>(newLimits.size());
    if(distributableSoldiers > assigned)
    {
        unsigned remaining = distributableSoldiers - assigned;
        const unsigned startIdx = (owner_.currentGF_ / 1000u + owner_.playerId) % static_cast<unsigned>(eligibleBuildings.size());

        while(remaining > 0)
        {
            bool assignedInThisRound = false;
            for(unsigned offset = 0; offset < eligibleBuildings.size() && remaining > 0; ++offset)
            {
                const unsigned idx = (startIdx + offset) % static_cast<unsigned>(eligibleBuildings.size());
                const unsigned cap = eligibleBuildings[idx]->GetMaxTroopsCt();
                if(newLimits[idx] < cap)
                {
                    ++newLimits[idx];
                    --remaining;
                    assignedInThisRound = true;
                }
            }

            if(!assignedInThisRound)
                break;
        }
    }

    for(unsigned i = 0; i < eligibleBuildings.size(); ++i)
    {
        const nobMilitary* milBld = eligibleBuildings[i];
        const unsigned limit = std::min(newLimits[i], milBld->GetMaxTroopsCt());
        if(milBld->GetTotalTroopLimit() != limit)
            owner_.aii.SetTotalTroopLimit(milBld->GetPos(), limit);
    }
}

double AIMilitaryLogistics::ComputeFulfillmentLevel(double* outTotalWeight) const
{
    return owner_.combatController_->ComputeFulfillmentLevel(outTotalWeight);
}

double AIMilitaryLogistics::ComputeEnemyFrontlineWeight() const
{
    return owner_.combatController_->ComputeEnemyFrontlineWeight();
}

double AIMilitaryLogistics::GetCombatFulfillmentLevel() const
{
    return owner_.combatController_->GetCombatFulfillmentLevel();
}

double AIMilitaryLogistics::GetCombatAttackWeight() const
{
    return owner_.combatController_->GetCombatAttackWeight();
}

bool AIMilitaryLogistics::IsInDefenseMode() const
{
    return owner_.combatController_->IsInDefenseMode();
}

void AIMilitaryLogistics::UpdateCombatMode()
{
    owner_.combatController_->UpdateCombatMode();
}

bool AIMilitaryLogistics::CanAttackInDefenseMode(const nobBaseMilitary& target, const unsigned attackersCount) const
{
    return owner_.combatController_->CanAttackInDefenseMode(target, attackersCount);
}

bool AIMilitaryLogistics::IsLonelyEnemyStronghold(const nobBaseMilitary& target) const
{
    return owner_.combatController_->IsLonelyEnemyStronghold(target);
}

void AIMilitaryLogistics::TryToAttack()
{
    owner_.combatController_->TryToAttack();
}

unsigned AIMilitaryLogistics::CalcPotentialAttackers(const nobBaseMilitary& target) const
{
    return owner_.combatController_->CalcPotentialAttackers(target);
}

double AIMilitaryLogistics::GetCaptureRiskEstimate(const nobBaseMilitary& building) const
{
    return owner_.combatController_->GetCaptureRiskEstimate(building);
}

void AIMilitaryLogistics::EvaluateCaptureRisks()
{
    owner_.combatController_->EvaluateCaptureRisks();
}

double AIMilitaryLogistics::ComputeCaptureRisk(const nobMilitary& building) const
{
    return owner_.combatController_->ComputeCaptureRisk(building);
}

void AIMilitaryLogistics::TrySeaAttack()
{
    owner_.combatController_->TrySeaAttack();
}

void AIPlayerJH::RememberLostMilitaryBuilding(const MapPoint pt)
{
    militaryLogistics_->RememberLostMilitaryBuilding(pt);
}

void AIPlayerJH::ForgetLostMilitaryBuilding(const MapPoint pt)
{
    militaryLogistics_->ForgetLostMilitaryBuilding(pt);
}

void AIPlayerJH::PruneRecentlyLostBuildings()
{
    militaryLogistics_->PruneRecentlyLostBuildings();
}

bool AIPlayerJH::IsRecentlyLostMilitaryBuilding(const MapPoint pt) const
{
    return militaryLogistics_->IsRecentlyLostMilitaryBuilding(pt);
}

void AIPlayerJH::MilUpgradeOptim() { militaryLogistics_->MilUpgradeOptim(); }

bool AIPlayerJH::HasFrontierBuildings() { return militaryLogistics_->HasFrontierBuildings(); }

void AIPlayerJH::UpdateTroopsLimit() { militaryLogistics_->UpdateTroopsLimit(); }

double AIPlayerJH::ComputeFulfillmentLevel(double* outTotalWeight) const
{
    return militaryLogistics_->ComputeFulfillmentLevel(outTotalWeight);
}

double AIPlayerJH::ComputeEnemyFrontlineWeight() const
{
    return militaryLogistics_->ComputeEnemyFrontlineWeight();
}

double AIPlayerJH::GetCombatFulfillmentLevel() const { return militaryLogistics_->GetCombatFulfillmentLevel(); }

double AIPlayerJH::GetCombatAttackWeight() const { return militaryLogistics_->GetCombatAttackWeight(); }

bool AIPlayerJH::IsInDefenseMode() const { return militaryLogistics_->IsInDefenseMode(); }

void AIPlayerJH::UpdateCombatMode() { militaryLogistics_->UpdateCombatMode(); }

bool AIPlayerJH::CanAttackInDefenseMode(const nobBaseMilitary& target, const unsigned attackersCount) const
{
    return militaryLogistics_->CanAttackInDefenseMode(target, attackersCount);
}

bool AIPlayerJH::IsLonelyEnemyStronghold(const nobBaseMilitary& target) const
{
    return militaryLogistics_->IsLonelyEnemyStronghold(target);
}

void AIPlayerJH::TryToAttack() { militaryLogistics_->TryToAttack(); }

const nobBaseMilitary* AIPlayerJH::SelectAttackTarget(TargetSelectionMode mode) const
{
    return combatController_->SelectAttackTarget(mode);
}

const nobBaseMilitary* AIPlayerJH::SelectAttackTargetRandom() const
{
    return combatController_->SelectAttackTargetRandom();
}

const nobBaseMilitary* AIPlayerJH::SelectAttackTargetPrudent() const
{
    return combatController_->SelectAttackTargetPrudent();
}

const nobBaseMilitary* AIPlayerJH::SelectAttackTargetBiting() const
{
    return combatController_->SelectAttackTargetBiting();
}

const nobBaseMilitary* AIPlayerJH::SelectAttackTargetAttrition() const
{
    return combatController_->SelectAttackTargetAttrition();
}

unsigned AIPlayerJH::CalcPotentialAttackers(const nobBaseMilitary& target) const
{
    return militaryLogistics_->CalcPotentialAttackers(target);
}

double AIPlayerJH::GetCaptureRiskEstimate(const nobBaseMilitary& building) const
{
    return militaryLogistics_->GetCaptureRiskEstimate(building);
}

void AIPlayerJH::EvaluateCaptureRisks() { militaryLogistics_->EvaluateCaptureRisks(); }

double AIPlayerJH::ComputeCaptureRisk(const nobMilitary& building) const
{
    return militaryLogistics_->ComputeCaptureRisk(building);
}

void AIPlayerJH::TrySeaAttack() { militaryLogistics_->TrySeaAttack(); }

} // namespace AIJH
