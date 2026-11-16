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
    RTTR_Assert(getSize() == MapExtent::all(0)); // Already init
    RTTR_Assert(mapSize.x > 0 && mapSize.y > 0); // No empty map
    Resize(mapSize);
    if(!lt)
        throw std::runtime_error("Invalid landscape");
    this->lt = lt;
    GameObject::ResetCounters();

    noNodeObj = std::make_unique<noNothing>();
}

void World::Unload()
{
    // Collect and destroy roads
    std::set<RoadSegment*> roadsegments;
    for(const auto& node : nodes)
    {
        if(!node.obj || node.obj->getGOT() != GO_Type::Flag)
            continue;
        for(const auto dir : helpers::EnumRange<Direction>{})
        {
            if(static_cast<noFlag*>(node.obj)->getRoute(dir))
            {
                roadsegments.insert(static_cast<noFlag*>(node.obj)->getRoute(dir));
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
    harborData.clear();
    description_ = WorldDescription();
    noNodeObj.reset();
    Resize(MapExtent::all(0));
}

void World::Resize(const MapExtent& newSize)
{
    MapBase::Resize(newSize);
    nodes.clear();
    militarySquares.Clear();
    if(getSize().x > 0)
    {
        nodes.resize(prodOfComponents(getSize()));
        militarySquares.Init(getSize());
    }
}

noBase& World::AddFigureImpl(const MapPoint pt, std::unique_ptr<noBase> fig)
{
    RTTR_Assert(fig);

    auto& figures = getNodeInt(pt).figures;
#if RTTR_ENABLE_ASSERTS
    RTTR_Assert(!helpers::containsPtr(figures, fig.get()));
    for(const MapPoint nb : getNeighbours(pt))
        RTTR_Assert(!helpers::containsPtr(getNode(nb).figures, fig.get())); // Added figure that is in surrounding?
#endif

    noBase& result = *fig;
    figures.push_back(std::move(fig));
    return result;
}

noBase* World::RemoveFigureImpl(const MapPoint pt, noBase& fig)
{
    return helpers::extractPtr(getNodeInt(pt).figures, &fig).release();
}

noBase* World::getNO(const MapPoint pt)
{
    if(getNode(pt).obj)
        return getNode(pt).obj;
    else
        return noNodeObj.get();
}

const noBase* World::getNO(const MapPoint pt) const
{
    if(getNode(pt).obj)
        return getNode(pt).obj;
    else
        return noNodeObj.get();
}

void World::setNO(const MapPoint pt, noBase* obj, const bool replace /* = false*/)
{
    RTTR_Assert(replace || obj == nullptr || getNode(pt).obj == nullptr);
#if RTTR_ENABLE_ASSERTS
    RTTR_Assert(!dynamic_cast<noMovable*>(obj)); // It should be a static, non-movable object
#endif
    getNodeInt(pt).obj = obj;
}

void World::DestroyNO(const MapPoint pt, const bool checkExists /* = true*/)
{
    noBase* obj = getNodeInt(pt).obj;
    if(obj)
    {
        // Destroy may remove the NO already from the map or replace it (e.g. building -> fire)
        // So remove from map, then destroy and free
        getNodeInt(pt).obj = nullptr;
        obj->Destroy();
        deletePtr(obj);
    } else
        RTTR_Assert(!checkExists);
}

/// Returns the GOT if an object or GOT_NOTHING if none
GO_Type World::getGOT(const MapPoint pt) const
{
    noBase* obj = getNode(pt).obj;
    if(obj)
        return obj->getGOT();
    else
        return GO_Type::Nothing;
}

void World::ReduceResource(const MapPoint pt)
{
    const uint8_t curAmount = getNodeInt(pt).resources.getAmount();
    RTTR_Assert(curAmount > 0);
    getNodeInt(pt).resources.setAmount(curAmount - 1u);
}

void World::setReserved(const MapPoint pt, const bool reserved)
{
    RTTR_Assert(getNodeInt(pt).reserved != reserved);
    getNodeInt(pt).reserved = reserved;
}

void World::setVisibility(const MapPoint pt, unsigned char player, Visibility vis, unsigned fowTime)
{
    FoWNode& node = getNodeInt(pt).fow[player];
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
    getNodeInt(pt).altitude = altitude;

    // Schattierung neu berechnen von diesem Punkt und den Punkten drumherum
    RecalcShadow(pt);
    for(const MapPoint nb : getNeighbours(pt))
        RecalcShadow(nb);

    // Abgeleiteter Klasse Bescheid sagen
    AltitudeChanged(pt);
}

bool World::isPlayerTerritory(const MapPoint pt, const unsigned char owner) const
{
    const unsigned char ptOwner = getNode(pt).owner;

    if(owner != 0 && ptOwner != owner)
        return false;

    // Neighbour nodes must belong to this player
    for(const MapPoint nb : getNeighbours(pt))
    {
        if(getNode(nb).owner != ptOwner)
            return false;
    }

    return true;
}

BuildingQuality World::GetBQ(const MapPoint pt, const unsigned char player) const
{
    return AdjustBQ(pt, player, getNode(pt).bq);
}

BuildingQuality World::AdjustBQ(const MapPoint pt, unsigned char player, BuildingQuality nodeBQ) const
{
    if(nodeBQ == BuildingQuality::Nothing || !isPlayerTerritory(pt, player + 1))
        return BuildingQuality::Nothing;
    // If we could build a building, but the buildings flag point is at the border, we can only build a flag
    if(nodeBQ != BuildingQuality::Flag && !isPlayerTerritory(getNeighbour(pt, Direction::SouthEast)))
    {
        // Check for close flags, that prohibit to build a flag but not a building at this spot
        for(const Direction dir : {Direction::West, Direction::NorthWest, Direction::NorthEast})
        {
            if(getNO(getNeighbour(pt, dir))->getBM() == BlockingManner::Flag)
                return BuildingQuality::Nothing;
        }
        return BuildingQuality::Flag;
    } else
        return nodeBQ;
}

bool World::hasFigureAt(const MapPoint pt, const noBase& figure) const
{
    return helpers::containsPtr(getNode(pt).figures, &figure);
}

WalkTerrain World::getTerrain(MapPoint pt, Direction dir) const
{
    // Manually inlined code from GetNeighbors. Measured to greatly improve performance
    const MapExtent size = getSize();
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
            return {getNode(wPt).t2, getNode(nwPt).t1};
        }
        case Direction::NorthWest:
        {
            const MapNode& node = getNode(nwPt);
            return {node.t1, node.t2};
        }
        case Direction::NorthEast:
        {
            return {getNode(nwPt).t2, getNode(nePt).t1};
        }
        case Direction::East:
        {
            return {getNode(nePt).t1, getNode(pt).t2};
        }
        case Direction::SouthEast:
        {
            const MapNode& node = getNode(pt);
            return {node.t2, node.t1};
        }
        case Direction::SouthWest:
        {
            return {getNode(pt).t1, getNode(wPt).t2};
        }
    }
    throw std::logic_error("Invalid direction");
}

helpers::EnumArray<DescIdx<TerrainDesc>, Direction> World::getTerrainsAround(MapPoint pt) const
{
    // Manually inlined code from GetNeighbors. Measured to greatly improve performance
    const MapExtent size = getSize();
    const MapCoord yminus1 = (pt.y == 0 ? size.y : pt.y) - 1;
    const MapCoord xplus1 = pt.x == size.x - 1 ? 0 : pt.x + 1;
    const MapCoord xminus1 = (pt.x == 0 ? size.x : pt.x) - 1;
    const bool isEvenRow = (pt.y & 1) == 0;

    const MapPoint wPt(xminus1, pt.y);
    const MapPoint nwPt(!isEvenRow ? pt.x : xminus1, yminus1);
    const MapPoint nePt(isEvenRow ? pt.x : xplus1, yminus1);

    const MapNode& nwNode = getNode(nwPt);
    const MapNode& neNode = getNode(nePt);
    const MapNode& curNode = getNode(pt);
    const MapNode& wNode = getNode(wPt);
    helpers::EnumArray<DescIdx<TerrainDesc>, Direction> result{nwNode.t1,  nwNode.t2,  neNode.t1,
                                                               curNode.t2, curNode.t1, wNode.t2};
    return result;
}

void World::SaveFOWNode(const MapPoint pt, const unsigned player, unsigned curTime)
{
    FoWNode& fow = getNodeInt(pt).fow[player];
    fow.last_update_time = curTime;

    // FOW-Objekt erzeugen
    fow.object = getNO(pt)->CreateFOWObject();

    // Wege speichern, aber nur richtige, keine, die gerade gebaut werden
    for(const auto dir : helpers::EnumRange<RoadDir>{})
        fow.roads[dir] = getNode(pt).roads[dir];

    // Store ownership so FoW boundary stones can be drawn
    fow.owner = getNode(pt).owner;
    // Grenzsteine merken
    fow.boundary_stones = getNode(pt).boundary_stones;
}

bool World::isSeaPoint(const MapPoint pt) const
{
    return World::isOfTerrain(pt, [](const auto& desc) { return desc.Is(ETerrain::Shippable); });
}

bool World::isWaterPoint(const MapPoint pt) const
{
    return World::isOfTerrain(pt, [](const auto& desc) { return desc.kind == TerrainKind::Water; });
}

unsigned World::getSeaSize(const SeaId seaId) const
{
    RTTR_Assert(seaId && seaId.value() <= seas.size());
    return seas[seaId].nodes_count;
}

SeaId World::getSeaId(const HarborId harborId, const Direction dir) const
{
    RTTR_Assert(harborId);
    return harborData[harborId].seaIds[dir];
}

bool World::isHarborAtSea(const HarborId harborId, const SeaId seaId) const
{
    return getCoastalPoint(harborId, seaId).isValid();
}

MapPoint World::getCoastalPoint(const HarborId harborId, const SeaId seaId) const
{
    RTTR_Assert(harborId);
    RTTR_Assert(seaId);

    // Take point at NW last as often there is no path from it if the harbor is north of an island
    for(auto dir : helpers::enumRange(Direction::NorthEast))
    {
        if(harborData[harborId].seaIds[dir] == seaId)
            return getNeighbour(harborData[harborId].pos, dir);
    }

    // Keinen Punkt gefunden
    return MapPoint::Invalid();
}

PointRoad World::getRoad(const MapPoint pt, RoadDir dir) const
{
    return getNode(pt).roads[dir];
}

PointRoad World::getPointRoad(MapPoint pt, Direction dir) const
{
    const RoadDir rDir = toRoadDir(pt, dir);
    return getRoad(pt, rDir);
}

PointRoad World::getPointFOWRoad(MapPoint pt, Direction dir, const unsigned char viewing_player) const
{
    const RoadDir rDir = toRoadDir(pt, dir);
    return getNode(pt).fow[viewing_player].roads[rDir];
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

MapPoint World::getHarborPoint(const HarborId harborId) const
{
    RTTR_Assert(harborId);

    return harborData[harborId].pos;
}

const std::vector<HarborPos::Neighbor>& World::getHarborNeighbors(const HarborId harborId,
                                                                  const ShipDirection& dir) const
{
    RTTR_Assert(harborId);
    return harborData[harborId].neighbors[dir];
}

unsigned World::getMinHarborDistance(HarborId haborId1, HarborId harborId2) const
{
    if(haborId1 == harborId2) // special case: distance to self
        return 0;
    unsigned minDistance = std::numeric_limits<unsigned>::max();
    for(const auto dir : helpers::EnumRange<ShipDirection>{})
    {
        for(const HarborPos::Neighbor& n : harborData[haborId1].neighbors[dir])
        {
            if(n.id == harborId2)
                minDistance = std::min(minDistance, n.distance);
        }
    }
    return minDistance;
}

unsigned World::getHarborDistance(HarborId haborId1, HarborId harborId2, SeaId sea) const
{
    if(haborId1 == harborId2) // special case: distance to self
        return 0;
    for(const auto dir : helpers::EnumRange<ShipDirection>{})
    {
        for(const HarborPos::Neighbor& n : harborData[haborId1].neighbors[dir])
        {
            if(n.id == harborId2 && n.sea == sea)
                return n.distance;
        }
    }
    RTTR_Assert(!"Should only query actual neighbors");
    return std::numeric_limits<unsigned>::max();
}

SeaId World::getSeaFromCoastalPoint(const MapPoint pt) const
{
    // Point itself must not be a sea
    if(getNode(pt).seaId)
        return SeaId::invalidValue();

    // Should not be inside water itself
    if(isWaterPoint(pt))
        return SeaId::invalidValue();

    // Surrounding must be valid sea
    for(const MapPoint nb : getNeighbours(pt))
    {
        SeaId seaId = getNode(nb).seaId;
        if(seaId)
        {
            // Check size (TODO: Others checks like harbor spots?)
            if(getSeaSize(seaId) > 20)
                return seaId;
        }
    }

    return SeaId::invalidValue();
}

void World::setRoad(const MapPoint pt, RoadDir roadDir, PointRoad type)
{
    getNodeInt(pt).roads[roadDir] = type;
}

bool World::setBQ(const MapPoint pt, BuildingQuality bq)
{
    BuildingQuality oldBQ = bq;
    std::swap(getNodeInt(pt).bq, oldBQ);
    return oldBQ != bq;
}

void World::RecalcShadow(const MapPoint pt)
{
    int altitude = getNode(pt).altitude;
    int A = getNeighbourNode(pt, Direction::NorthEast).altitude - altitude;
    int B = getNode(getNeighbour2(pt, 0)).altitude - altitude;
    int C = getNode(getNeighbour(pt, Direction::West)).altitude - altitude;
    int D = getNode(getNeighbour2(pt, 11)).altitude - altitude;

    int shadingS2 = 64 + 9 * A - 3 * B - 6 * C - 9 * D;
    if(shadingS2 > 128)
        shadingS2 = 128;
    else if(shadingS2 < 0)
        shadingS2 = 0;
    getNodeInt(pt).shadow = shadingS2;
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
