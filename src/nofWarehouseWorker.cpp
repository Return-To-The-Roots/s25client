// $Id: nofWarehouseWorker.cpp 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "main.h"
#include "nofWarehouseWorker.h"

#include "Loader.h"
#include "Ware.h"
#include "GameWorld.h"
#include "noRoadNode.h"
#include "noFlag.h"
#include "nobBaseWarehouse.h"
#include "Random.h"
#include "EventManager.h"
#include "SerializedGameData.h"

#include "glSmartBitmap.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

nofWarehouseWorker::nofWarehouseWorker(const unsigned short x, const unsigned short y, const unsigned char player, Ware* ware, const bool task)
    : noFigure(JOB_HELPER, x, y, player, gwg->GetSpecObj<noRoadNode>(gwg->GetXA(x, y, 4), gwg->GetYA(x, y, 4))),
      carried_ware(ware), task(task), fat((RANDOM.Rand(__FILE__, __LINE__, obj_id, 2)) ? true : false)
{
    // Zur Inventur hinzufügen, sind ja sonst nicht registriert
    gwg->GetPlayer(player)->IncreaseInventoryJob(JOB_HELPER, 1);

    /// Straße (also die 1-er-Straße vor dem Lagerhaus) setzen
    assert(gwg->GetSpecObj<noFlag>(gwg->GetXA(x, y, 4), gwg->GetYA(x, y, 4))->routes[1]->GetLength() == 1);
    cur_rs = gwg->GetSpecObj<noFlag>(gwg->GetXA(x, y, 4), gwg->GetYA(x, y, 4))->routes[1];
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

    if(carried_ware)
    {
        gwg->GetPlayer(player)->RemoveWare(carried_ware);
        gwg->GetPlayer(player)->DecreaseInventoryWare(carried_ware->type, 1);
    }
}


void nofWarehouseWorker::Serialize_nofWarehouseWorker(SerializedGameData* sgd) const
{
    Serialize_noFigure(sgd);

    sgd->PushObject(carried_ware, true);
    sgd->PushBool(task);
    sgd->PushBool(fat);
}

nofWarehouseWorker::nofWarehouseWorker(SerializedGameData* sgd, const unsigned obj_id) : noFigure(sgd, obj_id),
    carried_ware(sgd->PopObject<Ware>(GOT_WARE)),
    task(sgd->PopBool()),
    fat(sgd->PopBool())
{
}


void nofWarehouseWorker::Draw(int x, int y)
{
    // Trage ich ne Ware oder nicht?
    if(carried_ware)
        DrawWalkingBobCarrier(x, y, carried_ware->type, fat);
//      // Japaner-Schild-Animation existiert leider nicht --> Römerschild nehmen
//      DrawWalking(x,y,LOADER.GetBobN("carrier"),(carried_ware->type==GD_SHIELDJAPANESE) ? GD_SHIELDROMANS:carried_ware->type,fat);
    else
        DrawWalkingBobJobs(x, y, fat ? JOB_TYPES_COUNT : 0);
//      DrawWalking(x,y,LOADER.GetBobN("jobs"),0,fat);
}

void nofWarehouseWorker::GoalReached()
{
    if(!task)
    {
        // Ware an der Fahne ablegen ( wenn noch genug Platz ist, 8 max pro Flagge!)
        // außerdem ggf. Waren wieder mit reinnehmen, deren Zíel zerstört wurde
        // ( dann ist goal = location )
        if(gwg->GetSpecObj<noFlag>(x, y)->GetWareCount() < 8 && carried_ware->goal != carried_ware->GetLocation())
        {
            carried_ware->LieAtFlag(gwg->GetSpecObj<noRoadNode>(x, y));

            // Ware soll ihren weiteren Weg berechnen
            carried_ware->RecalcRoute();

            // Ware ablegen
            gwg->GetSpecObj<noFlag>(x, y)->AddWare(carried_ware);

            // Ich trage keine Ware mehr
            carried_ware = 0;
        }
        else
            // ansonsten Ware wieder mit reinnehmen
            carried_ware->Carry(gwg->GetSpecObj<noRoadNode>(gwg->GetXA(x, y, 1), gwg->GetYA(x, y, 1)));
    }
    else
    {
        // Ware aufnehmen
        carried_ware = gwg->GetSpecObj<noFlag>(x, y)->SelectWare(1, false, this);

        if (carried_ware)
            carried_ware->Carry(gwg->GetSpecObj<noRoadNode>(gwg->GetXA(x, y, 1), gwg->GetYA(x, y, 1)));
    }

    // Wieder ins Schloss gehen
    StartWalking(1);
    InitializeRoadWalking(gwg->GetSpecObj<nobBaseWarehouse>(gwg->GetXA(x, y, 1), gwg->GetYA(x, y, 1))->routes[4],
                          0, false);
}

void nofWarehouseWorker::Walked()
{
    // Wieder im Schloss angekommen

    if(!task)
    {
        // Bringe ich wieder ne Ware mit? Dann muss die mit zu den waiting_wares, da die Flagge evtl voll war
        if(carried_ware)
        {
            // Ware ins Lagerhaus einlagern (falls es noch existiert und nicht abgebrannt wurde)
            if(gwg->GetNO(x, y)->GetType() == NOP_BUILDING)
                gwg->GetSpecObj<nobBaseWarehouse>(x, y)->AddWaitingWare(carried_ware);
            else
            {
                // Lagerhaus abgebrannt --> Ware vernichten
                carried_ware->WareLost(player);
                delete carried_ware;
            }
            // Ich trage keine Ware mehr
            carried_ware = 0;
        }
    }
    else
    {
        if(carried_ware)
        {
            // Ware ins Lagerhaus einlagern (falls es noch existiert und nicht abgebrannt wurde)
            if(gwg->GetNO(x, y)->GetType() == NOP_BUILDING)
                gwg->GetSpecObj<nobBaseWarehouse>(x, y)->AddWare(carried_ware);
            else
            {
                // Lagerhaus abgebrannt --> Ware vernichten
                carried_ware->WareLost(player);
                delete carried_ware;
            }
            // Ich trage keine Ware mehr
            carried_ware = 0;
        }
    }

    // dann mich killen
    gwg->RemoveFigure(this, x, y);
    em->AddToKillList(this);

    // Von der Inventur wieder abziehen
    gwg->GetPlayer(player)->DecreaseInventoryJob(JOB_HELPER, 1);
}

void nofWarehouseWorker::AbrogateWorkplace()
{
    // Wenn ich noch ne Ware in der Hand habe, muss die gelöscht werden
    if(carried_ware)
    {
        carried_ware->WareLost(player);
        delete carried_ware;
        carried_ware = 0;
    }

    StartWandering();
}

void nofWarehouseWorker::HandleDerivedEvent(const unsigned int id)
{
}

void nofWarehouseWorker::CarryWare(Ware* ware)
{
}
