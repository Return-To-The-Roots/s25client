// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "world/World.h"
#include "nodeObjs/noFlag.h"
#include "nodeObjs/noNothing.h"
#if RTTR_ENABLE_ASSERTS
#    include "nodeObjs/noMovable.h"
#endif
#include "FOWObjects.h"
#include "RoadSegment.h"
#include "enum_cast.hpp"
#include "helpers/containerUtils.h"
#include "helpers/pointerContainerUtils.h"
#include "gameTypes/ShipDirection.h"
#include "gameData/TerrainDesc.h"
#include <memory>
#include <set>
#include <stdexcept>

World::World() : noNodeObj(nullptr) {}

World::~World()
{
    Unload();
}

void World::Init(const MapExtent& mapSize, DescIdx<LandscapeDesc> lt)
{
    RTTR_Assert(GetSize() == MapExtent::all(0)); // Already init
    RTTR_Assert(mapSize.x > 0 && mapSize.y > 0); // No empty map
    Resize(mapSize);
    if(!lt)
        throw std::runtime_error("Invalid landscape");
    this->lt = lt;
    GameObject::ResetCounters();

    // Dummy so that the harbor "0" might be used for ships with no particular destination
    harbor_pos.push_back(HarborPos(MapPoint::Invalid()));
    noNodeObj = std::make_unique<noNothing>();
}

void World::Unload()
{
    // Collect and destroy roads
    std::set<RoadSegment*> roadsegments;
    for(const auto& node : nodes)
    {
        if(!node.obj || node.obj->GetGOT() != GO_Type::Flag)
            continue;
        for(const auto dir : helpers::EnumRange<Direction>{})
        {
            if(static_cast<noFlag*>(node.obj)->GetRoute(dir))
            {
                roadsegments.insert(static_cast<noFlag*>(node.obj)->GetRoute(dir));
            }
        }
    }

    for(auto* roadsegment : roadsegments)
        delete roadsegment;

    // Objekte vernichten
    for(auto& node : nodes)
        deletePtr(node.obj);

    // Figuren vernichten
    for(auto& node : nodes)
        node.figures.clear();

    catapult_stones.clear();
    harbor_pos.clear();
    description_ = WorldDescription();
    noNodeObj.reset();
    Resize(MapExtent::all(0));
}

void World::Resize(const MapExtent& newSize)
{
    MapBase::Resize(newSize);
    nodes.clear();
    militarySquares.Clear();
    if(GetSize().x > 0)
    {
        nodes.resize(prodOfComponents(GetSize()));
        militarySquares.Init(GetSize());
    }
}

noBase& World::AddFigureImpl(const MapPoint pt, std::unique_ptr<noBase> fig)
{
    RTTR_Assert(fig);

    auto& figures = GetNodeInt(pt).figures;
#if RTTR_ENABLE_ASSERTS
    RTTR_Assert(!helpers::containsPtr(figures, fig.get()));
    for(const MapPoint nb : GetNeighbours(pt))
        RTTR_Assert(!helpers::containsPtr(GetNode(nb).figures, fig.get())); // Added figure that is in surrounding?
#endif

    noBase& result = *fig;
    figures.push_back(std::move(fig));
    return result;
}

noBase* World::RemoveFigureImpl(const MapPoint pt, noBase& fig)
{
    return helpers::extractPtr(GetNodeInt(pt).figures, &fig).release();
}

noBase* World::GetNO(const MapPoint pt)
{
    if(GetNode(pt).obj)
        return GetNode(pt).obj;
    else
        return noNodeObj.get();
}

const noBase* World::GetNO(const MapPoint pt) const
{
    if(GetNode(pt).obj)
        return GetNode(pt).obj;
    else
        return noNodeObj.get();
}

void World::SetNO(const MapPoint pt, noBase* obj, const bool replace /* = false*/)
{
    RTTR_Assert(replace || obj == nullptr || GetNode(pt).obj == nullptr);
#if RTTR_ENABLE_ASSERTS
    RTTR_Assert(!dynamic_cast<noMovable*>(obj)); // It should be a static, non-movable object
#endif
    GetNodeInt(pt).obj = obj;
}

