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

#ifndef FindWhConditions_h__
#define FindWhConditions_h__

#include "gameTypes/GoodTypes.h"
#include "gameTypes/JobTypes.h"

class nobBaseWarehouse;

/// Vorgefertigte Bedingungsfunktionen für FindWarehouse, param jeweils Pointer auf die einzelnen Strukturen
namespace FW
{
    struct Param_Ware { GoodType type; unsigned count; };
    bool Condition_Ware(nobBaseWarehouse* wh, const void* param);
    struct Param_Job { Job type; unsigned count; };
    bool Condition_Job(nobBaseWarehouse* wh, const void* param);
    struct Param_WareAndJob { Param_Ware ware; Param_Job job; };
    bool Condition_WareAndJob(nobBaseWarehouse* wh, const void* param);

    bool Condition_Troops(nobBaseWarehouse* wh, const void* param);   // param = &unsigned --> count
    bool Condition_StoreWare(nobBaseWarehouse* wh, const void* param);   // param = &GoodType -> Warentyp
    bool Condition_StoreFigure(nobBaseWarehouse* wh, const void* param);   // param = &Job -> Jobtyp

                                                                           // Die Lagerhäuser lagern die jeweiligen Waren ein
    bool Condition_WantStoreWare(nobBaseWarehouse* wh, const void* param);   // param = &GoodType -> Warentyp
    bool Condition_WantStoreFigure(nobBaseWarehouse* wh, const void* param);   // param = &Job -> Jobtyp

                                                                               // Lagerhäuser enthalten die jeweilien Waren, liefern sie aber NICHT gleichzeitig ein
    bool Condition_StoreAndDontWantWare(nobBaseWarehouse* wh, const void* param);   // param = &GoodType -> Warentyp
    bool Condition_StoreAndDontWantFigure(nobBaseWarehouse* wh, const void* param);   // param = &Job -> Jobtyp


    bool NoCondition(nobBaseWarehouse* wh, const void* param);
}

#endif // FindWhConditions_h__