// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AIPlayerJH.h"

#include "ai/aijh/runtime/AIMapState.h"

namespace AIJH {

AINodeResource AIPlayerJH::CalcResource(MapPoint pt)
{
    return mapState_->CalcResource(pt);
}

void AIPlayerJH::InitReachableNodes()
{
    mapState_->InitReachableNodes();
}

void AIPlayerJH::IterativeReachableNodeChecker(std::queue<MapPoint> toCheck)
{
    mapState_->IterativeReachableNodeChecker(std::move(toCheck));
}

void AIPlayerJH::UpdateReachableNodes(const std::vector<MapPoint>& pts)
{
    mapState_->UpdateReachableNodes(pts);
}

void AIPlayerJH::InitNodes()
{
    mapState_->InitNodes();
}

void AIPlayerJH::UpdateNodesAround(const MapPoint pt, unsigned radius)
{
    mapState_->UpdateNodesAround(pt, radius);
}

void AIPlayerJH::InitResourceMaps()
{
    mapState_->InitResourceMaps();
}

void AIPlayerJH::SetFarmedNodes(const MapPoint pt, bool set)
{
    mapState_->SetFarmedNodes(pt, set);
}

MapPoint AIPlayerJH::FindBestPosition(const MapPoint& pt, AIResource res, BuildingQuality size, unsigned radius,
                                      int minimum)
{
    return mapState_->FindBestPosition(pt, res, size, radius, minimum);
}

void AIPlayerJH::RecalcGround(const MapPoint buildingPos, std::vector<Direction>& route_road)
{
    mapState_->RecalcGround(buildingPos, route_road);
}

void AIPlayerJH::SaveResourceMapsToFile()
{
    mapState_->SaveResourceMapsToFile();
}

int AIPlayerJH::GetResMapValue(const MapPoint pt, AIResource res) const
{
    return mapState_->GetResMapValue(pt, res);
}

AIResourceMap& AIPlayerJH::GetResMap(AIResource res)
{
    return mapState_->GetResMap(res);
}

const AIResourceMap& AIPlayerJH::GetResMap(AIResource res) const
{
    return mapState_->GetResMap(res);
}

Node& AIPlayerJH::GetAINode(MapPoint pt)
{
    return mapState_->GetAINode(pt);
}

const Node& AIPlayerJH::GetAINode(MapPoint pt) const
{
    return mapState_->GetAINode(pt);
}

} // namespace AIJH