void World::DestroyNO(const MapPoint pt, const bool checkExists /* = true*/)
{
    noBase* obj = GetNodeInt(pt).obj;
    if(obj)
    {
        // Destroy may remove the NO already from the map or replace it (e.g. building -> fire)
        // So remove from map, then destroy and free
        GetNodeInt(pt).obj = nullptr;
        obj->Destroy();
        deletePtr(obj);
    } else
        RTTR_Assert(!checkExists);
}

/// Returns the GOT if an object or GOT_NOTHING if none
GO_Type World::GetGOT(const MapPoint pt) const
{
    noBase* obj = GetNode(pt).obj;
    if(obj)
        return obj->GetGOT();
    else
        return GO_Type::Nothing;
}

void World::ReduceResource(const MapPoint pt)
{
    const uint8_t curAmount = GetNodeInt(pt).resources.getAmount();
    RTTR_Assert(curAmount > 0);
    GetNodeInt(pt).resources.setAmount(curAmount - 1u);
}

void World::SetReserved(const MapPoint pt, const bool reserved)
{
    RTTR_Assert(GetNodeInt(pt).reserved != reserved);
    GetNodeInt(pt).reserved = reserved;
}

void World::SetVisibility(const MapPoint pt, unsigned char player, Visibility vis, unsigned fowTime)
{
    FoWNode& node = GetNodeInt(pt).fow[player];
    Visibility oldVis = node.visibility;
    if(oldVis == vis)
        return;

    node.visibility = vis;
    if(vis == Visibility::Visible)
        node.object.reset();
    else if(vis == Visibility::FogOfWar)
        SaveFOWNode(pt, player, fowTime);
    VisibilityChanged(pt, player, oldVis, vis);
}

void World::ChangeAltitude(const MapPoint pt, const unsigned char altitude)
{
    GetNodeInt(pt).altitude = altitude;

    // Schattierung neu berechnen von diesem Punkt und den Punkten drumherum
    RecalcShadow(pt);
    for(const MapPoint nb : GetNeighbours(pt))
        RecalcShadow(nb);

    // Abgeleiteter Klasse Bescheid sagen
    AltitudeChanged(pt);
}

bool World::IsPlayerTerritory(const MapPoint pt, const unsigned char owner) const
{
    const unsigned char ptOwner = GetNode(pt).owner;

    if(owner != 0 && ptOwner != owner)
        return false;

    // Neighbour nodes must belong to this player
    for(const MapPoint nb : GetNeighbours(pt))
    {
        if(GetNode(nb).owner != ptOwner)
            return false;
    }

    return true;
}

BuildingQuality World::GetBQ(const MapPoint pt, const unsigned char player) const
{
    return AdjustBQ(pt, player, GetNode(pt).bq);
}

BuildingQuality World::AdjustBQ(const MapPoint pt, unsigned char player, BuildingQuality nodeBQ) const
{
    if(nodeBQ == BuildingQuality::Nothing || !IsPlayerTerritory(pt, player + 1))
        return BuildingQuality::Nothing;
    // If we could build a building, but the buildings flag point is at the border, we can only build a flag
    if(nodeBQ != BuildingQuality::Flag && !IsPlayerTerritory(GetNeighbour(pt, Direction::SouthEast)))
    {
        // Check for close flags, that prohibit to build a flag but not a building at this spot
        for(const Direction dir : {Direction::West, Direction::NorthWest, Direction::NorthEast})
        {
            if(GetNO(GetNeighbour(pt, dir))->GetBM() == BlockingManner::Flag)
                return BuildingQuality::Nothing;
        }
        return BuildingQuality::Flag;
    } else
        return nodeBQ;
}

bool World::HasFigureAt(const MapPoint pt, const noBase& figure) const
{
    return helpers::containsPtr(GetNode(pt).figures, &figure);
}

