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

#include "defines.h"
#include "world/World.h"
#include "nodeObjs/noNothing.h"
#include "nodeObjs/noTree.h"
#include "nodeObjs/noFlag.h"
#include "FOWObjects.h"
#include "gameData/MilitaryConsts.h"
#include "gameData/TerrainData.h"
#include "RoadSegment.h"
#include <set>

// Include last!
#include "DebugNew.h"

World::World(): width_(0), height_(0), lt(LT_GREENLAND), noNodeObj(new noNothing()), noFowObj(new fowNothing())
{
    noTree::ResetInstanceCounter();
    GameObject::ResetCounter();
}

World::~World()
{
    Unload();
    delete noNodeObj;
    delete noFowObj;
}

void World::Init()
{
    const unsigned numNodes = width_ * height_;

    // Map-Knoten erzeugen
    nodes.resize(numNodes);
    military_squares.resize((width_ / MILITARY_SQUARE_SIZE + 1) * (height_ / MILITARY_SQUARE_SIZE + 1));
}

void World::Unload()
{
    // Straßen sammeln und alle dann vernichten
    std::set<RoadSegment*> roadsegments;
    for(std::vector<MapNode>::const_iterator it = nodes.begin(); it != nodes.end(); ++it)
    {
        if(!it->obj || it->obj->GetGOT() != GOT_FLAG)
            continue;
        for(unsigned r = 0; r < 6; ++r)
        {
            if(static_cast<noFlag*>(it->obj)->routes[r])
            {
                roadsegments.insert(static_cast<noFlag*>(it->obj)->routes[r]);
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

    nodes.clear();
    military_squares.clear();
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
        res.x = (pt.y & 1) ? pt.x : (((pt.x == 0) ? width_ : pt.x) - 1);
        res.y = pt.y + 1;
        if(res.y == height_)
            res.y = 0;
        break;
    }

    return res;
}

MapPoint World::GetNeighbour2(const MapPoint pt, unsigned dir) const
{
    if(dir >= 12)
        throw std::logic_error("Invalid direction!");

    static const int ADD_Y[12] =
    { 0, -1, -2, -2, -2, -1, 0, 1, 2, 2, 2, 1 };

    int tx;
    switch(dir)
    {
    default: throw std::logic_error("Invalid direction!");
    case 0: tx = pt.x - 2; break;
    case 1: tx = pt.x - 2 + ((pt.y & 1) ? 1 : 0); break;
    case 2: tx = pt.x - 1; break;
    case 3: tx = pt.x; break;
    case 4: tx = pt.x + 1; break;
    case 5: tx = pt.x + 2 - ((pt.y & 1) ? 0 : 1); break;
    case 6: tx = pt.x + 2; break;
    case 7: tx = pt.x - 2 + ((pt.y & 1) ? 1 : 0); break;
    case 8: tx = pt.x - 1; break;
    case 9: tx = pt.x; break;
    case 10: tx = pt.x + 1; break;
    case 11: tx = pt.x + 2 - ((pt.y & 1) ? 0 : 1);
    }
    MapPoint res = MakeMapPoint(Point<int>(tx, int(pt.y) + ADD_Y[dir]));
    return res;
}

/// Ermittelt Abstand zwischen 2 Punkten auf der Map unter Berücksichtigung der Kartengrenzüberquerung
unsigned World::CalcDistance(const int x1, const int y1, const int x2, const int y2) const
{
    int dx = ((x1 - x2) * 2) + (y1 & 1) - (y2 & 1);
    int dy = ((y1 > y2) ? (y1 - y2) : (y2 - y1)) * 2;

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

MapPoint World::MakeMapPoint(Point<int> pt) const
{
    // Shift into range
    pt.x %= width_;
    pt.y %= height_;
    // Handle negative values (sign is implementation defined, but |value| < width)
    if(pt.x < 0)
        pt.x += width_;
    if(pt.y < 0)
        pt.y += height_;
    RTTR_Assert(pt.x >= 0 && pt.y >= 0);
    RTTR_Assert(pt.x < width_ && pt.y < height_);
    return MapPoint(pt);
}

noBase* World::GetNO(const MapPoint pt)
{
    if(GetNode(pt).obj)
        return GetNode(pt).obj;
    else
        return noNodeObj;
}

const noBase* World::GetNO(const MapPoint pt) const
{
    if(GetNode(pt).obj)
        return GetNode(pt).obj;
    else
        return noNodeObj;
}

const FOWObject* World::GetFOWObject(const MapPoint pt, const unsigned spectator_player) const
{
    if(GetNode(pt).fow[spectator_player].object)
        return GetNode(pt).fow[spectator_player].object;
    else
        return noFowObj;
}

/// Gibt den GOT des an diesem Punkt befindlichen Objekts zurück bzw. GOT_NOTHING, wenn keins existiert
GO_Type World::GetGOT(const MapPoint pt) const
{
    noBase* obj = GetNode(pt).obj;
    if(obj)
        return obj->GetGOT();
    else
        return GOT_NOTHING;
}

///////////////////////////////////////////////////////////////////////////////
/**
*  liefert das Terrain um den Punkt X, Y.
*
*  @author OLiver
*  @author FloSoft
*/
TerrainType World::GetTerrainAround(const MapPoint pt, unsigned char dir)  const
{
    switch(dir)
    {
    case 0: return GetNeighbourNode(pt, 1).t1;
    case 1: return GetNeighbourNode(pt, 1).t2;
    case 2: return GetNeighbourNode(pt, 2).t1;
    case 3: return GetNode(pt).t2;
    case 4: return GetNode(pt).t1;
    case 5: return GetNeighbourNode(pt, 0).t2;
    }

    throw std::logic_error("Invalid direction");
}


///////////////////////////////////////////////////////////////////////////////
/**
*  Gibt das Terrain zurück, über das ein Mensch/Tier laufen müsste, von X, Y
*  in Richtung DIR (Vorwärts).
*
*  @author OLiver
*/
TerrainType World::GetWalkingTerrain1(const MapPoint pt, unsigned char dir)  const
{
    RTTR_Assert(dir < 6);
    return (dir == 0) ? GetTerrainAround(pt, 5) : GetTerrainAround(pt, dir - 1);
}

///////////////////////////////////////////////////////////////////////////////
/**
*  Gibt das Terrain zurück, über das ein Mensch/Tier laufen müsste, von X, Y
*  in Richtung DIR (Rückwärts).
*
*  @author OLiver
*/
TerrainType World::GetWalkingTerrain2(const MapPoint pt, unsigned char dir)  const
{
    RTTR_Assert(dir < 6);
    return GetTerrainAround(pt, dir);
}

/// Gibt zurück, ob ein Punkt vollständig von Wasser umgeben ist
bool World::IsSeaPoint(const MapPoint pt) const
{
    for(unsigned i = 0; i < 6; ++i)
    {
        if(!TerrainData::IsWater(GetTerrainAround(pt, i)))
            return false;
    }

    return true;
}

///////////////////////////////////////////////////////////////////////////////
/**
*  liefert den Straßen-Wert an der Stelle X, Y (berichtigt).
*
*
*  @author OLiver
*/
unsigned char World::GetRoad(const MapPoint pt, unsigned char dir, bool all) const
{
    RTTR_Assert(pt.x < width_ && pt.y < height_);
    RTTR_Assert(dir < 3);

    const MapNode& node = GetNode(pt);
    // Entweder muss es eine richtige Straße sein oder es müssen auch visuelle Straßen erlaubt sein
    if(all || node.roads_real[(unsigned)dir])
        return node.roads[(unsigned)dir];

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
/**
*  liefert den Straßen-Wert um den Punkt X, Y.
*
*  @author OLiver
*/
unsigned char World::GetPointRoad(const MapPoint pt, unsigned char dir, bool all) const
{
    RTTR_Assert(dir < 6);

    if(dir >= 3)
        return GetRoad(pt, dir - 3, all);
    else
        return GetRoad(GetNeighbour(pt, dir), dir, all);
}

unsigned char World::GetPointFOWRoad(MapPoint pt, unsigned char dir, const unsigned char viewing_player) const
{
    RTTR_Assert(dir < 6);

    if(dir >= 3)
        dir = dir - 3;
    else
        pt = GetNeighbour(pt, dir);

    return GetNode(pt).fow[viewing_player].roads[dir];
}

///////////////////////////////////////////////////////////////////////////////
/**
*  setzt den virtuellen Straßen-Wert an der Stelle X, Y (berichtigt).
*
*  @author OLiver
*/
void World::SetVirtualRoad(const MapPoint pt, unsigned char dir, unsigned char type)
{
    RTTR_Assert(dir < 3);

    GetNode(pt).roads[dir] = type;
}

///////////////////////////////////////////////////////////////////////////////
/**
*  setzt den virtuellen Straßen-Wert um den Punkt X, Y.
*
*  @author OLiver
*/
void World::SetPointVirtualRoad(const MapPoint pt, unsigned char dir, unsigned char type)
{
    RTTR_Assert(dir < 6);

    if(dir >= 3)
        SetVirtualRoad(pt, dir - 3, type);
    else
        SetVirtualRoad(GetNeighbour(pt, dir), dir, type);
}

/// Gibt die Koordinaten eines bestimmten Hafenpunktes zurück
MapPoint World::GetHarborPoint(const unsigned harbor_id) const
{
    RTTR_Assert(harbor_id);

    return harbor_pos[harbor_id].pos;
}

void World::RecalcShadow(const MapPoint pt)
{
    int altitude = GetNode(pt).altitude;
    int A = GetNeighbourNode(pt, 2).altitude - altitude;
    int B = GetNode(GetNeighbour2(pt, 0)).altitude - altitude;
    int C = GetNode(GetNeighbour(pt, 0)).altitude - altitude;
    int D = GetNode(GetNeighbour2(pt, 7)).altitude - altitude;

    int shadingS2 = 64 + 9 * A - 3 * B - 6 * C - 9 * D;
    if(shadingS2 > 128)
        shadingS2 = 128;
    else if(shadingS2 < 0)
        shadingS2 = 0;
    GetNode(pt).shadow = shadingS2;
}
