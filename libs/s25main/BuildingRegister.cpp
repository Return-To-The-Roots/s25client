// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "BuildingRegister.h"
#include "LeatherLoader.h"
#include "SerializedGameData.h"
#include "WineLoader.h"
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
    sgd.PushObjectContainer(warehouses);
    sgd.PushObjectContainer(harbors, true);
    for(const auto bld : helpers::enumRange<BuildingType>())
    {
        if(BuildingProperties::isUsual(bld))
            sgd.PushObjectContainer(buildings[bld], true);
    }
    sgd.PushObjectContainer(building_sites, true);
    sgd.PushObjectContainer(military_buildings, true);
}

void BuildingRegister::Deserialize(SerializedGameData& sgd)
{
    sgd.PopObjectContainer(warehouses);
    sgd.PopObjectContainer(harbors, GO_Type::NobHarborbuilding);

    if(sgd.getGameDataVersion() >= 6)
    {
        for(const auto bld : helpers::enumRange<BuildingType>())
        {
            if(sgd.getGameDataVersion() < 11 && wineaddon::isWineAddonBuildingType(bld))
                continue;

            if(sgd.getGameDataVersion() < 12 && leatheraddon::isLeatherAddonBuildingType(bld))
                continue;

            if(BuildingProperties::isUsual(bld))
                sgd.PopObjectContainer(buildings[bld], GO_Type::NobUsual);
        }
        sgd.PopObjectContainer(building_sites, GO_Type::Buildingsite);
        sgd.PopObjectContainer(military_buildings, GO_Type::NobMilitary);
    } else if(sgd.getGameDataVersion() >= 2)
        Deserialize2(sgd);
}

void BuildingRegister::Deserialize2(SerializedGameData& sgd)
{
    // Pop all buildings starting at the first usual building
    for(unsigned i = static_cast<uint8_t>(BuildingType::GraniteMine); i < helpers::NumEnumValues_v<BuildingType>; ++i)
        sgd.PopObjectContainer(buildings[BuildingType(i)], GO_Type::NobUsual);
    sgd.PopObjectContainer(building_sites, GO_Type::Buildingsite);
    sgd.PopObjectContainer(military_buildings, GO_Type::NobMilitary);
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
    if(BuildingProperties::isMilitary(bldType))
    {
        RTTR_Assert(!helpers::contains(military_buildings, bld));
        military_buildings.push_back(static_cast<nobMilitary*>(bld));
    } else if(BuildingProperties::isWareHouse(bldType))
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
    if(BuildingProperties::isMilitary(bldType))
    {
        RTTR_Assert(helpers::contains(military_buildings, bld));
        military_buildings.remove(static_cast<nobMilitary*>(bld));
    } else if(BuildingProperties::isWareHouse(bldType))
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
const std::list<nobUsual*>& BuildingRegister::getBuildings(const BuildingType type) const
{
    RTTR_Assert(BuildingProperties::isUsual(type));
    return buildings[type];
}

/// Liefert die Anzahl aller Gebäude einzeln
BuildingCount BuildingRegister::getBuildingNums() const
{
    BuildingCount bc{};

    // Normale Gebäude zählen
    for(const auto bld : helpers::enumRange<BuildingType>())
        bc.buildings[bld] = buildings[bld].size();
    // Lagerhäuser zählen
    for(const nobBaseWarehouse* bld : warehouses)
        ++bc.buildings[bld->getBuildingType()];
    // Militärgebäude zählen
    for(const nobMilitary* bld : military_buildings)
        ++bc.buildings[bld->getBuildingType()];
    // Baustellen zählen
    for(const noBuildingSite* bld : building_sites)
        ++bc.buildingSites[bld->getBuildingType()];
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
    if(holds_alternative<std::nullopt_t>(BLD_WORK_DESC[bldType].producedWare))
        return 0;
    unsigned productivity = 0;
    const auto& buildings = getBuildings(bldType);
    const unsigned numBlds = buildings.size();
    if(numBlds > 0)
    {
        for(const nobUsual* bld : buildings)
            productivity += bld->getProductivity();
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
        if(holds_alternative<std::nullopt_t>(BLD_WORK_DESC[bldType].producedWare))
            continue;

        const auto& buildings = getBuildings(bldType);
        numBlds += buildings.size();
        for(const nobUsual* bld : buildings)
            totalProductivity += bld->getProductivity();
    }
    if(numBlds == 0)
        return 0;
    return totalProductivity / numBlds;
}
