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

#ifndef BuildingPlanner_h__
#define BuildingPlanner_h__

#include "gameTypes/BuildingCount.h"
#include "gameTypes/BuildingType.h"
#include <vector>

namespace AIJH {

class AIPlayerJH;

class BuildingPlanner
{
public:
    BuildingPlanner(const AIPlayerJH& aijh);
    /// Refresh the number of buildings by asking the GameClientPlayer
    void RefreshBuildingCount();

    /// Return the number of buildings and buildingsites of a specific type (refresh with RefreshBuildingCount())
    unsigned GetBuildingCount(BuildingType type) const;
    /// Return the number of buildingsites of a specific type (refresh with RefreshBuildingCount())
    unsigned GetBuildingSitesCount(BuildingType type) const;
    /// Get amount of (completed) military buildings
    unsigned GetMilitaryBldCount() const;
    /// Get amount of construction sites of military buildings
    unsigned GetMilitaryBldSiteCount() const;

    void InitBuildingsWanted();
    void UpdateBuildingsWanted();

    /// Return the number of buildings that we want to build of the current type
    int GetNumAdditionalBuildingsWanted(BuildingType type) const;
    /// Checks whether the ai wants to construct more mil buildings atm
    bool WantMoreMilitaryBlds() const;

private:
    const AIPlayerJH& aijh;
    /// Number of buildings and building sites of this player (refreshed by RefreshBuildingCount())
    BuildingCount buildingCounts;
    /// Contains how many buildings of every type is wanted
    std::vector<unsigned> buildingsWanted;
};
} // namespace AIJH

#endif // BuildingPlanner_h__
