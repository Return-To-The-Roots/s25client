// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AICombatController.h"

#include "ai/AIInterface.h"
#include "ai/AICommandSink.h"
#include "ai/AIQueryService.h"
#include "ai/aijh/config/AIConfig.h"
#include "GamePlayer.h"
#include "helpers/containerUtils.h"
#include "buildings/nobHarborBuilding.h"
#include "buildings/nobMilitary.h"
#include "nodeObjs/noFlag.h"
#include "nodeObjs/noShip.h"
#include "figures/nofPassiveSoldier.h"
#include "gameData/MilitaryConsts.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdlib>
#include <deque>
#include <random>
#include <vector>
#include "world/GameWorldBase.h"

namespace AIJH {

namespace {

const std::array<unsigned, NUM_SOLDIER_RANKS> kSoldierAttackWeights = {3, 4, 5, 6, 7};

double rollProbability()
{
    return static_cast<double>(rand()) / static_cast<double>(RAND_MAX);
}

} // namespace

AICombatController::AICombatController(AICombatContext& owner) : owner_(owner) {}

double AICombatController::ComputeFulfillmentLevel(double* outTotalWeight) const
{
    double totalWeight = 0.0;
    unsigned frontierCount = 0;

    const AIQueryService& queries = owner_.GetInterface().Queries();
    const std::list<nobMilitary*>& militaryBuildings = queries.GetMilitaryBuildings();
    for(const nobMilitary* milBld : militaryBuildings)
    {
        if(!milBld)
            continue;
        if(milBld->GetFrontierDistance() != FrontierDistance::Near)
            continue;
        if(milBld->IsNewBuilt())
            continue;

        const noFlag* flag = milBld->GetFlag();
        if(!flag)
            continue;

        ++frontierCount;

        const std::vector<nofPassiveSoldier*> attackers = milBld->GetSoldiersForAttack(flag->GetPos());
        if(attackers.empty())
            continue;

        bool skippedHighest = false;
        for(nofPassiveSoldier* soldier : attackers)
        {
            if(!soldier)
                continue;
            if(!skippedHighest)
            {
                skippedHighest = true;
                continue;
            }

            const size_t rankIdx = std::min<size_t>(soldier->GetRank(), kSoldierAttackWeights.size() - 1);
            totalWeight += kSoldierAttackWeights[rankIdx];
        }
    }

    if(outTotalWeight)
        *outTotalWeight = totalWeight;
    if(frontierCount == 0)
        return 0.0;

    return totalWeight / static_cast<double>(frontierCount);
}

double AICombatController::ComputeEnemyFrontlineWeight() const
{
    double totalWeight = 0.0;
    const AIQueryService& queries = owner_.GetInterface().Queries();
    const GameWorldBase& world = owner_.GetWorld();
    const unsigned numPlayers = queries.GetNumPlayers();

    for(unsigned enemyId = 0; enemyId < numPlayers; ++enemyId)
    {
        if(enemyId == owner_.GetPlayerId())
            continue;
        if(!queries.IsPlayerAttackable(enemyId))
            continue;

        const GamePlayer& enemyPlayer = world.GetPlayer(enemyId);
        const std::list<nobMilitary*>& enemyBuildings = enemyPlayer.GetBuildingRegister().GetMilitaryBuildings();
        for(const nobMilitary* building : enemyBuildings)
        {
            if(!building)
                continue;
            if(building->GetFrontierDistance() != FrontierDistance::Near)
                continue;
            if(building->IsNewBuilt())
                continue;

            const noFlag* flag = building->GetFlag();
            if(!flag)
                continue;

            const std::vector<nofPassiveSoldier*> attackers = building->GetSoldiersForAttack(flag->GetPos());
            if(attackers.empty())
                continue;

            bool skippedHighest = false;
            for(nofPassiveSoldier* soldier : attackers)
            {
                if(!soldier)
                    continue;
                if(!skippedHighest)
                {
                    skippedHighest = true;
                    continue;
                }

                const size_t rankIdx = std::min<size_t>(soldier->GetRank(), kSoldierAttackWeights.size() - 1);
                totalWeight += kSoldierAttackWeights[rankIdx];
            }
        }
    }

    return totalWeight;
}

void AICombatController::UpdateCombatMode()
{
    const auto& combatCfg = owner_.GetConfig().combat;
    const double lowLevel = combatCfg.fulfillmentLow;
    const double mediumLevel = combatCfg.fulfillmentMedium;
    const double highLevel = combatCfg.fulfillmentHigh;
    const double halfPi = std::acos(-1.0) * 0.5;

    double totalWeight = 0.0;
    const double fulfillment = ComputeFulfillmentLevel(&totalWeight);
    combatFulfillmentLevel_ = fulfillment;
    combatAttackWeight_ = totalWeight;
    const double enemyWeight = ComputeEnemyFrontlineWeight();

    if(totalWeight > enemyWeight * 3.0)
    {
        attackMode_ = CombatMode::AttackMode;
        return;
    }

    if(fulfillment >= highLevel)
    {
        attackMode_ = CombatMode::AttackMode;
        return;
    }

    if(attackMode_ == CombatMode::DefenseMode)
    {
        if(fulfillment <= lowLevel)
            return;

        if(fulfillment > mediumLevel && fulfillment < highLevel)
        {
            double normalized = (highLevel - fulfillment) / (highLevel - mediumLevel);
            normalized = std::clamp(normalized, 0.0, 1.0);
            const double probability = std::cos(normalized * halfPi);
            if(rollProbability() <= probability)
                attackMode_ = CombatMode::AttackMode;
        }
    }
    else
    {
        if(fulfillment <= lowLevel)
        {
            attackMode_ = CombatMode::DefenseMode;
            return;
        }

        if(fulfillment > lowLevel && fulfillment < mediumLevel)
        {
            double normalized = (fulfillment - lowLevel) / (mediumLevel - lowLevel);
            normalized = std::clamp(normalized, 0.0, 1.0);
            const double probability = std::cos(normalized * halfPi);
            if(rollProbability() <= probability)
                attackMode_ = CombatMode::DefenseMode;
        }
    }
}

bool AICombatController::CanAttackInDefenseMode(const nobBaseMilitary& target, const unsigned attackersCount) const
{
    if(attackersCount == 0)
        return false;

    const double lowLevel = owner_.GetConfig().combat.fulfillmentLow;
    const bool wantsRetake =
      combatFulfillmentLevel_ <= lowLevel && owner_.IsRecentlyLostMilitaryBuilding(target.GetPos());

    if(!wantsRetake && !IsLonelyEnemyStronghold(target))
        return false;

    const auto* enemyMilitary = dynamic_cast<const nobMilitary*>(&target);
    if(enemyMilitary)
    {
        const unsigned defenders = enemyMilitary->GetNumTroops();
        if(attackersCount <= defenders)
            return false;
    }
    else if(target.DefendersAvailable())
        return false;

    return true;
}

bool AICombatController::IsLonelyEnemyStronghold(const nobBaseMilitary& target) const
{
    const sortedMilitaryBlds nearby = owner_.GetWorld().LookForMilitaryBuildings(target.GetPos(), 12);
    unsigned nearbyEnemy = 0;

    for(const nobBaseMilitary* candidate : nearby)
    {
        if(candidate == &target)
            continue;
        if(candidate->GetPlayer() != target.GetPlayer())
            continue;
        if(candidate->GetGOT() != GO_Type::NobMilitary)
            continue;

        ++nearbyEnemy;
        if(nearbyEnemy > 1)
            return false;
    }

    return true;
}

void AICombatController::TryToAttack()
{
    AICommandSink& commands = owner_.GetInterface().Commands();
    const nobBaseMilitary* target = SelectAttackTarget(targetSelectionMode_);
    if(!target)
        return;

    const unsigned attackersCount = CalcPotentialAttackers(*target);
    if(attackersCount == 0)
        return;

    if(commands.Attack(target->GetPos(), attackersCount, true))
        owner_.TrackCombatStart(*target);
}

unsigned AICombatController::CalcPotentialAttackers(const nobBaseMilitary& target) const
{
    unsigned attackersCount = 0;
    const sortedMilitaryBlds myBuildings = owner_.GetWorld().LookForMilitaryBuildings(target.GetPos(), 2);
    for(const nobBaseMilitary* otherMilBld : myBuildings)
    {
        if(otherMilBld->GetPlayer() == owner_.GetPlayerId())
        {
            const auto* myMil = dynamic_cast<const nobMilitary*>(otherMilBld);
            if(!myMil || myMil->IsUnderAttack())
                continue;

            unsigned newAttackers = 0;
            myMil->GetSoldiersStrengthForAttack(target.GetPos(), newAttackers);
            attackersCount += newAttackers;
        }
    }
    return attackersCount;
}

double AICombatController::GetCaptureRiskEstimate(const nobBaseMilitary& building) const
{
    const auto* mil = dynamic_cast<const nobMilitary*>(&building);
    if(!mil)
        return 0.0;
    return mil->GetCaptureRiskEstimate();
}

void AICombatController::EvaluateCaptureRisks()
{
    const AIQueryService& queries = owner_.GetInterface().Queries();
    for(nobMilitary* building : queries.GetMilitaryBuildings())
    {
        if(!building)
            continue;
        if(building->IsUnderAttack())
            continue;

        const double risk = ComputeCaptureRisk(*building);
        building->SetCaptureRiskEstimate(risk);
    }
}

double AICombatController::ComputeCaptureRisk(const nobMilitary& building) const
{
    return owner_.GetWorld().ComputeCaptureRisk(building);
}

void AICombatController::TrySeaAttack()
{
    const AIQueryService& queries = owner_.GetInterface().Queries();
    AICommandSink& commands = owner_.GetInterface().Commands();
    if(queries.GetNumShips() < 1)
        return;
    if(queries.GetHarbors().empty())
        return;
    const GameWorldBase& world = owner_.GetWorld();
    std::vector<unsigned short> seaidswithattackers;
    std::vector<unsigned> attackersatseaid;
    std::vector<int> invalidseas;
    std::deque<const nobBaseMilitary*> potentialTargets;
    std::deque<const nobBaseMilitary*> undefendedTargets;
    std::vector<int> searcharoundharborspots;
    for(const noShip* ship : queries.GetShips())
    {
        if(!helpers::contains(seaidswithattackers, ship->GetSeaID())
           && !helpers::contains(invalidseas, ship->GetSeaID()))
        {
            const unsigned attackercount =
              world.GetNumSoldiersForSeaAttackAtSea(owner_.GetPlayerId(), ship->GetSeaID(), false);
            if(attackercount)
            {
                seaidswithattackers.push_back(ship->GetSeaID());
                attackersatseaid.push_back(attackercount);
            }
            else
                invalidseas.push_back(ship->GetSeaID());
        }
    }
    if(seaidswithattackers.empty())
        return;

    for(unsigned i = 1; i < world.GetNumHarborPoints(); i++)
    {
        const nobHarborBuilding* hb;
        if((hb = world.GetSpecObj<nobHarborBuilding>(world.GetHarborPoint(i))))
        {
            if(queries.IsVisible(hb->GetPos()))
            {
                if(queries.IsPlayerAttackable(hb->GetPlayer()))
                {
                    const std::vector<unsigned short> testseaidswithattackers =
                      world.GetFilteredSeaIDsForAttack(world.GetHarborPoint(i), seaidswithattackers, owner_.GetPlayerId());
                    if(!testseaidswithattackers.empty())
                    {
                        if(!hb->DefendersAvailable())
                            undefendedTargets.push_back(hb);
                        else
                            potentialTargets.push_back(hb);
                    }
                }
                else
                    searcharoundharborspots.push_back(i);
            }
        }
        else
            searcharoundharborspots.push_back(i);
    }
    auto prng = std::mt19937(std::random_device()());
    if(!undefendedTargets.empty())
    {
        std::shuffle(undefendedTargets.begin(), undefendedTargets.end(), prng);
        for(const nobBaseMilitary* targetMilBld : undefendedTargets)
        {
            std::vector<GameWorldBase::PotentialSeaAttacker> attackers =
              world.GetSoldiersForSeaAttack(owner_.GetPlayerId(), targetMilBld->GetPos());
            if(!attackers.empty() && commands.SeaAttack(targetMilBld->GetPos(), 1, true))
            {
                owner_.TrackCombatStart(*targetMilBld);
                return;
            }
        }
    }

    unsigned limit = 15;
    unsigned skip = 0;
    if(searcharoundharborspots.size() > 15)
        skip = std::max<int>(rand() % (searcharoundharborspots.size() / 15 + 1) * 15, 1) - 1;
    for(unsigned i = skip; i < searcharoundharborspots.size() && limit > 0; i++)
    {
        --limit;
        const sortedMilitaryBlds buildings =
          world.LookForMilitaryBuildings(world.GetHarborPoint(searcharoundharborspots[i]), 2);
        for(const nobBaseMilitary* milBld : buildings)
        {
            if(queries.IsPlayerAttackable(milBld->GetPlayer()) && queries.IsVisible(milBld->GetPos()))
            {
                const auto* enemyTarget = dynamic_cast<const nobMilitary*>(milBld);
                if(enemyTarget && enemyTarget->IsNewBuilt())
                    continue;

                if(milBld->GetGOT() != GO_Type::NobMilitary && !milBld->DefendersAvailable())
                {
                    const std::vector<unsigned short> testseaidswithattackers =
                      world.GetFilteredSeaIDsForAttack(milBld->GetPos(), seaidswithattackers, owner_.GetPlayerId());
                    if(!testseaidswithattackers.empty())
                        undefendedTargets.push_back(milBld);
                }
                else
                    potentialTargets.push_back(milBld);
            }
        }
    }

    if(!undefendedTargets.empty())
    {
        std::shuffle(undefendedTargets.begin(), undefendedTargets.end(), prng);
        for(const nobBaseMilitary* targetMilBld : undefendedTargets)
        {
            std::vector<GameWorldBase::PotentialSeaAttacker> attackers =
              world.GetSoldiersForSeaAttack(owner_.GetPlayerId(), targetMilBld->GetPos());
            if(!attackers.empty() && commands.SeaAttack(targetMilBld->GetPos(), 1, true))
            {
                owner_.TrackCombatStart(*targetMilBld);
                return;
            }
        }
    }

    std::shuffle(potentialTargets.begin(), potentialTargets.end(), prng);
    for(const nobBaseMilitary* ship : potentialTargets)
    {
        const std::vector<unsigned short> testseaidswithattackers =
          world.GetFilteredSeaIDsForAttack(ship->GetPos(), seaidswithattackers, owner_.GetPlayerId());
        if(!testseaidswithattackers.empty())
        {
            std::vector<GameWorldBase::PotentialSeaAttacker> attackers =
              world.GetSoldiersForSeaAttack(owner_.GetPlayerId(), ship->GetPos());
            if(!attackers.empty() && commands.SeaAttack(ship->GetPos(), attackers.size(), true))
            {
                owner_.TrackCombatStart(*ship);
                return;
            }
        }
    }
}

} // namespace AIJH
