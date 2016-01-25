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

#include "defines.h"
#include "FindWhConditions.h"
#include "buildings/nobBaseWarehouse.h"
#include "gameData/ShieldConsts.h"
#include "gameData/JobConsts.h"

namespace FW
{

    bool Condition_WareAndJob(nobBaseWarehouse* wh, const void* param)
    {
        return (Condition_Ware(wh, &static_cast<const Param_WareAndJob*>(param)->ware) &&
            Condition_Job(wh, &static_cast<const Param_WareAndJob*>(param)->job));
    }

    bool Condition_Troops(nobBaseWarehouse* wh, const void* param)
    {
        return (wh->GetSoldiersCount() >= *static_cast<const unsigned*>(param));
    }

    bool NoCondition(nobBaseWarehouse* wh, const void* param)
    {
        return true;
    }

    bool Condition_StoreWare(nobBaseWarehouse* wh, const void* param)
    {
        // Einlagern darf nicht verboten sein
        // Schilder beachten!
        const GoodType good = ConvertShields(*static_cast<const GoodType*>(param));
        return !wh->GetInventorySetting(good).IsSet(EInventorySetting::STOP);
    }


    bool Condition_StoreFigure(nobBaseWarehouse* wh, const void* param)
    {
        // Einlagern darf nicht verboten sein, Bootstypen zu normalen Trägern machen
        Job job = *static_cast<const Job*>(param);
        if(job == JOB_BOATCARRIER)
            job = JOB_HELPER;

        return (!wh->GetInventorySetting(job).IsSet(EInventorySetting::STOP));
    }

    bool Condition_WantStoreFigure(nobBaseWarehouse* wh, const void* param)
    {
        // Einlagern muss gewollt sein
        Job job = *static_cast<const Job*>(param);
        if(job == JOB_BOATCARRIER)
            job = JOB_HELPER;
        return (wh->GetInventorySetting(job).IsSet(EInventorySetting::COLLECT));
    }

    bool Condition_WantStoreWare(nobBaseWarehouse* wh, const void* param)
    {
        // Einlagern muss gewollt sein
        // Schilder beachten!
        GoodType gt = ConvertShields(*static_cast<const GoodType*>(param));
        return (wh->GetInventorySetting(gt).IsSet(EInventorySetting::COLLECT));
    }

    // Lagerhäuser enthalten die jeweilien Waren, liefern sie aber NICHT gleichzeitig ein
    bool Condition_StoreAndDontWantWare(nobBaseWarehouse* wh, const void* param)
    {
        Param_Ware pw = { *static_cast<const GoodType*>(param), 1 };
        return (Condition_Ware(wh, &pw) && !Condition_WantStoreWare(wh, param));
    }// param = &GoodType -> Warentyp

     // Warehouse does not collect the job and has job in store
    bool Condition_StoreAndDontWantFigure(nobBaseWarehouse* wh, const void* param)
    {
        Job job = *static_cast<const Job*>(param);
        return ((wh->GetRealFiguresCount(job) >= 1) && !Condition_WantStoreFigure(wh, param));
    }
    // param = &Job -> Jobtyp

    bool Condition_Ware(nobBaseWarehouse* wh, const void* param)
    {
        return (wh->GetRealWaresCount(static_cast<const Param_Ware*>(param)->type) >= static_cast<const Param_Ware*>(param)->count);
    }

    bool Condition_Job(nobBaseWarehouse* wh, const void* param)
    {
        if(wh->GetRealFiguresCount(static_cast<const Param_Job*>(param)->type) >= static_cast<const Param_Job*>(param)->count)
            return true;
        else
        {
            // die entsprechende Figur ist nicht vorhanden, wenn das Werkzeug der Figur und ein Mann (Träger) zum Rekrutieren
            // da ist, geht das auch, nur bei Eseln nicht !!
            bool tool_available = (JOB_CONSTS[static_cast<const Param_Job*>(param)->type].tool != GD_NOTHING) ?
                (wh->GetRealWaresCount(JOB_CONSTS[static_cast<const Param_Job*>(param)->type].tool) > 0) : true;

            if(static_cast<const Param_Job*>(param)->type == JOB_PACKDONKEY)
                tool_available = false;

            if(wh->GetRealFiguresCount(JOB_HELPER) && tool_available)
                return true;

            return false;
        }
    }

} // namespace FW