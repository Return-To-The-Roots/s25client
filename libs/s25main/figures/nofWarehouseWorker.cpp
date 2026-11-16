// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofWarehouseWorker.h"
#include "EventManager.h"
#include "GamePlayer.h"
#include "SerializedGameData.h"
#include "Ware.h"
#include "buildings/nobBaseWarehouse.h"
#include "random/Random.h"
#include "world/GameWorld.h"
#include "nodeObjs/noFlag.h"
#include "nodeObjs/noRoadNode.h"

nofWarehouseWorker::nofWarehouseWorker(const MapPoint pos, const unsigned char player, std::unique_ptr<Ware> ware,
                                       const bool task)
    : noFigure(Job::Helper, pos, player, world->getSpecObj<noFlag>(world->getNeighbour(pos, Direction::SouthEast))),
      carried_ware(std::move(ware)), shouldBringWareIn(task), fat((RANDOM_RAND(2)) != 0)
{
    // New figure
    world->getPlayer(player).increaseInventoryJob(Job::Helper, 1);

    /// Set the road to building flag (1-piece)
    cur_rs = static_cast<noFlag*>(getGoal())->getRoute(Direction::NorthWest);
    RTTR_Assert(cur_rs->GetLength() == 1);
    rs_dir = true;
}

nofWarehouseWorker::~nofWarehouseWorker() = default;

void nofWarehouseWorker::destroy()
{
    RTTR_Assert(!carried_ware); // TODO Check if this holds true and remove the LooseWare below
    looseWare();
}

void nofWarehouseWorker::serialize(SerializedGameData& sgd) const
{
    noFigure::serialize(sgd);

    sgd.pushObject(carried_ware, true);
    sgd.PushBool(shouldBringWareIn);
    sgd.PushBool(fat);
}

nofWarehouseWorker::nofWarehouseWorker(SerializedGameData& sgd, const unsigned obj_id)
    : noFigure(sgd, obj_id), carried_ware(sgd.popObject<Ware>(GO_Type::Ware)), shouldBringWareIn(sgd.PopBool()),
      fat(sgd.PopBool())
{}

void nofWarehouseWorker::draw(DrawPoint drawPt)
{
    if(carried_ware)
        drawWalkingCarrier(drawPt, carried_ware->type, fat);
    else
        drawWalkingCarrier(drawPt, std::nullopt, fat);
}

void nofWarehouseWorker::goalReached()
{
    // When worker is still working, the warehouse (and its flag) exists
    auto& wh = assertNonNull(world->getSpecObj<nobBaseWarehouse>(world->getNeighbour(pos, Direction::NorthWest)));
    auto& flag = assertNonNull(wh.getFlag());
    if(!shouldBringWareIn)
    {
        // Put ware down at flag if enough space.
        // Might need to take it back in if goal was destroyed or changed to the warehouse
        if(flag.hasSpaceForWare() && carried_ware->getGoal() && carried_ware->getGoal() != &wh)
        {
            // TODO: Remove assert. Was added to verify prior condition
            RTTR_Assert(carried_ware->getGoal() != carried_ware->getLocation());

            carried_ware->waitAtFlag(flag);
            carried_ware->recalcRoute();
            flag.addWare(std::move(carried_ware));
        } else
        {
            // Bring back in
            carried_ware->carry(wh);
        }
    } else
    {
        // Take ware if any
        carried_ware = flag.selectWare(Direction::NorthWest, false, this);
        if(carried_ware)
            carried_ware->carry(wh);
    }

    // Start walking back
    startWalking(Direction::NorthWest);
    initializeRoadWalking(wh.getRoute(Direction::SouthEast), 0, false);
}

void nofWarehouseWorker::walked()
{
    // Arrived back. Check if we were supposed to bring a ware or carry on out
    if(!shouldBringWareIn)
    {
        // If I still carry a ware than either the flag was full or I should not bring it there.
        // So re-add it to waiting wares or to inventory
        if(carried_ware)
        {
            // Add to warehouse inventory (if it still exists and was not burnt down)
            if(world->getNO(pos)->getType() == NodalObjectType::Building)
            {
                auto* wh = world->getSpecObj<nobBaseWarehouse>(pos);
                // Store the ware if its goal is this warehouse or it has no goal (anymore)
                // Else it wants to go somewhere else, so add to waiting wares
                if(!carried_ware->getGoal() || carried_ware->getGoal() == wh)
                {
                    // TODO: Remove assert. Was added to verify prior condition
                    RTTR_Assert(!carried_ware->getGoal() || carried_ware->getGoal() == carried_ware->getLocation());
                    wh->addWare(std::move(carried_ware));
                } else
                    wh->addWaitingWare(std::move(carried_ware));
            } else
                looseWare(); // Warehouse is missing --> destroy ware
            RTTR_Assert(carried_ware == nullptr);
        }
    } else
    {
        if(carried_ware)
        {
            // Add ware to warehouse if it still exists
            if(world->getNO(pos)->getType() == NodalObjectType::Building)
                world->getSpecObj<nobBaseWarehouse>(pos)->addWare(std::move(carried_ware));
            else
                looseWare(); // Warehouse is missing --> destroy ware
            RTTR_Assert(carried_ware == nullptr);
        }
    }

    // Remove myself
    getEvMgr().addToKillList(world->RemoveFigure(pos, *this));
    world->getPlayer(player).decreaseInventoryJob(Job::Helper, 1);
}

void nofWarehouseWorker::abrogateWorkplace()
{
    looseWare();
    startWandering();
}

void nofWarehouseWorker::looseWare()
{
    // Destroy carried ware if any
    if(carried_ware)
    {
        carried_ware->wareLost(player);
        carried_ware->destroy();
        carried_ware.reset();
    }
}

void nofWarehouseWorker::handleDerivedEvent(const unsigned /*id*/) {}

void nofWarehouseWorker::carryWare(Ware* /*ware*/) {}