WalkTerrain World::GetTerrain(MapPoint pt, Direction dir) const
{
    // Manually inlined code from GetNeighbors. Measured to greatly improve performance
    const MapExtent size = GetSize();
    const MapCoord yminus1 = (pt.y == 0 ? size.y : pt.y) - 1;
    const MapCoord xplus1 = pt.x == size.x - 1 ? 0 : pt.x + 1;
    const MapCoord xminus1 = (pt.x == 0 ? size.x : pt.x) - 1;
    const bool isEvenRow = (pt.y & 1) == 0;

    const MapPoint wPt(xminus1, pt.y);
    const MapPoint nwPt(!isEvenRow ? pt.x : xminus1, yminus1);
    const MapPoint nePt(isEvenRow ? pt.x : xplus1, yminus1);
    switch(dir)
    {
        case Direction::West:
        {
            return {GetNode(wPt).t2, GetNode(nwPt).t1};
        }
        case Direction::NorthWest:
        {
            const MapNode& node = GetNode(nwPt);
            return {node.t1, node.t2};
        }
        case Direction::NorthEast:
        {
            return {GetNode(nwPt).t2, GetNode(nePt).t1};
        }
        case Direction::East:
        {
            return {GetNode(nePt).t1, GetNode(pt).t2};
        }
        case Direction::SouthEast:
        {
            const MapNode& node = GetNode(pt);
            return {node.t2, node.t1};
        }
        case Direction::SouthWest:
        {
            return {GetNode(pt).t1, GetNode(wPt).t2};
        }
    }
    throw std::logic_error("Invalid direction");
}

helpers::EnumArray<DescIdx<TerrainDesc>, Direction> World::GetTerrainsAround(MapPoint pt) const
{
    // Manually inlined code from GetNeighbors. Measured to greatly improve performance
    const MapExtent size = GetSize();
    const MapCoord yminus1 = (pt.y == 0 ? size.y : pt.y) - 1;
    const MapCoord xplus1 = pt.x == size.x - 1 ? 0 : pt.x + 1;
    const MapCoord xminus1 = (pt.x == 0 ? size.x : pt.x) - 1;
    const bool isEvenRow = (pt.y & 1) == 0;

    const MapPoint wPt(xminus1, pt.y);
    const MapPoint nwPt(!isEvenRow ? pt.x : xminus1, yminus1);
    const MapPoint nePt(isEvenRow ? pt.x : xplus1, yminus1);

    const MapNode& nwNode = GetNode(nwPt);
    const MapNode& neNode = GetNode(nePt);
    const MapNode& curNode = GetNode(pt);
    const MapNode& wNode = GetNode(wPt);
    helpers::EnumArray<DescIdx<TerrainDesc>, Direction> result{nwNode.t1,  nwNode.t2,  neNode.t1,
                                                               curNode.t2, curNode.t1, wNode.t2};
    return result;
}

void World::SaveFOWNode(const MapPoint pt, const unsigned player, unsigned curTime)
{
    FoWNode& fow = GetNodeInt(pt).fow[player];
    fow.last_update_time = curTime;

    // FOW-Objekt erzeugen
    fow.object = GetNO(pt)->CreateFOWObject();

    // Wege speichern, aber nur richtige, keine, die gerade gebaut werden
    for(const auto dir : helpers::EnumRange<RoadDir>{})
        fow.roads[dir] = GetNode(pt).roads[dir];

    // Store ownership so FoW boundary stones can be drawn
    fow.owner = GetNode(pt).owner;
    // Grenzsteine merken
    fow.boundary_stones = GetNode(pt).boundary_stones;
}

bool World::IsSeaPoint(const MapPoint pt) const
{
    return World::IsOfTerrain(pt, [](const auto& desc) { return desc.Is(ETerrain::Shippable); });
}

bool World::IsWaterPoint(const MapPoint pt) const
{
    return World::IsOfTerrain(pt, [](const auto& desc) { return desc.kind == TerrainKind::Water; });
}

unsigned World::GetSeaSize(const unsigned seaId) const
{
    RTTR_Assert(seaId > 0 && seaId <= seas.size());
    return seas[seaId - 1].nodes_count;
}

unsigned short World::GetSeaId(const unsigned harborId, const Direction dir) const
{
    RTTR_Assert(harborId);
    return harbor_pos[harborId].seaIds[dir];
}

/// Grenzt der Hafen an ein bestimmtes Meer an?
bool World::IsHarborAtSea(const unsigned harborId, const unsigned short seaId) const
{
    return GetCoastalPoint(harborId, seaId).isValid();
}

