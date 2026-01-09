// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofSoldier.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "world/GameWorld.h"
#include "gameTypes/JobTypes.h"
#include "gameData/MilitaryConsts.h"

nofSoldier::nofSoldier(const MapPoint pos, const unsigned char player, nobBaseMilitary* const goal,
                       nobBaseMilitary* const home, const unsigned char rank, bool armor)
    : nofArmored(SOLDIER_JOBS[rank], pos, player, goal, armor), building(home), hitpoints(HITPOINTS[rank])
{
    RTTR_Assert(IsSoldier());
}

nofSoldier::nofSoldier(const MapPoint pos, const unsigned char player, nobBaseMilitary& home, const unsigned char rank,
                       bool armor)
    : nofArmored(SOLDIER_JOBS[rank], pos, player, armor), building(&home), hitpoints(HITPOINTS[rank])
{
    RTTR_Assert(IsSoldier());
}

void nofSoldier::Serialize(SerializedGameData& sgd) const
{
    nofArmored::Serialize(sgd);

    if(fs != FigureState::Wander && fs != FigureState::GoHome)
        sgd.PushObject(building);

    sgd.PushUnsignedChar(hitpoints);
}

nofSoldier::nofSoldier(SerializedGameData& sgd, const unsigned obj_id) : nofArmored(sgd, obj_id)
{
    RTTR_Assert(IsSoldier());

    if(fs != FigureState::Wander && fs != FigureState::GoHome)
        building = sgd.PopObject<nobBaseMilitary>();
    else
        building = nullptr;

    hitpoints = sgd.PopUnsignedChar();
}

void nofSoldier::DrawSoldierWaiting(DrawPoint drawPt)
{
    const GamePlayer& owner = world->GetPlayer(player);
    LOADER.getBobSprite(owner.nation, job_, GetCurMoveDir(), 2).drawForPlayer(drawPt, owner.color);
}

void nofSoldier::AbrogateWorkplace()
{
    // Militärgebäude Bescheid sagen, dass ich nicht kommen kann
    if(building)
    {
        building->SoldierLost(this);
        building = nullptr;
    }
}

unsigned char nofSoldier::GetRank() const
{
    return getSoldierRank(job_);
}

unsigned char nofSoldier::GetHitpoints() const
{
    return hitpoints;
}
