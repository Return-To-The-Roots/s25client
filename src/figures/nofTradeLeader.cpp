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

#include "defines.h" // IWYU pragma: keep
#include "nofTradeLeader.h"
#include "nofTradeDonkey.h"
#include "world/GameWorldGame.h"
#include "buildings/nobBaseWarehouse.h"
#include "SerializedGameData.h"
#include "GamePlayer.h"
#include "gameData/GameConsts.h"

nofTradeLeader::nofTradeLeader(const MapPoint pos, const unsigned char player, const TradeRoute& tr, const MapPoint homePos, const MapPoint goalPos)
    : noFigure(JOB_HELPER, pos, player), tr(tr), successor(NULL), homePos(homePos), goalPos(goalPos)
{}

nofTradeLeader::nofTradeLeader(SerializedGameData& sgd, const unsigned obj_id)
    : noFigure(sgd, obj_id),
      tr(sgd, *gwg, player),
      successor(sgd.PopObject<nofTradeDonkey>(GOT_NOF_TRADEDONKEY)),
      homePos(sgd.PopMapPoint()),
      goalPos(sgd.PopMapPoint())
{
}


void nofTradeLeader::Serialize(SerializedGameData& sgd) const
{
    Serialize_noFigure(sgd);

    tr.Serialize(sgd);

    sgd.PushObject(successor, true);
    sgd.PushMapPoint(homePos);
    sgd.PushMapPoint(goalPos);
}

void nofTradeLeader::GoalReached()
{
    if(successor){
        successor->AddNextDir(REACHED_GOAL);
        successor = NULL;
    }

    noBase* nob = gwg->GetNO(goalPos);
    RTTR_Assert(dynamic_cast<nobBaseWarehouse*>(nob));
    gwg->GetPlayer(static_cast<nobBaseWarehouse*>(nob)->GetPlayer()).IncreaseInventoryJob(this->GetJobType(), 1);
    gwg->RemoveFigure(this, pos);
    static_cast<nobBaseWarehouse*>(nob)->AddFigure(this);
}

void nofTradeLeader::Walked()
{
    noBase* nob = gwg->GetNO(goalPos);

    // Does target still exist?
    if(nob->GetType() != NOP_BUILDING || !static_cast<noBuilding*>(nob)->IsWarehouse())
    {
        if(TryToGoHome())
            Walked();
        else
        {
            CancelTradeCaravane();
            WanderFailedTrade();
        }
        return;
    }else if(pos == goalPos)
        GoalReached();
    else
    {
        RTTR_Assert(pos == tr.GetCurPos());
        unsigned char next_dir = tr.GetNextDir();
        if(next_dir == INVALID_DIR)
        {
            if(TryToGoHome())
                Walked();
            else
            {
                CancelTradeCaravane();
                WanderFailedTrade();
            }
            return;
        }else if(next_dir == REACHED_GOAL)
            next_dir = Direction::NORTHWEST; // Walk into building
        StartWalking(Direction::fromInt(next_dir));
        if(successor)
            successor->AddNextDir(next_dir);
    }
}

void nofTradeLeader::HandleDerivedEvent(const unsigned  /*id*/)
{
}
void nofTradeLeader::AbrogateWorkplace()
{
}

void nofTradeLeader::Draw(DrawPoint drawPt)
{
    DrawWalking(drawPt);
}

void nofTradeLeader::LostWork()
{
}

/// Tries to go to the home ware house, otherwise start wandering
bool nofTradeLeader::TryToGoHome()
{
    if(goalPos == homePos)
        return false; // Already going home

    goalPos = homePos;

    noBase* homeWh = gwg->GetNO(goalPos);
    // Does target still exist?
    if(homeWh->GetType() != NOP_BUILDING || !static_cast<noBuilding*>(homeWh)->IsWarehouse())
        return false;

    // Find a way back home
    MapPoint homeFlagPos = gwg->GetNeighbour(homePos, 4);
    tr.AssignNewGoal(this->GetPos(), homeFlagPos);
    return tr.IsValid();
}

void nofTradeLeader::CancelTradeCaravane()
{
    if(successor)
    {
        successor->CancelTradeCaravane();
        successor = NULL;
    }
}
