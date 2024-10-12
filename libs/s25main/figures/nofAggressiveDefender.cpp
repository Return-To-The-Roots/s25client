// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofAggressiveDefender.h"
#include "GlobalGameSettings.h"
#include "SerializedGameData.h"
#include "addons/const_addons.h"
#include "nofAttacker.h"
#include "nofPassiveSoldier.h"
#include "random/Random.h"
#include "world/GameWorld.h"

nofAggressiveDefender::nofAggressiveDefender(const MapPoint pos, const unsigned char player, nobBaseMilitary& home,
                                             const unsigned char rank, nofAttacker& attacker)
    : nofActiveSoldier(pos, player, home, rank, SoldierState::AggressivedefendingWalkingToAggressor),
      attacker(&attacker), attacked_goal(attacker.GetAttackedGoal())
{
    // Notify building to protect
    attacked_goal->LinkAggressiveDefender(*this);
}

nofAggressiveDefender::nofAggressiveDefender(const nofPassiveSoldier& other, nofAttacker& attacker)
    : nofActiveSoldier(other, SoldierState::AggressivedefendingWalkingToAggressor), attacker(&attacker),
      attacked_goal(attacker.GetAttackedGoal())
{
    // Notify building to protect
    attacked_goal->LinkAggressiveDefender(*this);
}

nofAggressiveDefender::~nofAggressiveDefender() = default;

void nofAggressiveDefender::Destroy()
{
    RTTR_Assert(!attacker);
    RTTR_Assert(!attacked_goal);
    nofActiveSoldier::Destroy();
}

void nofAggressiveDefender::Serialize(SerializedGameData& sgd) const
{
    nofActiveSoldier::Serialize(sgd);

    if(state != SoldierState::WalkingHome && state != SoldierState::FigureWork)
    {
        sgd.PushObject(attacker, true);
        sgd.PushObject(attacked_goal);
    }
}

nofAggressiveDefender::nofAggressiveDefender(SerializedGameData& sgd, const unsigned obj_id)
    : nofActiveSoldier(sgd, obj_id)
{
    if(state != SoldierState::WalkingHome && state != SoldierState::FigureWork)
    {
        attacker = sgd.PopObject<nofAttacker>(GO_Type::NofAttacker);
        attacked_goal = sgd.PopObject<nobBaseMilitary>();
    } else
    {
        attacker = nullptr;
        attacked_goal = nullptr;
    }
}

void nofAggressiveDefender::Walked()
{
    if(state == SoldierState::AggressivedefendingWalkingToAggressor)
        MissAggressiveDefendingWalk();
    else
        nofActiveSoldier::Walked();
}

void nofAggressiveDefender::HomeDestroyed()
{
    building = nullptr;
}

void nofAggressiveDefender::HomeDestroyedAtBegin()
{
    building = nullptr;

    // angegriffenem Gebäude Bescheid sagen, dass wir doch nicht mehr kommen
    InformTargetsAboutCancelling();

    state = SoldierState::FigureWork;

    // Rumirren
    StartWandering();
    StartWalking(RANDOM_ENUM(Direction));
}

void nofAggressiveDefender::CancelAtAttackedBld()
{
    if(attacked_goal)
    {
        attacked_goal->UnlinkAggressiveDefender(*this);
        attacked_goal = nullptr;
    }
}

void nofAggressiveDefender::WonFighting()
{
    // addon BattlefieldPromotion active? -> increase rank!
    if(world->GetGGS().isEnabled(AddonId::BATTLEFIELD_PROMOTION))
        IncreaseRank();

    // Ist evtl. unser Heimatgebäude zerstört?
    if(!building)
    {
        // Ziel Bescheid sagen
        InformTargetsAboutCancelling();

        // Rumirren
        state = SoldierState::FigureWork;
        StartWandering();
        Wander();

        return;
    }

    // Continue walking to our attacker
    MissAggressiveDefendingContinueWalking();
}

void nofAggressiveDefender::LostFighting()
{
    // Meinem zu Hause Bescheid sagen, dass ich nicht mehr lebe (damit neue Truppen reinkönnen),
    // falls es noch existiert
    AbrogateWorkplace();

    // Ziel Bescheid sagen, das ich verteidigt hatte
    InformTargetsAboutCancelling();
}

