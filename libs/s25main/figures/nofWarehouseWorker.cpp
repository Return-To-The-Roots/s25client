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
    : noFigure(Job::Helper, pos, player, world->GetSpecObj<noFlag>(world->GetNeighbour(pos, Direction::SouthEast))),
      carried_ware(std::move(ware)), shouldBringWareIn(task), fat((RANDOM_RAND(2)) != 0)
{
    // New figure
    world->GetPlayer(player).IncreaseInventoryJob(Job::Helper, 1);

    /// Set the road to building flag (1-piece)
    cur_rs = static_cast<noFlag*>(GetGoal())->GetRoute(Direction::NorthWest);
    RTTR_Assert(cur_rs->GetLength() == 1);
    rs_dir = true;
}

nofWarehouseWorker::~nofWarehouseWorker() = default;

void nofWarehouseWorker::Destroy()
{
    RTTR_Assert(!carried_ware); // TODO Check if this holds true and remove the LooseWare below
    LooseWare();
}

void nofWarehouseWorker::Serialize(SerializedGameData& sgd) const
{
    noFigure::Serialize(sgd);

    sgd.PushObject(carried_ware, true);
    sgd.PushBool(shouldBringWareIn);
    sgd.PushBool(fat);
}

nofWarehouseWorker::nofWarehouseWorker(SerializedGameData& sgd, const unsigned obj_id)
    : noFigure(sgd, obj_id), carried_ware(sgd.PopObject<Ware>(GO_Type::Ware)), shouldBringWareIn(sgd.PopBool()),
      fat(sgd.PopBool())
{}

void nofWarehouseWorker::Draw(DrawPoint drawPt)
{
    if(carried_ware)
        DrawWalkingCarrier(drawPt, carried_ware->type, fat);
    else
        DrawWalkingCarrier(drawPt, boost::none, fat);
}

void nofWarehouseWorker::GoalReached()
{
    nobBaseWarehouse* wh = world->GetSpecObj<nobBaseWarehouse>(world->GetNeighbour(pos, Direction::NorthWest));
    RTTR_Assert(wh); // When worker is still working, the warehouse (and its flag) exists
    auto* flag = wh->GetFlag();
    RTTR_Assert(flag);
    if(!shouldBringWareIn)
    {
        // Put ware down at flag if enough space.
        // Might need to take it back in if goal was destroyed or changed to the warehouse
        if(flag->HasSpaceForWare() && carried_ware->GetGoal() && carried_ware->GetGoal() != wh)
        {
            // TODO: Remove assert. Was added to verify prior condition
            RTTR_Assert(carried_ware->GetGoal() != carried_ware->GetLocation());

            carried_ware->WaitAtFlag(flag);
            carried_ware->RecalcRoute();
            flag->AddWare(std::move(carried_ware));
        } else
        {
            // Bring back in
            carried_ware->Carry(wh);
        }
    } else
    {
        // Take ware if any
        carried_ware = flag->SelectWare(Direction::NorthWest, false, this);
        if(carried_ware)
            carried_ware->Carry(wh);
    }

    // Start walking back
    StartWalking(Direction::NorthWest);
    InitializeRoadWalking(wh->GetRoute(Direction::SouthEast), 0, false);
}

void nofWarehouseWorker::Walked()
{
    // Arrived back. Check if we were supposed to bring a ware or carry on out
    if(!shouldBringWareIn)
    {
        // If I still carry a ware than either the flag was full or I should not bring it there.
        // So re-add it to waiting wares or to inventory
        if(carried_ware)
        {
            // Add to warehouse inventory (if it still exists and was not burnt down)
            if(world->GetNO(pos)->GetType() == NodalObjectType::Building)
            {
                auto* wh = world->GetSpecObj<nobBaseWarehouse>(pos);
                // Store the ware if its goal is this warehouse or it has no goal (anymore)
                // Else it wants to go somewhere else, so add to waiting wares
                if(!carried_ware->GetGoal() || carried_ware->GetGoal() == wh)
                {
                    // TODO: Remove assert. Was added to verify prior condition
                    RTTR_Assert(!carried_ware->GetGoal() || carried_ware->GetGoal() == carried_ware->GetLocation());
                    wh->AddWare(std::move(carried_ware));
                } else
                    wh->AddWaitingWare(std::move(carried_ware));
            } else
                LooseWare(); // Warehouse is missing --> destroy ware
            RTTR_Assert(carried_ware == nullptr);
        }
    } else
    {
        if(carried_ware)
        {
            // Add ware to warehouse if it still exists
            if(world->GetNO(pos)->GetType() == NodalObjectType::Building)
                world->GetSpecObj<nobBaseWarehouse>(pos)->AddWare(std::move(carried_ware));
            else
                LooseWare(); // Warehouse is missing --> destroy ware
            RTTR_Assert(carried_ware == nullptr);
        }
    }

    // Remove myself
    GetEvMgr().AddToKillList(world->RemoveFigure(pos, *this));
    world->GetPlayer(player).DecreaseInventoryJob(Job::Helper, 1);
}

void nofWarehouseWorker::AbrogateWorkplace()
{
    LooseWare();
    StartWandering();
}

void nofWarehouseWorker::LooseWare()
{
    // Destroy carried ware if any
    if(carried_ware)
    {
        carried_ware->WareLost(player);
        carried_ware->Destroy();
        carried_ware.reset();
    }
}

void nofWarehouseWorker::HandleDerivedEvent(const unsigned /*id*/) {}

void nofWarehouseWorker::CarryWare(Ware* /*ware*/) {}
