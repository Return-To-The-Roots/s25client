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
#include "nofActiveSoldier.h"
#include "buildings/nobMilitary.h"
#include "Loader.h"
#include "gameData/GameConsts.h"
#include "Random.h"
#include "GameWorld.h"
#include "nodeObjs/noFighting.h"
#include "GameClient.h"
#include "SerializedGameData.h"

#include "ogl/glSmartBitmap.h"
#include "Log.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

nofActiveSoldier::nofActiveSoldier(const MapPoint pos, const unsigned char player,
                                   nobBaseMilitary* const home, const unsigned char rank, const SoldierState init_state)
    : nofSoldier(pos, player, home, rank), state(init_state), enemy(NULL)

{
}

nofActiveSoldier::nofActiveSoldier(const nofSoldier& other, const SoldierState init_state) :
    nofSoldier(other), state(init_state), enemy(NULL) {}


void nofActiveSoldier::Serialize_nofActiveSoldier(SerializedGameData* sgd) const
{
    Serialize_nofSoldier(sgd);

    sgd->PushUnsignedChar(static_cast<unsigned char>(state));
    sgd->PushObject(enemy, false);
    sgd->PushMapPoint(fightSpot_);
}


nofActiveSoldier::nofActiveSoldier(SerializedGameData* sgd, const unsigned obj_id) : nofSoldier(sgd, obj_id),
    state(SoldierState(sgd->PopUnsignedChar())),
    enemy(sgd->PopObject<nofActiveSoldier>(GOT_UNKNOWN))
{
    fightSpot_ = sgd->PopMapPoint();
}



void nofActiveSoldier::GoalReached()
{
    // We reached the military building
    // Add myself to the building
    if(!building)
    {
        if((building = gwg->GetSpecObj<nobMilitary>(this->GetPos())))
            LOG.lprintf("nofActiveSoldier::GoalRoached() - no valid 'building' but found one at soldier's position (%i,%i) (gf: %u)\n", pos.x, pos.y,GAMECLIENT.GetGFNumber());
        else
            LOG.lprintf("nofActiveSoldier::GoalRoached() - no valid 'building' also didnt find one at soldier's position (%i,%i) (gf: %u)\n", pos.x, pos.y,GAMECLIENT.GetGFNumber());
    }
    static_cast<nobMilitary*>(building)->AddActiveSoldier(this);

    // And remove myself from the map
    gwg->RemoveFigure(this, pos);
}

void nofActiveSoldier::ReturnHome()
{
    // Set appropriate state
    state = STATE_WALKINGHOME;
    // Start walking
    WalkingHome();
}


void nofActiveSoldier::WalkingHome()
{
    // Is our home military building destroyed?
    if(!building)
    {
        // Start wandering around
        state = STATE_FIGUREWORK;
        StartWandering();
        Wander();

        return;
    }


    // Walking home to our military building

    // Are we already at the flag?
    if(GetPos() == building->GetFlag()->GetPos())
    {
        // Enter via the door
        StartWalking(1);
        return;
    }
    // or are have we come into the building?
    if(GetPos() == building->GetPos())
    {
        // We're there!
        building->AddActiveSoldier(this);
        // Remove myself from the map
        gwg->RemoveFigure(this, pos);
        return;
    }
    unsigned char dir = gwg->FindHumanPath(pos, building->GetFlag()->GetPos(), 100);
    // Or we don't find a route?
    if(dir == 0xFF)
    {
        // Start wandering around then
        StartWandering();
        state = STATE_FIGUREWORK;
        Wander();

        // Inform our home building that we're not coming anymore
        building->SoldierLost(this);
    }
    // All ok?
    else
    {
        // Find all sorts of enemies (attackers, aggressive defenders..) nearby
        if(FindEnemiesNearby())
            // Enemy found -> abort, because nofActiveSoldier handles all things now (inclusive one walking step)
            return;

        // Start walking
        StartWalking(dir);
    }
}


