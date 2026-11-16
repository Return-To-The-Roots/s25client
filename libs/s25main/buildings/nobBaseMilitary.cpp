// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
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
#include <boost/pointer_cast.hpp>
#include <limits>

nobBaseMilitary::nobBaseMilitary(const BuildingType type, const MapPoint pos, const unsigned char player,
                                 const Nation nation)
    : noBuilding(type, pos, player, nation), leaving_event(nullptr), go_out(false), defender_(nullptr)
{}

nobBaseMilitary::~nobBaseMilitary() = default;

void nobBaseMilitary::DestroyBuilding()
{
    // Notify soldiers on mission.
    // ATTENTION: iterators can be deleted in HomeDestroyed, -> copy first
    std::vector<nofActiveSoldier*> tmpTroopsOnMission(troops_on_mission.begin(), troops_on_mission.end());
    for(auto* it : tmpTroopsOnMission)
        it->homeDestroyed();
    troops_on_mission.clear();

    // Notify soldiers attacking this building
    // ATTENTION: iterators can be deleted in AttackedGoalDestroyed, -> copy first
    std::vector<nofAttacker*> tmpAggressors(aggressors.begin(), aggressors.end());
    for(auto* tmpAggressor : tmpAggressors)
        tmpAggressor->attackedGoalDestroyed();
    aggressors.clear();

    // Notify soldiers from other buildings defending this
    std::vector<nofAggressiveDefender*> tmpDefenders(aggressive_defenders.begin(), aggressive_defenders.end());
    for(auto* tmpDefender : tmpDefenders)
        tmpDefender->attackedGoalDestroyed();
    aggressive_defenders.clear();

    if(defender_)
    {
        defender_->homeDestroyed();
        defender_ = nullptr;
    }

    getEvMgr().removeEvent(leaving_event);

    // Put all leaving figures outside
    for(auto& fig : leave_house)
    {
        noFigure& figRef = world->addFigure(pos, std::move(fig));

        // DoJobWorks implies ActiveSoldier or TradeDonkey
        // Notify the soldier
        if(figRef.isDoingJobWorks() && dynamic_cast<nofActiveSoldier*>(&figRef))
            static_cast<nofActiveSoldier&>(figRef).homeDestroyedAtBegin();
        else
        {
            figRef.abrogate();
            figRef.startWandering();
            figRef.startWalking(RANDOM_ENUM(Direction));
        }
    }

    leave_house.clear();

    // Let close military buildings recalculate distances to update flags etc.
    sortedMilitaryBlds buildings = world->lookForMilitaryBuildings(pos, 3);
    for(auto* building : buildings)
    {
        if(building->getPlayer() != player && BuildingProperties::isMilitary(building->getBuildingType()))
            static_cast<nobMilitary*>(building)->lookForEnemyBuildings(this);
    }
}

void nobBaseMilitary::Serialize(SerializedGameData& sgd) const
{
    noBuilding::Serialize(sgd);

    sgd.pushObjectContainer(leave_house);
    sgd.pushEvent(leaving_event);
    sgd.PushBool(go_out);
    sgd.pushObjectContainer(troops_on_mission);
    sgd.pushObjectContainer(aggressors, true);
    sgd.pushObjectContainer(aggressive_defenders, true);
    sgd.pushObject(defender_, true);
}

nobBaseMilitary::nobBaseMilitary(SerializedGameData& sgd, const unsigned obj_id) : noBuilding(sgd, obj_id)
{
    sgd.popObjectContainer(leave_house);
    leaving_event = sgd.popEvent();
    go_out = sgd.PopBool();
    if(sgd.getGameDataVersion() < 14)
        sgd.PopUnsignedInt(); // former age
    sgd.popObjectContainer(troops_on_mission);
    sgd.popObjectContainer(aggressors, GO_Type::NofAttacker);
    sgd.popObjectContainer(aggressive_defenders, GO_Type::NofAggressivedefender);
    defender_ = sgd.popObject<nofDefender>(GO_Type::NofDefender);
}

void nobBaseMilitary::addLeavingEvent()
{
    if(!go_out)
    {
        leaving_event = getEvMgr().addEvent(this, 20 + RANDOM_RAND(10));
        go_out = true;
    }
}

void nobBaseMilitary::addLeavingFigure(std::unique_ptr<noFigure> fig)
{
    addLeavingEvent();
    leave_house.push_back(std::move(fig));
}

nofAttacker* nobBaseMilitary::findAggressor(nofAggressiveDefender& defender)
{
    // Look for other attackers on this building that are close and ready to fight
    for(nofAttacker* aggressor : aggressors)
    {
        // The attacker must be ready to fight and must not already have another hunting defender
        if(!aggressor->isReadyForFight() || aggressor->getHuntingDefender())
            continue;

        const MapPoint attackerPos = aggressor->getPos();
        const MapPoint defenderPos = defender.getPos();
        if(attackerPos == defenderPos)
        {
            // Both are at same pos --> Go!
            aggressor->letsFight(defender);
            return aggressor;
        }
        // Check roughly the distance
        if(world->calcDistance(attackerPos, defenderPos) <= 5)
        {
            // Check it further (e.g. if they have to walk around a river...)
            if(world->findHumanPath(attackerPos, defenderPos, 5))
            {
                aggressor->letsFight(defender);
                return aggressor;
            }
        }
    }

    return nullptr;
}

