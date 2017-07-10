// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#include "defines.h" // IWYU pragma: keep
#include "world/World.h"
#include "world/MapGeometry.h"
#include "nodeObjs/noNothing.h"
#include "nodeObjs/noTree.h"
#include "nodeObjs/noFlag.h"
#if RTTR_ENABLE_ASSERTS
#   include "nodeObjs/noMovable.h"
#endif
#include "FOWObjects.h"
#include "gameData/TerrainData.h"
#include "RoadSegment.h"
#include "gameTypes/ShipDirection.h"
#include "helpers/containerUtils.h"
#include <set>

World::World(): width_(0), height_(0), lt(LT_GREENLAND), noNodeObj(NULL)
{
    noTree::ResetInstanceCounter();
    GameObject::ResetCounter();
}

World::~World()
{
    Unload();
}

void World::Init(unsigned short width, unsigned short height, LandscapeType lt)
{
    RTTR_Assert(width_ == 0 && height_ == 0); // Already init
    RTTR_Assert(width > 0 && height > 0);     // No empty map
    width_ = width;
    height_ = height;
    this->lt = lt;
    // Map-Knoten erzeugen
    nodes.resize(width_ * height_);
    militarySquares.Init(width, height);

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
        delete (*it);

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
            delete (*it);

        nodeFigures.clear();
    }

    catapult_stones.clear();

    width_ = height_ = 0;

    nodes.clear();
    militarySquares.Clear();
    harbor_pos.clear();
    noNodeObj.reset();
}

MapPoint World::GetNeighbour(const MapPoint pt, const Direction dir) const
{
    /*  Note that every 2nd row is shifted by half a triangle to the left, therefore:
    Modifications for the dirs:
    current row:    Even    Odd
                 W  -1|0   -1|0
    D           NW  -1|-1   0|-1
    I           NE   0|-1   1|-1
    R            E   1|0    1|0
                SE   0|1    1|1
                SW  -1|1    0|1
    */

    MapPoint res;
    switch(static_cast<Direction::Type>(dir))
    {
    case Direction::WEST: // -1|0   -1|0
        res.x = ((pt.x == 0) ? width_ : pt.x) - 1;
        res.y = pt.y;
        break;
    case Direction::NORTHWEST: // -1|-1   0|-1
        res.x = (pt.y & 1) ? pt.x : (((pt.x == 0) ? width_ : pt.x) - 1);
        res.y = ((pt.y == 0) ? height_ : pt.y) - 1;
        break;
    case Direction::NORTHEAST: // 0|-1  -1|-1
        res.x = (!(pt.y & 1)) ? pt.x : ((pt.x == width_ - 1) ? 0 : pt.x + 1);
        res.y = ((pt.y == 0) ? height_ : pt.y) - 1;
        break;
    case Direction::EAST: // 1|0    1|0
        res.x = pt.x + 1;
        if(res.x == width_)
            res.x = 0;
        res.y = pt.y;
        break;
    case Direction::SOUTHEAST: // 1|1    0|1
        res.x = (!(pt.y & 1)) ? pt.x : ((pt.x == width_ - 1) ? 0 : pt.x + 1);
        res.y = pt.y + 1;
        if(res.y == height_)
            res.y = 0;
        break;
    default:
        RTTR_Assert(dir == Direction::SOUTHWEST); // 0|1   -1|1
        res.x = (pt.y & 1) ? pt.x : (((pt.x == 0) ? width_ : pt.x) - 1); //-V537
        res.y = pt.y + 1;
        if(res.y == height_)
            res.y = 0;
        break;
    }

    // This should be the same, but faster
    //RTTR_Assert(res == MakeMapPoint(::GetNeighbour(Point<int>(pt), dir)));
    return res;
}

MapPoint World::GetNeighbour2(const MapPoint pt, unsigned dir) const
{
    return MakeMapPoint(::GetNeighbour2(Point<int>(pt), dir));
}

unsigned World::CalcDistance(Point<int> p1, Point<int> p2) const
{
    int dx = ((p1.x - p2.x) * 2) + (p1.y & 1) - (p2.y & 1);
    int dy = ((p1.y > p2.y) ? (p1.y - p2.y) : (p2.y - p1.y)) * 2;

    if(dx < 0)
        dx = -dx;

    if(dy > height_)
    {
        dy = (height_ * 2) - dy;
    }

    if(dx > width_)
    {
        dx = (width_ * 2) - dx;
    }

    dx -= dy / 2;

    return((dy + (dx > 0 ? dx : 0)) / 2);
}

ShipDirection World::GetShipDir(MapPoint fromPt, MapPoint toPt) const
{
    // First divide into NORTH/SOUTH by only looking at the y-Difference. On equal we choose SOUTH
    // Then choose between main dir (S/N) or partial E/W:
    //     6 directions -> 60deg covered per direction, mainDir +- 30deg
    //     -> Switching at an angle of 60deg compared to x-axis
    //     hence: |dy/dx| > tan(60deg) -> main dir, else add E or W

    unsigned dy = SafeDiff(fromPt.y, toPt.y);
    unsigned dx = SafeDiff(fromPt.x, toPt.x);
    // Handle wrapping. Also swap coordinates when wrapping (we reverse the direction)
    if(dy > height_ / 2u)
    {
        dy = height_ - dy;
        using std::swap;
        swap(fromPt.y, toPt.y);
    }
    if(dx > width_ / 2u)
    {
        dx = width_ - dx;
        using std::swap;
        swap(fromPt.x, toPt.x);
    }
    // tan(60deg) ~= 1.73205080757. Divider at |dy| > |dx| * 1.732 using fixed point math
    // (floating point math may lead to different results among configurations/platforms)
    bool isMainDir = dy * 1000 > dx * 1732;
    if(toPt.y < fromPt.y)
    {
        // North
        if(isMainDir)
            return ShipDirection::NORTH;
        else if(toPt.x < fromPt.x)
            return ShipDirection::NORTHWEST;
        else
            return ShipDirection::NORTHEAST;
    } else
    {
        // South
        if(isMainDir)
            return ShipDirection::SOUTH;
        else if(toPt.x < fromPt.x)
            return ShipDirection::SOUTHWEST;
        else
            return ShipDirection::SOUTHEAST;
    }
}

