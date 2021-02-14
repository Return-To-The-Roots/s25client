// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "nofActiveSoldier.h"
#include "EventManager.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "SerializedGameData.h"
#include "buildings/nobMilitary.h"
#include "world/GameWorldGame.h"
#include "nodeObjs/noFighting.h"
#include "nodeObjs/noFlag.h"
#include "gameData/MilitaryConsts.h"
#include "s25util/Log.h"
#include <stdexcept>

nofActiveSoldier::nofActiveSoldier(const MapPoint pos, const unsigned char player, nobBaseMilitary* const home,
                                   const unsigned char rank, const SoldierState init_state)
    : nofSoldier(pos, player, home, rank), state(init_state), enemy(nullptr)

{}

nofActiveSoldier::nofActiveSoldier(const nofSoldier& other, const SoldierState init_state)
    : nofSoldier(other), state(init_state), enemy(nullptr)
{}

void nofActiveSoldier::Serialize(SerializedGameData& sgd) const
{
    nofSoldier::Serialize(sgd);

    sgd.PushEnum<uint8_t>(state);
    sgd.PushObject(enemy);
    helpers::pushPoint(sgd, fightSpot_);
}

nofActiveSoldier::nofActiveSoldier(SerializedGameData& sgd, const unsigned obj_id)
    : nofSoldier(sgd, obj_id), state(sgd.Pop<SoldierState>()), enemy(sgd.PopObject<nofActiveSoldier>())
{
    fightSpot_ = sgd.PopMapPoint();
}

void nofActiveSoldier::GoalReached()
{
    // We reached the military building
    // Add myself to the building
    if(!building)
    {
        RTTR_Assert(false);
        building = gwg->GetSpecObj<nobMilitary>(this->GetPos());
        if(building)
            LOG.write("nofActiveSoldier::GoalRoached() - no valid 'building' but found one at soldier's position "
                      "(%i,%i) (gf: %u)\n")
              % pos.x % pos.y % GetEvMgr().GetCurrentGF();
        else
        {
            LOG.write("nofActiveSoldier::GoalRoached() - no valid 'building' also didn't find one at soldier's "
                      "position (%i,%i) (gf: %u)\n")
              % pos.x % pos.y % GetEvMgr().GetCurrentGF();
            throw std::runtime_error("No building found for soldier");
        }
    }
    building->AddActiveSoldier(this);

    // And remove myself from the map
    gwg->RemoveFigure(pos, this);
}

void nofActiveSoldier::ReturnHome()
{
    // Set appropriate state
    state = SoldierState::WalkingHome;
    // Start walking
    WalkingHome();
}

void nofActiveSoldier::WalkingHome()
{
    // Is our home military building destroyed?
    if(!building)
    {
        // Start wandering around
        state = SoldierState::FigureWork;
        StartWandering();
        Wander();

        return;
    }

    // Walking home to our military building

    // Are we already at the flag?
    if(GetPos() == building->GetFlag()->GetPos())
    {
        // Enter via the door
        StartWalking(Direction::NorthWest);
        return;
    }
    // or are we at the building?
    if(GetPos() == building->GetPos())
    {
        // We're there!
        building->AddActiveSoldier(this);
        // Remove myself from the map
        gwg->RemoveFigure(pos, this);
        return;
    }
    const auto dir = gwg->FindHumanPath(pos, building->GetFlag()->GetPos(), 100);
    if(dir)
    {
        // Find all sorts of enemies (attackers, aggressive defenders..) nearby
        if(FindEnemiesNearby())
            // Enemy found -> abort, because nofActiveSoldier handles all things now (inclusive one walking step)
            return;

        // Start walking
        StartWalking(*dir);
    } else
    {
        // Inform our home building that we're not coming anymore
        Abrogate();
        // Start wandering around then
        StartWandering();
        state = SoldierState::FigureWork;
        Wander();
    }
}

