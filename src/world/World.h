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
#include "helpers/Deleter.h"
#include <boost/interprocess/smart_ptr/unique_ptr.hpp>
#include <vector>
#include <list>

class noNothing;
class CatapultStone;
class FOWObject;
class noBase;
struct ShipDirection;
template <typename T> struct Point;

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

    boost::interprocess::unique_ptr<noBase, Deleter<noBase> > noNodeObj;

protected:
    /// Internal method for access to nodes with write access
    MapNode& GetNodeInt(const MapPoint pt);
    MapNode& GetNeighbourNodeInt(const MapPoint pt, Direction dir);
public:
    /// Currently flying catapult stones
    std::list<CatapultStone*> catapult_stones; 
    MilitarySquares militarySquares;

    World();
    virtual ~World();

    /// Initialize the world
    virtual void Init(unsigned short width, unsigned short height, LandscapeType lt);
    /// Clean up (free objects and reset world to uninitialized state)
    virtual void Unload();

    /// Return the size of the world
    unsigned short GetWidth() const { return width_; }
    unsigned short GetHeight() const { return height_; }

    /// Return the type of the landscape
    LandscapeType GetLandscapeType() const { return lt; }

    /// Get coordinates of neighbor in the given direction
    MapPoint GetNeighbour(const MapPoint pt, const Direction dir) const;
    /// Return neighboring point (2nd layer: dir 0-11)
    MapPoint GetNeighbour2(const MapPoint, unsigned dir) const;
    // Convenience functions for the above function
    MapCoord GetXA(const MapCoord x, const MapCoord y, unsigned dir) const;
    MapCoord GetXA(const MapPoint pt, unsigned dir) const;
    MapCoord GetYA(const MapCoord x, const MapCoord y, unsigned dir) const;
    MapPoint GetNeighbour(const MapPoint pt, const unsigned dir) const;

    /// Return all points in a radius around pt (excluding pt) that satisfy a given condition. 
    /// Points can be transformed (e.g. to flags at those points) by the functor taking a map point and a radius
    /// Number of results is constrained to maxResults (if > 0)
    /// Overloads are used due to missing template default args until C++11
    template<unsigned T_maxResults, class T_TransformPt, class T_IsValidPt>
    std::vector<typename T_TransformPt::result_type>
    GetPointsInRadius(const MapPoint pt, const unsigned radius, T_TransformPt transformPt, T_IsValidPt isValid, bool includePt = false) const;

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

    std::vector<MapPoint> GetPointsInRadiusWithCenter(const MapPoint pt, const unsigned radius) const
    {
        return GetPointsInRadius<0>(pt, radius, Identity<MapPoint>(), ReturnConst<bool, true>(), true);
    }

    /// Returns true, if the IsValid functor returns true for any point in the given radius
    /// If includePt is true, then the point itself is also checked
    template<class T_IsValidPt>
    bool CheckPointsInRadius(const MapPoint pt, const unsigned radius, T_IsValidPt isValid, bool includePt) const;


    /// Return the distance between 2 points on the map (includes wrapping around map borders)
    unsigned CalcDistance(const Point<int> p1, const Point<int> p2) const;
    unsigned CalcDistance(const MapPoint p1, const MapPoint p2) const { return CalcDistance(Point<int>(p1), Point<int>(p2)); }
    /// Return the direction for ships for going from one point to another
    ShipDirection GetShipDir(MapPoint fromPt, MapPoint toPt) const;

    /// Returns a MapPoint from a point. This ensures, the coords are actually in the map [0, mapSize)
    MapPoint MakeMapPoint(Point<int> pt) const;

    /// Returns the linear index for a map point
    unsigned GetIdx(const MapPoint pt) const;

    /// Return the node at that point
    const MapNode& GetNode(const MapPoint pt) const;
    /// Return the neighboring node
    const MapNode& GetNeighbourNode(const MapPoint pt, Direction dir) const;

    void AddFigure(noBase* fig, const MapPoint pt);
    void RemoveFigure(noBase* fig, const MapPoint pt);
    /// Return the NO from that point or a "nothing"-object if there is none
    noBase* GetNO(const MapPoint pt);
    /// Return the NO from that point or a "nothing"-object if there is none
    const noBase* GetNO(const MapPoint pt) const;
    /// Places a NO at a given position. If replace is true, the old object is replaced, else it is assumed as non-existent
    void SetNO(const MapPoint pt, noBase* obj, const bool replace = false);
    /// Destroys the object at the given node and removes it from the map. If checkExists is false than it is ok, if there is no obj
    void DestroyNO(const MapPoint pt, const bool checkExists = true);
    /// Return the game object type of the object at that point or GOT_NONE of there is none
    GO_Type GetGOT(const MapPoint pt) const;
    void ReduceResource(const MapPoint pt);
    void SetResource(const MapPoint pt, const unsigned char newResource){ GetNodeInt(pt).resources = newResource; }
    void SetOwner(const MapPoint pt, const unsigned char newOwner){ GetNodeInt(pt).owner = newOwner; }
    void SetReserved(const MapPoint pt, const bool reserved);
    void SetVisibility(const MapPoint pt, const unsigned char player, const Visibility vis, const unsigned curTime);

    void ChangeAltitude(const MapPoint pt, const unsigned char altitude);

    /// Check if the point completely belongs to a player (if false but point itself belongs to player then it is a border)
    bool IsPlayerTerritory(const MapPoint pt) const;
    /// Return the BQ for the given player at the point (including ownership constraints)
    BuildingQuality GetBQ(const MapPoint pt, const unsigned char player) const;
    /// Incorporates node ownership into the given BQ
    BuildingQuality AdjustBQ(const MapPoint pt, unsigned char player, BuildingQuality nodeBQ) const;

    /// Return the figures currently on the node
    const std::list<noBase*>& GetFigures(const MapPoint pt) const { return GetNode(pt).figures; }

    /// Return a specific object or NULL
    template<typename T> T* GetSpecObj(const MapPoint pt) { return dynamic_cast<T*>(GetNode(pt).obj); }
    /// Return a specific object or NULL
    template<typename T> const T* GetSpecObj(const MapPoint pt) const { return dynamic_cast<const T*>(GetNode(pt).obj); }

    /// Return the terrain to the right when walking from the point in the given direction
    /// 0 = left upper triangle, 1 = triangle above, ..., 4 = triangle below
    TerrainType GetRightTerrain(const MapPoint pt, Direction dir) const;
    /// Return the terrain to the left when walking from the point in the given direction
    TerrainType GetLeftTerrain(const MapPoint pt, Direction dir) const;
    /// Create the FOW-objects, -streets, etc for a point and player
    void SaveFOWNode(const MapPoint pt, const unsigned player, unsigned curTime);
    unsigned GetNumSeas() const { return seas.size(); }
    /// Return whether a node is inside a (shippable) sea (surrounded by shippable water)
    bool IsSeaPoint(const MapPoint pt) const;
    /// Return true, if the point is surrounded by water
    bool IsWaterPoint(const MapPoint pt) const;
    unsigned GetSeaSize(const unsigned seaId) const;
    /// Return the id of the sea at which the coast in the given direction of the harbor lies. 0 = None
    unsigned short GetSeaId(const unsigned harborId, const Direction dir) const;
    /// Is the harbor at the given sea
    bool IsHarborAtSea(const unsigned harborId, const unsigned short seaId) const;
    /// Return the coast pt for a given harbor (where ships can land) if any
    MapPoint GetCoastalPoint(const unsigned harborId, const unsigned short seaId) const;
    /// Return the number of harbor points
    unsigned GetHarborPointCount() const { return harbor_pos.size() - 1; }
    /// Return the coordinates for a given harbor point
    MapPoint GetHarborPoint(const unsigned harborId) const;
    /// Return the ID of the harbor point on that node or 0 if there is none
    unsigned GetHarborPointID(const MapPoint pt) const { return GetNode(pt).harborId; }
    const std::vector<HarborPos::Neighbor>& GetHarborNeighbors(const unsigned harborId, const ShipDirection& dir) const;
    /// Return the sea id if this is a point at a coast to a sea where ships can go. Else returns 0
    unsigned short GetSeaFromCoastalPoint(const MapPoint pt) const;

    /// Return the road type of this point in the given direction (E, SE, SW) or 0 if no road
    unsigned char GetRoad(const MapPoint pt, unsigned char dir) const;
    /// Return the road type from this point in the given direction (Full circle direction)
    unsigned char GetPointRoad(const MapPoint pt, Direction dir) const;
    /// Return the FOW road type for a player
    unsigned char GetPointFOWRoad(MapPoint pt, Direction dir, const unsigned char viewing_player) const;

    /// Adds a catapult stone currently flying
    void AddCatapultStone(CatapultStone* cs);
    void RemoveCatapultStone(CatapultStone* cs);

