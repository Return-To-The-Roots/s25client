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

#include "defines.h" // IWYU pragma: keep
#include "BuildingRegister.h"
#include "SerializedGameData.h"
#include "buildings/noBuildingSite.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobHarborBuilding.h"
#include "buildings/nobMilitary.h"
#include "buildings/nobUsual.h"
#include "helpers/containerUtils.h"
#include "gameData/BuildingConsts.h"
#include "gameData/BuildingProperties.h"
#include <boost/foreach.hpp>

void BuildingRegister::Serialize(SerializedGameData& sgd)
{
    sgd.PushObjectContainer(warehouses, false);
    sgd.PushObjectContainer(harbors, true);
}

void BuildingRegister::Serialize2(SerializedGameData& sgd)
{
    for(unsigned i = 0; i < 30; ++i)
        sgd.PushObjectContainer(buildings[i], true);
    sgd.PushObjectContainer(building_sites, true);
    sgd.PushObjectContainer(military_buildings, true);
}

void BuildingRegister::Deserialize(SerializedGameData& sgd)
{
    sgd.PopObjectContainer(warehouses, GOT_UNKNOWN);
    sgd.PopObjectContainer(harbors, GOT_NOB_HARBORBUILDING);
}

void BuildingRegister::Deserialize2(SerializedGameData& sgd)
{
    for(unsigned i = 0; i < 30; ++i)
        sgd.PopObjectContainer(buildings[i], GOT_NOB_USUAL);
    sgd.PopObjectContainer(building_sites, GOT_BUILDINGSITE);
    sgd.PopObjectContainer(military_buildings, GOT_NOB_MILITARY);
}

void BuildingRegister::Add(noBuildingSite* building_site)
{
    RTTR_Assert(!helpers::contains(building_sites, building_site));
    building_sites.push_back(building_site);
}

void BuildingRegister::Remove(noBuildingSite* building_site)
{
    RTTR_Assert(helpers::contains(building_sites, building_site));
    building_sites.remove(building_site);
}

void BuildingRegister::Add(noBuilding* bld, BuildingType bldType)
{
    if(BuildingProperties::IsMilitary(bldType))
    {
        RTTR_Assert(!helpers::contains(military_buildings, bld));
        military_buildings.push_back(static_cast<nobMilitary*>(bld));
    } else if(BuildingProperties::IsWareHouse(bldType))
    {
        RTTR_Assert(!helpers::contains(warehouses, bld));
        warehouses.push_back(static_cast<nobBaseWarehouse*>(bld));
    } else
    {
        RTTR_Assert(!helpers::contains(buildings[bldType - FIRST_USUAL_BUILDING], bld));
        buildings[bldType - FIRST_USUAL_BUILDING].push_back(static_cast<nobUsual*>(bld));
    }
    if(bldType == BLD_HARBORBUILDING)
    {
        RTTR_Assert(!helpers::contains(harbors, bld));
        harbors.push_back(static_cast<nobHarborBuilding*>(bld));
    }
}

void BuildingRegister::Remove(noBuilding* bld, BuildingType bldType)
{
    if(BuildingProperties::IsMilitary(bldType))
    {
        RTTR_Assert(helpers::contains(military_buildings, bld));
        military_buildings.remove(static_cast<nobMilitary*>(bld));
    } else if(BuildingProperties::IsWareHouse(bldType))
    {
        RTTR_Assert(helpers::contains(warehouses, bld));
        warehouses.remove(static_cast<nobBaseWarehouse*>(bld));
    } else
    {
        RTTR_Assert(helpers::contains(buildings[bldType - FIRST_USUAL_BUILDING], bld));
        buildings[bldType - FIRST_USUAL_BUILDING].remove(static_cast<nobUsual*>(bld));
    }
    if(bldType == BLD_HARBORBUILDING)
    {
        RTTR_Assert(helpers::contains(harbors, bld));
        harbors.remove(static_cast<nobHarborBuilding*>(bld));
    }
}

/// Gibt Liste von Gebäuden des Spieler zurück
const std::list<nobUsual*>& BuildingRegister::GetBuildings(const BuildingType type) const
{
    RTTR_Assert(static_cast<unsigned>(type) >= FIRST_USUAL_BUILDING);

    return buildings[type - FIRST_USUAL_BUILDING];
}

/// Liefert die Anzahl aller Gebäude einzeln
BuildingCount BuildingRegister::GetBuildingCount() const
{
    BuildingCount bc;
    std::fill(bc.buildings.begin(), bc.buildings.end(), 0);
    std::fill(bc.buildingSites.begin(), bc.buildingSites.end(), 0);

    // Normale Gebäude zählen
    for(unsigned i = 0; i < BUILDING_TYPES_COUNT - FIRST_USUAL_BUILDING; ++i)
        bc.buildings[i + FIRST_USUAL_BUILDING] = buildings[i].size();
    // Lagerhäuser zählen
    BOOST_FOREACH(const nobBaseWarehouse* bld, warehouses)
        ++bc.buildings[bld->GetBuildingType()];
    // Militärgebäude zählen
    BOOST_FOREACH(const nobMilitary* bld, military_buildings)
        ++bc.buildings[bld->GetBuildingType()];
    // Baustellen zählen
    BOOST_FOREACH(const noBuildingSite* bld, building_sites)
        ++bc.buildingSites[bld->GetBuildingType()];
    return bc;
}

void BuildingRegister::CalcProductivities(std::vector<unsigned short>& productivities) const
{
    RTTR_Assert(productivities.size() == BUILDING_TYPES_COUNT);

    for(unsigned i = 0; i < BUILDING_TYPES_COUNT; ++i)
        productivities[i] = static_cast<unsigned short>(CalcAverageProductivity(BuildingType(i)));
}

unsigned BuildingRegister::CalcAverageProductivity(BuildingType bldType) const
{
    if(BLD_WORK_DESC[bldType].producedWare == GD_NOTHING)
        return 0;
    unsigned productivity = 0;
    unsigned numBlds = GetBuildings(bldType).size();
    if(numBlds > 0)
    {
        BOOST_FOREACH(const nobUsual* bld, GetBuildings(bldType))
            productivity += bld->GetProductivity();
        productivity /= numBlds;
    }
    return productivity;
}

unsigned short BuildingRegister::CalcAverageProductivity() const
{
    unsigned totalProductivity = 0;
    unsigned numBlds = 0;
    for(unsigned i = 0; i < BUILDING_TYPES_COUNT; ++i)
    {
        BuildingType bldType = BuildingType(i);
        if(BLD_WORK_DESC[bldType].producedWare == GD_NOTHING)
            continue;

        BOOST_FOREACH(const nobUsual* bld, GetBuildings(bldType))
            totalProductivity += bld->GetProductivity();

        numBlds += GetBuildings(bldType).size();
    }
    if(numBlds == 0)
        return 0;
    return totalProductivity / numBlds;
}