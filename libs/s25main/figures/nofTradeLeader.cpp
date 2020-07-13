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

#include "commonDefines.h"
#include "nofTradeLeader.h"
#include "EventManager.h"
#include "GameObject.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "buildings/nobBaseWarehouse.h"
#include "nofTradeDonkey.h"
#include "postSystem/PostMsgWithBuilding.h"
#include "world/GameWorldGame.h"
#include "gameData/BuildingProperties.h"
#include "gameData/GameConsts.h"
#include "gameData/JobConsts.h"
#include "gameData/NationConsts.h"
#include <boost/format.hpp>
#include <utility>

nofTradeLeader::nofTradeLeader(const MapPoint pos, const unsigned char player, TradeRoute tr, const MapPoint homePos,
                               const MapPoint goalPos)
    : noFigure(JOB_HELPER, pos, player), tr(std::move(tr)), successor(nullptr), homePos(homePos), goalPos(goalPos)
{}

nofTradeLeader::nofTradeLeader(SerializedGameData& sgd, const unsigned obj_id)
    : noFigure(sgd, obj_id), tr(sgd, *gwg, player), successor(sgd.PopObject<nofTradeDonkey>(GOT_NOF_TRADEDONKEY)),
      homePos(sgd.PopMapPoint()), goalPos(sgd.PopMapPoint())
{}

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
    noBase* nob = gwg->GetNO(goalPos);
    auto* targetWarehouse = checkedCast<nobBaseWarehouse*>(nob);
    if(successor)
    {
        unsigned amountWares = 0;
        Job jobType = successor->GetJobType();
        GoodType goodType = successor->GetCarriedWare();
        nofTradeDonkey* successorDonkey = successor;
        while(successorDonkey != nullptr)
        {
            amountWares++;
            successorDonkey = successorDonkey->GetSuccessor();
        }
        GamePlayer& owner = gwg->GetPlayer(player);
        std::string waresName = _(goodType == GD_NOTHING ? JOB_NAMES[jobType] : WARE_NAMES[goodType]);
        std::string text =
          str(boost::format(_("Trade caravan with %s %s arrives from player '%s'.")) % amountWares % waresName % owner.name);
        SendPostMessage(targetWarehouse->GetPlayer(),
                        std::make_unique<PostMsgWithBuilding>(GetEvMgr().GetCurrentGF(), text, PostCategory::Economy, *targetWarehouse));
        successor->AddNextDir(REACHED_GOAL);
        successor = nullptr;
    }

    gwg->GetPlayer(targetWarehouse->GetPlayer()).IncreaseInventoryJob(this->GetJobType(), 1);
    gwg->RemoveFigure(pos, this);
    targetWarehouse->AddFigure(this);
}

void nofTradeLeader::Walked()
{
    noBase* nob = gwg->GetNO(goalPos);

    // Does target still exist?
    if(nob->GetType() != NOP_BUILDING || !BuildingProperties::IsWareHouse(static_cast<noBuilding*>(nob)->GetBuildingType()))
    {
        if(TryToGoHome())
            Walked();
        else
        {
            CancelTradeCaravane();
            WanderFailedTrade();
        }
        return;
    } else if(pos == goalPos)
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
        } else if(next_dir == REACHED_GOAL)
            next_dir = Direction::NORTHWEST; // Walk into building
        StartWalking(Direction::fromInt(next_dir));
        if(successor)
            successor->AddNextDir(next_dir);
    }
}

void nofTradeLeader::HandleDerivedEvent(const unsigned /*id*/) {}
void nofTradeLeader::AbrogateWorkplace() {}

void nofTradeLeader::Draw(DrawPoint drawPt)
{
    DrawWalkingBobJobs(drawPt, JOB_SCOUT);
}

void nofTradeLeader::LostWork() {}

/// Tries to go to the home ware house, otherwise start wandering
bool nofTradeLeader::TryToGoHome()
{
    if(goalPos == homePos)
        return false; // Already going home

    goalPos = homePos;

    noBase* homeWh = gwg->GetNO(goalPos);
    // Does target still exist?
    if(homeWh->GetType() != NOP_BUILDING || !BuildingProperties::IsWareHouse(static_cast<noBuilding*>(homeWh)->GetBuildingType()))
        return false;

    // Find a way back home
    MapPoint homeFlagPos = gwg->GetNeighbour(homePos, Direction::SOUTHEAST);
    tr.AssignNewGoal(this->GetPos(), homeFlagPos);
    return tr.IsValid();
}

void nofTradeLeader::CancelTradeCaravane()
{
    if(successor)
    {
        successor->CancelTradeCaravane();
        successor = nullptr;
    }
}
