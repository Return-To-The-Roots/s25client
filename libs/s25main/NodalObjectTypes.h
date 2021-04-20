// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
