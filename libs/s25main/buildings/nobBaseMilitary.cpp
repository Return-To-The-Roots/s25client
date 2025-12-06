// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nobBaseMilitary.h"
#include "EventManager.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "ReturnMapPointWithRadius.h"
#include "SerializedGameData.h"
#include "addons/const_addons.h"
#include "figures/nofAggressiveDefender.h"
#include "figures/nofAttacker.h"
#include "figures/nofDefender.h"
#include "helpers/containerUtils.h"
#include "nobMilitary.h"
#include "random/Random.h"
#include "world/GameWorld.h"
#include "gameData/BuildingProperties.h"
#include "gameData/GameConsts.h"
#include <boost/pointer_cast.hpp>
#include <limits>

nobBaseMilitary::nobBaseMilitary(const BuildingType type, const MapPoint pos, const unsigned char player,
                                 const Nation nation)
    : noBuilding(type, pos, player, nation), leaving_event(nullptr), go_out(false), defender_(nullptr),
      captured_gf_(GetEvMgr().GetCurrentGF()), origin_owner_(player)
{}

nobBaseMilitary::~nobBaseMilitary() = default;

void nobBaseMilitary::DestroyBuilding()
{
    // Notify soldiers that might still be on a mission
    // ATTENTION: iterators can be deleted in HomeDestroyed, -> copy first
    std::vector<nofActiveSoldier*> tmpTroopsOnMission(troops_on_mission.begin(), troops_on_mission.end());
    for(auto* it : tmpTroopsOnMission)
        it->HomeDestroyed();
    troops_on_mission.clear();

    // Inform attackers currently targeting this building
    // ATTENTION: iterators can be deleted in AttackedGoalDestroyed, -> copy first
    std::vector<nofAttacker*> tmpAggressors(aggressors.begin(), aggressors.end());
    for(auto* tmpAggressor : tmpAggressors)
        tmpAggressor->AttackedGoalDestroyed();
    aggressors.clear();

    // Tell aggressive defenders that they can return home
    std::vector<nofAggressiveDefender*> tmpDefenders(aggressive_defenders.begin(), aggressive_defenders.end());
    for(auto* tmpDefender : tmpDefenders)
        tmpDefender->AttackedGoalDestroyed();
    aggressive_defenders.clear();

    // Inform the stationed defender
    if(defender_)
    {
        defender_->HomeDestroyed();
        defender_ = nullptr;
    }

    // Cancel the queued leaving event
    GetEvMgr().RemoveEvent(leaving_event);

    // Send out soldiers that were still queued to leave
    for(auto& fig : leave_house)
    {
        noFigure& figRef = world->AddFigure(pos, std::move(fig));

        if(figRef.DoJobWorks() && dynamic_cast<nofActiveSoldier*>(&figRef))
            // Active soldiers (or trade donkeys) performing job work need an explicit notification
            static_cast<nofActiveSoldier&>(figRef).HomeDestroyedAtBegin();
        else
        {
            figRef.Abrogate();
            figRef.StartWandering();
            figRef.StartWalking(RANDOM_ENUM(Direction));
        }
    }

    leave_house.clear();

    // Search the surroundings for hostile military buildings so they can recalculate their border flags
    // now that this building no longer exists
    sortedMilitaryBlds buildings = world->LookForMilitaryBuildings(pos, 3);
    for(auto* building : buildings)
    {
        if(building->GetPlayer() != player && BuildingProperties::IsMilitary(building->GetBuildingType()))
            static_cast<nobMilitary*>(building)->LookForEnemyBuildings(this);
    }
}

void nobBaseMilitary::Serialize(SerializedGameData& sgd) const
{
    noBuilding::Serialize(sgd);

    sgd.PushObjectContainer(leave_house);
    sgd.PushEvent(leaving_event);
    sgd.PushBool(go_out);
    sgd.PushUnsignedInt(0); // former age, compatibility with 0.7, remove it in furher versions
    sgd.PushObjectContainer(troops_on_mission);
    sgd.PushObjectContainer(aggressors, true);
    sgd.PushObjectContainer(aggressive_defenders, true);
    sgd.PushObject(defender_, true);
    sgd.PushUnsignedChar(origin_owner_);
    sgd.PushUnsignedInt(captured_gf_);
}

