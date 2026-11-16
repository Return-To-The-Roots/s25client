// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
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
      attacker(&attacker), attacked_goal(attacker.getAttackedGoal())
{
    // Notify building to protect
    attacked_goal->linkAggressiveDefender(*this);
}

nofAggressiveDefender::nofAggressiveDefender(const nofPassiveSoldier& other, nofAttacker& attacker)
    : nofActiveSoldier(other, SoldierState::AggressivedefendingWalkingToAggressor), attacker(&attacker),
      attacked_goal(attacker.getAttackedGoal())
{
    // Notify building to protect
    attacked_goal->linkAggressiveDefender(*this);
}

nofAggressiveDefender::~nofAggressiveDefender() = default;

void nofAggressiveDefender::destroy()
{
    RTTR_Assert(!attacker);
    RTTR_Assert(!attacked_goal);
    nofActiveSoldier::destroy();
}

void nofAggressiveDefender::Serialize(SerializedGameData& sgd) const
{
    nofActiveSoldier::Serialize(sgd);

    if(state != SoldierState::WalkingHome && state != SoldierState::FigureWork)
    {
        sgd.pushObject(attacker, true);
        sgd.pushObject(attacked_goal);
    }
}

nofAggressiveDefender::nofAggressiveDefender(SerializedGameData& sgd, const unsigned obj_id)
    : nofActiveSoldier(sgd, obj_id)
{
    if(state != SoldierState::WalkingHome && state != SoldierState::FigureWork)
    {
        attacker = sgd.popObject<nofAttacker>(GO_Type::NofAttacker);
        attacked_goal = sgd.popObject<nobBaseMilitary>();
    } else
    {
        attacker = nullptr;
        attacked_goal = nullptr;
    }
}

void nofAggressiveDefender::walked()
{
    if(state == SoldierState::AggressivedefendingWalkingToAggressor)
        missAggressiveDefendingWalk();
    else
        nofActiveSoldier::walked();
}

void nofAggressiveDefender::homeDestroyed()
{
    homeBld = nullptr;
}

void nofAggressiveDefender::homeDestroyedAtBegin()
{
    homeBld = nullptr;

    // angegriffenem Gebäude Bescheid sagen, dass wir doch nicht mehr kommen
    informTargetsAboutCancelling();

    state = SoldierState::FigureWork;

    // Rumirren
    startWandering();
    startWalking(RANDOM_ENUM(Direction));
}

void nofAggressiveDefender::cancelAtAttackedBld()
{
    if(attacked_goal)
    {
        attacked_goal->unlinkAggressiveDefender(*this);
        attacked_goal = nullptr;
    }
}

void nofAggressiveDefender::wonFighting()
{
    // addon BattlefieldPromotion active? -> increase rank!
    if(world->getGGS().isEnabled(AddonId::BATTLEFIELD_PROMOTION))
        increaseRank();

    // Ist evtl. unser Heimatgebäude zerstört?
    if(!homeBld)
    {
        // Ziel Bescheid sagen
        informTargetsAboutCancelling();

        // Rumirren
        state = SoldierState::FigureWork;
        startWandering();
        wander();

        return;
    }

    // Continue walking to our attacker
    missAggressiveDefendingContinueWalking();
}

void nofAggressiveDefender::lostFighting()
{
    // Meinem zu Hause Bescheid sagen, dass ich nicht mehr lebe (damit neue Truppen reinkönnen),
    // falls es noch existiert
    abrogateWorkplace();

    // Ziel Bescheid sagen, das ich verteidigt hatte
    informTargetsAboutCancelling();
}

void nofAggressiveDefender::missionAggressiveDefendingLookForNewAggressor()
{
    RTTR_Assert(!attacker);
    // Wenns das Zielgebäude nich mehr gibt, gleich nach Hause gehen!
    if(!attacked_goal)
    {
        returnHomeMissionAggressiveDefending();
        return;
    }

    // Vermeiden, dass in FindAggressor nochmal der Soldat zum Loslaufen gezwungen wird, weil er als state
    // noch drinstehen hat, dass er auf einen Kampf wartet
    state = SoldierState::AggressivedefendingWalkingToAggressor;

    // nach anderen suchen, die in meiner Nähe sind und mich evtl noch mit denen kloppen
    attacker = attacked_goal->findAggressor(*this);
    if(attacker)
    {
        // zum Angreifer gehen und mit ihm kämpfen
        missAggressiveDefendingWalk();
    } else
    {
        // keiner will mehr mit mir kämpfen, dann geh ich halt wieder nach Hause
        returnHomeMissionAggressiveDefending();
    }
}

void nofAggressiveDefender::attackedGoalDestroyed()
{
    cancelAtAttacker();
    attacked_goal = nullptr;

    /*// Stehen wir? Dann losgehen
    if(state == Waitingforfight)
        ReturnHome();*/
}

void nofAggressiveDefender::missAggressiveDefendingContinueWalking()
{
    state = SoldierState::AggressivedefendingWalkingToAggressor;
    missAggressiveDefendingWalk();
}

void nofAggressiveDefender::missAggressiveDefendingWalk()
{
    if(!homeBld)
    {
        // Home destroyed so abort and wander around
        informTargetsAboutCancelling();
        state = SoldierState::FigureWork;
        startWandering();
        wander();
        return;
    }

    if(!attacked_goal)
    {
        // Similar when the defended building is gone
        returnHomeMissionAggressiveDefending();
        return;
    }

    if(!attacker)
    {
        // Search for a new attacker if lost
        missionAggressiveDefendingLookForNewAggressor();
        return;
    }

    // Does he still want to fight?
    if(!attacker->isReadyForFight())
    {
        // Look for a new one
        cancelAtAttacker();
        missionAggressiveDefendingLookForNewAggressor();
        return;
    }

    // Look for enemies
    if(tryFightingNearbyEnemy())
    {
        // Enemy found -> abort, because nofActiveSoldier handles all things now
        // Note it is ok, if the enemy is our attacker.
        // If we win, we will either see, that the attacker is busy or be notified because he did
        // If we loose, we will tell him later
        return;
    }

    RTTR_Assert(pos != attacker->getPos()); // If so, why was it not found?

    // Calc next walking direction
    const auto dir = world->findHumanPath(pos, attacker->getPos(), 100, true);

    if(dir)
    {
        // Continue walking towards him
        startWalking(*dir);
    } else
    {
        // No route found
        // Look for new attacker
        cancelAtAttacker();
        missionAggressiveDefendingLookForNewAggressor();
    }
}

void nofAggressiveDefender::returnHomeMissionAggressiveDefending()
{
    informTargetsAboutCancelling();
    returnHome();
}

void nofAggressiveDefender::attackerLost()
{
    RTTR_Assert(attacker);
    attacker = nullptr;
    // TODO(Replay) When still in leave queue abort to avoid going out and right back
}

void nofAggressiveDefender::informTargetsAboutCancelling()
{
    nofActiveSoldier::informTargetsAboutCancelling();
    cancelAtAttacker();
    cancelAtAttackedBld();
}

void nofAggressiveDefender::cancelAtAttacker()
{
    if(attacker)
    {
        RTTR_Assert(attacker->getHuntingDefender() == this);
        attacker->aggressiveDefenderLost();
        attacker = nullptr;
    }
}

nofActiveSoldier::SoldierState nofAggressiveDefender::freeFightAborted()
{
    // Continue with normal walking towards our goal
    return SoldierState::AggressivedefendingWalkingToAggressor;
}