void nofActiveSoldier::Draw(int x, int y)
{
    switch(state)
    {
        default:
            break;
        case STATE_WAITINGFORFIGHT:
        case STATE_ATTACKING_WAITINGAROUNDBUILDING:
        case STATE_ATTACKING_WAITINGFORDEFENDER:
        case STATE_DEFENDING_WAITING:
        {
            // Draw waiting states
            //Loader::bob_jobs_cache[GAMECLIENT.GetPlayer(player)->nation][job][dir][2].draw(x,y,COLOR_WHITE,COLORS[GAMECLIENT.GetPlayer(player)->color]);
            DrawSoldierWalking(x, y, true); //cannot draw from Soldiers & Scouts from Loader::bob_jobs_cache v9102
        } break;
        case STATE_FIGUREWORK:
        case STATE_MEETENEMY:
        case STATE_ATTACKING_WALKINGTOGOAL:
        case STATE_AGGRESSIVEDEFENDING_WALKINGTOAGGRESSOR:
        case STATE_WALKINGHOME:
        case STATE_DEFENDING_WALKINGTO:
        case STATE_DEFENDING_WALKINGFROM:
        case STATE_ATTACKING_CAPTURINGFIRST:
        case STATE_ATTACKING_CAPTURINGNEXT:
        case STATE_ATTACKING_ATTACKINGFLAG:
        case STATE_SEAATTACKING_GOTOHARBOR:
        case STATE_SEAATTACKING_RETURNTOSHIP:
        {
            // Draw walking states
            DrawSoldierWalking(x, y);
        } break;
    }
}

void nofActiveSoldier::HandleDerivedEvent(const unsigned int id)
{
    // That's not supposed to happen!
    assert(false);
}

/// Gets the visual range radius of this soldier
unsigned nofActiveSoldier::GetVisualRange() const
{
    return VISUALRANGE_SOLDIER;
}

/// Examines hostile people on roads and expels them
void nofActiveSoldier::ExpelEnemies()
{
    // Collect the figures nearby in a large bucket
    std::vector<noFigure*> figures;

    // At the position of the soldier
    const std::list<noBase*>& fieldFigures = gwg->GetFigures(pos);
    for(std::list<noBase*>::const_iterator it = fieldFigures.begin(); it != fieldFigures.end(); ++it)
    {
        if((*it)->GetType() == NOP_FIGURE)
            figures.push_back(static_cast<noFigure*>(*it));
    }

    // And around this point
    for(unsigned i = 0; i < 6; ++i)
    {
        const std::list<noBase*>& fieldFigures = gwg->GetFigures(gwg->GetNeighbour(pos, i));
        for(std::list<noBase*>::const_iterator it = fieldFigures.begin(); it != fieldFigures.end(); ++it)
        {
            // Normal settler?
            // Don't disturb hedgehogs and rabbits!
            if((*it)->GetType() == NOP_FIGURE)
            {
                noFigure* fig = static_cast<noFigure*>(*it);
                // The people have to be either on the point itself or they have to walk there
                if(fig->GetPos() == pos)
                    figures.push_back(fig);
                else if(fig->GetDestinationForCurrentMove() == pos)
                    figures.push_back(fig);
            }
        }
    }

    // Let's see which things are netted and sort the wrong things out
    // ( Don't annoy Erika Steinbach! )
    for(unsigned i = 0; i < figures.size(); ++i)
    {
        noFigure* fig = figures[i];
        // Enemy of us and no soldier?
        // And he has to walking on the road (don't disturb free workers like woodcutters etc.)
        if(!players->getElement(player)->IsAlly(fig->GetPlayer()) &&
                !(fig->GetJobType() >= JOB_PRIVATE && fig->GetJobType() <= JOB_GENERAL)
                && fig->IsWalkingOnRoad())
        {
            // Then he should start wandering around
            fig->Abrogate();
            fig->StartWandering();
            // Not walking? (Could be carriers who are waiting for wares on roads)
            if(!fig->IsMoving())
                // Go, go, go
                fig->StartWalking(RANDOM.Rand(__FILE__, __LINE__, GetObjId(), 6));
        }
    }
}


/// Handle walking for nofActiveSoldier speciefic sates
void nofActiveSoldier::Walked()
{
    switch(state)
    {
        default: return;
        case STATE_WALKINGHOME: WalkingHome(); return;
        case STATE_MEETENEMY: MeetingEnemy(); return;
    }

}



