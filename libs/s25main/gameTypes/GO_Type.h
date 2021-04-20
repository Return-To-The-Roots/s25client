// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdint>

/// To be able to load old savegames and keep ids unique, please insert new
/// items at the end of the list.
enum class GO_Type : uint16_t
{
    Nothing = 1, // TODO: Revert to 0 when SaveGameVersion is increased
    NobHq,
    NobMilitary,
    NobStorehouse,
    NobUsual,
    NobShipyard,
    NobHarborbuilding,
    Buildingsite,
    NofAggressivedefender,
    NofAttacker,
    NofDefender,
    NofPassivesoldier,
    NofWellguy,
    NofCarrier,
    NofWoodcutter,
    NofFisher,
    NofForester,
    NofCarpenter,
    NofStonemason,
    NofHunter,
    NofFarmer,
    NofMiller,
    NofBaker,
    NofButcher,
    NofMiner,
    NofBrewer,
    NofPigbreeder,
    NofDonkeybreeder,
    NofIronfounder,
    NofMinter,
    NofMetalworker,
    NofArmorer,
    NofBuilder,
    NofPlaner,
    NofGeologist,
    NofShipwright,
    NofScoutFree,
    NofScoutLookouttower,
    NofWarehouseworker,
    NofCatapultman,
    NofPassiveworker,
    NofCharburner,
    Extension,
    Envobject,
    Fire,
    Flag,
    Grainfield,
    Granite,
    Sign,
    Skeleton,
    Staticobject,
    Disappearingmapenvobject,
    Tree,
    Animal,
    Fighting,
    Roadsegment,
    Ware,
    Catapultstone,
    Burnedwarehouse,
    Shipbuildingsite,
    Ship,
    Charburnerpile,
    NofTradeleader,
    NofTradedonkey,
    Economymodehandler
};
constexpr auto maxEnumValue(GO_Type)
{
    return GO_Type::Economymodehandler;
}
