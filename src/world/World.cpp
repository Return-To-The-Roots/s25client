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
#include "helpers/containerUtils.h"
#include <set>

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

void World::Init(const unsigned short width, const unsigned short height, LandscapeType lt)
{
    width_ = width;
    height_ = height;
    this->lt = lt;
    // Map-Knoten erzeugen
    nodes.resize(width_ * height_);
    militarySquares.Init(width, height);

    // Dummy-Hafenpos für Index 0 einfügen
    // -> the dummy is so that the harbor "0" might be used for ships with no particular destination
    harbor_pos.push_back(MapPoint::Invalid());
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
    militarySquares.Clear();
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
    RTTR_Assert(res == MakeMapPoint(::GetNeighbour(Point<int>(pt), dir)));
    return res;
}

MapPoint World::GetNeighbour2(const MapPoint pt, unsigned dir) const
{
    return MakeMapPoint(::GetNeighbour2(Point<int>(pt), dir));
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

/// Bestimmt die Schifffahrtrichtung, in der ein Punkt relativ zu einem anderen liegt
unsigned char World::GetShipDir(Point<int> pos1, Point<int> pos2)
{
    // Richtung bestimmen, in der dieser Punkt relativ zum Ausgangspunkt steht
    unsigned char exp_dir = 0xff;

    unsigned diff = SafeDiff<int>(pos1.y, pos2.y);
    if(!diff)
        diff = 1;
    // Oben?
    bool marginal_x = ((SafeDiff<int>(pos1.x, pos2.x) * 1000 / diff) < 180);
    if(pos2.y < pos1.y)
    {
        if(marginal_x)
            exp_dir = 0;
        else if(pos2.x < pos1.x)
            exp_dir = 5;
        else
            exp_dir = 1;
    } else
    {
        if(marginal_x)
            exp_dir = 3;
        else if(pos2.x < pos1.x)
            exp_dir = 4;
        else
            exp_dir = 2;
    }

    return exp_dir;
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
        return noNodeObj;
}

const noBase* World::GetNO(const MapPoint pt) const
{
    if(GetNode(pt).obj)
        return GetNode(pt).obj;
    else
        return noNodeObj;
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
    MapNode& node = GetNodeInt(pt);
    if(node.fow[player].visibility == vis)
        return;

    node.fow[player].visibility = vis;
    if(vis == VIS_VISIBLE)
        deletePtr(node.fow[player].object);  // Etwaige FOW-Objekte zerstören
    else if(vis == VIS_FOW)
        SaveFOWNode(pt, player, curTime);
}

void World::ChangeAltitude(const MapPoint pt, const unsigned char altitude)
{
    // Höhe verändern
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

    // Umliegende Punkte dürfen keinem anderen gehören
    for(unsigned i = 0; i < 6; ++i)
    {
        if(GetNeighbourNode(pt, i).owner != owner)
            return false;
    }

    return true;
}

bool World::FlagNear(const MapPoint pt) const
{
    for(unsigned char i = 0; i < 6; ++i)
    {
        if(GetNO(GetNeighbour(pt, i))->GetType() == NOP_FLAG)
            return true;
    }
    return false;
}

BuildingQuality World::CalcBQ(const MapPoint pt, const bool visual, const bool flagonly /*= false*/) const
{

    ///////////////////////
    // 1. nach Terrain

    unsigned building_hits = 0;
    unsigned mine_hits = 0;
    unsigned flag_hits = 0;

    // bebaubar?
    for(unsigned char i = 0; i < 6; ++i)
    {
        TerrainBQ bq = TerrainData::GetBuildingQuality(GetTerrainAround(pt, i));
        if(bq == TerrainBQ::CASTLE)
            ++building_hits;
        else if(bq == TerrainBQ::MINE)
            ++mine_hits;
        else if(bq == TerrainBQ::FLAG)
            ++flag_hits;
        else if(bq == TerrainBQ::DANGER)
            return BQ_NOTHING;
    }

    BuildingQuality val;
    if(flag_hits)
        val = BQ_FLAG;
    else if(mine_hits == 6)
        val = BQ_MINE;
    else if(mine_hits)
        val = BQ_FLAG;
    else if(building_hits == 6)
        val = BQ_CASTLE;
    else if(building_hits)
        val = BQ_FLAG;
    else
        return BQ_NOTHING;


    //////////////////////////////////////
    // 2. nach Terrain

    unsigned char ph = GetNode(pt).altitude, th;

    // Bergwerke anders handhaben
    if(val == BQ_CASTLE)
    {

        if((th = GetNeighbourNode(pt, 4).altitude) > ph)
        {
            if(th - ph > 1)
                val = BQ_FLAG;
        }

        // 2. Außenschale prüfen ( keine Hütten werden ab Steigung 3 )
        for(unsigned i = 0; i < 12; ++i)
        {
            if((th = GetNode(GetNeighbour2(pt, i)).altitude) > ph)
            {
                if(th - ph > 2)
                {
                    val = BQ_HUT;
                    break;
                }
            }

            if((th = GetNode(GetNeighbour2(pt, i)).altitude) < ph)
            {
                if(ph - th > 2)
                {
                    val = BQ_HUT;
                    break;
                }
            }
        }

        // 1. Auäcnschale ( käcnen Flaggen werden ab Steigung 4)
        for(unsigned i = 0; i < 6; ++i)
        {
            if((th = GetNeighbourNode(pt, i).altitude) > ph)
            {
                if(th - ph > 3)
                    val = BQ_FLAG;
            }

            if((th = GetNeighbourNode(pt, i).altitude) < ph)
            {
                if(ph - th > 3)
                    val = BQ_FLAG;
            }
        }
    } else if((th = GetNeighbourNode(pt, 4).altitude) > ph)
    {
        if(th - ph > 3)
            val = BQ_FLAG;
    }

    //////////////////////////////////////////
    // 3. nach Objekten

    if(flagonly && FlagNear(pt))
        return BQ_NOTHING;


    // allgemein nix bauen auf folgenden Objekten:

    if(GetNO(pt)->GetBM() != noBase::BM_NOTBLOCKING)
        return BQ_NOTHING;

    // Don't build anything around charburner piles
    for(unsigned i = 0; i < 6; ++i)
    {
        if(GetNO(GetNeighbour(pt, i))->GetBM() == noBase::BM_CHARBURNERPILE)
            return BQ_NOTHING;
    }

    if(val > 2 && val != BQ_MINE)
    {
        for(unsigned i = 0; i < 6; ++i)
        {
            // Baum --> rundrum Hütte
            if(GetNO(GetNeighbour(pt, i))->GetType() == NOP_TREE)
            {
                val = BQ_HUT;
                break;
            }

            /*// StaticObject --> rundrum Flagge/Hütte
            else if(GetNO(GetXA(x, y, i), GetYA(x, y, i))->GetType() == NOP_OBJECT)
            {
            const noStaticObject *obj = GetSpecObj<noStaticObject>(GetXA(x, y, i), GetYA(x, y, i));
            if(obj->GetSize() == 2)
            val = BQ_FLAG;
            else
            val = BQ_HUT;

            break;
            }*/
        }
    }

    // Stein, Feuer und Getreidefeld --> rundrum Flagge
    for(unsigned i = 0; i < 6; ++i)
    {
        const noBase* nob = GetNO(GetNeighbour(pt, i));
        if(nob->GetBM() == noBase::BM_GRANITE)
        {
            val = BQ_FLAG;
            break;
        }
    }

    // Flagge
    if(val == BQ_CASTLE)
    {
        for(unsigned char i = 0; i < 3; ++i)
        {
            if(GetNeighbourNode(pt, i).obj)
            {
                if(GetNeighbourNode(pt, i).obj->GetBM() == noBase::BM_FLAG)
                    val = BQ_HOUSE;
            }
        }
    }

    if(GetNO(GetNeighbour(pt, 3))->GetBM() == noBase::BM_FLAG)
        return BQ_NOTHING;
    if(GetNO(GetNeighbour(pt, 5))->GetBM() == noBase::BM_FLAG)
        return BQ_NOTHING;

    // Gebäude
    if(val == BQ_CASTLE)
    {
        for(unsigned i = 0; i < 12; ++i)
        {
            noBase::BlockingManner bm = GetNO(GetNeighbour2(pt, i))->GetBM();

            if(bm >= noBase::BM_HUT && bm <= noBase::BM_MINE)
                val = BQ_HOUSE;
        }
    }

    for(unsigned i = 0; i < 3; ++i)
    {
        if(val == BQ_CASTLE)
        {
            for(unsigned char c = 0; c < 6; ++c)
            {
                if(GetPointRoad(GetNeighbour(pt, i), c, visual))
                {
                    val = BQ_HOUSE;
                    break;
                }
            }
        }
    }

    for(unsigned char c = 0; c < 6; ++c)
    {
        if(GetPointRoad(pt, c, visual))
        {
            val = BQ_FLAG;
            break;
        }
    }

    if(val == BQ_FLAG)
    {
        for(unsigned char i = 0; i < 6; ++i)
        {
            if(GetNO(GetNeighbour(pt, i))->GetBM() == noBase::BM_FLAG)
                return BQ_NOTHING;
        }
    }


    if(flagonly)
        return BQ_FLAG;

    if(val == BQ_FLAG)
    {
        for(unsigned char i = 0; i < 3; ++i)
            if(GetNO(GetNeighbour(pt, i))->GetBM() == noBase::BM_FLAG)
                return BQ_NOTHING;
    }


    // Schloss bis hierhin und ist hier ein Hafenplatz?
    if(val == BQ_CASTLE && GetNode(pt).harbor_id)
        // Dann machen wir einen Hafen draus
        val = BQ_HARBOR;

    if(val >= BQ_HUT && val <= BQ_HARBOR)
    {
        if(GetNO(GetNeighbour(pt, 4))->GetBM() == noBase::BM_FLAG)
            return val;

        if(CalcBQ(GetNeighbour(pt, 4), visual, true))
        {
            return val;
        } else
        {

            for(unsigned char i = 0; i < 3; ++i)
                if(GetNO(GetNeighbour(pt, i))->GetBM() == noBase::BM_FLAG)
                    return BQ_NOTHING;
            return BQ_FLAG;
        }
    }

    return val;
}

void World::RecalcBQ(const MapPoint pt)
{
    GetNodeInt(pt).bq = CalcBQ(pt, false);
    GetNodeInt(pt).bqVisual = CalcBQ(pt, true);
}

BuildingQuality World::GetBQ(const MapPoint pt, const unsigned char player, const bool visual /*= true*/) const
{
    if(GetNode(pt).owner != player + 1 || !IsPlayerTerritory(pt))
        return BQ_NOTHING;
    BuildingQuality bq = visual ?  GetNode(pt).bqVisual : GetNode(pt).bq;
    // If we could build a building, but the buildings flag point is at the border, we can only build a flag
    if(bq != BQ_NOTHING && !IsPlayerTerritory(GetNeighbour(pt, Direction::SOUTHEAST)))
        bq = BQ_FLAG;
    return bq;
}

/**
 *  liefert das Terrain um den Punkt X, Y.
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


/**
 *  Gibt das Terrain zurück, über das ein Mensch/Tier laufen müsste, von X, Y
 *  in Richtung DIR (Vorwärts).
 */
TerrainType World::GetWalkingTerrain1(const MapPoint pt, unsigned char dir)  const
{
    RTTR_Assert(dir < 6);
    return (dir == 0) ? GetTerrainAround(pt, 5) : GetTerrainAround(pt, dir - 1);
}

/**
 *  Gibt das Terrain zurück, über das ein Mensch/Tier laufen müsste, von X, Y
 *  in Richtung DIR (Rückwärts).
 */
TerrainType World::GetWalkingTerrain2(const MapPoint pt, unsigned char dir)  const
{
    RTTR_Assert(dir < 6);
    return GetTerrainAround(pt, dir);
}

void World::SaveFOWNode(const MapPoint pt, const unsigned player, unsigned curTime)
{
    MapNode::FoWData& fow = GetNodeInt(pt).fow[player];
    fow.last_update_time = curTime;

    // FOW-Objekt erzeugen
    noBase* obj = GetNO(pt);
    delete fow.object;
    fow.object = obj->CreateFOWObject();


    // Wege speichern, aber nur richtige, keine, die gerade gebaut werden
    for(unsigned i = 0; i < 3; ++i)
    {
        if(GetNode(pt).roads_real[i])
            fow.roads[i] = GetNode(pt).roads[i];
        else
            fow.roads[i] = 0;
    }

    // Besitzverhältnisse speichern, damit auch die Grenzsteine im FoW gezeichnet werden können
    fow.owner = GetNode(pt).owner;
    // Grenzsteine merken
    fow.boundary_stones = GetNode(pt).boundary_stones;
}

bool World::IsSeaPoint(const MapPoint pt) const
{
    for(unsigned i = 0; i < 6; ++i)
    {
        if(!TerrainData::IsUsableByShip(GetTerrainAround(pt, i)))
            return false;
    }

    return true;
}

bool World::IsWaterPoint(const MapPoint pt) const
{
    for(unsigned i = 0; i < 6; ++i)
    {
        if(!TerrainData::IsWater(GetTerrainAround(pt, i)))
            return false;
    }

    return true;
}

/// Grenzt der Hafen an ein bestimmtes Meer an?
bool World::IsAtThisSea(const unsigned harbor_id, const unsigned short sea_id) const
{
    for(unsigned i = 0; i < 6; ++i)
    {
        if(sea_id == harbor_pos[harbor_id].cps[i].sea_id)
            return true;
    }
    return false;
}

/// Gibt den Punkt eines bestimmtes Meeres um den Hafen herum an, sodass Schiffe diesen anfahren können
MapPoint World::GetCoastalPoint(const unsigned harbor_id, const unsigned short sea_id) const
{
    RTTR_Assert(harbor_id);

    for(unsigned i = 0; i < 6; ++i)
    {
        if(harbor_pos[harbor_id].cps[i].sea_id == sea_id)
        {
            return GetNeighbour(harbor_pos[harbor_id].pos, i);
        }
    }

    // Keinen Punkt gefunden
    return MapPoint(0xFFFF, 0xFFFF);
}


/**
 *  liefert den Straßen-Wert an der Stelle X, Y (berichtigt).
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

/**
 *  liefert den Straßen-Wert um den Punkt X, Y.
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

/**
 *  setzt den virtuellen Straßen-Wert an der Stelle X, Y (berichtigt).
 */
void World::SetVirtualRoad(const MapPoint pt, unsigned char dir, unsigned char type)
{
    RTTR_Assert(dir < 3);

    GetNodeInt(pt).roads[dir] = type;
}

/**
 *  setzt den virtuellen Straßen-Wert um den Punkt X, Y.
 */
void World::SetPointVirtualRoad(const MapPoint pt, unsigned char dir, unsigned char type)
{
    RTTR_Assert(dir < 6);

    if(dir >= 3)
        SetVirtualRoad(pt, dir - 3, type);
    else
        SetVirtualRoad(GetNeighbour(pt, dir), dir, type);
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

/// Gibt die Koordinaten eines bestimmten Hafenpunktes zurück
MapPoint World::GetHarborPoint(const unsigned harbor_id) const
{
    RTTR_Assert(harbor_id);

    return harbor_pos[harbor_id].pos;
}

/// Ermittelt, ob ein Punkt Küstenpunkt ist, d.h. Zugang zu einem schiffbaren Meer hat
unsigned short World::IsCoastalPoint(const MapPoint pt) const
{
    // Punkt muss selbst zu keinem Meer gehören
    if(GetNode(pt).sea_id)
        return 0;

    // Should not be inside water itself
    if(IsWaterPoint(pt))
        return 0;

    // Um den Punkt herum muss ein gültiger Meeres Punkt sein
    for(unsigned i = 0; i < 6; ++i)
    {
        unsigned short sea_id = GetNeighbourNode(pt, i).sea_id;
        if(sea_id)
        {
            // Dieses Meer schiffbar (todo: andere Kritierien wie Hafenplätze etc.)?
            if(GetSeaSize(sea_id) > 20)
                return sea_id;
        }
    }

    return 0;
}

void World::ApplyRoad(const MapPoint pt, unsigned char dir)
{
    GetNodeInt(pt).roads_real[dir] = GetNode(pt).roads[dir] != 0;
}

void World::RecalcShadow(const MapPoint pt)
{
    int altitude = GetNode(pt).altitude;
    int A = GetNeighbourNode(pt, Direction::NORTHEAST).altitude - altitude;
    int B = GetNode(GetNeighbour2(pt, 0)).altitude - altitude;
    int C = GetNode(GetNeighbour(pt, Direction::WEST)).altitude - altitude;
    int D = GetNode(GetNeighbour2(pt, 7)).altitude - altitude;

    int shadingS2 = 64 + 9 * A - 3 * B - 6 * C - 9 * D;
    if(shadingS2 > 128)
        shadingS2 = 128;
    else if(shadingS2 < 0)
        shadingS2 = 0;
    GetNodeInt(pt).shadow = shadingS2;
}
