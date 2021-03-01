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
