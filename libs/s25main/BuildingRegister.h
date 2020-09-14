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
    /// Calculate and fill the average productivities for all buildings. Vector must hold 1 entry per building type
    void CalcProductivities(std::vector<unsigned short>& productivities) const;
    /// Calculate the average productivity for a building type
    unsigned CalcAverageProductivity(BuildingType bldType) const;
    /// Calculate the average productivity for all buildings
    unsigned short CalcAverageProductivity() const;

private:
    std::list<noBuildingSite*> building_sites;
    std::array<std::list<nobUsual*>, 30> buildings;
    std::list<nobMilitary*> military_buildings;
    std::list<nobHarborBuilding*> harbors;
    std::list<nobBaseWarehouse*> warehouses;
};
