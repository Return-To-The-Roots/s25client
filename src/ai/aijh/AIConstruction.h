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
#ifndef AICONSTRUCTION_H_INCLUDED
#define AICONSTRUCTION_H_INCLUDED

#pragma once

#include "gameTypes/BuildingCount.h"
#include "gameTypes/BuildingType.h"
#include "gameTypes/Direction.h"
#include "gameTypes/MapCoordinates.h"
#include <boost/container/static_vector.hpp>
#include <deque>
#include <stdint.h>
#include <vector>

class AIInterface;
class noFlag;
class noRoadNode;
class nobMilitary;
namespace boost {
template<class T, std::size_t N>
class array;
}

namespace AIJH {
class AIPlayerJH;
class Job;
class BuildJob;
class ConnectJob;

class AIConstruction
{
public:
    AIConstruction(AIInterface& aii, AIPlayerJH& aijh);
    ~AIConstruction();

    /// Adds a build job to the queue
    void AddBuildJob(BuildJob* job, bool front);

    BuildJob* GetBuildJob();
    unsigned GetBuildJobNum() const { return buildJobs.size(); }
    unsigned GetConnectJobNum() const { return connectJobs.size(); }

    void AddConnectFlagJob(const noFlag* flag);

    bool BuildJobAvailable() const { return !buildJobs.empty(); }
    /// Finds flags in the area around pt
    std::vector<const noFlag*> FindFlags(const MapPoint pt, unsigned short radius);
    /// returns true if the military building should be connected to the roadsystem
    bool MilitaryBuildingWantsRoad(nobMilitary* milbld, unsigned listpos);
    /// Connects a specific flag to a roadsystem nearby and returns true if succesful. Also returns the route of the future road.
    bool ConnectFlagToRoadSytem(const noFlag* flag, std::vector<Direction>& route, unsigned maxSearchRadius = 14);
    /// Builds a street between two roadnodes and sets flags on it, if route is empty, it will be calculated
    bool BuildRoad(const noRoadNode* start, const noRoadNode* target, std::vector<Direction>& route);
    /// whenever a given route contains 2 segment alternatives these get tested for their buildquality and the one with the lower bq is
    /// picked for the final path
    bool MinorRoadImprovements(const noRoadNode* start, const noRoadNode* target, std::vector<Direction>& route);
    /// Checks whether a flag is connected to the road system or not (connected = has path to HQ)
    bool IsConnectedToRoadSystem(const noFlag* flag) const;

    BuildingType GetSmallestAllowedMilBuilding() const;
    BuildingType GetBiggestAllowedMilBuilding() const;
    /// Randomly chooses a military building, preferring bigger buildings if enemy nearby
    BuildingType ChooseMilitaryBuilding(const MapPoint pt);
    /// Returns the number of buildings and buildingsites of a specific type (refresh with RefreshBuildingCount())
    unsigned GetBuildingCount(BuildingType type) const;
    /// Returns the number of buildingsites of a specific type (refresh with RefreshBuildingCount())
    unsigned GetBuildingSitesCount(BuildingType type) const;
    /// Refreshes the number of buildings by asking the GameClientPlayer and recalcs some wanted buildings
    void RefreshBuildingCount();
    /// Checks whether a building type is wanted atm
    bool Wanted(BuildingType type) const;
    /// Checks whether the ai wants to construct more mil buildings atm
    bool WantMoreMilitaryBlds() const;
    /// Tries to build a second road to a flag, which is in any way better than the first one
    bool BuildAlternativeRoad(const noFlag* flag, std::vector<Direction>& route);

    bool OtherStoreInRadius(MapPoint pt, unsigned radius);

    bool OtherUsualBuildingInRadius(MapPoint pt, unsigned radius, BuildingType bt);

    noFlag* FindTargetStoreHouseFlag(const MapPoint pt) const;

    bool CanStillConstructHere(const MapPoint pt) const;

    void ExecuteJobs(unsigned limit);
    /// Set flags along the road starting at the given node in the given direction
    void SetFlagsAlongRoad(const noRoadNode& roadNode, Direction dir);
    /// To be called after a new construction site was added
    void ConstructionOrdered(const BuildJob& job);
    /// To be called when the current pending construction orders were processed (usually on NWF)
    void ConstructionsExecuted();

private:
    void InitMilitaryBldTypes();
    /// Initializes the wanted-buildings-vector
    void InitBuildingsWanted();
    /// Get amount of (completed) military buildings
    unsigned GetMilitaryBldCount() const;
    /// Get amount of construction sites of military buildings
    unsigned GetMilitaryBldSiteCount() const;

    AIInterface& aii;
    AIPlayerJH& aijh;
    /// Stores the bld types that are military blds as a cache. Assumes that at most 1/4 of the blds are military
    boost::container::static_vector<BuildingType, BUILDING_TYPES_COUNT / 4u> militaryBldTypes;
    /// Contains how many buildings of every type is wanted
    std::vector<unsigned> buildingsWanted;
    /// Contains the build jobs the AI should try to execute
    std::deque<BuildJob*> buildJobs;
    std::deque<ConnectJob*> connectJobs;
    /// contains the locations pt at which the ai has done some kind of construction since the last nwf
    // -> so the commands are not yet executed and for now the ai will just not build again in the area until the next nwf
    std::deque<MapPoint> constructionlocations;
    // contains the type and amount of buildings ordered since the last nwf
    std::vector<uint8_t> constructionorders;
    /// Number of buildings and building sites of this player (refreshed by RefreshBuildingCount())
    BuildingCount buildingCounts;
};

} // namespace AIJH

#endif //! AICONSTRUCTION_H_INCLUDED
