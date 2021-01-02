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

#include "BuildingFactory.h"
#include "GamePlayer.h"
#include "buildings/nobHQ.h"
#include "buildings/nobHarborBuilding.h"
#include "buildings/nobMilitary.h"
#include "buildings/nobShipYard.h"
#include "buildings/nobStorehouse.h"
#include "buildings/nobUsual.h"
#include "world/GameWorldBase.h"

noBuilding* BuildingFactory::CreateBuilding(GameWorldBase& gwg, const BuildingType type, const MapPoint pt,
                                            const unsigned char player, const Nation nation)
{
    noBuilding* bld;
    switch(type)
    {
        case BuildingType::Headquarters: bld = new nobHQ(pt, player, nation); break;
        case BuildingType::Storehouse: bld = new nobStorehouse(pt, player, nation); break;
        case BuildingType::HarborBuilding: bld = new nobHarborBuilding(pt, player, nation); break;
        case BuildingType::Barracks:
        case BuildingType::Guardhouse:
        case BuildingType::Watchtower:
        case BuildingType::Fortress: bld = new nobMilitary(type, pt, player, nation); break;
        case BuildingType::Shipyard: bld = new nobShipYard(pt, player, nation); break;
        default: bld = new nobUsual(type, pt, player, nation); break;
    }
    gwg.SetNO(pt, bld);
    // Don't do this in ctor as building might not be fully initialized yet
    gwg.GetPlayer(player).AddBuilding(bld, type);

    return bld;
}
