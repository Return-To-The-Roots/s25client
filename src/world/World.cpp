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

#include "rttrDefines.h" // IWYU pragma: keep
#include "world/World.h"
#include "world/MapGeometry.h"
#include "nodeObjs/noFlag.h"
#include "nodeObjs/noNothing.h"
#include "nodeObjs/noTree.h"
#if RTTR_ENABLE_ASSERTS
#include "nodeObjs/noMovable.h"
#endif
#include "FOWObjects.h"
#include "RoadSegment.h"
#include "helpers/containerUtils.h"
#include "gameTypes/ShipDirection.h"
#include "gameData/TerrainDesc.h"
#include <boost/bind.hpp>
#include <boost/foreach.hpp>
#include <set>
#include <stdexcept>

World::World() : noNodeObj(NULL) {}

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
    harbor_pos.push_back(MapPoint::Invalid());
    noNodeObj.reset(new noNothing);
}

void World::Unload()
{
    // Collect and destroy roads
    std::set<RoadSegment*> roadsegments;
    for(std::vector<MapNode>::const_iterator it = nodes.begin(); it != nodes.end(); ++it)
    {
        if(!it->obj || it->obj->GetGOT() != GOT_FLAG)
            continue;
        for(unsigned dir = 0; dir < Direction::COUNT; ++dir)
        {
            if(static_cast<noFlag*>(it->obj)->GetRoute(Direction::fromInt(dir)))
            {
                roadsegments.insert(static_cast<noFlag*>(it->obj)->GetRoute(Direction::fromInt(dir)));
            }
        }
    }

    for(std::set<RoadSegment*>::iterator it = roadsegments.begin(); it != roadsegments.end(); ++it)
        delete(*it);

    // Objekte vernichten
    for(std::vector<MapNode>::iterator it = nodes.begin(); it != nodes.end(); ++it)
    {
        deletePtr(it->obj);

        for(unsigned z = 0; z < it->fow.size(); ++z)
        {
            deletePtr(it->fow[z].object);
        }
    }

    // Figuren vernichten
    for(std::vector<MapNode>::iterator itNode = nodes.begin(); itNode != nodes.end(); ++itNode)
    {
        std::list<noBase*>& nodeFigures = itNode->figures;
        for(std::list<noBase*>::iterator it = nodeFigures.begin(); it != nodeFigures.end(); ++it)
            delete(*it);

        nodeFigures.clear();
    }

    catapult_stones.clear();
    harbor_pos.clear();
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

void World::AddFigure(const MapPoint pt, noBase* fig)
{
    if(!fig)
        return;

    std::list<noBase*>& figures = GetNodeInt(pt).figures;
    RTTR_Assert(!helpers::contains(figures, fig));
    figures.push_back(fig);

#if RTTR_ENABLE_ASSERTS
    for(unsigned char i = 0; i < 6; ++i)
    {
        MapPoint nb = GetNeighbour(pt, Direction::fromInt(i));
        RTTR_Assert(!helpers::contains(GetNode(nb).figures, fig)); // Added figure that is in surrounding?
    }
#endif
}

void World::RemoveFigure(const MapPoint pt, noBase* fig)
{
    RTTR_Assert(helpers::contains(GetNode(pt).figures, fig));
    GetNodeInt(pt).figures.remove(fig);
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
    RTTR_Assert(replace || obj == NULL || GetNode(pt).obj == NULL);
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
        GetNodeInt(pt).obj = NULL;
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
        return GOT_NOTHING;
}

void World::ReduceResource(const MapPoint pt)
{
    uint8_t curAmount = GetNodeInt(pt).resources.getAmount();
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
    if(vis == VIS_VISIBLE)
        deletePtr(node.object);
    else if(vis == VIS_FOW)
        SaveFOWNode(pt, player, fowTime);
    VisibilityChanged(pt, player, oldVis, vis);
}

void World::ChangeAltitude(const MapPoint pt, const unsigned char altitude)
{
    GetNodeInt(pt).altitude = altitude;

    // Schattierung neu berechnen von diesem Punkt und den Punkten drumherum
    RecalcShadow(pt);
    for(unsigned i = 0; i < 6; ++i)
        RecalcShadow(GetNeighbour(pt, Direction::fromInt(i)));

    // Abgeleiteter Klasse Bescheid sagen
    AltitudeChanged(pt);
}

bool World::IsPlayerTerritory(const MapPoint pt, const unsigned char owner) const
{
    const unsigned char ptOwner = GetNode(pt).owner;

    if(owner != 0 && ptOwner != owner)
        return false;

    // Neighbour nodes must belong to this player
    for(unsigned i = 0; i < Direction::COUNT; ++i)
    {
        if(GetNeighbourNode(pt, Direction::fromInt(i)).owner != ptOwner)
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
    if(nodeBQ == BQ_NOTHING || !IsPlayerTerritory(pt, player + 1))
        return BQ_NOTHING;
    // If we could build a building, but the buildings flag point is at the border, we can only build a flag
    if(nodeBQ != BQ_FLAG && !IsPlayerTerritory(GetNeighbour(pt, Direction::SOUTHEAST)))
    {
        // Check for close flags, that prohibit to build a flag but not a building at this spot
        for(unsigned i = Direction::WEST; i <= Direction::NORTHEAST; i++)
        {
            if(GetNO(GetNeighbour(pt, Direction::fromInt(i)))->GetBM() == BlockingManner::Flag)
                return BQ_NOTHING;
        }
        return BQ_FLAG;
    } else
        return nodeBQ;
}

DescIdx<TerrainDesc> World::GetRightTerrain(const MapPoint pt, Direction dir) const
{
    switch(dir.native_value())
    {
        case Direction::WEST: return GetNeighbourNode(pt, Direction::NORTHWEST).t1;
        case Direction::NORTHWEST: return GetNeighbourNode(pt, Direction::NORTHWEST).t2;
        case Direction::NORTHEAST: return GetNeighbourNode(pt, Direction::NORTHEAST).t1;
        case Direction::EAST: return GetNode(pt).t2;
        case Direction::SOUTHEAST: return GetNode(pt).t1;
        case Direction::SOUTHWEST: return GetNeighbourNode(pt, Direction::WEST).t2;
    }
    throw std::logic_error("Invalid direction");
}

DescIdx<TerrainDesc> World::GetLeftTerrain(const MapPoint pt, Direction dir) const
{
    // We can find the left terrain by going a bit more left/counter-clockwise and take the right terrain
    return GetRightTerrain(pt, dir - 1u);
}

void World::SaveFOWNode(const MapPoint pt, const unsigned player, unsigned curTime)
{
    FoWNode& fow = GetNodeInt(pt).fow[player];
    fow.last_update_time = curTime;

    // FOW-Objekt erzeugen
    noBase* obj = GetNO(pt);
    deletePtr(fow.object);
    fow.object = obj->CreateFOWObject();

    // Wege speichern, aber nur richtige, keine, die gerade gebaut werden
    for(unsigned i = 0; i < 3; ++i)
        fow.roads[i] = GetNode(pt).roads[i];

    // Store ownership so FoW boundary stones can be drawn
    fow.owner = GetNode(pt).owner;
    // Grenzsteine merken
    fow.boundary_stones = GetNode(pt).boundary_stones;
}

bool World::IsSeaPoint(const MapPoint pt) const
{
    return World::IsOfTerrain(pt, boost::bind(&TerrainDesc::Is, _1, ETerrain::Shippable));
}

bool World::IsWaterPoint(const MapPoint pt) const
{
    return World::IsOfTerrain(pt, boost::bind(&TerrainDesc::kind, _1) == TerrainKind::WATER);
}

unsigned World::GetSeaSize(const unsigned seaId) const
{
    RTTR_Assert(seaId > 0 && seaId <= seas.size());
    return seas[seaId - 1].nodes_count;
}

unsigned short World::GetSeaId(const unsigned harborId, const Direction dir) const
{
    RTTR_Assert(harborId);
    return harbor_pos[harborId].cps[dir.toUInt()].seaId;
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
    for(unsigned i = 2; i < 6 + 2; ++i)
    {
        if(harbor_pos[harborId].cps[i % 6].seaId == seaId)
            return GetNeighbour(harbor_pos[harborId].pos, Direction(i));
    }

    // Keinen Punkt gefunden
    return MapPoint::Invalid();
}

unsigned char World::GetRoad(const MapPoint pt, unsigned char dir) const
{
    RTTR_Assert(dir < 3);

    return GetNode(pt).roads[dir];
}

unsigned char World::GetPointRoad(const MapPoint pt, Direction dir) const
{
    if(dir.toUInt() >= 3u)
        return GetRoad(pt, dir.toUInt() - 3u);
    else
        return GetRoad(GetNeighbour(pt, dir), dir.toUInt());
}

unsigned char World::GetPointFOWRoad(MapPoint pt, Direction dir, const unsigned char viewing_player) const
{
    if(dir.toUInt() >= 3)
        dir = dir - 3u;
    else
        pt = GetNeighbour(pt, dir);

    return GetNode(pt).fow[viewing_player].roads[dir.toUInt()];
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

const std::vector<HarborPos::Neighbor>& World::GetHarborNeighbors(const unsigned harborId, const ShipDirection& dir) const
{
    RTTR_Assert(harborId);
    return harbor_pos[harborId].neighbors[dir.toUInt()];
}

/// Berechnet die Entfernung zwischen 2 Hafenpunkten
unsigned World::CalcHarborDistance(unsigned habor_id1, unsigned harborId2) const
{
    if(habor_id1 == harborId2) // special case: distance to self
        return 0;
    for(unsigned i = 0; i < 6; ++i)
    {
        BOOST_FOREACH(const HarborPos::Neighbor& n, harbor_pos[habor_id1].neighbors[i])
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
    for(unsigned i = 0; i < Direction::COUNT; ++i)
    {
        unsigned short seaId = GetNeighbourNode(pt, Direction::fromInt(i)).seaId;
        if(seaId)
        {
            // Check size (TODO: Others checks like harbor spots?)
            if(GetSeaSize(seaId) > 20)
                return seaId;
        }
    }

    return 0;
}

void World::SetRoad(const MapPoint pt, unsigned char roadDir, unsigned char type)
{
    RTTR_Assert(roadDir < 3);
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
    int A = GetNeighbourNode(pt, Direction::NORTHEAST).altitude - altitude;
    int B = GetNode(GetNeighbour2(pt, 0)).altitude - altitude;
    int C = GetNode(GetNeighbour(pt, Direction::WEST)).altitude - altitude;
    int D = GetNode(GetNeighbour2(pt, 11)).altitude - altitude;

    int shadingS2 = 64 + 9 * A - 3 * B - 6 * C - 9 * D;
    if(shadingS2 > 128)
        shadingS2 = 128;
    else if(shadingS2 < 0)
        shadingS2 = 0;
    GetNodeInt(pt).shadow = shadingS2;
}