MapPoint World::GetCoastalPoint(const unsigned harborId, const unsigned short seaId) const
{
    RTTR_Assert(harborId);
    RTTR_Assert(seaId);

    // Take point at NW last as often there is no path from it if the harbor is north of an island
    for(auto dir : helpers::enumRange(Direction::NorthEast))
    {
        if(harbor_pos[harborId].seaIds[dir] == seaId)
            return GetNeighbour(harbor_pos[harborId].pos, dir);
    }

    // Keinen Punkt gefunden
    return MapPoint::Invalid();
}

PointRoad World::GetRoad(const MapPoint pt, RoadDir dir) const
{
    return GetNode(pt).roads[dir];
}

PointRoad World::GetPointRoad(MapPoint pt, Direction dir) const
{
    const RoadDir rDir = toRoadDir(pt, dir);
    return GetRoad(pt, rDir);
}

PointRoad World::GetPointFOWRoad(MapPoint pt, Direction dir, const unsigned char viewing_player) const
{
    const RoadDir rDir = toRoadDir(pt, dir);
    return GetNode(pt).fow[viewing_player].roads[rDir];
}

void World::AddCatapultStone(CatapultStone* cs)
{
    RTTR_Assert(!helpers::contains(catapult_stones, cs));
    catapult_stones.push_back(cs);
}

void World::RemoveCatapultStone(CatapultStone* cs)
{
    RTTR_Assert(helpers::contains(catapult_stones, cs));
    catapult_stones.remove(cs);
}

MapPoint World::GetHarborPoint(const unsigned harborId) const
{
    RTTR_Assert(harborId);

    return harbor_pos[harborId].pos;
}

const std::vector<HarborPos::Neighbor>& World::GetHarborNeighbors(const unsigned harborId,
                                                                  const ShipDirection& dir) const
{
    RTTR_Assert(harborId);
    return harbor_pos[harborId].neighbors[dir];
}

/// Berechnet die Entfernung zwischen 2 Hafenpunkten
unsigned World::CalcHarborDistance(unsigned habor_id1, unsigned harborId2) const
{
    if(habor_id1 == harborId2) // special case: distance to self
        return 0;
    for(const auto dir : helpers::EnumRange<ShipDirection>{})
    {
        for(const HarborPos::Neighbor& n : harbor_pos[habor_id1].neighbors[dir])
        {
            if(n.id == harborId2)
                return n.distance;
        }
    }

    return 0xffffffff;
}

unsigned short World::GetSeaFromCoastalPoint(const MapPoint pt) const
{
    // Point itself must not be a sea
    if(GetNode(pt).seaId)
        return 0;

    // Should not be inside water itself
    if(IsWaterPoint(pt))
        return 0;

    // Surrounding must be valid sea
    for(const MapPoint nb : GetNeighbours(pt))
    {
        unsigned short seaId = GetNode(nb).seaId;
        if(seaId)
        {
            // Check size (TODO: Others checks like harbor spots?)
            if(GetSeaSize(seaId) > 20)
                return seaId;
        }
    }

    return 0;
}

void World::SetRoad(const MapPoint pt, RoadDir roadDir, PointRoad type)
{
    GetNodeInt(pt).roads[roadDir] = type;
}

bool World::SetBQ(const MapPoint pt, BuildingQuality bq)
{
    BuildingQuality oldBQ = bq;
    std::swap(GetNodeInt(pt).bq, oldBQ);
    return oldBQ != bq;
}

void World::RecalcShadow(const MapPoint pt)
{
    int altitude = GetNode(pt).altitude;
    int A = GetNeighbourNode(pt, Direction::NorthEast).altitude - altitude;
    int B = GetNode(GetNeighbour2(pt, 0)).altitude - altitude;
    int C = GetNode(GetNeighbour(pt, Direction::West)).altitude - altitude;
    int D = GetNode(GetNeighbour2(pt, 11)).altitude - altitude;

    int shadingS2 = 64 + 9 * A - 3 * B - 6 * C - 9 * D;
    if(shadingS2 > 128)
        shadingS2 = 128;
    else if(shadingS2 < 0)
        shadingS2 = 0;
    GetNodeInt(pt).shadow = shadingS2;
}

void World::MakeWholeMapVisibleForAllPlayers()
{
    for(auto& mapNode : nodes)
    {
        for(auto& fowNode : mapNode.fow)
        {
            fowNode.visibility = Visibility::Visible;
            fowNode.object.reset();
        }
    }
}
