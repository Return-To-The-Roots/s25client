// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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
    // Zur Inventur hinzufügen, sind ja sonst nicht registriert
    world->GetPlayer(player).IncreaseInventoryJob(Job::Helper, 1);

    /// Straße (also die 1-er-Straße vor dem Lagerhaus) setzen
    cur_rs = static_cast<noFlag*>(GetGoal())->GetRoute(Direction::NorthWest);
    RTTR_Assert(cur_rs->GetLength() == 1);
    rs_dir = true;
}

nofWarehouseWorker::~nofWarehouseWorker() = default;

void nofWarehouseWorker::Destroy()
{
    // Ware vernichten (abmelden)
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
    // Trage ich ne Ware oder nicht?
    if(carried_ware)
        DrawWalkingCarrier(drawPt, carried_ware->type, fat);
    else
        DrawWalkingCarrier(drawPt, boost::none, fat);
}

void nofWarehouseWorker::GoalReached()
{
    const nobBaseWarehouse* wh = world->GetSpecObj<nobBaseWarehouse>(world->GetNeighbour(pos, Direction::NorthWest));
    if(!shouldBringWareIn)
    {
        // Ware an der Fahne ablegen ( wenn noch genug Platz ist, 8 max pro Flagge!)
        // außerdem ggf. Waren wieder mit reinnehmen, deren Zi­el zerstört wurde
        // ( dann ist goal = location )
        if(world->GetSpecObj<noFlag>(pos)->HasSpaceForWare() && carried_ware->GetGoal() != carried_ware->GetLocation()
           && carried_ware->GetGoal() != wh)
        {
            carried_ware->WaitAtFlag(world->GetSpecObj<noFlag>(pos));

            // Ware soll ihren weiteren Weg berechnen
            carried_ware->RecalcRoute();

            // Ware ablegen
            world->GetSpecObj<noFlag>(pos)->AddWare(std::move(carried_ware));
        } else
            // ansonsten Ware wieder mit reinnehmen
            carried_ware->Carry(world->GetSpecObj<noRoadNode>(world->GetNeighbour(pos, Direction::NorthWest)));
    } else
    {
        // Ware aufnehmen
        carried_ware = world->GetSpecObj<noFlag>(pos)->SelectWare(Direction::NorthWest, false, this);

        if(carried_ware)
            carried_ware->Carry(world->GetSpecObj<noRoadNode>(world->GetNeighbour(pos, Direction::NorthWest)));
    }

    // Wieder ins Schloss gehen
    StartWalking(Direction::NorthWest);
    InitializeRoadWalking(wh->GetRoute(Direction::SouthEast), 0, false);
}

void nofWarehouseWorker::Walked()
{
    // Wieder im Schloss angekommen
    if(!shouldBringWareIn)
    {
        // If I still cary a ware than either the flag was full or I should not bring it there (goal=warehouse or goal
        // destroyed -> goal=location) So re-add it to waiting wares or to inventory
        if(carried_ware)
        {
            // Ware ins Lagerhaus einlagern (falls es noch existiert und nicht abgebrannt wurde)
            if(world->GetNO(pos)->GetType() == NodalObjectType::Building)
            {
                auto* wh = world->GetSpecObj<nobBaseWarehouse>(pos);
                if(carried_ware->GetGoal() == carried_ware->GetLocation() || carried_ware->GetGoal() == wh)
                    wh->AddWare(std::move(carried_ware));
                else
                    wh->AddWaitingWare(std::move(carried_ware));
            } else
            {
                // Lagerhaus abgebrannt --> Ware vernichten
                LooseWare();
            }
            // Ich trage keine Ware mehr
            RTTR_Assert(carried_ware == nullptr);
        }
    } else
    {
        if(carried_ware)
        {
            // Ware ins Lagerhaus einlagern (falls es noch existiert und nicht abgebrannt wurde)
            if(world->GetNO(pos)->GetType() == NodalObjectType::Building)
                world->GetSpecObj<nobBaseWarehouse>(pos)->AddWare(std::move(carried_ware));
            else
            {
                // Lagerhaus abgebrannt --> Ware vernichten
                LooseWare();
            }
            // Ich trage keine Ware mehr
            RTTR_Assert(carried_ware == nullptr);
        }
    }

    // dann mich killen
    GetEvMgr().AddToKillList(world->RemoveFigure(pos, *this));

    // Von der Inventur wieder abziehen
    world->GetPlayer(player).DecreaseInventoryJob(Job::Helper, 1);
}

void nofWarehouseWorker::AbrogateWorkplace()
{
    LooseWare();
    StartWandering();
}

void nofWarehouseWorker::LooseWare()
{
    // Wenn ich noch ne Ware in der Hand habe, muss die gelöscht werden
    if(carried_ware)
    {
        carried_ware->WareLost(player);
        carried_ware->Destroy();
        carried_ware.reset();
    }
}

void nofWarehouseWorker::HandleDerivedEvent(const unsigned /*id*/) {}

void nofWarehouseWorker::CarryWare(Ware* /*ware*/) {}
