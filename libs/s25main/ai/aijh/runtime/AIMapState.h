// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "ai/aijh/runtime/AIMap.h"
#include "ai/aijh/runtime/AIResourceMap.h"

#include <queue>
#include <vector>

class AIQueryService;
class GameWorldBase;

namespace AIJH {

class AIPlayerJH;

class AIMapState
{
public:
    explicit AIMapState(AIPlayerJH& owner);

    AINodeResource CalcResource(MapPoint pt);
    void InitNodes();
    void InitResourceMaps();
    void InitReachableNodes();
    void IterativeReachableNodeChecker(std::queue<MapPoint> toCheck);
    void UpdateReachableNodes(const std::vector<MapPoint>& pts);
    void UpdateNodesAround(MapPoint pt, unsigned radius);
    void SetFarmedNodes(MapPoint pt, bool set);
    MapPoint FindBestPosition(MapPoint pt, AIResource res, BuildingQuality size, unsigned radius, int minimum);
    void RecalcGround(MapPoint buildingPos, std::vector<Direction>& route_road);
    void SaveResourceMapsToFile();
    void RefreshBuildingQualities();

    Node& GetAINode(MapPoint pt) { return aiMap_[pt]; }
    const Node& GetAINode(MapPoint pt) const { return aiMap_[pt]; }
    const AIMap& GetMap() const { return aiMap_; }
    AIResourceMap& GetResMap(AIResource res) { return resourceMaps_[res]; }
    const AIResourceMap& GetResMap(AIResource res) const { return resourceMaps_[res]; }
    int GetResMapValue(MapPoint pt, AIResource res) const { return resourceMaps_[res][pt]; }
    std::vector<MapPoint>& GetNodesWithOutdatedBQ() { return nodesWithOutdatedBQ_; }

private:
    AIPlayerJH& owner_;
    AIMap aiMap_;
    helpers::EnumArray<AIResourceMap, AIResource> resourceMaps_;
    std::vector<MapPoint> nodesWithOutdatedBQ_;
};

} // namespace AIJH
