// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofPassiveSoldier.h"
#include "EventManager.h"
#include "GamePlayer.h"
#include "SerializedGameData.h"
#include "buildings/nobMilitary.h"
#include "helpers/containerUtils.h"
#include "helpers/pointerContainerUtils.h"
#include "random/Random.h"
#include "world/GameWorld.h"
#include "gameData/MilitaryConsts.h"

nofPassiveSoldier::nofPassiveSoldier(const nofSoldier& soldier) : nofSoldier(soldier), healing_event(nullptr)
{
    if(hitpoints > HITPOINTS[GetRank()])
        hitpoints = HITPOINTS[GetRank()];
    // Soldier returned home from a mission --> heal if necessary!
    Heal();
    // Reset walk event; they no longer walk
    current_ev = nullptr;
}

nofPassiveSoldier::nofPassiveSoldier(const MapPoint pos, const unsigned char player, nobBaseMilitary* const goal,
                                     nobMilitary* const home, const unsigned char rank)
    : nofSoldier(pos, player, goal, home, rank), healing_event(nullptr)
{}

nofPassiveSoldier::~nofPassiveSoldier() = default;

void nofPassiveSoldier::Destroy()
{
    GetEvMgr().RemoveEvent(healing_event);
    nofSoldier::Destroy();
}

void nofPassiveSoldier::Serialize(SerializedGameData& sgd) const
{
    nofSoldier::Serialize(sgd);

    sgd.PushEvent(healing_event);
}

nofPassiveSoldier::nofPassiveSoldier(SerializedGameData& sgd, const unsigned obj_id)
    : nofSoldier(sgd, obj_id), healing_event(sgd.PopEvent())
{}

void nofPassiveSoldier::Draw(DrawPoint drawPt)
{
    // Draw soldier walking normally
    DrawWalkingBobJobs(drawPt, job_);
}

void nofPassiveSoldier::HandleDerivedEvent(const unsigned id)
{
    switch(id)
    {
        // "Healing event"
        case 1:
        {
            healing_event = nullptr;

            // Are we still inside the house?
            if(fs == FigureState::Job)
            {
                // Then heal if we are not already healthy
                if(hitpoints < HITPOINTS[GetRank()])
                {
                    ++hitpoints;

                    // Still not healthy? Then schedule a new event!
                    if(hitpoints < HITPOINTS[GetRank()])
                        healing_event = GetEvMgr().AddEvent(this, GetHealingInterval(), 1);
                }
            }
        }
        break;
    }
}

void nofPassiveSoldier::Heal()
{
    // Already registered an event?
    // Then remove it; he was probably outside before
    if(healing_event)
    {
        GetEvMgr().RemoveEvent(healing_event);
        healing_event = nullptr;
    }

    // Is he injured?
    // Then he needs to be healed
    if(hitpoints < HITPOINTS[GetRank()])
        healing_event = GetEvMgr().AddEvent(this, GetHealingInterval(), 1);
}

unsigned nofPassiveSoldier::GetHealingInterval() const
{
    unsigned interval = CONVALESCE_TIME + RANDOM_RAND(CONVALESCE_TIME_RANDOM);
    const nobBaseMilitary* current_building = building;
    if(current_building)
    {
        if(current_building->GetOriginOwner() == GetPlayer())
        {
            interval /= 2;
            if(interval == 0)
                interval = 1;
        } else
        {
            interval *= 2;
        }
    }

    return interval;
}

void nofPassiveSoldier::GoalReached()
{
    static_cast<nobMilitary*>(building)->AddPassiveSoldier(world->RemoveFigure(pos, *this));
}

void nofPassiveSoldier::LeaveBuilding()
{
    // Go home to a warehouse
    rs_dir = true;
    rs_pos = 1;
    cur_rs = building->GetRoute(Direction::SouthEast);
    GoHome();

    building = nullptr;
}

void nofPassiveSoldier::Upgrade()
{
    // During upgrading we must not stay in the building list because it would break the sorted order
    RTTR_Assert(!building || !static_cast<nobMilitary*>(building)->IsInTroops(*this));
    // One rank higher
    job_ = Job(unsigned(job_) + 1);

    // Heal again and adjust hitpoints
    GamePlayer& owner = world->GetPlayer(player);
    hitpoints = HITPOINTS[GetRank()];

    // Adjust inventory up and down accordingly
    owner.IncreaseInventoryJob(job_, 1);
    owner.DecreaseInventoryJob(Job(unsigned(job_) - 1), 1);
}

void nofPassiveSoldier::Walked()
{
    throw std::logic_error("Passive soldiers shall not walk");
}

void nofPassiveSoldier::NotNeeded()
{
    building = nullptr;
    GoHome();
}

nobMilitary* nofPassiveSoldier::getHome() const
{
    return checkedCast<nobMilitary*>(building);
}
