// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofTradeLeader.h"
#include "EventManager.h"
#include "GameObject.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "buildings/nobBaseWarehouse.h"
#include "commonDefines.h"
#include "nofTradeDonkey.h"
#include "postSystem/PostMsgWithBuilding.h"
#include "world/GameWorld.h"
#include "gameData/BuildingProperties.h"
#include "gameData/GameConsts.h"
#include "gameData/GoodConsts.h"
#include "gameData/JobConsts.h"
#include "gameData/NationConsts.h"
#include <boost/format.hpp>
#include <utility>

nofTradeLeader::nofTradeLeader(const MapPoint pos, const unsigned char player, TradeRoute tr, const MapPoint homePos,
                               const MapPoint goalPos)
    : noFigure(Job::Helper, pos, player), tr(std::move(tr)), successor(nullptr), homePos(homePos), goalPos(goalPos)
{}

nofTradeLeader::nofTradeLeader(SerializedGameData& sgd, const unsigned obj_id)
    : noFigure(sgd, obj_id), tr(sgd, *world, player), successor(sgd.PopObject<nofTradeDonkey>(GO_Type::NofTradedonkey)),
      homePos(sgd.PopMapPoint()), goalPos(sgd.PopMapPoint())
{}

void nofTradeLeader::Serialize(SerializedGameData& sgd) const
{
    noFigure::Serialize(sgd);

    tr.Serialize(sgd);

    sgd.PushObject(successor, true);
    helpers::pushPoint(sgd, homePos);
    helpers::pushPoint(sgd, goalPos);
}

void nofTradeLeader::GoalReached()
{
    noBase* nob = world->GetNO(goalPos);
    auto* targetWarehouse = checkedCast<nobBaseWarehouse*>(nob);
    if(successor)
    {
        unsigned amountWares = 0;
        Job jobType = successor->GetJobType();
        const auto goodType = successor->GetCarriedWare();
        nofTradeDonkey* successorDonkey = successor;
        while(successorDonkey != nullptr)
        {
            amountWares++;
            successorDonkey = successorDonkey->GetSuccessor();
        }
        GamePlayer& owner = world->GetPlayer(player);
        std::string waresName = _(!goodType ? JOB_NAMES[jobType] : WARE_NAMES[*goodType]);
        std::string text = str(boost::format(_("Trade caravan with %s %s arrives from player '%s'.")) % amountWares
                               % waresName % owner.name);
        SendPostMessage(targetWarehouse->GetPlayer(),
                        std::make_unique<PostMsgWithBuilding>(GetEvMgr().GetCurrentGF(), text, PostCategory::Economy,
                                                              *targetWarehouse));
        successor->AddNextDir(TradeDirection::ReachedGoal);
        successor = nullptr;
    }

    world->GetPlayer(targetWarehouse->GetPlayer()).IncreaseInventoryJob(this->GetJobType(), 1);
    targetWarehouse->AddFigure(world->RemoveFigure(pos, *this));
}

void nofTradeLeader::Walked()
{
    noBase* nob = world->GetNO(goalPos);

    // Does target still exist?
    if(nob->GetType() != NodalObjectType::Building
       || !BuildingProperties::IsWareHouse(static_cast<noBuilding*>(nob)->GetBuildingType()))
    {
        if(TryToGoHome())
            Walked();
        else
        {
            CancelTradeCaravane();
            WanderFailedTrade();
        }
    } else if(pos == goalPos)
        GoalReached();
    else
    {
        RTTR_Assert(pos == tr.GetCurPos());
        auto next_dir = tr.GetNextDir();
        if(!next_dir)
        {
            if(TryToGoHome())
                Walked();
            else
            {
                CancelTradeCaravane();
                WanderFailedTrade();
            }
        } else
        {
            if(*next_dir == TradeDirection::ReachedGoal)
                next_dir = TradeDirection(Direction::NorthWest); // Walk into building
            StartWalking(toDirection(*next_dir));
            if(successor)
                successor->AddNextDir(*next_dir);
        }
    }
}

void nofTradeLeader::HandleDerivedEvent(const unsigned /*id*/) {}
void nofTradeLeader::AbrogateWorkplace() {}

void nofTradeLeader::Draw(DrawPoint drawPt)
{
    DrawWalkingBobJobs(drawPt, Job::Scout);
}

void nofTradeLeader::LostWork() {}

/// Tries to go to the home ware house, otherwise start wandering
bool nofTradeLeader::TryToGoHome()
{
    if(goalPos == homePos)
        return false; // Already going home

    goalPos = homePos;

    noBase* homeWh = world->GetNO(goalPos);
    // Does target still exist?
    if(homeWh->GetType() != NodalObjectType::Building
       || !BuildingProperties::IsWareHouse(static_cast<noBuilding*>(homeWh)->GetBuildingType()))
        return false;

    // Find a way back home
    MapPoint homeFlagPos = world->GetNeighbour(homePos, Direction::SouthEast);
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
