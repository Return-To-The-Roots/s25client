// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameTypes/BuildingCount.h"
#include <list>
#include <vector>

class noBuilding;
class noBuildingSite;
class nobUsual;
class nobMilitary;
class nobHarborBuilding;
class nobBaseWarehouse;
class SerializedGameData;

class BuildingRegister
{
public:
    /// Serialisieren
    void Serialize(SerializedGameData& sgd) const;
    // Deserialisieren
    void Deserialize(SerializedGameData& sgd);
    /// Compatibility with old savegames
    void Deserialize2(SerializedGameData& sgd);

    void Add(noBuildingSite* building_site);
    void Remove(noBuildingSite* building_site);
    void Add(noBuilding* bld, BuildingType bldType);
    void Remove(noBuilding* bld, BuildingType bldType);

    const std::list<noBuildingSite*>& GetBuildingSites() const { return building_sites; }
    const std::list<nobUsual*>& GetBuildings(BuildingType type) const;
    const std::list<nobMilitary*>& GetMilitaryBuildings() const { return military_buildings; }
    const std::list<nobHarborBuilding*>& GetHarbors() const { return harbors; }
    const std::list<nobBaseWarehouse*>& GetStorehouses() const { return warehouses; }

    /// Liefert die Anzahl aller Gebäude einzeln
    BuildingCount GetBuildingNums() const;
    /// Calculate and fill the average productivities for all buildings.
    helpers::EnumArray<uint16_t, BuildingType> CalcProductivities() const;
    /// Calculate the average productivity for a building type
    unsigned CalcAverageProductivity(BuildingType bldType) const;
    /// Calculate the average productivity for all buildings
    unsigned short CalcAverageProductivity() const;

private:
    std::list<noBuildingSite*> building_sites;
    // Only "usual" buildings
    helpers::EnumArray<std::list<nobUsual*>, BuildingType> buildings;
    std::list<nobMilitary*> military_buildings;
    std::list<nobHarborBuilding*> harbors;
    std::list<nobBaseWarehouse*> warehouses;
};