protected:
    /// Notify derived classes of changed altitude
    virtual void AltitudeChanged(const MapPoint pt) = 0;
    /// Notify derived classes of changed visibility
    virtual void VisibilityChanged(const MapPoint pt, unsigned player) = 0;
    /// Sets the road for the given (road) direction
    void SetRoad(const MapPoint pt, unsigned char roadDir, unsigned char type);
    BoundaryStones& GetBoundaryStones(const MapPoint pt){ return GetNodeInt(pt).boundary_stones; }
    /// Set the BQ at the point and return true if it was changed
    bool SetBQ(const MapPoint pt, BuildingQuality bq);

    /// Recalculates the shade of a point
    void RecalcShadow(const MapPoint pt);
};

//////////////////////////////////////////////////////////////////////////
// Implementation
//////////////////////////////////////////////////////////////////////////

// Convenience functions
inline MapCoord World::GetXA(const MapCoord x, const MapCoord y, unsigned dir) const { return GetXA(MapPoint(x, y), dir); }
inline MapCoord World::GetXA(const MapPoint pt, unsigned dir) const { return GetNeighbour(pt, dir).x; }
inline MapCoord World::GetYA(const MapCoord x, const MapCoord y, unsigned dir) const { return GetNeighbour(MapPoint(x, y), dir).y; }
inline MapPoint World::GetNeighbour(const MapPoint pt, const unsigned dir) const { return GetNeighbour(pt, Direction::fromInt(dir)); }

