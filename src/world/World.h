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

#ifndef World_h__
#define World_h__

#include "world/MilitarySquares.h"
#include "gameTypes/GO_Type.h"
#include "gameTypes/MapNode.h"
#include "gameTypes/HarborPos.h"
#include "gameTypes/MapTypes.h"
#include "gameTypes/LandscapeType.h"
#include "gameTypes/Direction.h"
#include "Identity.h"
#include "ReturnConst.h"
#include <vector>
#include <list>

class noBuildingSite;
class noNothing;
class fowNothing;
class CatapultStone;

/// Base class representing the world itself, no algorithms, handlers etc!
class World
{
    /// Informationen über die Weltmeere
    struct Sea
    {
        /// Anzahl der Knoten, welches sich in diesem Meer befinden
        unsigned nodes_count;

        Sea(): nodes_count(0) {}
        Sea(const unsigned nodes_count): nodes_count(nodes_count) {}
    };

    friend class MapLoader;
    friend class MapSerializer;

    /// Size of the map in nodes
    unsigned short width_, height_;
    /// Landschafts-Typ
    LandscapeType lt;

    /// Eigenschaften von einem Punkt auf der Map
    std::vector<MapNode> nodes;

    std::vector<Sea> seas;

    /// Alle Hafenpositionen
    std::vector<HarborPos> harbor_pos;

    noNothing* noNodeObj;
    fowNothing* noFowObj;

    /// Internal method for access to nodes with write access
    inline MapNode& GetNodeInt(const MapPoint pt);
    inline MapNode& GetNeighbourNodeInt(const MapPoint pt, const unsigned i);
public:
    /// Currently flying catapultstones
    std::list<CatapultStone*> catapult_stones; 
    MilitarySquares militarySquares;

    World();
    virtual ~World();

    // Grundlegende Initialisierungen
    virtual void Init(const unsigned short width, const unsigned short height, LandscapeType lt);
    /// Aufräumen
    virtual void Unload();

    /// Größe der Map abfragen
    unsigned short GetWidth() const { return width_; }
    unsigned short GetHeight() const { return height_; }

    /// Landschaftstyp abfragen
    LandscapeType GetLandscapeType() const { return lt; }

    /// Gets coordinates of neightbour in the given direction
    MapPoint GetNeighbour(const MapPoint pt, const Direction dir) const;
    /// Returns neighbouring point (2nd layer: dir 0-11)
    MapPoint GetNeighbour2(const MapPoint, unsigned dir) const;
    // Convenience functions for the above function
    inline MapCoord GetXA(const MapCoord x, const MapCoord y, unsigned dir) const;
    inline MapCoord GetXA(const MapPoint pt, unsigned dir) const;
    inline  MapCoord GetYA(const MapCoord x, const MapCoord y, unsigned dir) const;
    inline MapPoint GetNeighbour(const MapPoint pt, const unsigned dir) const;

    /// Returns all points in a radius around pt (excluding pt) that satisfy a given condition. 
    /// Points can be transformed (e.g. to flags at those points) by the functor taking a map point and a radius
    /// Number of results is constrained to maxResults (if > 0)
    /// Overloads are used due to missing template default args until C++11
    template<unsigned T_maxResults, class T_TransformPt, class T_IsValidPt>
    inline std::vector<typename T_TransformPt::result_type>
    GetPointsInRadius(const MapPoint pt, const unsigned radius, T_TransformPt transformPt, T_IsValidPt isValid) const;
    template<class T_TransformPt>
    std::vector<typename T_TransformPt::result_type>
    GetPointsInRadius(const MapPoint pt, const unsigned radius, T_TransformPt transformPt) const
    {
        return GetPointsInRadius<0>(pt, radius, transformPt, ReturnConst<bool, true>());
    }
    std::vector<MapPoint> GetPointsInRadius(const MapPoint pt, const unsigned radius) const
    {
        return GetPointsInRadius<0>(pt, radius, Identity<MapPoint>(), ReturnConst<bool, true>());
    }

