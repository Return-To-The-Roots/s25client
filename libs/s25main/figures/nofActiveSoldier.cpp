// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
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

void nofActiveSoldier::serialize(SerializedGameData& sgd) const
{
    nofSoldier::serialize(sgd);

    sgd.pushEnum<uint8_t>(state);
    sgd.pushObject(enemy);
    helpers::pushPoint(sgd, fightSpot_);
}

nofActiveSoldier::nofActiveSoldier(SerializedGameData& sgd, const unsigned obj_id)
    : nofSoldier(sgd, obj_id), state(sgd.pop<SoldierState>()), enemy(sgd.popObject<nofActiveSoldier>())
{
    fightSpot_ = sgd.popMapPoint();
}

void nofActiveSoldier::goalReached()
{
    // We reached the military building
    // Add myself to the building
    if(!homeBld)
    {
        RTTR_Assert(false);
        homeBld = world->getSpecObj<nobMilitary>(this->getPos());
        if(homeBld)
            LOG.write("nofActiveSoldier::GoalRoached() - no valid 'building' but found one at soldier's position "
                      "(%i,%i) (gf: %u)\n")
              % pos.x % pos.y % getEvMgr().getCurrentGF();
        else
        {
            LOG.write("nofActiveSoldier::GoalRoached() - no valid 'building' also didn't find one at soldier's "
                      "position (%i,%i) (gf: %u)\n")
              % pos.x % pos.y % getEvMgr().getCurrentGF();
            throw std::runtime_error("No building found for soldier");
        }
    }
    homeBld->addActiveSoldier(world->RemoveFigure(pos, *this));
}

void nofActiveSoldier::returnHome()
{
    state = SoldierState::WalkingHome;
    walkingHome();
}

void nofActiveSoldier::walkingHome()
{
    // Is our home military building destroyed?
    if(!homeBld)
    {
        // Start wandering around
        state = SoldierState::FigureWork;
        startWandering();
        wander();

        return;
    }

    // Walking home to our military building

    if(getPos() == homeBld->getFlagPos())   // Are we already at the flag?
        startWalking(Direction::NorthWest); // Enter via the door
    else if(getPos() == homeBld->getPos())  // or are we at the building
        homeBld->addActiveSoldier(world->RemoveFigure(pos, *this));
    else
    {
        const auto dir = world->findHumanPath(pos, homeBld->getFlagPos(), 100);

        if(dir)
        {
            // Find all sorts of enemies (attackers, aggressive defenders..) nearby
            if(tryFightingNearbyEnemy())
                // Enemy found -> abort, because nofActiveSoldier handles all things now (inclusive one walking step)
                return;

            // Start walking
            startWalking(*dir);
        } else
        {
            // Inform our home building that we're not coming anymore
            abrogate();
            // Start wandering around then
            startWandering();
            state = SoldierState::FigureWork;
            wander();
        }
    }
}

void nofActiveSoldier::draw(DrawPoint drawPt)
{
    switch(state)
    {
        default: break;
        case SoldierState::WaitingForFight:
        case SoldierState::AttackingWaitingAroundBuilding:
        case SoldierState::AttackingWaitingForDefender:
        case SoldierState::DefendingWaiting:
        {
            drawSoldierWaiting(drawPt);
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
            drawWalkingBobJobs(drawPt, job_);
            break;
        }
    }
}

unsigned nofActiveSoldier::getVisualRange() const
{
    return VISUALRANGE_SOLDIER;
}

void nofActiveSoldier::expelEnemies() const
{
    // Collect the figures nearby
    std::vector<noFigure*> figures;

    // At the position of the soldier
    for(noBase& fieldFigure : world->getFigures(pos))
    {
        if(fieldFigure.getType() == NodalObjectType::Figure)
            figures.push_back(static_cast<noFigure*>(&fieldFigure));
    }
    // And around this point
    for(const MapPoint nb : world->getNeighbours(pos))
    {
        for(noBase& fieldFigure : world->getFigures(nb))
        {
            // Normal settler?
            // Don't disturb hedgehogs and rabbits!
            if(fieldFigure.getType() == NodalObjectType::Figure)
            {
                auto& fig = static_cast<noFigure&>(fieldFigure);
                // The people have to be either on the point itself or they have to walk there
                if(fig.getPos() == pos || fig.getDestinationForCurrentMove() == pos)
                    figures.push_back(&fig);
            }
        }
    }

    const GamePlayer& owner = world->getPlayer(player);
    for(auto* fig : figures)
    {
        // Enemy of us but no soldier, and walking on the road (i.e. no free workers like woodcutters etc.)
        if(!owner.isAlly(fig->getPlayer()) && !fig->IsSoldier() && fig->isWalkingOnRoad())
        {
            // Then he should start wandering around
            fig->abrogate();
            fig->startWandering();
            // Not walking? (Could be carriers who are waiting for wares on roads)
            if(!fig->isMoving() && !fig->walkInRandomDir())
            {
                // No possible way found (unlikely but just in case)
                fig->die();
            }
        }
    }
}

void nofActiveSoldier::walked()
{
    switch(state)
    {
        default: return;
        case SoldierState::WalkingHome: walkingHome(); return;
        case SoldierState::MeetEnemy: meetingEnemy(); return;
    }
}