nobBaseMilitary::nobBaseMilitary(SerializedGameData& sgd, const unsigned obj_id) : noBuilding(sgd, obj_id)
{
    sgd.PopObjectContainer(leave_house);
    leaving_event = sgd.PopEvent();
    go_out = sgd.PopBool();
    sgd.PopUnsignedInt(); // former age, compatibility with 0.7, remove it in furher versions
    sgd.PopObjectContainer(troops_on_mission);
    sgd.PopObjectContainer(aggressors, GO_Type::NofAttacker);
    sgd.PopObjectContainer(aggressive_defenders, GO_Type::NofAggressivedefender);
    defender_ = sgd.PopObject<nofDefender>(GO_Type::NofDefender);
    if(sgd.GetGameDataVersion() >= 14)
    {
        origin_owner_ = sgd.PopUnsignedChar();
        captured_gf_ = sgd.PopUnsignedInt();
    } else
    {
        captured_gf_ = (sgd.GetGameDataVersion() >= 13) ? sgd.PopUnsignedInt() : 0;
        origin_owner_ = player;
    }
}

void nobBaseMilitary::AddLeavingEvent()
{
    // Schedule a new event if nobody is currently leaving
    if(!go_out)
    {
        leaving_event = GetEvMgr().AddEvent(this, 20 + RANDOM_RAND(10));
        go_out = true;
    }
}

void nobBaseMilitary::AddLeavingFigure(std::unique_ptr<noFigure> fig)
{
    AddLeavingEvent();
    leave_house.push_back(std::move(fig));
}

nofAttacker* nobBaseMilitary::FindAggressor(nofAggressiveDefender& defender)
{
    // Look for other attackers on this building that are close and ready to fight
    for(nofAttacker* aggressor : aggressors)
    {
        // The attacker must be ready to fight and must not already have another hunting defender
        if(!aggressor->IsReadyForFight() || aggressor->GetHuntingDefender())
            continue;

        const MapPoint attackerPos = aggressor->GetPos();
        const MapPoint defenderPos = defender.GetPos();
        if(attackerPos == defenderPos)
        {
            // Both are at same pos --> Go!
            aggressor->LetsFight(defender);
            return aggressor;
        }
        // Check roughly the distance
        if(world->CalcDistance(attackerPos, defenderPos) <= 5)
        {
            // Check it further (e.g. if they have to walk around a river...)
            if(world->FindHumanPath(attackerPos, defenderPos, 5))
            {
                aggressor->LetsFight(defender);
                return aggressor;
            }
        }
    }

    return nullptr;
}

MapPoint nobBaseMilitary::FindAnAttackerPlace(unsigned short& ret_radius, const nofAttacker& soldier)
{
    const MapPoint flagPos = world->GetNeighbour(pos, Direction::SouthEast);

    // Use the flag position only if the building is not currently being captured; otherwise soldiers may desert
    // Only real military buildings can be captured
    bool capturing =
      (BuildingProperties::IsMilitary(bldType_)) ? (static_cast<nobMilitary*>(this)->IsBeingCaptured()) : false;

    if(!capturing && world->IsValidPointForFighting(flagPos, soldier, false))
    {
        ret_radius = 0;
        return flagPos;
    }

    const MapPoint soldierPos = soldier.GetPos();
    // Get points AROUND the flag. Never AT the flag
    const auto nodes = world->GetPointsInRadius(flagPos, 3, ReturnMapPointWithRadius{});

    // Evaluate all candidate points and choose the one with the shortest path
    // Keep track of the shortest path length found so far
    unsigned min_length = std::numeric_limits<unsigned>::max();
    MapPoint minPt = MapPoint::Invalid();
    ret_radius = 100;
    for(const auto& node : nodes)
    {
        // We found a point with a better radius
        if(node.second > ret_radius)
            break;

        if(!world->ValidWaitingAroundBuildingPoint(node.first, pos))
            continue;

        // Same point as the soldier's current position? Then we are done; we cannot find a shorter path
        if(soldierPos == node.first)
        {
            ret_radius = node.second;
            return node.first;
        }

        unsigned length = 0;
        // Path to candidate point found
        if(world->FindHumanPath(soldierPos, node.first, 100, false, &length))
        {
            // Shorter than the best path so far? Temporarily remember this point
            if(length < min_length)
            {
                minPt = node.first;
                ret_radius = node.second;
                min_length = length;
            }
        }
    }
    return minPt;
}

