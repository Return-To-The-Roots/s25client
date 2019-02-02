// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

#include "rttrDefines.h" // IWYU pragma: keep
#include "nofAggressiveDefender.h"
#include "GlobalGameSettings.h"
#include "SerializedGameData.h"
#include "addons/const_addons.h"
#include "network/GameClient.h"
#include "nofAttacker.h"
#include "nofPassiveSoldier.h"
#include "random/Random.h"
#include "world/GameWorldGame.h"

nofAggressiveDefender::nofAggressiveDefender(const MapPoint pos, const unsigned char player, nobBaseMilitary* const home,
                                             const unsigned char rank, nofAttacker* const attacker)
    : nofActiveSoldier(pos, player, home, rank, STATE_AGGRESSIVEDEFENDING_WALKINGTOAGGRESSOR), attacker(attacker),
      attacked_goal(attacker->GetAttackedGoal())
{
    // Angegriffenem Gebäude Bescheid sagen
    attacked_goal->LinkAggressiveDefender(this);
}

nofAggressiveDefender::nofAggressiveDefender(nofPassiveSoldier* other, nofAttacker* const attacker)
    : nofActiveSoldier(*other, STATE_AGGRESSIVEDEFENDING_WALKINGTOAGGRESSOR), attacker(attacker), attacked_goal(attacker->GetAttackedGoal())
{
    // Angegriffenem Gebäude Bescheid sagen
    attacked_goal->LinkAggressiveDefender(this);
}

nofAggressiveDefender::~nofAggressiveDefender() {}

void nofAggressiveDefender::Destroy_nofAggressiveDefender()
{
    RTTR_Assert(!attacker);
    RTTR_Assert(!attacked_goal);
    Destroy_nofActiveSoldier();
}

void nofAggressiveDefender::Serialize_nofAggressiveDefender(SerializedGameData& sgd) const
{
    Serialize_nofActiveSoldier(sgd);

    if(state != STATE_WALKINGHOME && state != STATE_FIGUREWORK)
    {
        sgd.PushObject(attacker, true);
        sgd.PushObject(attacked_goal, false);
    }
}

nofAggressiveDefender::nofAggressiveDefender(SerializedGameData& sgd, const unsigned obj_id) : nofActiveSoldier(sgd, obj_id)
{
    if(state != STATE_WALKINGHOME && state != STATE_FIGUREWORK)
    {
        attacker = sgd.PopObject<nofAttacker>(GOT_NOF_ATTACKER);
        attacked_goal = sgd.PopObject<nobBaseMilitary>(GOT_UNKNOWN);
    } else
    {
        attacker = nullptr;
        attacked_goal = nullptr;
    }
}

void nofAggressiveDefender::Walked()
{
    // Was bestimmtes machen, je nachdem welchen Status wir gerade haben
    switch(state)
    {
        default: nofActiveSoldier::Walked(); return;
        case STATE_AGGRESSIVEDEFENDING_WALKINGTOAGGRESSOR: { MissAggressiveDefendingWalk();
        }
            return;
    }
}

/// Wenn ein Heimat-Militärgebäude bei Missionseinsätzen zerstört wurde
void nofAggressiveDefender::HomeDestroyed()
{
    building = nullptr;
}

void nofAggressiveDefender::HomeDestroyedAtBegin()
{
    building = nullptr;

    // angegriffenem Gebäude Bescheid sagen, dass wir doch nicht mehr kommen
    InformTargetsAboutCancelling();

    state = STATE_FIGUREWORK;

    // Rumirren
    StartWandering();
    StartWalking(Direction(RANDOM.Rand(__FILE__, __LINE__, GetObjId(), 6)));
}

void nofAggressiveDefender::CancelAtAttackedBld()
{
    if(attacked_goal)
    {
        attacked_goal->UnlinkAggressiveDefender(this);
        attacked_goal = nullptr;
    }
}

/// Wenn ein Kampf gewonnen wurde
void nofAggressiveDefender::WonFighting()
{
    // addon BattlefieldPromotion active? -> increase rank!
    if(gwg->GetGGS().isEnabled(AddonId::BATTLEFIELD_PROMOTION))
        IncreaseRank();

    // Ist evtl. unser Heimatgebäude zerstört?
    if(!building)
    {
        // Ziel Bescheid sagen
        InformTargetsAboutCancelling();

        // Rumirren
        state = STATE_FIGUREWORK;
        StartWandering();
        Wander();

        return;
    }

    // Continue walking to our attacker
    MissAggressiveDefendingContinueWalking();
}

/// Wenn ein Kampf verloren wurde (Tod)
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

    /// Vermeiden, dass in FindAggressor nochmal der Soldat zum Loslaufen gezwungen wird, weil er als state
    // noch drinstehen hat, dass er auf einen Kampf wartet
    state = STATE_AGGRESSIVEDEFENDING_WALKINGTOAGGRESSOR;

    // nach anderen suchen, die in meiner Nähe sind und mich evtl noch mit denen kloppen
    attacker = attacked_goal->FindAggressor(this);
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
    if(state == STATE_WAITINGFORFIGHT)
        ReturnHome();*/
}

void nofAggressiveDefender::MissAggressiveDefendingContinueWalking()
{
    state = STATE_AGGRESSIVEDEFENDING_WALKINGTOAGGRESSOR;
    MissAggressiveDefendingWalk();
}

void nofAggressiveDefender::MissAggressiveDefendingWalk()
{
    // Ist evtl. unser Heimatgebäude zerstört?
    if(!building)
    {
        InformTargetsAboutCancelling();

        // Rumirren
        state = STATE_FIGUREWORK;
        StartWandering();
        Wander();
        return;
    }

    // Wenns das Zielgebäude nich mehr gibt, gleich nach Hause gehen!
    if(!attacked_goal)
    {
        ReturnHomeMissionAggressiveDefending();
        return;
    }

    // Does the attacker still exists?
    if(!attacker)
    {
        // Look for a new one
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
    if(FindEnemiesNearby())
    {
        // Enemy found -> abort, because nofActiveSoldier handles all things now
        // Note it is ok, if the enemy is our attacker.
        // If we win, we will either see, that the attacker is busy or be notified because he did
        // If we loose, we will tell him later
        return;
    }

    RTTR_Assert(pos != attacker->GetPos()); // If so, why was it not found?

    // Calc next walking direction
    unsigned char dir = gwg->FindHumanPath(pos, attacker->GetPos(), 100, true);

    if(dir == 0xFF)
    {
        // No route found
        // Look for new attacker
        CancelAtAttacker();
        MissionAggressiveDefendingLookForNewAggressor();
    } else
    {
        // Continue walking towards him
        StartWalking(Direction(dir));
    }
}

void nofAggressiveDefender::ReturnHomeMissionAggressiveDefending()
{
    // Zielen Bescheid sagen
    InformTargetsAboutCancelling();
    // Und nach Hause gehen
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

/// Sagt den verschiedenen Zielen Bescheid, dass wir doch nicht mehr kommen können
void nofAggressiveDefender::InformTargetsAboutCancelling()
{
    nofActiveSoldier::InformTargetsAboutCancelling();
    // Angreifer Bescheid sagen
    CancelAtAttacker();
    // Ziel Bescheid sagen
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

/// The derived classes regain control after a fight of nofActiveSoldier
void nofAggressiveDefender::FreeFightEnded()
{
    nofActiveSoldier::FreeFightEnded();
    // Continue with normal walking towards our goal
    state = STATE_AGGRESSIVEDEFENDING_WALKINGTOAGGRESSOR;
}
