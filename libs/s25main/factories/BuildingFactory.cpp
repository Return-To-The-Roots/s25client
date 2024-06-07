// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "BuildingFactory.h"
#include "GamePlayer.h"
#include "buildings/nobHQ.h"
#include "buildings/nobHarborBuilding.h"
#include "buildings/nobMilitary.h"
#include "buildings/nobShipYard.h"
#include "buildings/nobStorehouse.h"
#include "buildings/nobTemple.h"
#include "buildings/nobUsual.h"
#include "world/GameWorldBase.h"

noBuilding* BuildingFactory::CreateBuilding(GameWorldBase& world, const BuildingType type, const MapPoint pt,
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
        case BuildingType::Temple: bld = new nobTemple(pt, player, nation); break;
        default: bld = new nobUsual(type, pt, player, nation); break;
    }
    world.SetNO(pt, bld);
    // Don't do this in ctor as building might not be fully initialized yet
    world.GetPlayer(player).AddBuilding(bld, type);

    return bld;
}
