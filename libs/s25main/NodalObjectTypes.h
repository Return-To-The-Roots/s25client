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

#pragma once

#include <cstdint>

enum class NodalObjectType : uint8_t
{
    Nothing,         // nichts
    Granite,         // Granit
    Tree,            // Baum
    Grainfield,      // Getreidefeld
    Environment,     // sonstige "Umweltobjekte", die keine besondere Funktion haben ( tote Bäume, Pilze, Sträucher )
    Object,          // sonstige "feste" Objekte, die keine besondere Funktion haben (Stalagmiten, Ruinen, usw)
    Building,        // Gebäcde
    Flag,            // Fahne
    Buildingsite,    // Baustelle
    Figure,          // Siedler-Leute
    Extension,       // Anbau von großen Gebäuden
    Fire,            // Ein Feuer von einem brennende (zerstörten) Gebäude
    Fighting,        // Kampf,
    Animal,          // Tier
    BurnedWarehouse, // abgebranntes Lagerhaus, aus dem die Menschen jetzt strömen
    Ship,            // Schiff
    CharburnerPile   // Holz-/Kohle-Haufen vom Köhler
};
constexpr auto maxEnumValue(NodalObjectType)
{
    return NodalObjectType::CharburnerPile;
}
