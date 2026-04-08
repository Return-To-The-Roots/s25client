// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ai/aijh/runtime/AIMapState.h"

#include "ai/aijh/runtime/AIPlayerJH.h"
#include "RttrForeachPt.h"
#include "GlobalGameSettings.h"
#include "addons/const_addons.h"
#include "boost/filesystem/fstream.hpp"
#include "gameData/TerrainDesc.h"
#include "helpers/EnumRange.h"
#include "helpers/containerUtils.h"
#include "nodeObjs/noFlag.h"
#include "pathfinding/PathConditionRoad.h"

#include <queue>
#include <utility>

namespace {

using AIJH::AIMapState;
using AIJH::AIMap;
using AIJH::AIPlayerJH;
using AIJH::AIResourceMap;

static bool isUnlimitedResource(const AIResource res, const GlobalGameSettings& ggs)
{
    switch(res)
    {
        case AIResource::Gold:
        case AIResource::Ironore:
        case AIResource::Coal: return ggs.isEnabled(AddonId::INEXHAUSTIBLE_MINES);
        case AIResource::Granite:
            return ggs.isEnabled(AddonId::INEXHAUSTIBLE_MINES) || ggs.isEnabled(AddonId::INEXHAUSTIBLE_GRANITEMINES);
        case AIResource::Fish: return ggs.isEnabled(AddonId::INEXHAUSTIBLE_FISH);
        default: return false;
    }
}

template<size_t... I>
static auto createResourceMaps(const AIQueryService& queries, const GameWorldBase& world, const AIMap& aiMap,
                               std::index_sequence<I...>)
{
    return helpers::EnumArray<AIResourceMap, AIResource>{
      AIResourceMap(AIResource(I), isUnlimitedResource(AIResource(I), world.GetGGS()), queries, world, aiMap)...};
}

static auto createResourceMaps(const AIQueryService& queries, const GameWorldBase& world, const AIMap& aiMap)
{
    return createResourceMaps(queries, world, aiMap,
                              std::make_index_sequence<helpers::NumEnumValues_v<AIResource>>{});
}

} // namespace

