// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "helpers/EnumArray.h"
#include "helpers/OptionalEnum.h"
#include "gameTypes/BuildingType.h"
#include "gameTypes/Direction.h"
#include "gameTypes/MapCoordinates.h"
#include <deque>
#include <memory>
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
class BuildingPlanner;
class BuildJob;
class ConnectJob;

class AIConstruction
{
public:
    AIConstruction(AIPlayerJH& aijh);
    ~AIConstruction();

    /// Adds a build job to the queue
    void addBuildJob(std::unique_ptr<BuildJob> job, bool front);

    std::unique_ptr<BuildJob> getBuildJob();
    unsigned getBuildJobNum() const { return buildJobs.size(); }
    unsigned getConnectJobNum() const { return connectJobs.size(); }

    void addConnectFlagJob(const noFlag* flag);

    bool buildJobAvailable() const { return !buildJobs.empty(); }
    /// Finds flags in the area around pt
    std::vector<const noFlag*> findFlags(MapPoint pt, unsigned short radius);
    /// returns true if the military building should be connected to the roadsystem
    bool militaryBuildingWantsRoad(const nobMilitary& milbld);
    /// Connects a specific flag to a roadsystem nearby and returns true if succesful. Also returns the route of the
    /// future road.
    bool connectFlagToRoadSytem(const noFlag* flag, std::vector<Direction>& route, unsigned maxSearchRadius = 14);
    /// Builds a street between two roadnodes and sets flags on it, if route is empty, it will be calculated
    bool BuildRoad(const noRoadNode* start, const noRoadNode* target, std::vector<Direction>& route);
    /// whenever a given route contains 2 segment alternatives these get tested for their buildquality and the one with
    /// the lower bq is picked for the final path
    bool minorRoadImprovements(const noRoadNode* start, const noRoadNode* target, std::vector<Direction>& route);
    /// Checks whether a flag is connected to the road system or not (connected = has path to HQ)
    bool isConnectedToRoadSystem(const noFlag* flag) const;

    helpers::OptionalEnum<BuildingType> getSmallestAllowedMilBuilding() const;
    helpers::OptionalEnum<BuildingType> getBiggestAllowedMilBuilding() const;
    /// Randomly chooses a military building, preferring bigger buildings if enemy nearby
    helpers::OptionalEnum<BuildingType> chooseMilitaryBuilding(MapPoint pt);
    /// Checks whether a building type is wanted atm
    bool wanted(BuildingType type) const;
    /// Tries to build a second road to a flag, which is in any way better than the first one
    bool buildAlternativeRoad(const noFlag* flag, std::vector<Direction>& route);

    bool otherStoreInRadius(MapPoint pt, unsigned radius);

    bool otherUsualBuildingInRadius(MapPoint pt, unsigned radius, BuildingType bt);

    noFlag* findTargetStoreHouseFlag(MapPoint pt) const;

    bool canStillConstructHere(MapPoint pt) const;

    void executeJobs(unsigned limit);
    /// Set flags along the road starting at the given node in the given direction
    void setFlagsAlongRoad(const noRoadNode& roadNode, Direction dir);
    /// To be called after a new construction site was added
    void constructionOrdered(const BuildJob& job);
    /// To be called when the current pending construction orders were processed (usually on NWF)
    void constructionsExecuted();

private:
    AIPlayerJH& aijh;
    AIInterface& aii;
    const BuildingPlanner& bldPlanner;
    /// Contains the build jobs the AI should try to execute
    std::deque<std::unique_ptr<BuildJob>> buildJobs;
    std::deque<std::unique_ptr<ConnectJob>> connectJobs;
    /// contains the locations pt at which the ai has done some kind of construction since the last nwf
    // -> so the commands are not yet executed and for now the ai will just not build again in the area until the next
    // nwf
    std::deque<MapPoint> constructionlocations;
    // contains the amount of buildings ordered since the last nwf
    helpers::EnumArray<uint8_t, BuildingType> constructionorders;
};

} // namespace AIJH
