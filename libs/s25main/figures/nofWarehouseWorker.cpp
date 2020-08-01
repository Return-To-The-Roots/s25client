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
#include "world/GameWorldGame.h"
#include "nodeObjs/noFlag.h"
#include "nodeObjs/noRoadNode.h"

nofWarehouseWorker::nofWarehouseWorker(const MapPoint pos, const unsigned char player, Ware* ware, const bool task)
    : noFigure(JOB_HELPER, pos, player, gwg->GetSpecObj<noFlag>(gwg->GetNeighbour(pos, Direction::SOUTHEAST))), carried_ware(ware),
      shouldBringWareIn(task), fat((RANDOM.Rand(__FILE__, __LINE__, GetObjId(), 2)) != 0)
{
    // Zur Inventur hinzufügen, sind ja sonst nicht registriert
    gwg->GetPlayer(player).IncreaseInventoryJob(JOB_HELPER, 1);

    /// Straße (also die 1-er-Straße vor dem Lagerhaus) setzen
    cur_rs = static_cast<noFlag*>(GetGoal())->GetRoute(Direction::NORTHWEST);
    RTTR_Assert(cur_rs->GetLength() == 1);
    rs_dir = true;
}

nofWarehouseWorker::~nofWarehouseWorker()
{
    // Ware vernichten (physisch)
    delete carried_ware;
}

void nofWarehouseWorker::Destroy_nofWarehouseWorker()
{
    // Ware vernichten (abmelden)
    RTTR_Assert(!carried_ware); // TODO Check if this holds true and remove the LooseWare below
    LooseWare();
}

void nofWarehouseWorker::Serialize_nofWarehouseWorker(SerializedGameData& sgd) const
{
    Serialize_noFigure(sgd);

    sgd.PushObject(carried_ware, true);
    sgd.PushBool(shouldBringWareIn);
    sgd.PushBool(fat);
}

nofWarehouseWorker::nofWarehouseWorker(SerializedGameData& sgd, const unsigned obj_id)
    : noFigure(sgd, obj_id), carried_ware(sgd.PopObject<Ware>(GOT_WARE)), shouldBringWareIn(sgd.PopBool()), fat(sgd.PopBool())
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
    const nobBaseWarehouse* wh = gwg->GetSpecObj<nobBaseWarehouse>(gwg->GetNeighbour(pos, Direction::NORTHWEST));
    if(!shouldBringWareIn)
    {
        // Ware an der Fahne ablegen ( wenn noch genug Platz ist, 8 max pro Flagge!)
        // außerdem ggf. Waren wieder mit reinnehmen, deren Zi­el zerstört wurde
        // ( dann ist goal = location )
        if(gwg->GetSpecObj<noFlag>(pos)->GetNumWares() < 8 && carried_ware->GetGoal() != carried_ware->GetLocation()
           && carried_ware->GetGoal() != wh)
        {
            carried_ware->WaitAtFlag(gwg->GetSpecObj<noFlag>(pos));

            // Ware soll ihren weiteren Weg berechnen
            carried_ware->RecalcRoute();

            // Ware ablegen
            gwg->GetSpecObj<noFlag>(pos)->AddWare(carried_ware);

            // Ich trage keine Ware mehr
            carried_ware = nullptr;
        } else
            // ansonsten Ware wieder mit reinnehmen
            carried_ware->Carry(gwg->GetSpecObj<noRoadNode>(gwg->GetNeighbour(pos, Direction::NORTHWEST)));
    } else
    {
        // Ware aufnehmen
        carried_ware = gwg->GetSpecObj<noFlag>(pos)->SelectWare(Direction::NORTHWEST, false, this);

        if(carried_ware)
            carried_ware->Carry(gwg->GetSpecObj<noRoadNode>(gwg->GetNeighbour(pos, Direction::NORTHWEST)));
    }

    // Wieder ins Schloss gehen
    StartWalking(Direction::NORTHWEST);
    InitializeRoadWalking(wh->GetRoute(Direction::SOUTHEAST), 0, false);
}

void nofWarehouseWorker::Walked()
{
    // Wieder im Schloss angekommen
    if(!shouldBringWareIn)
    {
        // If I still cary a ware than either the flag was full or I should not bring it there (goal=warehouse or goal destroyed ->
        // goal=location) So re-add it to waiting wares or to inventory
        if(carried_ware)
        {
            // Ware ins Lagerhaus einlagern (falls es noch existiert und nicht abgebrannt wurde)
            if(gwg->GetNO(pos)->GetType() == NOP_BUILDING)
            {
                auto* wh = gwg->GetSpecObj<nobBaseWarehouse>(pos);
                if(carried_ware->GetGoal() == carried_ware->GetLocation() || carried_ware->GetGoal() == wh)
                    wh->AddWare(carried_ware);
                else
                    wh->AddWaitingWare(carried_ware);
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
            if(gwg->GetNO(pos)->GetType() == NOP_BUILDING)
                gwg->GetSpecObj<nobBaseWarehouse>(pos)->AddWare(carried_ware);
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
    gwg->RemoveFigure(pos, this);
    GetEvMgr().AddToKillList(this);

    // Von der Inventur wieder abziehen
    gwg->GetPlayer(player).DecreaseInventoryJob(JOB_HELPER, 1);
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
        deletePtr(carried_ware);
    }
}

void nofWarehouseWorker::HandleDerivedEvent(const unsigned /*id*/) {}

void nofWarehouseWorker::CarryWare(Ware* /*ware*/) {}