void nofActiveSoldier::Draw(DrawPoint drawPt)
{
    switch(state)
    {
        default: break;
        case SoldierState::WaitingForFight:
        case SoldierState::AttackingWaitingAroundBuilding:
        case SoldierState::AttackingWaitingForDefender:
        case SoldierState::DefendingWaiting:
        {
            // Draw waiting states
            DrawSoldierWaiting(drawPt);
        }
        break;
        case SoldierState::FigureWork:
        case SoldierState::MeetEnemy:
        case SoldierState::AttackingWalkingToGoal:
        case SoldierState::AggressivedefendingWalkingToAggressor:
        case SoldierState::WalkingHome:
        case SoldierState::DefendingWalkingTo:
        case SoldierState::DefendingWalkingFrom:
        case SoldierState::AttackingCapturingFirst:
        case SoldierState::AttackingCapturingNext:
        case SoldierState::AttackingAttackingFlag:
        case SoldierState::SeaattackingGoToHarbor:
        case SoldierState::SeaattackingReturnToShip:
        {
            // Draw walking states
            DrawWalkingBobJobs(drawPt, job_);
        }
        break;
    }
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
    for(auto* fieldFigure : fieldFigures)
    {
        if(fieldFigure->GetType() == NodalObjectType::Figure)
            figures.push_back(static_cast<noFigure*>(fieldFigure));
    }

    // And around this point
    for(const auto dir : helpers::EnumRange<Direction>{})
    {
        const std::list<noBase*>& fieldFigures = gwg->GetFigures(gwg->GetNeighbour(pos, dir));
        for(auto* fieldFigure : fieldFigures)
        {
            // Normal settler?
            // Don't disturb hedgehogs and rabbits!
            if(fieldFigure->GetType() == NodalObjectType::Figure)
            {
                auto* fig = static_cast<noFigure*>(fieldFigure);
                // The people have to be either on the point itself or they have to walk there
                if(fig->GetPos() == pos || fig->GetDestinationForCurrentMove() == pos)
                    figures.push_back(fig);
            }
        }
    }

    // Let's see which things are netted and sort the wrong things out
    // ( Don't annoy Erika Steinbach! )
    for(auto* fig : figures)
    {
        // Enemy of us and no soldier?
        // And he has to walking on the road (don't disturb free workers like woodcutters etc.)
        if(!gwg->GetPlayer(player).IsAlly(fig->GetPlayer()) && !fig->IsSoldier() && fig->IsWalkingOnRoad())
        {
            // Then he should start wandering around
            fig->Abrogate();
            fig->StartWandering();
            // Not walking? (Could be carriers who are waiting for wares on roads)
            if(!fig->IsMoving())
            {
                if(!fig->WalkInRandomDir())
                {
                    // No possible way found (unlikely but just in case)
                    fig->Die();
                }
            }
        }
    }
}

/// Handle walking for nofActiveSoldier speciefic sates
void nofActiveSoldier::Walked()
{
    switch(state)
    {
        default: return;
        case SoldierState::WalkingHome: WalkingHome(); return;
        case SoldierState::MeetEnemy: MeetingEnemy(); return;
    }
}

/// Looks for enemies nearby which want to fight with this soldier
/// Returns true if it found one
bool nofActiveSoldier::FindEnemiesNearby(unsigned char excludedOwner)
{
    RTTR_Assert(enemy == nullptr);
    enemy = nullptr;

    // Get all points in a radius of 2
    std::vector<MapPoint> pts = gwg->GetPointsInRadiusWithCenter(pos, 2);

    for(const auto& curPos : pts)
    {
        for(noBase* object : gwg->GetFigures(curPos))
        {
            auto* soldier = dynamic_cast<nofActiveSoldier*>(object);
            if(!soldier || soldier->GetPlayer() == excludedOwner)
                continue;
            if(soldier->IsReadyForFight() && !gwg->GetPlayer(soldier->GetPlayer()).IsAlly(player))
            {
                enemy = soldier;
                break;
            }
        }
        if(enemy)
            break;
    }

    // No enemy found? Goodbye
    if(!enemy)
        return false;

    // Try to find fighting spot
    if(excludedOwner == 255)
    {
        if(!GetFightSpotNear(enemy, &fightSpot_))
        {
            // No success? Then no fight
            enemy = nullptr;
            return false;
        }
    } else // we have an excluded owner for our new enemy and that only happens in ffa situations when we won against
           // the last defender so our fightspot is the exact location we have right now
    {
        fightSpot_ = pos;
    }

    // We try to meet us now
    state = SoldierState::MeetEnemy;
    // Inform the other soldier
    enemy->MeetEnemy(this, fightSpot_);

    // Walk to him
    MeetingEnemy();

    return true;
}
/// increase rank
void nofActiveSoldier::IncreaseRank()
{
    // max rank reached? -> dont increase!
    if(GetRank() >= gwg->GetGGS().GetMaxMilitaryRank())
        return;

    // Einen Rang höher
    // Inventur entsprechend erhöhen und verringern
    gwg->GetPlayer(player).DecreaseInventoryJob(job_, 1);
    job_ = Job(unsigned(job_) + 1);
    gwg->GetPlayer(player).IncreaseInventoryJob(job_, 1);
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
    if(GetPos() == fightSpot_)
    {
        // Enemy already there?
        if(enemy->GetPos() == fightSpot_ && enemy->GetState() == SoldierState::WaitingForFight)
        {
            // Start fighting
            gwg->AddFigure(pos, new noFighting(enemy, this));

            enemy->FightingStarted();
            FightingStarted();

            return;
        } else
        {
            // Is the fighting point still valid (could be another fight there already e.g.)?
            // And the enemy still on the way?
            if(!gwg->ValidPointForFighting(pos, false, this) || enemy->GetState() != SoldierState::MeetEnemy)
            {
                // No
                // Abort the whole fighting fun with the enemy
                enemy->FreeFightEnded();

                FreeFightEnded();
                Walked();
            }
            // Spot is still ok, let's wait for the enemy
            else
            {
                RTTR_Assert(enemy->enemy == this);
                state = SoldierState::WaitingForFight;
                return;
            }
        }
    }
    // Not at the fighting spot yet, continue walking there
    else
    {
        const auto dir = gwg->FindHumanPath(pos, fightSpot_, MAX_ATTACKING_RUN_DISTANCE);
        if(dir)
        {
            StartWalking(*dir);
        } else
        {
            // qx: Couldnt find a way from current location to fighting spot -> cancel fight (Fix for #1189150)
            enemy->FreeFightEnded();
            FreeFightEnded();
            Walked();
        }
        return;
    }
}