void nofAggressiveDefender::MissionAggressiveDefendingLookForNewAggressor()
{
    RTTR_Assert(!attacker);
    // Wenns das Zielgebäude nich mehr gibt, gleich nach Hause gehen!
    if(!attacked_goal)
    {
        ReturnHomeMissionAggressiveDefending();
        return;
    }

    // Vermeiden, dass in FindAggressor nochmal der Soldat zum Loslaufen gezwungen wird, weil er als state
    // noch drinstehen hat, dass er auf einen Kampf wartet
    state = SoldierState::AggressivedefendingWalkingToAggressor;

    // nach anderen suchen, die in meiner Nähe sind und mich evtl noch mit denen kloppen
    attacker = attacked_goal->FindAggressor(*this);
    if(attacker)
    {
        // zum Angreifer gehen und mit ihm kämpfen
        MissAggressiveDefendingWalk();
    } else
    {
        // keiner will mehr mit mir kämpfen, dann geh ich halt wieder nach Hause
        ReturnHomeMissionAggressiveDefending();
    }
}

void nofAggressiveDefender::AttackedGoalDestroyed()
{
    CancelAtAttacker();
    attacked_goal = nullptr;

    /*// Stehen wir? Dann losgehen
    if(state == Waitingforfight)
        ReturnHome();*/
}

void nofAggressiveDefender::MissAggressiveDefendingContinueWalking()
{
    state = SoldierState::AggressivedefendingWalkingToAggressor;
    MissAggressiveDefendingWalk();
}

void nofAggressiveDefender::MissAggressiveDefendingWalk()
{
    if(!building)
    {
        // Home destroyed so abort and wander around
        InformTargetsAboutCancelling();
        state = SoldierState::FigureWork;
        StartWandering();
        Wander();
        return;
    }

    if(!attacked_goal)
    {
        // Similar when the defended building is gone
        ReturnHomeMissionAggressiveDefending();
        return;
    }

    if(!attacker)
    {
        // Search for a new attacker if lost
        MissionAggressiveDefendingLookForNewAggressor();
        return;
    }

    // Does he still want to fight?
    if(!attacker->IsReadyForFight())
    {
        // Look for a new one
        CancelAtAttacker();
        MissionAggressiveDefendingLookForNewAggressor();
        return;
    }

    // Look for enemies
    if(TryFightingNearbyEnemy())
    {
        // Enemy found -> abort, because nofActiveSoldier handles all things now
        // Note it is ok, if the enemy is our attacker.
        // If we win, we will either see, that the attacker is busy or be notified because he did
        // If we loose, we will tell him later
        return;
    }

    RTTR_Assert(pos != attacker->GetPos()); // If so, why was it not found?

    // Calc next walking direction
    const auto dir = world->FindHumanPath(pos, attacker->GetPos(), 100, true);

    if(dir)
    {
        // Continue walking towards him
        StartWalking(*dir);
    } else
    {
        // No route found
        // Look for new attacker
        CancelAtAttacker();
        MissionAggressiveDefendingLookForNewAggressor();
    }
}

void nofAggressiveDefender::ReturnHomeMissionAggressiveDefending()
{
    InformTargetsAboutCancelling();
    ReturnHome();
}

void nofAggressiveDefender::AttackerLost()
{
    RTTR_Assert(attacker);
    attacker = nullptr;
}

void nofAggressiveDefender::NeedForHomeDefence()
{
    InformTargetsAboutCancelling();
}

void nofAggressiveDefender::InformTargetsAboutCancelling()
{
    nofActiveSoldier::InformTargetsAboutCancelling();
    CancelAtAttacker();
    CancelAtAttackedBld();
}

void nofAggressiveDefender::CancelAtAttacker()
{
    if(attacker)
    {
        RTTR_Assert(attacker->GetHuntingDefender() == this);
        attacker->AggressiveDefenderLost();
        attacker = nullptr;
    }
}

nofActiveSoldier::SoldierState nofAggressiveDefender::FreeFightAborted()
{
    // Continue with normal walking towards our goal
    return SoldierState::AggressivedefendingWalkingToAggressor;
}
