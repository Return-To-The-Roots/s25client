// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
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
        sgd.PushObject(attacker, true);
}

nofDefender::nofDefender(SerializedGameData& sgd, const unsigned obj_id) : nofActiveSoldier(sgd, obj_id)
{
    if(state != SoldierState::FigureWork)
        attacker = sgd.PopObject<nofAttacker>(GO_Type::NofAttacker);
    else
        attacker = nullptr;
}

void nofDefender::Walked()
{
    switch(state)
    {
        case SoldierState::DefendingWalkingTo:
            // Start fight with attacker
            world->AddFigure(pos, std::make_unique<noFighting>(*attacker, *this));
            state = SoldierState::Fighting;
            attacker->FightVsDefenderStarted();
            break;
        case SoldierState::DefendingWalkingFrom:
            if(!building)
            {
                // Home destroyed -> Start wandering around
                attacker = nullptr;
                state = SoldierState::FigureWork;
                StartWandering();
                Wander();
            } else
            {
                // Arrived in building
                if(attacker)
                {
                    // An attacker arrived in the meantime so go back out
                    state = SoldierState::DefendingWalkingTo;
                    StartWalking(Direction::SouthEast);
                } else
                {
                    nobBaseMilitary* bld = building;
                    RTTR_Assert(bld->GetDefender() == this); // I should be the defender
                    bld->AddActiveSoldier(world->RemoveFigure(pos, *this));
                    RTTR_Assert(!bld->GetDefender()); // No defender anymore
                }
            }
            break;
        default: break;
    }
}

void nofDefender::HomeDestroyed()
{
    building = nullptr;

    switch(state)
    {
        case SoldierState::DefendingWaiting:
            // Handle now as we are not moving
            attacker = nullptr;
            state = SoldierState::FigureWork;
            StartWandering();
            Wander();
            break;
        case SoldierState::DefendingWalkingTo:
        case SoldierState::DefendingWalkingFrom:
            attacker = nullptr;
            StartWandering();
            state = SoldierState::FigureWork;
            break;
        case SoldierState::Fighting:
            // Just continue fighting if we have already started started
        default: break;
    }
}

void nofDefender::HomeDestroyedAtBegin()
{
    building = nullptr;

    state = SoldierState::FigureWork;

    StartWandering();
    StartWalking(RANDOM_ENUM(Direction));
}

void nofDefender::WonFighting()
{
    // addon BattlefieldPromotion active? -> increase rank!
    if(world->GetGGS().isEnabled(AddonId::BATTLEFIELD_PROMOTION))
        IncreaseRank();
    // Attacker is dead
    attacker = nullptr;

    if(!building)
    {
        // Home destroyed so abort and wander around
        state = SoldierState::FigureWork;
        StartWandering();
        Wander();

        return;
    }

    attacker = building->FindAttackerNearBuilding();
    if(attacker)
    {
        // New attacker found so wait for him
        state = SoldierState::DefendingWaiting;
    } else
    {
        // Otherwise go back into the building
        state = SoldierState::DefendingWalkingFrom;
        StartWalking(Direction::NorthWest);
    }
}

void nofDefender::LostFighting()
{
    attacker = nullptr;

    // Notify building if it still exists
    if(building)
    {
        building->NoDefender();
        // A military building potentially needs to get new soldiers if this wasn't the last one
        if(BuildingProperties::IsMilitary(building->GetBuildingType()))
        {
            RTTR_Assert(dynamic_cast<nobBaseMilitary*>(building));
            if(static_cast<nobMilitary*>(building)->GetNumTroops())
                static_cast<nobMilitary*>(building)->RegulateTroops();
        }
        building = nullptr;
    }
}

void nofDefender::AttackerArrested()
{
    attacker = building->FindAttackerNearBuilding();
    if(!attacker)
    {
        // Go back into the building
        state = SoldierState::DefendingWalkingFrom;
        StartWalking(Direction::NorthWest);
    }
}
