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
    : noBuilding(type, pos, player, nation), leaving_event(nullptr), go_out(false), defender_(nullptr)
{}

nobBaseMilitary::~nobBaseMilitary() = default;

void nobBaseMilitary::DestroyBuilding()
{
    // Soldaten Bescheid sagen, die evtl auf Mission sind
    // ATTENTION: iterators can be deleted in HomeDestroyed, -> copy first
    std::vector<nofActiveSoldier*> tmpTroopsOnMission(troops_on_mission.begin(), troops_on_mission.end());
    for(auto* it : tmpTroopsOnMission)
        it->HomeDestroyed();
    troops_on_mission.clear();

    // Und die, die das Gebäude evtl gerade angreifen
    // ATTENTION: iterators can be deleted in AttackedGoalDestroyed, -> copy first
    std::vector<nofAttacker*> tmpAggressors(aggressors.begin(), aggressors.end());
    for(auto* tmpAggressor : tmpAggressors)
        tmpAggressor->AttackedGoalDestroyed();
    aggressors.clear();

    // Aggressiv-Verteidigenden Soldaten Bescheid sagen, dass sie nach Hause gehen können
    std::vector<nofAggressiveDefender*> tmpDefenders(aggressive_defenders.begin(), aggressive_defenders.end());
    for(auto* tmpDefender : tmpDefenders)
        tmpDefender->AttackedGoalDestroyed();
    aggressive_defenders.clear();

    // Verteidiger Bescheid sagen
    if(defender_)
    {
        defender_->HomeDestroyed();
        defender_ = nullptr;
    }

    // Warteschlangenevent vernichten
    GetEvMgr().RemoveEvent(leaving_event);

    // Soldaten, die noch in der Warteschlange hängen, rausschicken
    for(auto& fig : leave_house)
    {
        noFigure& figRef = world->AddFigure(pos, std::move(fig));

        if(figRef.DoJobWorks() && dynamic_cast<nofActiveSoldier*>(&figRef))
            // Wenn er Job-Arbeiten verrichtet, ists ein ActiveSoldier oder TradeDonkey --> dem Soldat muss extra noch
            // Bescheid gesagt werden!
            static_cast<nofActiveSoldier&>(figRef).HomeDestroyedAtBegin();
        else
        {
            figRef.Abrogate();
            figRef.StartWandering();
            figRef.StartWalking(RANDOM_ENUM(Direction));
        }
    }

    leave_house.clear();

    // Umgebung nach feindlichen Militärgebäuden absuchen und die ihre Grenzflaggen neu berechnen lassen
    // da, wir ja nicht mehr existieren
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
}

