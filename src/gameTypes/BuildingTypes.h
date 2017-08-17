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

#ifndef BuildingTypes_h__
#define BuildingTypes_h__

#include "BuildingQuality.h"
#include "GoodTypes.h"
#include "JobTypes.h"

struct BuildingCost
{
    unsigned char boards;
    unsigned char stones;
};

// Größe der Gebäude
enum BuildingSize
{
    BZ_HUT = 0,
    BZ_HOUSE,
    BZ_CASTLE,
    BZ_MINE
};

// Konstanten zu den "normalen Gebäuden" (Betrieben), beginnt erst mit Granitmine
struct UsualBuilding
{
    /// Arbeitertyp, der in diesem Gebäude arbeitet
    Job job;
    /// Ware, die das Gebäude produziert
    GoodType produced_ware;
    /// Anzahl Waren, die das Gebäude benötigt
    unsigned char wares_needed_count;
    /// Waren, die das Gebäude benötigt
    GoodType wares_needed[3];
};

/// Rauch-Konstanten zu den "normalen Gebäuden" (Betrieben), beginnt erst mit Granitmine
struct SmokeConst
{
    /// Art des Rauches (von 1-4), 0 = kein Rauch!
    unsigned char type;
    /// Position des Rauches relativ zum Nullpunkt des Gebäudes
    signed char x, y;
};

#endif // BuildingTypes_h__