bool nobBaseMilitary::CallDefender(nofAttacker& attacker)
{
    // Do we already have a defender outside (for example, returning home)?
    if(defender_)
    {
        // Reuse that defender and assign the new attacker
        defender_->NewAttacker(attacker);
        // Block any soldiers scheduled to leave for attacks or aggressive defense
        CancelJobs();

        return true;
    }
    // Otherwise request a new defender from inside the building
    else
    {
        auto defender = ProvideDefender(attacker);
        if(!defender)
            return false; // Building empty -> Can be conquered

        // Block any soldiers scheduled to leave for attacks or aggressive defense
        CancelJobs();
        // Queue the defender to exit the building
        defender_ = defender.get();
        AddLeavingFigure(std::move(defender));

        return true;
    }
}

nofAttacker* nobBaseMilitary::FindAttackerNearBuilding()
{
    // Iterate over all attacking soldiers and pick the one closest to the building
    nofAttacker* best_attacker = nullptr;
    unsigned best_radius = 0xFFFFFFFF;

    for(auto* aggressor : aggressors)
    {
        // Is the soldier actually ready to fight (waiting around the flag or already advancing)?
        if(aggressor->IsAttackerReady())
        {
            // Better than the previous best candidate?
            if(aggressor->GetRadius() < best_radius || !best_attacker)
            {
                best_attacker = aggressor;
                best_radius = best_attacker->GetRadius();
            }
        }
    }

    if(best_attacker)
        best_attacker->AttackDefenderAtFlag();

    // Return the chosen attacker (or null if none qualified)
    return best_attacker;
}

void nobBaseMilitary::CheckArrestedAttackers()
{
    for(nofAttacker* aggressor : aggressors)
    {
        // Is the soldier actually ready to fight, i.e. waiting around the flag?
        if(aggressor->IsAttackerReady())
        {
            // And can he reach the flag at all? (He might be blocked by others in the second row.)
            if(world->FindHumanPath(aggressor->GetPos(), world->GetNeighbour(pos, Direction::SouthEast), 5, false))
            {
                // Then send him to the flag
                aggressor->AttackFlag();
                return;
            }
        }
    }
}

bool nobBaseMilitary::SendSuccessor(const MapPoint pt, const unsigned short radius)
{
    for(nofAttacker* aggressor : aggressors)
    {
        // Is the soldier currently waiting around the flag?
        if(aggressor->IsAttackerReady())
        {
            // Is he positioned further out? Otherwise sending him makes no sense
            if(aggressor->GetRadius() > radius)
            {
                // Can he reach the desired point?
                if(world->FindHumanPath(aggressor->GetPos(), pt, 50, false))
                {
                    // Then send him there
                    aggressor->StartSucceeding(pt, radius);
                    return true;
                }
            }
        }
    }

    return false;
}

bool nobBaseMilitary::IsAttackable(unsigned playerIdx) const
{
    // If we are in peaceful mode -> not attackable
    if(world->GetGGS().getSelection(AddonId::PEACEFULMODE))
        return false;

    // If we cannot be seen by the player -> not attackable
    if(world->CalcVisiblityWithAllies(pos, playerIdx) != Visibility::Visible)
        return false;
    // Else it depends on the team settings
    return world->GetPlayer(player).IsAttackable(playerIdx);
}

bool nobBaseMilitary::IsAggressor(const nofAttacker& attacker) const
{
    return helpers::contains(aggressors, &attacker);
}

bool nobBaseMilitary::IsAggressiveDefender(const nofAggressiveDefender& soldier) const
{
    return helpers::contains(aggressive_defenders, &soldier);
}

bool nobBaseMilitary::IsOnMission(const nofActiveSoldier& soldier) const
{
    return helpers::contains(troops_on_mission, &soldier);
}

/// Cancels an attack or aggressive defense started by this building by moving queued soldiers back inside.
/// Used when an attacker is already at the flag and defenders should not leave.
void nobBaseMilitary::CancelJobs()
{
    // Reconsider soldiers that are still queued to leave
    for(auto it = leave_house.begin(); it != leave_house.end();)
    {
        // Only touch soldiers performing job work; regular defenders should still leave to fight
        if((*it)->DoJobWorks() && (*it)->GetGOT() != GO_Type::NofDefender)
        {
            auto soldier = boost::dynamic_pointer_cast<nofActiveSoldier>(std::move(*it));
            RTTR_Assert(soldier);

            // Active soldiers performing job work must inform their targets that the mission is cancelled
            soldier->InformTargetsAboutCancelling();
            // Move the soldier back into the building
            this->AddActiveSoldier(std::move(soldier));
            it = leave_house.erase(it);
        } else
            ++it;
    }

    // leave_house.clear();
}