    /// Returns true, if the IsValid functor returns true for any point in the given radius
    /// If includePt is true, then the point itself is also checked
    template<class T_IsValidPt>
    inline bool
    CheckPointsInRadius(const MapPoint pt, const unsigned radius, T_IsValidPt isValid, bool includePt) const;


    /// Ermittelt Abstand zwischen 2 Punkten auf der Map unter Berücksichtigung der Kartengrenzüberquerung
    unsigned CalcDistance(int x1, int y1, int x2, int y2) const;
    unsigned CalcDistance(const MapPoint p1, const MapPoint p2) const { return CalcDistance(p1.x, p1.y, p2.x, p2.y); }
    /// Bestimmt die Schifffahrtrichtung, in der ein Punkt relativ zu einem anderen liegt
    unsigned char GetShipDir(Point<int> pos1, Point<int> pos2);

    /// Returns a MapPoint from a point. This ensures, the coords are actually in the map [0, mapSize)
    MapPoint MakeMapPoint(Point<int> pt) const;

    // Returns the linear index for a map point
    inline unsigned GetIdx(const MapPoint pt) const;

    /// Gibt Map-Knotenpunkt zurück
    inline const MapNode& GetNode(const MapPoint pt) const;
    /// Gibt MapKnotenpunkt darum zurück
    inline const MapNode& GetNeighbourNode(const MapPoint pt, const unsigned i) const;

    void AddFigure(noBase* fig, const MapPoint pt);
    void RemoveFigure(noBase* fig, const MapPoint pt);
    // Gibt ein NO zurück, falls keins existiert, wird ein "Nothing-Objekt" zurückgegeben
    noBase* GetNO(const MapPoint pt);
    // Gibt ein NO zurück, falls keins existiert, wird ein "Nothing-Objekt" zurückgegeben
    const noBase* GetNO(const MapPoint pt) const;
    /// Places a nodeobject at a given position. If replace is true, the old object is replace, else it is assumed as non-existant
    void SetNO(const MapPoint pt, noBase* obj, const bool replace = false);
    /// Destroys the object at the given node and removes it from the map. If checkExists is false than it is ok, if there is no obj
    void DestroyNO(const MapPoint pt, const bool checkExists = true);
    /// Gibt ein FOW-Objekt zurück, falls keins existiert, wird ein "Nothing-Objekt" zurückgegeben
    const FOWObject* GetFOWObject(const MapPoint pt, const unsigned spectator_player) const;
    /// Gibt den GOT des an diesem Punkt befindlichen Objekts zurück bzw. GOT_NOTHING, wenn keins existiert
    GO_Type GetGOT(const MapPoint pt) const;
    void ReduceResource(const MapPoint pt);
    void SetResource(const MapPoint pt, const unsigned char newResource){ GetNodeInt(pt).resources = newResource; }
    void SetOwner(const MapPoint pt, const unsigned char newOwner){ GetNodeInt(pt).owner = newOwner; }
    void SetReserved(const MapPoint pt, const bool reserved);
    void SetVisibility(const MapPoint pt, const unsigned char player, const Visibility vis, const unsigned curTime);

    void ChangeAltitude(const MapPoint pt, const unsigned char altitude);

    /// Prüft, ob der Pkut zu dem Spieler gehört (wenn er der Besitzer ist und es false zurückliefert, ist es Grenzgebiet)
    bool IsPlayerTerritory(const MapPoint pt) const;
    /// Ist eine Flagge irgendwo um x,y ?
    bool FlagNear(const MapPoint pt) const;
    /// Bauqualitäten berechnen, bei flagonly gibt er nur 1 zurück, wenn eine Flagge möglich ist
    BuildingQuality CalcBQ(const MapPoint pt, const unsigned char player, const bool flagonly = false, const bool visual = true, const bool ignore_player = false) const;
    /// Setzt die errechnete BQ gleich mit
    void CalcAndSetBQ(const MapPoint pt, const unsigned char player, const bool flagonly = false, const bool visual = true);

    /// Gibt Figuren, die sich auf einem bestimmten Punkt befinden, zurück
    /// nicht bei laufenden Figuren oder
    const std::list<noBase*>& GetFigures(const MapPoint pt) const { return GetNode(pt).figures; }

