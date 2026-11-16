// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofDefender.h"

#include "GlobalGameSettings.h"
#include "SerializedGameData.h"
#include "addons/const_addons.h"
#include "buildings/nobMilitary.h"
#include "nofAttacker.h"
#include "nofPassiveSoldier.h"
#include "random/Random.h"
#include "world/GameWorld.h"
#include "nodeObjs/noFighting.h"
#include "gameData/BuildingProperties.h"

nofDefender::nofDefender(const MapPoint pos, const unsigned char player, nobBaseMilitary& home,
                         const unsigned char rank, nofAttacker& attacker)
    : nofActiveSoldier(pos, player, home, rank, SoldierState::DefendingWalkingTo), attacker(&attacker)
{}

nofDefender::nofDefender(const nofPassiveSoldier& other, nofAttacker& attacker)
    : nofActiveSoldier(other, SoldierState::DefendingWalkingTo), attacker(&attacker)
{}

void nofDefender::Serialize(SerializedGameData& sgd) const
{
    nofActiveSoldier::Serialize(sgd);

    if(state != SoldierState::FigureWork)
        sgd.pushObject(attacker, true);
}

nofDefender::nofDefender(SerializedGameData& sgd, const unsigned obj_id) : nofActiveSoldier(sgd, obj_id)
{
    if(state != SoldierState::FigureWork)
        attacker = sgd.popObject<nofAttacker>(GO_Type::NofAttacker);
    else
        attacker = nullptr;
}

void nofDefender::walked()
{
    switch(state)
    {
        case SoldierState::DefendingWalkingTo:
            // Start fight with attacker
            world->addFigure(pos, std::make_unique<noFighting>(*attacker, *this));
            state = SoldierState::Fighting;
            attacker->fightVsDefenderStarted();
            break;
        case SoldierState::DefendingWalkingFrom:
            if(!homeBld)
            {
                // Home destroyed -> Start wandering around
                attacker = nullptr;
                state = SoldierState::FigureWork;
                startWandering();
                wander();
            } else
            {
                // Arrived in building
                if(attacker)
                {
                    // An attacker arrived in the meantime so go back out
                    state = SoldierState::DefendingWalkingTo;
                    startWalking(Direction::SouthEast);
                } else
                {
                    nobBaseMilitary* bld = homeBld;
                    RTTR_Assert(bld->getDefender() == this); // I should be the defender
                    bld->addActiveSoldier(world->RemoveFigure(pos, *this));
                    RTTR_Assert(!bld->getDefender()); // No defender anymore
                }
            }
            break;
        default: break;
    }
}

void nofDefender::homeDestroyed()
{
    homeBld = nullptr;

    switch(state)
    {
        case SoldierState::DefendingWaiting:
            // Handle now as we are not moving
            attacker = nullptr;
            state = SoldierState::FigureWork;
            startWandering();
            wander();
            break;
        case SoldierState::DefendingWalkingTo:
        case SoldierState::DefendingWalkingFrom:
            attacker = nullptr;
            startWandering();
            state = SoldierState::FigureWork;
            break;
        case SoldierState::Fighting:
            // Just continue fighting if we have already started started
        default: break;
    }
}

void nofDefender::homeDestroyedAtBegin()
{
    homeBld = nullptr;

    state = SoldierState::FigureWork;

    startWandering();
    startWalking(RANDOM_ENUM(Direction));
}

void nofDefender::wonFighting()
{
    // addon BattlefieldPromotion active? -> increase rank!
    if(world->getGGS().isEnabled(AddonId::BATTLEFIELD_PROMOTION))
        increaseRank();
    // Attacker is dead
    attacker = nullptr;

    if(!homeBld)
    {
        // Home destroyed so abort and wander around
        state = SoldierState::FigureWork;
        startWandering();
        wander();

        return;
    }

    attacker = homeBld->findAttackerNearBuilding();
    if(attacker)
    {
        // New attacker found so wait for him
        state = SoldierState::DefendingWaiting;
    } else
    {
        // Otherwise go back into the building
        state = SoldierState::DefendingWalkingFrom;
        startWalking(Direction::NorthWest);
    }
}

void nofDefender::lostFighting()
{
    attacker = nullptr;

    // Notify building if it still exists
    if(homeBld)
    {
        homeBld->resetDefender();
        // A military building potentially needs to get new soldiers if this wasn't the last one
        if(BuildingProperties::isMilitary(homeBld->getBuildingType()))
        {
            RTTR_Assert(dynamic_cast<nobBaseMilitary*>(homeBld));
            if(static_cast<nobMilitary*>(homeBld)->getNumTroops())
                static_cast<nobMilitary*>(homeBld)->regulateTroops();
        }
        homeBld = nullptr;
    }
}

void nofDefender::attackerArrested()
{
    attacker = homeBld->findAttackerNearBuilding();
    if(!attacker)
    {
        // Go back into the building
        state = SoldierState::DefendingWalkingFrom;
        startWalking(Direction::NorthWest);
    }
}