bool nofActiveSoldier::tryFightingNearbyEnemy(const std::optional<unsigned char>& excludedOwner)
{
    RTTR_Assert(enemy == nullptr);
    enemy = nullptr;

    const GamePlayer& owner = world->getPlayer(player);
    // Check all close points
    for(const auto& curPos : world->GetPointsInRadiusWithCenter(pos, 2))
    {
        for(noBase& object : world->getFigures(curPos))
        {
            auto* soldier = dynamic_cast<nofActiveSoldier*>(&object);
            if(!soldier || soldier->getPlayer() == excludedOwner)
                continue;
            if(soldier->isReadyForFight() && !owner.isAlly(soldier->getPlayer()))
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
    } else if(const auto fightSpot = getFightSpotNear(*enemy))
        fightSpot_ = *fightSpot;
    else
    {
        // No success? Then no fight
        enemy = nullptr;
        return false;
    }

    // We try to meet now
    state = SoldierState::MeetEnemy;
    enemy->meetEnemy(*this, fightSpot_);

    // Walk to him
    meetingEnemy();

    return true;
}

void nofActiveSoldier::increaseRank()
{
    if(getRank() < world->getGGS().getMaxMilitaryRank())
    {
        // Promote and adjust inventory accordingly
        world->getPlayer(player).decreaseInventoryJob(job_, 1);
        job_ = Job(rttr::enum_cast(job_) + 1);
        world->getPlayer(player).increaseInventoryJob(job_, 1);
    }
}

void nofActiveSoldier::meetingEnemy()
{
    // At this point we must still have an enemy which we were walking towards
    RTTR_Assert(enemy);

    // Reached the fighting place?
    if(getPos() == fightSpot_)
    {
        // Enemy already there?
        if(enemy->getState() == SoldierState::WaitingForFight)
        {
            RTTR_Assert(enemy->getPos() == fightSpot_);
            // Start fighting
            world->addFigure(pos, std::make_unique<noFighting>(*enemy, *this));

            enemy->fightingStarted();
            fightingStarted();
        } else
        {
            // Is fighting point still valid (there could e.g. be another fight already)
            if(world->isValidPointForFighting(pos, *this, false))
            {
                // Enemy should be on the way, so wait here
                RTTR_Assert(enemy->enemy == this);
                RTTR_Assert(enemy->getState() == SoldierState::MeetEnemy);
                state = SoldierState::WaitingForFight;
            } else
            {
                abortFreeFight();
                walked();
            }
        }
    } else
    {
        // Not at the fighting spot yet, continue walking there
        const auto dir = world->findHumanPath(pos, fightSpot_, MAX_ATTACKING_RUN_DISTANCE);
        if(dir)
            startWalking(*dir);
        else
        {
            // No way from current location to fighting spot -> cancel fight
            abortFreeFight();
            walked();
        }
    }
}

void nofActiveSoldier::abortFreeFight()
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
    state = freeFightAborted();
    if(old_enemy)
        old_enemy->state = old_enemy->freeFightAborted();
    // Now the (possibly) 2 soldiers are in a state where they could even engage in another fight with each other
    if(enemyWasWaiting)
    {
        // Act as-if the enemy just arrived at his position so he'll start moving again if required
        RTTR_Assert(!old_enemy->isMoving());
        old_enemy->walked();
    }
}

void nofActiveSoldier::informTargetsAboutCancelling()
{
    if(enemy)
        abortFreeFight();
}

void nofActiveSoldier::takeHit()
{
    if(hasArmor_)
    {
        hasArmor_ = false;
        world->getPlayer(player).decreaseInventoryJob(jobEnumToAmoredSoldierEnum(getJobType()), 1);
    } else
    {
        RTTR_Assert(hitpoints > 0u);
        --hitpoints;
    }
}

bool nofActiveSoldier::isReadyForFight() const
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

void nofActiveSoldier::meetEnemy(nofActiveSoldier& other, const MapPoint figh_spot)
{
    enemy = &other;
    fightSpot_ = figh_spot;

    SoldierState old_state = state;
    state = SoldierState::MeetEnemy;

    // In some cases we have to start walking
    if(old_state == SoldierState::AttackingWaitingAroundBuilding)
        meetingEnemy();
}

std::optional<MapPoint> nofActiveSoldier::getFightSpotNear(const nofActiveSoldier& other)
{
    // Calc middle between the two soldiers and use this as origin spot for the search of more fight spots
    MapPoint otherPos = world->getNeighbour(other.getPos(), other.getCurMoveDir());
    MapPoint middle((pos + otherPos) / 2u);

    // The point is supposed to be in the middle between the 2 soldiers (and guaranteed to be inside the map)
    // Maximum distance between 2 points is mapSize/2 (due to wrap around)
    // --> maximum distance between each point and the middle is mapSize/4
    // So if we see, that this is not the case, we take the "middle" point on the other half of the map

    const unsigned short mapWidth = world->getWidth();
    const unsigned short mapHeight = world->getHeight();

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
    RTTR_Assert(world->calcDistance(otherPos, middle) <= std::max<unsigned>(mapWidth, mapHeight) / 4u);

    const auto isGoodFightingSpot = [world = this->world, pos = this->pos, this, &other](MapPoint pt) {
        return world->isValidPointForFighting(pt, *this, true)
               && (pos == pt || world->findHumanPath(pos, pt, MEET_FOR_FIGHT_DISTANCE * 2, false))
               && (other.getPos() == pt
                   || world->findHumanPath(other.getPos(), pt, MEET_FOR_FIGHT_DISTANCE * 2, false));
    };
    const std::vector<MapPoint> pts =
      world->GetMatchingPointsInRadius<1>(middle, MEET_FOR_FIGHT_DISTANCE, isGoodFightingSpot, true);
    if(pts.empty())
        return std::nullopt;
    else
        return pts.front();
}

void nofActiveSoldier::fightingStarted()
{
    state = SoldierState::Fighting;
    enemy = nullptr;
}
