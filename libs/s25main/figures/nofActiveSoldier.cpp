// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofActiveSoldier.h"
#include "EventManager.h"
#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "SerializedGameData.h"
#include "buildings/nobMilitary.h"
#include "world/GameWorld.h"
#include "nodeObjs/noFighting.h"
#include "nodeObjs/noFlag.h"
#include "gameData/MilitaryConsts.h"
#include "s25util/Log.h"
#include <stdexcept>

nofActiveSoldier::nofActiveSoldier(const MapPoint pos, const unsigned char player, nobBaseMilitary& home,
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
        building = world->GetSpecObj<nobMilitary>(this->GetPos());
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
    building->AddActiveSoldier(world->RemoveFigure(pos, *this));
}

void nofActiveSoldier::ReturnHome()
{
    state = SoldierState::WalkingHome;
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

    if(GetPos() == building->GetFlagPos()) // Are we already at the flag?

        StartWalking(Direction::NorthWest); // Enter via the door
    else if(GetPos() == building->GetPos()) // or are we at the building
        building->AddActiveSoldier(world->RemoveFigure(pos, *this));
    else
    {
        const auto dir = world->FindHumanPath(pos, building->GetFlagPos(), 100);

        if(dir)
        {
            // Find all sorts of enemies (attackers, aggressive defenders..) nearby
            if(TryFightingNearbyEnemy())
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
            DrawSoldierWaiting(drawPt);
            DrawArmorNotWalking(drawPt);
            break;
        }
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
            DrawWalkingBobJobs(drawPt, job_);
            DrawArmorWalking(drawPt);
            break;
        }
    }
}

unsigned nofActiveSoldier::GetVisualRange() const
{
    return VISUALRANGE_SOLDIER;
}

void nofActiveSoldier::ExpelEnemies() const
{
    // Collect the figures nearby
    std::vector<noFigure*> figures;

    // At the position of the soldier
    for(noBase& fieldFigure : world->GetFigures(pos))
    {
        if(fieldFigure.GetType() == NodalObjectType::Figure)
            figures.push_back(static_cast<noFigure*>(&fieldFigure));
    }
    // And around this point
    for(const MapPoint nb : world->GetNeighbours(pos))
    {
        for(noBase& fieldFigure : world->GetFigures(nb))
        {
            // Normal settler?
            // Don't disturb hedgehogs and rabbits!
            if(fieldFigure.GetType() == NodalObjectType::Figure)
            {
                auto& fig = static_cast<noFigure&>(fieldFigure);
                // The people have to be either on the point itself or they have to walk there
                if(fig.GetPos() == pos || fig.GetDestinationForCurrentMove() == pos)
                    figures.push_back(&fig);
            }
        }
    }

    const GamePlayer& owner = world->GetPlayer(player);
    for(auto* fig : figures)
    {
        // Enemy of us but no soldier, and walking on the road (i.e. no free workers like woodcutters etc.)
        if(!owner.IsAlly(fig->GetPlayer()) && !fig->IsSoldier() && fig->IsWalkingOnRoad())
        {
            // Then he should start wandering around
            fig->Abrogate();
            fig->StartWandering();
            // Not walking? (Could be carriers who are waiting for wares on roads)
            if(!fig->IsMoving() && !fig->WalkInRandomDir())
            {
                // No possible way found (unlikely but just in case)
                fig->Die();
            }
        }
    }
}

void nofActiveSoldier::Walked()
{
    switch(state)
    {
        default: return;
        case SoldierState::WalkingHome: WalkingHome(); return;
        case SoldierState::MeetEnemy: MeetingEnemy(); return;
    }
}

