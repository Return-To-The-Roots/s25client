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

#include "BuildingRegister.h"
#include "SerializedGameData.h"
#include "buildings/noBuildingSite.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobHarborBuilding.h"
#include "buildings/nobMilitary.h"
#include "buildings/nobUsual.h"
#include "enum_cast.hpp"
#include "helpers/EnumRange.h"
#include "helpers/containerUtils.h"
#include "gameData/BuildingConsts.h"
#include "gameData/BuildingProperties.h"

void BuildingRegister::Serialize(SerializedGameData& sgd) const
{
    sgd.PushObjectContainer(warehouses, false);
    sgd.PushObjectContainer(harbors, true);
    for(const auto bld : helpers::enumRange<BuildingType>())
    {
        if(BuildingProperties::IsUsual(bld))
            sgd.PushObjectContainer(buildings[bld], true);
    }
    sgd.PushObjectContainer(building_sites, true);
    sgd.PushObjectContainer(military_buildings, true);
}

void BuildingRegister::Deserialize(SerializedGameData& sgd)
{
    sgd.PopObjectContainer(warehouses, GOT_UNKNOWN);
    sgd.PopObjectContainer(harbors, GOT_NOB_HARBORBUILDING);

    if(sgd.GetGameDataVersion() >= 6)
    {
        for(const auto bld : helpers::enumRange<BuildingType>())
        {
            if(BuildingProperties::IsUsual(bld))
                sgd.PopObjectContainer(buildings[bld], GOT_NOB_USUAL);
        }
        sgd.PopObjectContainer(building_sites, GOT_BUILDINGSITE);
        sgd.PopObjectContainer(military_buildings, GOT_NOB_MILITARY);
    } else if(sgd.GetGameDataVersion() >= 2)
        Deserialize2(sgd);
}

void BuildingRegister::Deserialize2(SerializedGameData& sgd)
{
    // Pop all buildings starting at the first usual building
    for(unsigned i = static_cast<uint8_t>(BuildingType::GraniteMine); i < helpers::NumEnumValues_v<BuildingType>; ++i)
        sgd.PopObjectContainer(buildings[BuildingType(i)], GOT_NOB_USUAL);
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
        RTTR_Assert(!helpers::contains(buildings[bldType], bld));
        buildings[bldType].push_back(static_cast<nobUsual*>(bld));
    }
    if(bldType == BuildingType::HarborBuilding)
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
        RTTR_Assert(helpers::contains(buildings[bldType], bld));
        buildings[bldType].remove(static_cast<nobUsual*>(bld));
    }
    if(bldType == BuildingType::HarborBuilding)
    {
        RTTR_Assert(helpers::contains(harbors, bld));
        harbors.remove(static_cast<nobHarborBuilding*>(bld));
    }
}

/// Gibt Liste von Gebäuden des Spieler zurück
const std::list<nobUsual*>& BuildingRegister::GetBuildings(const BuildingType type) const
{
    RTTR_Assert(BuildingProperties::IsUsual(type));
    return buildings[type];
}

/// Liefert die Anzahl aller Gebäude einzeln
BuildingCount BuildingRegister::GetBuildingNums() const
{
    BuildingCount bc{};

    // Normale Gebäude zählen
    for(const auto bld : helpers::enumRange<BuildingType>())
        bc.buildings[bld] = buildings[bld].size();
    // Lagerhäuser zählen
    for(const nobBaseWarehouse* bld : warehouses)
        ++bc.buildings[bld->GetBuildingType()];
    // Militärgebäude zählen
    for(const nobMilitary* bld : military_buildings)
        ++bc.buildings[bld->GetBuildingType()];
    // Baustellen zählen
    for(const noBuildingSite* bld : building_sites)
        ++bc.buildingSites[bld->GetBuildingType()];
    return bc;
}

helpers::EnumArray<uint16_t, BuildingType> BuildingRegister::CalcProductivities() const
{
    helpers::EnumArray<uint16_t, BuildingType> productivities;

    for(const auto bld : helpers::enumRange<BuildingType>())
        productivities[bld] = static_cast<uint16_t>(CalcAverageProductivity(bld));
    return productivities;
}

unsigned BuildingRegister::CalcAverageProductivity(BuildingType bldType) const
{
    if(!BLD_WORK_DESC[bldType].producedWare)
        return 0;
    unsigned productivity = 0;
    unsigned numBlds = GetBuildings(bldType).size();
    if(numBlds > 0)
    {
        for(const nobUsual* bld : GetBuildings(bldType))
            productivity += bld->GetProductivity();
        productivity /= numBlds;
    }
    return productivity;
}

unsigned short BuildingRegister::CalcAverageProductivity() const
{
    unsigned totalProductivity = 0;
    unsigned numBlds = 0;
    for(const auto bldType : helpers::enumRange<BuildingType>())
    {
        if(!BLD_WORK_DESC[bldType].producedWare)
            continue;

        for(const nobUsual* bld : GetBuildings(bldType))
            totalProductivity += bld->GetProductivity();

        numBlds += GetBuildings(bldType).size();
    }
    if(numBlds == 0)
        return 0;
    return totalProductivity / numBlds;
}
