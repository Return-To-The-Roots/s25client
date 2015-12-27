// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

///////////////////////////////////////////////////////////////////////////////
// Header


#include "defines.h"
#include "nofAggressiveDefender.h"
#include "nofAttacker.h"
#include "nofPassiveSoldier.h"
#include "buildings/nobMilitary.h"
#include "Loader.h"
#include "GameClient.h"
#include "Random.h"
#include "nodeObjs/noFighting.h"
#include "SerializedGameData.h"
#include "MapGeometry.h"
#include "buildings/nobBaseWarehouse.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

nofAggressiveDefender::nofAggressiveDefender(const MapPoint pos, const unsigned char player,
        nobBaseMilitary* const home, const unsigned char rank, nofAttacker* const attacker)
    : nofActiveSoldier(pos, player, home, rank, STATE_AGGRESSIVEDEFENDING_WALKINGTOAGGRESSOR), attacker(attacker),
      attacked_goal(attacker->GetAttackedGoal())
{
    // Angegriffenem Gebäude Bescheid sagen
    attacked_goal->LinkAggressiveDefender(this);
}

nofAggressiveDefender::nofAggressiveDefender(nofPassiveSoldier* other, nofAttacker* const attacker)
    : nofActiveSoldier(*other, STATE_AGGRESSIVEDEFENDING_WALKINGTOAGGRESSOR), attacker(attacker),
      attacked_goal(attacker->GetAttackedGoal())
{
    // Angegriffenem Gebäude Bescheid sagen
    attacked_goal->LinkAggressiveDefender(this);
}

nofAggressiveDefender::~nofAggressiveDefender()
{
    //assert(!GAMECLIENT.GetPlayer(player).GetFirstWH()->TestOnMission(this));
}

void nofAggressiveDefender::Destroy_nofAggressiveDefender()
{
    assert(!attacker);
    assert(!attacked_goal);
    Destroy_nofActiveSoldier();

    //// Debugging
    //assert(!GAMECLIENT.GetPlayer(player).GetFirstWH()->TestOnMission(this));
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
    }
    else
    {
        attacker = NULL;
        attacked_goal = NULL;
    }
}

void nofAggressiveDefender::Walked()
{
    // Was bestimmtes machen, je nachdem welchen Status wir gerade haben
    switch(state)
    {
        default: nofActiveSoldier::Walked(); return;
        case STATE_AGGRESSIVEDEFENDING_WALKINGTOAGGRESSOR:
        {
            MissAggressiveDefendingWalk();
        } return;
    }
}

/// Wenn ein Heimat-Militärgebäude bei Missionseinsätzen zerstört wurde
void nofAggressiveDefender::HomeDestroyed()
{
    building = NULL;
}

void nofAggressiveDefender::HomeDestroyedAtBegin()
{
    building = NULL;

    // angegriffenem Gebäude Bescheid sagen, dass wir doch nicht mehr kommen
    CancelAtAttackedBld();


    state = STATE_FIGUREWORK;

    // Rumirren
    StartWandering();
    StartWalking(RANDOM.Rand(__FILE__, __LINE__, GetObjId(), 6));
}

void nofAggressiveDefender::CancelAtAttackedBld()
{
    if(attacked_goal)
    {
        attacked_goal->UnlinkAggressiveDefender(this);
        attacked_goal = NULL;
    }
}

/// Wenn ein Kampf gewonnen wurde
void nofAggressiveDefender::WonFighting()
{
	//addon BattlefieldPromotion active? -> increase rank!
	if(GAMECLIENT.GetGGS().isEnabled(ADDON_BATTLEFIELD_PROMOTION))
		IncreaseRank();
    // Angreifer tot
    attacker  = NULL;

    // Ist evtl. unser Heimatgebäude zerstört?
    if(!building)
    {
        // Rumirren
        state = STATE_FIGUREWORK;
        StartWandering();
        Wander();

        // Ziel Bescheid sagen
        CancelAtAttackedBld();

        return;
    }

    // Angreifer ist tot, nach anderen suchen, die in meiner Nähe sind und mich evtl noch mit denen kloppen
    MissionAggressiveDefendingLookForNewAggressor();
}

/// Wenn ein Kampf verloren wurde (Tod)
void nofAggressiveDefender::LostFighting()
{
    // Meinem zu Hause Bescheid sagen, dass ich nicht mehr lebe (damit neue Truppen reinkönnen),
    // falls es noch existiert
    AbrogateWorkplace();

    // Ziel Bescheid sagen, das ich verteidigt hatte
    CancelAtAttackedBld();
    attacker = NULL;
}


void nofAggressiveDefender::MissionAggressiveDefendingLookForNewAggressor()
{
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
        if(state == STATE_MEETENEMY)
            MeetingEnemy();
        else
            MissAggressiveDefendingWalk();
    }
    else
    {
        // keiner will mehr mit mir kämpfen, dann geh ich halt wieder nach Hause
        ReturnHomeMissionAggressiveDefending();
    }
}

void nofAggressiveDefender::AttackedGoalDestroyed()
{
    attacker = NULL;
    attacked_goal = NULL;

    /*// Stehen wir? Dann losgehen
    if(state == STATE_WAITINGFORFIGHT)
        ReturnHome();*/
}

void nofAggressiveDefender::MissAggressiveDefendingContinueWalking()
{
    state =  STATE_AGGRESSIVEDEFENDING_WALKINGTOAGGRESSOR;
    MissAggressiveDefendingWalk();
}


void nofAggressiveDefender::MissAggressiveDefendingWalk()
{
    // Ist evtl. unser Heimatgebäude zerstört?
    if(!building)
    {
        attacker = NULL;

        // Ziel Bescheid sagen
        CancelAtAttackedBld();

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
        MissionAggressiveDefendingLookForNewAggressor();
        return;
    }

    // Look for enemies
    if(FindEnemiesNearby())
        // Enemy found -> abort, because nofActiveSoldier handles all things now
        return;

    assert(pos != attacker->GetPos()); // If so, why was it not found?

    // Calc next walking direction
    unsigned char dir = gwg->FindHumanPath(pos, attacker->GetPos(), 100, true);

    if(dir == 0xFF)
    {
        // No route found
        // Look for new attacker
        MissionAggressiveDefendingLookForNewAggressor();
    }
    else
    {
        // Continue walking towards him
        StartWalking(dir);
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
    attacker = NULL;

    // Wenn wir auf die gewartet hatten, müssen wir uns einen neuen Angreifer suchen
    if(state == STATE_WAITINGFORFIGHT)
        MissionAggressiveDefendingLookForNewAggressor();
}


void nofAggressiveDefender::NeedForHomeDefence()
{
    // Angreifer Bescheid sagen
    attacker = NULL;

    // Ziel Bescheid sagen
    CancelAtAttackedBld();
}

/// Sagt den verschiedenen Zielen Bescheid, dass wir doch nicht mehr kommen können
void nofAggressiveDefender::InformTargetsAboutCancelling() //-V524
{
    // Angreifer Bescheid sagen
    attacker = NULL;
    // Ziel Bescheid sagen
    CancelAtAttackedBld();
}


/// The derived classes regain control after a fight of nofActiveSoldier
void nofAggressiveDefender::FreeFightEnded()
{
    nofActiveSoldier::FreeFightEnded();
    // Continue with normal walking towards our goal
    state = STATE_AGGRESSIVEDEFENDING_WALKINGTOAGGRESSOR;
}

