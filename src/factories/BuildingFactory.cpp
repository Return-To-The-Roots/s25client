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

#include "BuildingFactory.h"
#include "GameWorldGame.h"
#include "buildings/nobStorehouse.h"
#include "buildings/nobHarborBuilding.h"
#include "buildings/nobMilitary.h"
#include "buildings/nobShipYard.h"
#include "buildings/nobUsual.h"
#include "GameClientPlayer.h"

noBuilding* BuildingFactory::CreateBuilding(GameWorldGame* gwg, const BuildingType type, const MapPoint pt, const unsigned char player, const Nation nation){
    noBuilding* bld;
    switch (type)
    {
    case BLD_STOREHOUSE:
        bld =  new nobStorehouse(pt, player, nation);
        break;
    case BLD_HARBORBUILDING:
        bld = new nobHarborBuilding(pt, player, nation);
        break;
    case BLD_BARRACKS:
    case BLD_GUARDHOUSE:
    case BLD_WATCHTOWER:
    case BLD_FORTRESS:
        bld = new nobMilitary(type, pt, player, nation);
        break;
    case BLD_SHIPYARD:
        bld = new nobShipYard(pt, player, nation);
        break;
    default:
        bld = new nobUsual(type, pt, player, nation);
        break;
    }
    gwg->SetNO(bld, pt);
    if(type == BLD_HARBORBUILDING)
    {
        // For harbors tell the economics about the new harbor
        // Attention: Must be used after the harbours is added to the world (setNO) so it cannot be done in the ctor
        gwg->GetPlayer(player).AddHarbor(static_cast<nobHarborBuilding*>(bld));
    }

    return bld;
}