    // Gibt ein spezifisches Objekt zurück
    template<typename T> T* GetSpecObj(const MapPoint pt) { return dynamic_cast<T*>(GetNode(pt).obj); }
    // Gibt ein spezifisches Objekt zurück
    template<typename T> const T* GetSpecObj(const MapPoint pt) const { return dynamic_cast<const T*>(GetNode(pt).obj); }

    /// Returns the terrain that is on the clockwise side of the edge in the given direction:
    /// 0 = left upper triangle, 1 = triangle above, ..., 4 = triangle below
    TerrainType GetTerrainAround(const MapPoint pt, unsigned char dir) const;
    /// Returns the terrain to the left when walking from the point in the given direction (forward direction?)
    TerrainType GetWalkingTerrain1(const MapPoint pt, unsigned char dir) const;
    /// Returns the terrain to the right when walking from the point in the given direction (backward direction?)
    TerrainType GetWalkingTerrain2(const MapPoint pt, unsigned char dir) const;
    /// Erzeugt FOW-Objekte, -Straßen und -Grensteine von aktuellen Punkt für einen bestimmten Spieler
    void SaveFOWNode(const MapPoint pt, const unsigned player, unsigned curTime);
    unsigned GetNumSeas() const { return seas.size(); }
    /// Gibt zurück, ob ein Punkt vollständig von Wasser umgeben ist
    bool IsSeaPoint(const MapPoint pt) const;
    inline const unsigned GetSeaSize(const unsigned seaId) const;
    /// Is the harbor at the given sea
    bool IsAtThisSea(const unsigned harbor_id, const unsigned short sea_id) const;
    /// Returns the coast pt for a given harbor (where ships can land) if any
    MapPoint GetCoastalPoint(const unsigned harbor_id, const unsigned short sea_id) const;
    inline unsigned short GetSeaId(const unsigned harborId, const Direction dir) const;
    /// Gibt die Anzahl an Hafenpunkten zurück
    unsigned GetHarborPointCount() const { return harbor_pos.size() - 1; }
    /// Gibt die Koordinaten eines bestimmten Hafenpunktes zurück
    MapPoint GetHarborPoint(const unsigned harbor_id) const;
    /// Gibt die ID eines Hafenpunktes zurück
    unsigned GetHarborPointID(const MapPoint pt) const { return GetNode(pt).harbor_id; }
    const std::vector<HarborPos::Neighbor>& GetHarborNeighbor(const unsigned harborId, const Direction dir) const { return harbor_pos[harborId].neighbors[dir.toUInt()]; }
    /// Returns the sea id if this is a point at a coast to a sea where ships can go. Else returns 0
    unsigned short IsCoastalPoint(const MapPoint pt) const;

    /// liefert den Straßen-Wert an der Stelle X,Y
    unsigned char GetRoad(const MapPoint pt, unsigned char dir, bool all = false) const;
    /// liefert den Straßen-Wert um den Punkt X,Y.
    unsigned char GetPointRoad(const MapPoint pt, unsigned char dir, bool all = false) const;
    /// liefert FOW-Straßen-Wert um den punkt X,Y
    unsigned char GetPointFOWRoad(MapPoint pt, unsigned char dir, const unsigned char viewing_player) const;

    /// setzt den virtuellen Straßen-Wert an der Stelle X,Y (berichtigt).
    void SetVirtualRoad(const MapPoint pt, unsigned char dir, unsigned char type);
    /// setzt den virtuellen Straßen-Wert um den Punkt X,Y.
    void SetPointVirtualRoad(const MapPoint pt, unsigned char dir, unsigned char type);

protected:
    /// Für abgeleitete Klasse, die dann das Terrain entsprechend neu generieren kann
    virtual void AltitudeChanged(const MapPoint pt) = 0;
    /// Sets the real road to whether a virtual road for this pt and direction exists
    void ApplyRoad(const MapPoint pt, unsigned char dir);
    MapNode::BoundaryStones& GetBoundaryStones(const MapPoint pt){ return GetNodeInt(pt).boundary_stones; }

    /// Berechnet die Schattierung eines Punktes neu
    void RecalcShadow(const MapPoint pt);
};