namespace AIJH {

AIMapState::AIMapState(AIPlayerJH& owner)
    : owner_(owner), resourceMaps_(createResourceMaps(owner.aii.Queries(), owner.gwb, aiMap_))
{}

AINodeResource AIMapState::CalcResource(MapPoint pt)
{
    const AISubSurfaceResource subRes = owner_.aii.GetSubsurfaceResource(pt);
    const AISurfaceResource surfRes = owner_.aii.GetSurfaceResource(pt);

    if(subRes == AISubSurfaceResource::Nothing)
    {
        if(surfRes == AISurfaceResource::Nothing)
        {
            if(owner_.gwb.IsOnRoad(pt))
                return AINodeResource::Nothing;
            if(!owner_.gwb.IsOfTerrain(pt, [](const TerrainDesc& desc) { return desc.IsVital(); }))
                return AINodeResource::Nothing;
            return AINodeResource::Plantspace;
        }
        return convertToNodeResource(surfRes);
    }

    switch(surfRes)
    {
        case AISurfaceResource::Stones:
        case AISurfaceResource::Wood: return AINodeResource::Multiple;
        case AISurfaceResource::Blocked: break;
        case AISurfaceResource::Nothing: return convertToNodeResource(subRes);
    }
    return AINodeResource::Nothing;
}

void AIMapState::InitReachableNodes()
{
    std::queue<MapPoint> toCheck;

    RTTR_FOREACH_PT(MapPoint, aiMap_.GetSize())
    {
        Node& node = aiMap_[pt];
        node.reachable = false;
        node.failed_penalty = 0;
        const auto* myFlag = owner_.gwb.GetSpecObj<noFlag>(pt);
        if(myFlag && myFlag->GetPlayer() == owner_.playerId)
        {
            node.reachable = true;
            toCheck.push(pt);
        }
    }

    IterativeReachableNodeChecker(std::move(toCheck));
}

void AIMapState::IterativeReachableNodeChecker(std::queue<MapPoint> toCheck)
{
    PathConditionRoad<GameWorldBase> roadPathChecker(owner_.gwb, false);
    while(!toCheck.empty())
    {
        const MapPoint curPt = toCheck.front();

        for(const MapPoint curNeighbour : aiMap_.GetNeighbours(curPt))
        {
            Node& node = aiMap_[curNeighbour];
            if(node.reachable)
                continue;

            if(roadPathChecker.IsNodeOk(curNeighbour))
            {
                if(node.failed_penalty == 0)
                {
                    node.reachable = true;
                    toCheck.push(curNeighbour);
                } else
                {
                    node.failed_penalty--;
                }
            }
        }
        toCheck.pop();
    }
}

void AIMapState::UpdateReachableNodes(const std::vector<MapPoint>& pts)
{
    std::queue<MapPoint> toCheck;

    for(const MapPoint& curPt : pts)
    {
        const auto* flag = owner_.gwb.GetSpecObj<noFlag>(curPt);
        if(flag && flag->GetPlayer() == owner_.playerId)
        {
            aiMap_[curPt].reachable = true;
            toCheck.push(curPt);
        } else
        {
            aiMap_[curPt].reachable = false;
        }
    }
    IterativeReachableNodeChecker(std::move(toCheck));
}

void AIMapState::InitNodes()
{
    aiMap_.Resize(owner_.gwb.GetSize());

    InitReachableNodes();

    RTTR_FOREACH_PT(MapPoint, aiMap_.GetSize())
    {
        Node& node = aiMap_[pt];

        node.bq = owner_.aii.GetBuildingQuality(pt);
        node.res = CalcResource(pt);
        node.owned = owner_.aii.IsOwnTerritory(pt);
        node.border = owner_.aii.IsBorder(pt);
        node.farmed = false;
    }
}

void AIMapState::UpdateNodesAround(MapPoint pt, unsigned radius)
{
    const std::vector<MapPoint> pts = owner_.gwb.GetPointsInRadius(pt, radius);
    UpdateReachableNodes(pts);
    for(const MapPoint& curPt : pts)
    {
        Node& node = aiMap_[curPt];
        node.bq = owner_.aii.GetBuildingQuality(curPt);
        node.owned = owner_.aii.IsOwnTerritory(curPt);
        node.border = owner_.aii.IsBorder(curPt);
    }
}

void AIMapState::InitResourceMaps()
{
    for(auto& resMap : resourceMaps_)
        resMap.init();
}

void AIMapState::SetFarmedNodes(MapPoint pt, bool set)
{
    const unsigned radius = 3;

    aiMap_[pt].farmed = set;
    const std::vector<MapPoint> pts = owner_.gwb.GetPointsInRadius(pt, radius);
    for(const MapPoint& curPt : pts)
        aiMap_[curPt].farmed = set;
}

MapPoint AIMapState::FindBestPosition(MapPoint pt, AIResource res, BuildingQuality size, unsigned radius, int minimum)
{
    resourceMaps_[res].updateAround(pt, radius);
    return resourceMaps_[res].findBestPosition(pt, size, radius, minimum).first;
}

void AIMapState::RecalcGround(MapPoint buildingPos, std::vector<Direction>& route_road)
{
    if(aiMap_[buildingPos].res == AIResource::Plantspace)
        aiMap_[buildingPos].res = AINodeResource::Nothing;

    const MapPoint flagPos = owner_.gwb.GetNeighbour(buildingPos, Direction::SouthEast);
    if(aiMap_[flagPos].res == AIResource::Plantspace)
        aiMap_[flagPos].res = AINodeResource::Nothing;

    MapPoint curPt = flagPos;
    for(const auto dir : route_road)
    {
        curPt = owner_.gwb.GetNeighbour(curPt, dir);
        if(aiMap_[curPt].res == AIResource::Plantspace)
            aiMap_[curPt].res = AINodeResource::Nothing;
    }
}

void AIMapState::SaveResourceMapsToFile()
{
    for(const auto res : helpers::enumRange<AIResource>())
    {
        bfs::ofstream file("resmap-" + std::to_string(static_cast<unsigned>(res)) + ".log");
        for(unsigned y = 0; y < aiMap_.GetHeight(); ++y)
        {
            if(y % 2 == 1)
                file << "  ";
            for(unsigned x = 0; x < aiMap_.GetWidth(); ++x)
                file << resourceMaps_[res][MapPoint(x, y)] << "   ";
            file << "\n";
        }
    }
}

void AIMapState::RefreshBuildingQualities()
{
    if(nodesWithOutdatedBQ_.empty())
        return;

    helpers::makeUnique(nodesWithOutdatedBQ_, MapPointLess());
    for(const MapPoint pt : nodesWithOutdatedBQ_)
        aiMap_[pt].bq = owner_.aii.GetBuildingQuality(pt);
    nodesWithOutdatedBQ_.clear();
}

} // namespace AIJH
