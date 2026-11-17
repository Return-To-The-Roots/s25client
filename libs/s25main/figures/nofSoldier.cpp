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
    : noFigure(SOLDIER_JOBS[rank], pos, player, goal), homeBld(home), hitpoints(HITPOINTS[rank])
{
    hasArmor_ = armor;
    RTTR_Assert(isSoldier());
}

nofSoldier::nofSoldier(const MapPoint pos, const unsigned char player, nobBaseMilitary& home, const unsigned char rank,
                       bool armor)
    : noFigure(SOLDIER_JOBS[rank], pos, player), homeBld(&home), hitpoints(HITPOINTS[rank])
{
    hasArmor_ = armor;
    RTTR_Assert(isSoldier());
}

void nofSoldier::serialize(SerializedGameData& sgd) const
{
    noFigure::serialize(sgd);

    if(fs != FigureState::Wander && fs != FigureState::GoHome)
        sgd.pushObject(homeBld);

    sgd.PushUnsignedChar(hitpoints);
}

nofSoldier::nofSoldier(SerializedGameData& sgd, const unsigned obj_id) : noFigure(sgd, obj_id)
{
    RTTR_Assert(isSoldier());

    if(fs != FigureState::Wander && fs != FigureState::GoHome)
        homeBld = sgd.popObject<nobBaseMilitary>();
    else
        homeBld = nullptr;

    hitpoints = sgd.PopUnsignedChar();
}

void nofSoldier::drawSoldierWaiting(DrawPoint drawPt)
{
    const GamePlayer& owner = world->getPlayer(player);
    LOADER.getBobSprite(owner.nation, job_, getCurMoveDir(), 2).drawForPlayer(drawPt, owner.color);
    if(hasArmor_)
        this->drawArmor(drawPt);
}

void nofSoldier::abrogateWorkplace()
{
    if(homeBld)
    {
        homeBld->soldierLost(this);
        homeBld = nullptr;
    }
}

unsigned char nofSoldier::getRank() const
{
    return getSoldierRank(job_);
}

unsigned char nofSoldier::getHitpoints() const
{
    return hitpoints;
}
