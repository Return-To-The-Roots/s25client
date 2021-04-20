// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "helpers/EnumArray.h"
#include "gameTypes/BuildingCount.h"
#include "gameTypes/BuildingType.h"

namespace AIJH {

class AIPlayerJH;

class BuildingPlanner
{
public:
    BuildingPlanner(const AIPlayerJH& aijh);
    /// Refresh the number of buildings by asking the GameClientPlayer
    void Update(unsigned gf, AIPlayerJH& aijh);

    /// Return the number of buildings and buildingsites of a specific type (refresh with RefreshBuildingCount())
    unsigned GetNumBuildings(BuildingType type) const;
    /// Return the number of buildingsites of a specific type (refresh with RefreshBuildingCount())
    unsigned GetNumBuildingSites(BuildingType type) const;
    /// Get amount of (completed) military buildings
    unsigned GetNumMilitaryBlds() const;
    /// Get amount of construction sites of military buildings
    unsigned GetNumMilitaryBldSites() const;

    void InitBuildingsWanted(const AIPlayerJH& aijh);
    void UpdateBuildingsWanted(const AIPlayerJH& aijh);

    /// Return the number of buildings that we want to build of the current type
    int GetNumAdditionalBuildingsWanted(BuildingType type) const;
    /// Checks whether the ai wants to construct more mil buildings atm
    bool WantMoreMilitaryBlds(const AIPlayerJH& aijh) const;
    bool IsExpansionRequired() const { return expansionRequired; }

private:
    /// Number of buildings and building sites of this player (refreshed by RefreshBuildingCount())
    BuildingCount buildingNums;
    /// Contains how many buildings of every type is wanted
    helpers::EnumArray<unsigned, BuildingType> buildingsWanted;
    bool expansionRequired;

    void RefreshBuildingNums(const AIPlayerJH& aijh);
    bool CalcIsExpansionRequired(AIPlayerJH& aijh, bool recalc) const;
};
} // namespace AIJH
