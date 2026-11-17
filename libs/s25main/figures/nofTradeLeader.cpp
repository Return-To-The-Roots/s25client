// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
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
    : noFigure(sgd, obj_id), tr(sgd, *world, player), successor(sgd.popObject<nofTradeDonkey>(GO_Type::NofTradedonkey)),
      homePos(sgd.popMapPoint()), goalPos(sgd.popMapPoint())
{}

void nofTradeLeader::serialize(SerializedGameData& sgd) const
{
    noFigure::serialize(sgd);

    tr.serialize(sgd);

    sgd.pushObject(successor, true);
    helpers::pushPoint(sgd, homePos);
    helpers::pushPoint(sgd, goalPos);
}

void nofTradeLeader::goalReached()
{
    noBase* nob = world->getNO(goalPos);
    auto* targetWarehouse = checkedCast<nobBaseWarehouse*>(nob);
    if(successor)
    {
        unsigned amountWares = 0;
        Job jobType = successor->getJobType();
        const auto goodType = successor->getCarriedWare();
        nofTradeDonkey* successorDonkey = successor;
        while(successorDonkey != nullptr)
        {
            amountWares++;
            successorDonkey = successorDonkey->getSuccessor();
        }
        GamePlayer& owner = world->getPlayer(player);
        std::string waresName = _(!goodType ? JOB_NAMES[jobType] : WARE_NAMES[*goodType]);
        std::string text = str(boost::format(_("Trade caravan with %s %s arrives from player '%s'.")) % amountWares
                               % waresName % owner.name);
        sendPostMessage(targetWarehouse->getPlayer(),
                        std::make_unique<PostMsgWithBuilding>(getEvMgr().getCurrentGF(), text, PostCategory::Economy,
                                                              *targetWarehouse));
        successor->addNextDir(TradeDirection::ReachedGoal);
        successor = nullptr;
    }

    world->getPlayer(targetWarehouse->getPlayer()).increaseInventoryJob(this->getJobType(), 1);
    targetWarehouse->addFigure(world->removeFigure(pos, *this));
}

void nofTradeLeader::walked()
{
    noBase* nob = world->getNO(goalPos);

    // Does target still exist?
    if(nob->getType() != NodalObjectType::Building
       || !BuildingProperties::isWareHouse(static_cast<noBuilding*>(nob)->getBuildingType()))
    {
        if(tryToGoHome())
            walked();
        else
        {
            cancelTradeCaravane();
            wanderFailedTrade();
        }
    } else if(pos == goalPos)
        goalReached();
    else
    {
        RTTR_Assert(pos == tr.getCurPos());
        auto next_dir = tr.getNextDir();
        if(!next_dir)
        {
            if(tryToGoHome())
                walked();
            else
            {
                cancelTradeCaravane();
                wanderFailedTrade();
            }
        } else
        {
            if(*next_dir == TradeDirection::ReachedGoal)
                next_dir = TradeDirection(Direction::NorthWest); // Walk into building
            startWalking(toDirection(*next_dir));
            if(successor)
                successor->addNextDir(*next_dir);
        }
    }
}

void nofTradeLeader::handleDerivedEvent(const unsigned /*id*/) {}
void nofTradeLeader::abrogateWorkplace() {}

void nofTradeLeader::draw(DrawPoint drawPt)
{
    drawWalkingBobJobs(drawPt, Job::Scout);
}

void nofTradeLeader::lostWork() {}

/// Tries to go to the home ware house, otherwise start wandering
bool nofTradeLeader::tryToGoHome()
{
    if(goalPos == homePos)
        return false; // Already going home

    goalPos = homePos;

    noBase* homeWh = world->getNO(goalPos);
    // Does target still exist?
    if(homeWh->getType() != NodalObjectType::Building
       || !BuildingProperties::isWareHouse(static_cast<noBuilding*>(homeWh)->getBuildingType()))
        return false;

    // Find a way back home
    MapPoint homeFlagPos = world->getNeighbour(homePos, Direction::SouthEast);
    tr.assignNewGoal(this->getPos(), homeFlagPos);
    return tr.isValid();
}

void nofTradeLeader::cancelTradeCaravane()
{
    if(successor)
    {
        successor->cancelTradeCaravane();
        successor = nullptr;
    }
}