void nobBaseMilitary::AddLeavingEvent()
{
    // Wenn gerade keiner rausgeht, muss neues Event angemeldet werden
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
    const MapPoint flagPos = GetFlagPos();
    const MapPoint soldierPos = soldier.GetPos();

    // Only consider  the flag position for fighting if the building isn't currently being captured.
    // Only "real" military buildings can be captured
    const bool capturing =
      BuildingProperties::IsMilitary(bldType_) && static_cast<nobMilitary*>(this)->IsBeingCaptured();

    if(!capturing && world->IsValidPointForFighting(flagPos, soldier, false))
    {
        // Also check if we can reach this.
        // If not, still consider the other points as the flag could become reachable by then.
        // TODO(Replay) Limit distance by MAX_ATTACKING_RUN_DISTANCE
        if(soldierPos == flagPos || world->FindHumanPath(soldierPos, flagPos) != boost::none)
        {
            ret_radius = 0;
            return flagPos;
        }
    }

    // Check all points around the flag and take shortest
    unsigned min_length = std::numeric_limits<unsigned>::max();
    MapPoint minPt = MapPoint::Invalid();
    ret_radius = 100;
    for(const auto& node : world->GetPointsInRadius(flagPos, 3, ReturnMapPointWithRadius{}))
    {
        // We found a point with a better radius
        if(node.second > ret_radius)
            break;

        if(!world->ValidWaitingAroundBuildingPoint(node.first, pos))
            continue;

        // If this is where the soldier currently is, it is already the shortest possible distance
        if(soldierPos == node.first)
        {
            ret_radius = node.second;
            return node.first;
        }

        unsigned length = 0;
        // Is there a path at all?
        // TODO(Replay) Limit distance by MAX_ATTACKING_RUN_DISTANCE instead of 100
        if(world->FindHumanPath(soldierPos, node.first, 100, false, &length))
        {
            // Take if shorter
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
    // Ist noch ein Verteidiger draußen (der z.B. grad wieder reingeht?
    if(defender_)
    {
        // Dann nehmen wir den, müssen ihm nur den neuen Angreifer mitteilen
        defender_->NewAttacker(attacker);
        // Leute, die aus diesem Gebäude zum Angriff/aggressiver Verteidigung rauskommen wollen,
        // blocken
        CancelJobs();

        return true;
    }
    // ansonsten einen neuen aus dem Gebäude holen
    else
    {
        auto defender = ProvideDefender(attacker);
        if(!defender)
            return false; // Building empty -> Can be conquered

        // Leute, die aus diesem Gebäude zum Angriff/aggressiver Verteidigung rauskommen wollen,
        // blocken
        CancelJobs();
        // Soldat muss noch rauskommen
        defender_ = defender.get();
        AddLeavingFigure(std::move(defender));

        return true;
    }
}

nofAttacker* nobBaseMilitary::FindAttackerNearBuilding()
{
    nofAttacker* best_attacker = nullptr;
    do
    {
        // Call the closest attacker to fight the defender
        best_attacker = nullptr;
        unsigned best_radius = 0xFFFFFFFF;
        for(auto* aggressor : aggressors)
        {
            // Only consider those waiting around the flag, otherwise the defender could go/stay inside
            if(aggressor->IsAttackerReady())
            {
                if(!best_attacker || aggressor->GetRadius() < best_radius)
                {
                    best_attacker = aggressor;
                    best_radius = aggressor->GetRadius();
                }
            }
        }
        // Return the attacker if found and he starts attacking, else try again
        // The current one will be removed from aggressors or not be ready again
        if(best_attacker && best_attacker->AttackDefenderAtFlag())
            return best_attacker;
    } while(best_attacker); // Stop if we didn't found any attacker at all
    return nullptr;
}

void nobBaseMilitary::CheckArrestedAttackers()
{
    for(nofAttacker* aggressor : aggressors)
    {
        // Ist der Soldat überhaupt bereit zum Kämpfen (also wartet er um die Flagge herum)?
        if(aggressor->IsAttackerReady())
        {
            // Und kommt er überhaupt zur Flagge (könnte ja in der 2. Reihe stehen, sodass die
            // vor ihm ihn den Weg versperren)?
            if(world->FindHumanPath(aggressor->GetPos(), world->GetNeighbour(pos, Direction::SouthEast), 5, false))
            {
                // dann kann der zur Flagge gehen
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
        // Wartet der Soldat überhaupt um die Flagge?
        if(aggressor->IsAttackerReady())
        {
            // Und steht er auch weiter außen?, sonst machts natürlich keinen Sinn..
            if(aggressor->GetRadius() > radius)
            {
                // Und findet er einen zu diesem Punkt?
                if(world->FindHumanPath(aggressor->GetPos(), pt, 50, false))
                {
                    // dann soll er dorthin gehen
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

/// Bricht einen aktuell von diesem Haus gestarteten Angriff/aggressive Verteidigung ab, d.h. setzt die Soldaten
/// aus der Warteschleife wieder in das Haus --> wenn Angreifer an der Fahne ist und Verteidiger rauskommen soll
void nobBaseMilitary::CancelJobs()
{
    // Soldaten, die noch in der Warteschlange hängen, rausschicken
    for(auto it = leave_house.begin(); it != leave_house.end();)
    {
        // Nur Soldaten nehmen (Job-Arbeiten) und keine (normalen) Verteidiger, da diese ja rauskommen
        // sollen zum Kampf
        if((*it)->DoJobWorks() && (*it)->GetGOT() != GO_Type::NofDefender)
        {
            auto soldier = boost::dynamic_pointer_cast<nofActiveSoldier>(std::move(*it));
            RTTR_Assert(soldier);

            // Wenn er Job-Arbeiten verrichtet, ists ein ActiveSoldier --> dem muss extra noch Bescheid gesagt werden!
            soldier->InformTargetsAboutCancelling();
            // Wieder in das Haus verfrachten
            this->AddActiveSoldier(std::move(soldier));
            it = leave_house.erase(it);
        } else
            ++it;
    }

    // leave_house.clear();
}