/// Looks for enemies nearby which want to fight with this soldier
/// Returns true if it found one
bool nofActiveSoldier::FindEnemiesNearby(unsigned char excludedOwner)
{
    // Vector with potential victims
    std::vector<nofActiveSoldier*> soldiersNearby;

    // Look in radius 1
    for(unsigned dir = 0; dir < 6; ++dir)
    {
        MapPoint t = gwg->GetNeighbour(pos, dir);
        std::vector<noBase*> objects = gwg->GetDynamicObjectsFrom(t);
        for(std::vector<noBase*>::iterator it = objects.begin(); it != objects.end(); ++it)
            if (dynamic_cast<nofActiveSoldier*>(*it) && dynamic_cast<nofActiveSoldier*>(*it)->GetPlayer()!=excludedOwner)
                soldiersNearby.push_back(dynamic_cast<nofActiveSoldier*>(*it));
    }

    // ... and radius 2
    for(unsigned dir = 0; dir < 12; ++dir)
    {
        MapPoint t = gwg->GetNeighbour2(pos, dir);
        std::vector<noBase*> objects = gwg->GetDynamicObjectsFrom(t);
        for(std::vector<noBase*>::iterator it = objects.begin(); it != objects.end(); ++it)
            if (dynamic_cast<nofActiveSoldier*>(*it) && dynamic_cast<nofActiveSoldier*>(*it)->GetPlayer()!=excludedOwner)
                soldiersNearby.push_back(dynamic_cast<nofActiveSoldier*>(*it));
    }


    enemy = NULL;

    // Check if the victims have nothing better to do
    for(unsigned i = 0; i < soldiersNearby.size(); ++i)
    {
        // Ready for fight and good enemy = Good victim
        if (soldiersNearby[i]->IsReadyForFight() && !GAMECLIENT.GetPlayer(soldiersNearby[i]->GetPlayer())->IsAlly(this->player))
        {
            enemy = soldiersNearby[i];
            break;
        }
    }

    // No enemy found? Goodbye
    if(!enemy)
        return false;

    // Try to find fighting spot
	if(excludedOwner==255)
	{
		if(!GetFightSpotNear(enemy, &fightSpot_))
			// No success? Then no fight
			return false;
	}
	else//we have an excluded owner for our new enemy and that only happens in ffa situations when we won against the last defender so our fightspot is the exact location we have right now
	{
		fightSpot_ = pos;
	}

    // We try to meet us now
    state = STATE_MEETENEMY;
    // Inform the other soldier
    enemy->MeetEnemy(this, fightSpot_);

    // Walk to him
    MeetingEnemy();

    return true;
}
/// increase rank
void nofActiveSoldier::IncreaseRank()
{   
	//max rank reached? -> dont increase!
	if(GetRank() >= GAMECLIENT.GetGGS().GetMaxMilitaryRank())
		return;

    // Einen Rang höher
    // Inventur entsprechend erhöhen und verringern
    gwg->GetPlayer(player)->DecreaseInventoryJob(job_, 1);
    job_ = Job(unsigned(job_) + 1);
    gwg->GetPlayer(player)->IncreaseInventoryJob(job_, 1);
}

/// Handle state "meet enemy" after each walking step
void nofActiveSoldier::MeetingEnemy()
{
    // Enemy vanished?
    if(!enemy)
    {
        FreeFightEnded();
        Walked();
        return;
    }

    // Reached the fighting place?
    if (GetPos() == fightSpot_)
    {
        // Enemy already there?
        if (enemy->GetPos() == fightSpot_ && enemy->GetState() == STATE_WAITINGFORFIGHT)
        {
            // Start fighting
            gwg->AddFigure(new noFighting(enemy, this), pos);

            enemy->FightingStarted();
            FightingStarted();

            return;
        }
        else
        {
            // Is the fighting point still valid (could be another fight there already e.g.)?
            // And the enemy still on the way?
            if (!gwg->ValidPointForFighting(pos, false,this) || !(enemy->GetState() == STATE_MEETENEMY))
            {
                // No
                // Abort the whole fighting fun with the enemy
                enemy->FreeFightEnded();
                enemy = NULL;

                FreeFightEnded();

                Walked();
            }
            // Spot is still ok, let's wait for the enemy
            else
            {
                //enemy = NULL;
                state = STATE_WAITINGFORFIGHT;
                return;
            }
        }
    }
    // Not at the fighting spot yet, continue walking there
    else
    {
        unsigned char dir = gwg->FindHumanPath(pos, fightSpot_, MAX_ATTACKING_RUN_DISTANCE);
        if (dir != 0xFF)
        {
            StartWalking(dir);
        }
        else
        {
            // qx: Couldnt find a way from current location to fighting spot -> cancel fight (Fix for #1189150)
            enemy->FreeFightEnded();
            enemy = NULL;
            FreeFightEnded();
            Walked();
        }
        return;
    }

}