MapPoint nobBaseMilitary::findAnAttackerPlace(unsigned short& ret_radius, const nofAttacker& soldier)
{
    const MapPoint flagPos = getFlagPos();
    const MapPoint soldierPos = soldier.getPos();

    // Only consider  the flag position for fighting if the building isn't currently being captured.
    // Only "real" military buildings can be captured
    const bool capturing =
      BuildingProperties::isMilitary(bldType_) && static_cast<nobMilitary*>(this)->isBeingCaptured();

    if(!capturing && world->isValidPointForFighting(flagPos, soldier, false))
    {
        // Also check if we can reach this.
        // If not, still consider the other points as the flag could become reachable by then.
        // TODO(Replay) Limit distance by MAX_ATTACKING_RUN_DISTANCE
        if(soldierPos == flagPos || world->findHumanPath(soldierPos, flagPos) != std::nullopt)
        {
            ret_radius = 0;
            return flagPos;
        }
    }

    // Check all points around the flag and take shortest
    unsigned min_length = std::numeric_limits<unsigned>::max();
    MapPoint minPt = MapPoint::invalid();
    constexpr auto MAX_RADIUS = 3;
    ret_radius = MAX_RADIUS;
    for(const auto& node : world->getPointsInRadius(flagPos, MAX_RADIUS, ReturnMapPointWithRadius{}))
    {
        // We found a point with a better radius
        if(node.second > ret_radius)
            break;

        if(!world->validWaitingAroundBuildingPoint(node.first, pos))
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
        if(world->findHumanPath(soldierPos, node.first, 100, false, &length))
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

bool nobBaseMilitary::callDefender(nofAttacker& attacker)
{
    // Stop attacking soldiers (including aggressive defenders) from leaving this building
    for(auto it = leave_house.begin(); it != leave_house.end();)
    {
        const auto* sldPtr = dynamic_cast<nofActiveSoldier*>(it->get());
        if(sldPtr && helpers::contains(troops_on_mission, sldPtr))
        {
            auto soldier = boost::static_pointer_cast<nofActiveSoldier>(std::move(*it));
            // At this point there shouldn't be a defender leaving as we are just requesting one
            RTTR_Assert(soldier->getGOT() != GO_Type::NofDefender);
            it = leave_house.erase(it);
            figureLeft(*soldier);

            soldier->informTargetsAboutCancelling();
            addActiveSoldier(std::move(soldier));
        } else
        {
            // If a leaving active soldier is not on a mission he doesn't belong to this building.
            // E.g. soldiers in harbors could be about to go home.
            RTTR_Assert(!sldPtr || sldPtr->getHomeBld() != this);
            ++it;
        }
    }
    // Use existing defender (e.g. just going back in) if possible
    if(defender_)
    {
        defender_->newAttacker(attacker);
        return true;
    }
    auto defender = provideDefender(attacker);
    if(!defender)
        return false; // Building empty -> Can be conquered

    // Soldat muss noch rauskommen
    defender_ = defender.get();
    addLeavingFigure(std::move(defender));
    return true;
}

nofAttacker* nobBaseMilitary::findAttackerNearBuilding()
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
            if(aggressor->isAttackerReady())
            {
                if(!best_attacker || aggressor->getRadius() < best_radius)
                {
                    best_attacker = aggressor;
                    best_radius = aggressor->getRadius();
                }
            }
        }
        // Return the attacker if found and he starts attacking, else try again
        // The current one will be removed from aggressors or not be ready again
        if(best_attacker && best_attacker->attackDefenderAtFlag())
            return best_attacker;
    } while(best_attacker); // Stop if we didn't found any attacker at all
    return nullptr;
}

void nobBaseMilitary::checkArrestedAttackers()
{
    for(nofAttacker* aggressor : aggressors)
    {
        // Ready, i.e. waiting around the flag?
        if(aggressor->isAttackerReady())
        {
            // Can he still reach the flag? Could be blocked by other soldiers
            if(world->findHumanPath(aggressor->getPos(), world->getNeighbour(pos, Direction::SouthEast), 5, false))
            {
                aggressor->attackFlag();
                return;
            }
        }
    }
}

bool nobBaseMilitary::sendSuccessor(const MapPoint pt, const unsigned short radius)
{
    for(nofAttacker* aggressor : aggressors)
    {
        if(aggressor->isAttackerReady())
        {
            // Only move closer
            if(aggressor->getRadius() > radius)
            {
                if(world->findHumanPath(aggressor->getPos(), pt, 50, false))
                {
                    aggressor->startSucceeding(pt, radius);
                    return true;
                }
            }
        }
    }

    return false;
}

bool nobBaseMilitary::isAttackable(unsigned playerIdx) const
{
    // If we are in peaceful mode -> not attackable
    if(world->getGGS().getSelection(AddonId::PEACEFULMODE))
        return false;

    // If we cannot be seen by the player -> not attackable
    if(world->calcVisiblityWithAllies(pos, playerIdx) != Visibility::Visible)
        return false;
    // Else it depends on the team settings
    return world->getPlayer(player).isAttackable(playerIdx);
}

bool nobBaseMilitary::isAggressor(const nofAttacker& attacker) const
{
    return helpers::contains(aggressors, &attacker);
}

bool nobBaseMilitary::isAggressiveDefender(const nofAggressiveDefender& soldier) const
{
    return helpers::contains(aggressive_defenders, &soldier);
}

bool nobBaseMilitary::isOnMission(const nofActiveSoldier& soldier) const
{
    return helpers::contains(troops_on_mission, &soldier);
}