void nofActiveSoldier::FreeFightEnded()
{
    enemy = nullptr;
}

void nofActiveSoldier::InformTargetsAboutCancelling()
{
    if(enemy)
    {
        enemy->FreeFightEnded();
        enemy = nullptr;
    }
}

void nofActiveSoldier::TakeHit()
{
    RTTR_Assert(hitpoints > 0u);
    --hitpoints;
}

/// Determines if this soldier is ready for a spontaneous  fight
bool nofActiveSoldier::IsReadyForFight() const
{
    switch(state)
    {
        default: return false;
        case SoldierState::WalkingHome:
        case SoldierState::AggressivedefendingWalkingToAggressor:
        case SoldierState::AttackingWalkingToGoal:
        case SoldierState::AttackingWaitingAroundBuilding: return true;
    }
}

/// Informs this soldier that another soldier starts meeting him
void nofActiveSoldier::MeetEnemy(nofActiveSoldier* other, const MapPoint figh_spot)
{
    // Remember these things
    enemy = other;
    this->fightSpot_ = figh_spot;

    SoldierState old_state = state;
    state = SoldierState::MeetEnemy;

    // In some cases we have to start walking
    if(old_state == SoldierState::AttackingWaitingAroundBuilding)
    {
        MeetingEnemy();
    }
}

/// Looks for an appropriate fighting spot between the two soldiers
/// Returns true if successful
bool nofActiveSoldier::GetFightSpotNear(nofActiveSoldier* other, MapPoint* fight_spot)
{
    // Calc middle between the two soldiers and use this as origin spot for the search of more fight spots
    MapPoint otherPos = gwg->GetNeighbour(other->GetPos(), other->GetCurMoveDir());
    MapPoint middle((pos + otherPos) / 2u);

    // The point is supposed to be in the middle between the 2 soldiers (and guaranteed to be inside the map)
    // Maximum distance between 2 points is mapSize/2 (due to wrap around)
    // --> maximum distance between each point and the middle is mapSize/4
    // So if we see, that this is not the case, we take the "middle" point on the other half of the map

    const unsigned short mapWidth = gwg->GetWidth();
    const unsigned short mapHeight = gwg->GetHeight();

    if(std::abs(otherPos.x - middle.x) > mapWidth / 4)
    {
        const unsigned short halfMapWidth = mapWidth / 2;
        if(middle.x >= halfMapWidth)
            middle.x -= halfMapWidth;
        else
            middle.x += halfMapWidth;
    }
    if(std::abs(otherPos.y - middle.y) > mapHeight / 4)
    {
        const unsigned short halfMapHeight = mapHeight / 2;
        if(middle.y >= halfMapHeight)
            middle.y -= halfMapHeight;
        else
            middle.y += halfMapHeight;
    }
    RTTR_Assert(gwg->CalcDistance(otherPos, middle) <= std::max<unsigned>(mapWidth, mapHeight) / 4u);

    const auto isGoodFightingSpot = [gwg = this->gwg, pos = this->pos, other](const auto& pt) {
        // Did we find a good spot?
        return gwg->ValidPointForFighting(pt, true, nullptr)
               && (pos == pt || gwg->FindHumanPath(pos, pt, MEET_FOR_FIGHT_DISTANCE * 2, false, nullptr))
               && (other->GetPos() == pt
                   || gwg->FindHumanPath(other->GetPos(), pt, MEET_FOR_FIGHT_DISTANCE * 2, false, nullptr));
    };
    const std::vector<MapPoint> pts =
      gwg->GetMatchingPointsInRadius<1>(middle, MEET_FOR_FIGHT_DISTANCE, isGoodFightingSpot, true);
    if(pts.empty())
        return false;
    *fight_spot = pts.front();
    return true;
}

/// Informs a waiting soldier about the start of a fight
void nofActiveSoldier::FightingStarted()
{
    state = SoldierState::Fighting;
    enemy = nullptr;
}