inline unsigned World::GetIdx(const MapPoint pt) const
{
    RTTR_Assert(pt.x < width_ && pt.y < height_);
    return static_cast<unsigned>(pt.y) * static_cast<unsigned>(width_) + static_cast<unsigned>(pt.x);
}

inline const MapNode& World::GetNode(const MapPoint pt) const
{
    return nodes[GetIdx(pt)];
}

inline MapNode& World::GetNodeInt(const MapPoint pt)
{
    return nodes[GetIdx(pt)];
}

inline const MapNode& World::GetNeighbourNode(const MapPoint pt, Direction dir) const
{
    return GetNode(GetNeighbour(pt, dir));
}

inline MapNode& World::GetNeighbourNodeInt(const MapPoint pt, Direction dir)
{
    return GetNodeInt(GetNeighbour(pt, dir));
}

template<unsigned T_maxResults, class T_TransformPt, class T_IsValidPt>
inline std::vector<typename T_TransformPt::result_type>
World::GetPointsInRadius(const MapPoint pt, const unsigned radius, T_TransformPt transformPt, T_IsValidPt isValid, bool includePt) const
{
    typedef typename T_TransformPt::result_type Element;
    std::vector<Element> result;
    if(includePt)
    {
        Element el = transformPt(pt, 0);
        if(isValid(el))
        {
            result.push_back(el);
            if(T_maxResults == 1u)
                return result;
        }
    }
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