MapPoint World::MakeMapPoint(Point<int> pt) const
{
    return ::MakeMapPoint(pt, width_, height_);
}

void World::AddFigure(noBase* fig, const MapPoint pt)
{
    if(!fig)
        return;

    std::list<noBase*>& figures = GetNodeInt(pt).figures;
    RTTR_Assert(!helpers::contains(figures, fig));
    figures.push_back(fig);

#if RTTR_ENABLE_ASSERTS
    for(unsigned char i = 0; i < 6; ++i)
    {
        MapPoint nb = GetNeighbour(pt, i);
        RTTR_Assert(!helpers::contains(GetNode(nb).figures, fig)); // Added figure that is in surrounding?
    }
#endif
}

void World::RemoveFigure(noBase* fig, const MapPoint pt)
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

void World::SetNO(const MapPoint pt, noBase* obj, const bool replace/* = false*/)
{
    RTTR_Assert(replace || obj == NULL || GetNode(pt).obj == NULL);
#if RTTR_ENABLE_ASSERTS
    RTTR_Assert(!dynamic_cast<noMovable*>(obj)); // It should be a static, non-movable object
#endif
    GetNodeInt(pt).obj = obj;
}

void World::DestroyNO(const MapPoint pt, const bool checkExists/* = true*/)
{
    noBase* obj = GetNodeInt(pt).obj;
    if(obj)
    {
        // Destroy may remove the NO already from the map or replace it (e.g. building -> fire)
        // So remove from map, then destroy and free
        GetNodeInt(pt).obj = NULL;
        obj->Destroy();
        deletePtr(obj);
    }else
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
    RTTR_Assert(GetNodeInt(pt).resources > 0);
    GetNodeInt(pt).resources--;
}

void World::SetReserved(const MapPoint pt, const bool reserved)
{
    RTTR_Assert(GetNodeInt(pt).reserved != reserved);
    GetNodeInt(pt).reserved = reserved;
}

void World::SetVisibility(const MapPoint pt, const unsigned char player, const Visibility vis, const unsigned curTime)
{
    FoWNode& node = GetNodeInt(pt).fow[player];
    if(node.visibility == vis)
        return;

    node.visibility = vis;
    if(vis == VIS_VISIBLE)
        deletePtr(node.object);
    else if(vis == VIS_FOW)
        SaveFOWNode(pt, player, curTime);
    VisibilityChanged(pt, player);
}

void World::ChangeAltitude(const MapPoint pt, const unsigned char altitude)
{
    GetNodeInt(pt).altitude = altitude;

    // Schattierung neu berechnen von diesem Punkt und den Punkten drumherum
    RecalcShadow(pt);
    for(unsigned i = 0; i < 6; ++i)
        RecalcShadow(GetNeighbour(pt, i));

    // Abgeleiteter Klasse Bescheid sagen
    AltitudeChanged(pt);
}

bool World::IsPlayerTerritory(const MapPoint pt) const
{
    const unsigned char owner = GetNode(pt).owner;

    // Neighbour nodes must belong to this player
    for(unsigned i = 0; i < Direction::COUNT; ++i)
    {
        if(GetNeighbourNode(pt, Direction::fromInt(i)).owner != owner)
            return false;
    }

    return true;
}

BuildingQuality World::GetBQ(const MapPoint pt, const unsigned char player) const
{
    return AdjustBQ(pt, player,  GetNode(pt).bq);
}

BuildingQuality World::AdjustBQ(const MapPoint pt, unsigned char player, BuildingQuality nodeBQ) const
{
    if(nodeBQ == BQ_NOTHING || GetNode(pt).owner != player + 1 || !IsPlayerTerritory(pt))
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

TerrainType World::GetRightTerrain(const MapPoint pt, Direction dir)  const
{
    switch(Direction::Type(dir))
    {
    case Direction::WEST:      return GetNeighbourNode(pt, Direction::NORTHWEST).t1;
    case Direction::NORTHWEST: return GetNeighbourNode(pt, Direction::NORTHWEST).t2;
    case Direction::NORTHEAST: return GetNeighbourNode(pt, Direction::NORTHEAST).t1;
    case Direction::EAST:      return GetNode(pt).t2;
    case Direction::SOUTHEAST: return GetNode(pt).t1;
    case Direction::SOUTHWEST: return GetNeighbourNode(pt, Direction::WEST).t2;
    }
    throw std::logic_error("Invalid direction");
}

TerrainType World::GetLeftTerrain(const MapPoint pt, Direction dir)  const
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
    for(unsigned i = 0; i < Direction::COUNT; ++i)
    {
        if(!TerrainData::IsUsableByShip(GetRightTerrain(pt, Direction::fromInt(i))))
            return false;
    }

    return true;
}

bool World::IsWaterPoint(const MapPoint pt) const
{
    for(unsigned i = 0; i < Direction::COUNT; ++i)
    {
        if(!TerrainData::IsWater(GetRightTerrain(pt, Direction::fromInt(i))))
            return false;
    }

    return true;
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