/// Determines if this soldier is ready for a spontaneous  fight
bool nofActiveSoldier::IsReadyForFight() const
{
    switch(state)
    {
        default: return false;
        case STATE_WALKINGHOME:
        case STATE_AGGRESSIVEDEFENDING_WALKINGTOAGGRESSOR:
        case STATE_ATTACKING_WALKINGTOGOAL:
        case STATE_ATTACKING_WAITINGAROUNDBUILDING:
            return true;
    }
}

/// Informs this soldier that another soldier starts meeting him
void nofActiveSoldier::MeetEnemy(nofActiveSoldier* other, const MapPoint figh_spot)
{
    // Remember these things
    enemy = other;
    this->fightSpot_ = figh_spot;

    SoldierState old_state = state;
    state = STATE_MEETENEMY;

    // In some cases we have to start walking
    if(old_state == STATE_ATTACKING_WAITINGAROUNDBUILDING)
    {
        MeetingEnemy();
    }



}

/// Looks for an appropriate fighting spot between the two soldiers
/// Returns true if successful
bool nofActiveSoldier::GetFightSpotNear(nofActiveSoldier* other, MapPoint * fight_spot)
{
    // Calc middle between the two soldiers and use this as origin spot for the search of more fight spots
    MapPoint otherPos = gwg->GetNeighbour(other->GetPos(), other->GetCurMoveDir());
    MapPoint middle( (pos.x + otherPos.x) / 2, (pos.y + otherPos.y) / 2 );

    // Did we cross the borders ? ( horizontally)
    if(SafeDiff(middle.x, pos.x) > MEET_FOR_FIGHT_DISTANCE)
    {
        // In this case: distance = distance of soldier 1 to left border + distance of soldier 2 to right border
        MapCoord minx = std::min(pos.x, other->GetX());
        MapCoord maxx = std::max(pos.x, other->GetX());

        MapCoord diff = minx + (gwg->GetWidth() - maxx);
        middle.x += diff / 2;
    }

    // Did we cross the borders ? ( vertically)
    if(SafeDiff(middle.y, pos.y) > MEET_FOR_FIGHT_DISTANCE)
    {
        // In this case: distance = distance of soldier 1 to left border + distance of soldier 2 to right border
        MapCoord miny = std::min(pos.y, other->GetY());
        MapCoord maxy = std::max(pos.y, other->GetY());

        MapCoord diff = miny + (gwg->GetHeight() - maxy);
        middle.y += diff / 2;
    }

    // We could have crossed the border due to our interpolating across the borders
    middle = gwg->ConvertCoords(Point<int>(middle));


    // Test Middle point first
    if(gwg->ValidPointForFighting(middle, true, NULL)
            && (GetPos() == middle || gwg->FindHumanPath(pos, middle, MEET_FOR_FIGHT_DISTANCE * 2, false, NULL) != 0xff)
            && (other->GetPos() == middle || gwg->FindHumanPath(other->GetPos(), middle, MEET_FOR_FIGHT_DISTANCE * 2, false, NULL) != 0xff))
    {
        // Great, then let's take this one
        *fight_spot = middle;
        return true;

    }

    // Points around
    for(MapCoord tx = gwg->GetXA(middle.x, middle.y, 0), r = 1; r <= MEET_FOR_FIGHT_DISTANCE; tx = gwg->GetXA(tx, middle.y, 0), ++r)
    {
        // Wurde ein Punkt in diesem Radius gefunden?
        // bool found_in_radius = false;

        MapPoint t2(tx, middle.y);
        for(unsigned i = 2; i < 8; ++i)
        {
            for(MapCoord r2 = 0; r2 < r; t2 = gwg->GetNeighbour(t2, i % 6), ++r2)
            {
                // Did we find a good spot?
                if(gwg->ValidPointForFighting(t2, true, NULL)
                        && gwg->FindHumanPath(pos, t2, MEET_FOR_FIGHT_DISTANCE * 2, false, NULL) != 0xff
                        && gwg->FindHumanPath(other->GetPos(), t2, MEET_FOR_FIGHT_DISTANCE * 2, false, NULL) != 0xff)

                {
                    // Great, then let's take this one
                    *fight_spot = t2;
                    return true;

                }
            }
        }
    }

    // No point found
    return false;

}


/// Informs a waiting soldier about the start of a fight
void nofActiveSoldier::FightingStarted()
{
    state = STATE_FIGHTING;
    enemy = NULL;
}

///////////////////////////////////////////////////////////////////////////////
// EOF
///////////////////////////////////////////////////////////////////////////////

