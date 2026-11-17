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
    void serialize(SerializedGameData& sgd) const;
    // Deserialisieren
    void deserialize(SerializedGameData& sgd);
    /// Compatibility with old savegames
    void deserialize2(SerializedGameData& sgd);

    void add(noBuildingSite* building_site);
    void remove(noBuildingSite* building_site);
    void add(noBuilding* bld, BuildingType bldType);
    void remove(noBuilding* bld, BuildingType bldType);

    const std::list<noBuildingSite*>& getBuildingSites() const { return building_sites; }
    const std::list<nobUsual*>& getBuildings(BuildingType type) const;
    const std::list<nobMilitary*>& getMilitaryBuildings() const { return military_buildings; }
    const std::list<nobHarborBuilding*>& getHarbors() const { return harbors; }
    const std::list<nobBaseWarehouse*>& getStorehouses() const { return warehouses; }

    /// Liefert die Anzahl aller Gebäude einzeln
    BuildingCount getBuildingNums() const;
    /// Calculate and fill the average productivities for all buildings.
    helpers::EnumArray<uint16_t, BuildingType> calcProductivities() const;
    /// Calculate the average productivity for a building type
    unsigned calcAverageProductivity(BuildingType bldType) const;
    /// Calculate the average productivity for all buildings
    unsigned short calcAverageProductivity() const;

private:
    std::list<noBuildingSite*> building_sites;
    // Only "usual" buildings
    helpers::EnumArray<std::list<nobUsual*>, BuildingType> buildings;
    std::list<nobMilitary*> military_buildings;
    std::list<nobHarborBuilding*> harbors;
    std::list<nobBaseWarehouse*> warehouses;
};