bool nofActiveSoldier::TryFightingNearbyEnemy(const std::optional<unsigned char>& excludedOwner)
{
    RTTR_Assert(enemy == nullptr);
    enemy = nullptr;

    const GamePlayer& owner = world->GetPlayer(player);
    // Check all close points
    for(const auto& curPos : world->GetPointsInRadiusWithCenter(pos, 2))
    {
        for(noBase& object : world->GetFigures(curPos))
        {
            auto* soldier = dynamic_cast<nofActiveSoldier*>(&object);
            if(!soldier || soldier->GetPlayer() == excludedOwner)
                continue;
            if(soldier->IsReadyForFight() && !owner.IsAlly(soldier->GetPlayer()))
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
    if(excludedOwner)
    {
        // we have an excluded owner for our new enemy and that only happens in ffa situations when we won against
        // the last defender so our fight spot is the exact location we have right now
        fightSpot_ = pos;
    } else if(const auto fightSpot = GetFightSpotNear(*enemy))
        fightSpot_ = *fightSpot;
    else
    {
        // No success? Then no fight
        enemy = nullptr;
        return false;
    }

    // We try to meet now
    state = SoldierState::MeetEnemy;
    enemy->MeetEnemy(*this, fightSpot_);

    // Walk to him
    MeetingEnemy();

    return true;
}

void nofActiveSoldier::IncreaseRank()
{
    if(GetRank() < world->GetGGS().GetMaxMilitaryRank())
    {
        // Promote and adjust inventory accordingly
        world->GetPlayer(player).DecreaseInventoryJob(job_, 1);
        job_ = Job(rttr::enum_cast(job_) + 1);
        world->GetPlayer(player).IncreaseInventoryJob(job_, 1);
    }
}

void nofActiveSoldier::MeetingEnemy()
{
    // At this point we must still have an enemy which we were walking towards
    RTTR_Assert(enemy);

    // Reached the fighting place?
    if(GetPos() == fightSpot_)
    {
        // Enemy already there?
        if(enemy->GetState() == SoldierState::WaitingForFight)
        {
            RTTR_Assert(enemy->GetPos() == fightSpot_);
            // Start fighting
            world->AddFigure(pos, std::make_unique<noFighting>(*enemy, *this));

            enemy->FightingStarted();
            FightingStarted();
        } else
        {
            // Is fighting point still valid (there could e.g. be another fight already)
            if(world->IsValidPointForFighting(pos, *this, false))
            {
                // Enemy should be on the way, so wait here
                RTTR_Assert(enemy->enemy == this);
                RTTR_Assert(enemy->GetState() == SoldierState::MeetEnemy);
                state = SoldierState::WaitingForFight;
            } else
            {
                AbortFreeFight();
                Walked();
            }
        }
    } else
    {
        // Not at the fighting spot yet, continue walking there
        const auto dir = world->FindHumanPath(pos, fightSpot_, MAX_ATTACKING_RUN_DISTANCE);
        if(dir)
            StartWalking(*dir);
        else
        {
            // No way from current location to fighting spot -> cancel fight
            AbortFreeFight();
            Walked();
        }
    }
}

void nofActiveSoldier::AbortFreeFight()
{
    const bool enemyWasWaiting = enemy && enemy->state == SoldierState::WaitingForFight;
    auto* old_enemy = enemy;
    // First clean up as much as possible: Reset enemies and set new state for both
    if(enemy)
    {
        RTTR_Assert(enemy->enemy == this);
        enemy->enemy = nullptr;
        enemy = nullptr;
    }
    state = FreeFightAborted();
    if(old_enemy)
        old_enemy->state = old_enemy->FreeFightAborted();
    // Now the (possibly) 2 soldiers are in a state where they could even engage in another fight with each other
    if(enemyWasWaiting)
    {
        // Act as-if the enemy just arrived at his position so he'll start moving again if required
        RTTR_Assert(!old_enemy->IsMoving());
        old_enemy->Walked();
    }
}

void nofActiveSoldier::InformTargetsAboutCancelling()
{
    if(enemy)
        AbortFreeFight();
}

void nofActiveSoldier::TakeHit()
{
    if(HasArmor())
        SetArmor(false);
    else
    {
        RTTR_Assert(hitpoints > 0u);
        --hitpoints;
    }
}

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

void nofActiveSoldier::MeetEnemy(nofActiveSoldier& other, const MapPoint figh_spot)
{
    enemy = &other;
    fightSpot_ = figh_spot;

    SoldierState old_state = state;
    state = SoldierState::MeetEnemy;

    // In some cases we have to start walking
    if(old_state == SoldierState::AttackingWaitingAroundBuilding)
        MeetingEnemy();
}

std::optional<MapPoint> nofActiveSoldier::GetFightSpotNear(const nofActiveSoldier& other)
{
    // Calc middle between the two soldiers and use this as origin spot for the search of more fight spots
    MapPoint otherPos = world->GetNeighbour(other.GetPos(), other.GetCurMoveDir());
    MapPoint middle((pos + otherPos) / 2u);

    // The point is supposed to be in the middle between the 2 soldiers (and guaranteed to be inside the map)
    // Maximum distance between 2 points is mapSize/2 (due to wrap around)
    // --> maximum distance between each point and the middle is mapSize/4
    // So if we see, that this is not the case, we take the "middle" point on the other half of the map

    const unsigned short mapWidth = world->GetWidth();
    const unsigned short mapHeight = world->GetHeight();

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
    RTTR_Assert(world->CalcDistance(otherPos, middle) <= std::max<unsigned>(mapWidth, mapHeight) / 4u);

    const auto isGoodFightingSpot = [world = this->world, pos = this->pos, this, &other](MapPoint pt) {
        return world->IsValidPointForFighting(pt, *this, true)
               && (pos == pt || world->FindHumanPath(pos, pt, MEET_FOR_FIGHT_DISTANCE * 2, false))
               && (other.GetPos() == pt
                   || world->FindHumanPath(other.GetPos(), pt, MEET_FOR_FIGHT_DISTANCE * 2, false));
    };
    const std::vector<MapPoint> pts =
      world->GetMatchingPointsInRadius<1>(middle, MEET_FOR_FIGHT_DISTANCE, isGoodFightingSpot, true);
    if(pts.empty())
        return std::nullopt;
    else
        return pts.front();
}

void nofActiveSoldier::FightingStarted()
{
    state = SoldierState::Fighting;
    enemy = nullptr;
}