//////////////////////////////////////////////////////////////////////////
// Implementation
//////////////////////////////////////////////////////////////////////////

// Convenience functions
MapCoord World::GetXA(const MapCoord x, const MapCoord y, unsigned dir) const { return GetXA(MapPoint(x, y), dir); }
MapCoord World::GetXA(const MapPoint pt, unsigned dir) const { return GetNeighbour(pt, dir).x; }
MapCoord World::GetYA(const MapCoord x, const MapCoord y, unsigned dir) const { return GetNeighbour(MapPoint(x, y), dir).y; }
MapPoint World::GetNeighbour(const MapPoint pt, const unsigned dir) const { return GetNeighbour(pt, Direction::fromInt(dir)); }

unsigned World::GetIdx(const MapPoint pt) const
{
    return static_cast<unsigned>(pt.y) * static_cast<unsigned>(width_) + static_cast<unsigned>(pt.x);
}

const MapNode& World::GetNode(const MapPoint pt) const
{
    RTTR_Assert(pt.x < width_ && pt.y < height_);
    return nodes[GetIdx(pt)];
}
MapNode& World::GetNodeInt(const MapPoint pt)
{
    RTTR_Assert(pt.x < width_ && pt.y < height_);
    return nodes[GetIdx(pt)];
}

const MapNode& World::GetNeighbourNode(const MapPoint pt, const unsigned i) const
{
    return GetNode(GetNeighbour(pt, i));
}
MapNode& World::GetNeighbourNodeInt(const MapPoint pt, const unsigned i)
{
    return GetNodeInt(GetNeighbour(pt, i));
}

const unsigned World::GetSeaSize(const unsigned seaId) const
{
    RTTR_Assert(seaId < seas.size());
    return seas[seaId].nodes_count;
}

unsigned short World::GetSeaId(const unsigned harborId, const Direction dir) const
{
    return harbor_pos[harborId].cps[dir.toUInt()].sea_id;
}

template<unsigned T_maxResults, class T_TransformPt, class T_IsValidPt>
std::vector<typename T_TransformPt::result_type>
World::GetPointsInRadius(const MapPoint pt, const unsigned radius, T_TransformPt transformPt, T_IsValidPt isValid) const
{
    typedef typename T_TransformPt::result_type Element;
    std::vector<Element> result;
    MapPoint curStartPt = pt;
    for(unsigned r = 1; r <= radius; ++r)
    {
        // Go one level/hull to the left
        curStartPt = GetNeighbour(curStartPt, Direction::WEST);
        // Now iterate over the "circle" of radius r by going r steps in one direction, turn right and repeat
        MapPoint curPt = curStartPt;
        for(unsigned i = Direction::NORTHEAST; i < Direction::NORTHEAST + Direction::COUNT; ++i)
        {
            for(unsigned step = 0; step < r; ++step)
            {
                Element el = transformPt(curPt, r);
                if(isValid(el))
                {
                    result.push_back(el);
                    if(T_maxResults && result.size() >= T_maxResults)
                        return result;
                }
                curPt = GetNeighbour(curPt, Direction(i).toUInt());
            }
        }
    }
    return result;
}

template<class T_IsValidPt>
inline bool
World::CheckPointsInRadius(const MapPoint pt, const unsigned radius, T_IsValidPt isValid, bool includePt) const
{
    if(includePt && isValid(pt))
        return true;
    MapPoint curStartPt = pt;
    for(unsigned r = 1; r <= radius; ++r)
    {
        // Go one level/hull to the left
        curStartPt = GetNeighbour(curStartPt, Direction::WEST);
        // Now iterate over the "circle" of radius r by going r steps in one direction, turn right and repeat
        MapPoint curPt = curStartPt;
        for(unsigned i = Direction::NORTHEAST; i < Direction::NORTHEAST + Direction::COUNT; ++i)
        {
            for(unsigned step = 0; step < r; ++step)
            {
                if(isValid(curPt))
                    return true;
                curPt = GetNeighbour(curPt, Direction(i).toUInt());
            }
        }
    }
    return false;
}

#endif // World_h__