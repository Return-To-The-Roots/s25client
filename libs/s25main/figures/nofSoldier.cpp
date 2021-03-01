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

#include "nofSoldier.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "world/GameWorldGame.h"
#include "gameTypes/JobTypes.h"
#include "gameData/MilitaryConsts.h"

nofSoldier::nofSoldier(const MapPoint pos, const unsigned char player, nobBaseMilitary* const goal,
                       nobBaseMilitary* const home, const unsigned char rank)
    : noFigure(SOLDIER_JOBS[rank], pos, player, goal), building(home), hitpoints(HITPOINTS[rank])
{
    RTTR_Assert(IsSoldier());
}

nofSoldier::nofSoldier(const MapPoint pos, const unsigned char player, nobBaseMilitary* const home,
                       const unsigned char rank)
    : noFigure(SOLDIER_JOBS[rank], pos, player), building(home), hitpoints(HITPOINTS[rank])
{
    RTTR_Assert(IsSoldier());
}

void nofSoldier::Serialize(SerializedGameData& sgd) const
{
    noFigure::Serialize(sgd);

    if(fs != FigureState::Wander && fs != FigureState::GoHome)
        sgd.PushObject(building, false);

    sgd.PushUnsignedChar(hitpoints);
}

nofSoldier::nofSoldier(SerializedGameData& sgd, const unsigned obj_id) : noFigure(sgd, obj_id)
{
    RTTR_Assert(IsSoldier());

    if(fs != FigureState::Wander && fs != FigureState::GoHome)
        building = sgd.PopObject<nobBaseMilitary>(GO_Type::Unknown);
    else
        building = nullptr;

    hitpoints = sgd.PopUnsignedChar();
}

void nofSoldier::DrawSoldierWaiting(DrawPoint drawPt)
{
    const GamePlayer& owner = gwg->GetPlayer(player);
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
