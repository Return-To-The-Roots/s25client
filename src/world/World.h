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

#ifndef World_h__
#define World_h__

#include "Identity.h"
#include "ReturnConst.h"
#include "helpers/Deleter.h"
#include "world/MapBase.h"
#include "world/MilitarySquares.h"
#include "gameTypes/Direction.h"
#include "gameTypes/GO_Type.h"
#include "gameTypes/HarborPos.h"
#include "gameTypes/LandscapeType.h"
#include "gameTypes/MapCoordinates.h"
#include "gameTypes/MapNode.h"
#include "gameTypes/MapTypes.h"
#include <boost/interprocess/smart_ptr/unique_ptr.hpp>
#include <list>
#include <vector>

class noNothing;
class CatapultStone;
class FOWObject;
class noBase;
struct ShipDirection;
/// Base class representing the world itself, no algorithms, handlers etc!
class World : public MapBase
{
    /// Informationen über die Weltmeere
    struct Sea
    {
        /// Anzahl der Knoten, welches sich in diesem Meer befinden
        unsigned nodes_count;

        Sea() : nodes_count(0) {}
        Sea(const unsigned nodes_count) : nodes_count(nodes_count) {}
    };

    friend class MapLoader;
    friend class MapSerializer;

    /// Landschafts-Typ
    LandscapeType lt;

    /// Eigenschaften von einem Punkt auf der Map
    std::vector<MapNode> nodes;

    std::vector<Sea> seas;

    /// Alle Hafenpositionen
    std::vector<HarborPos> harbor_pos;

    boost::interprocess::unique_ptr<noBase, Deleter<noBase> > noNodeObj;
    void Resize(const MapExtent& newSize) override;

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
    virtual void Init(const MapExtent& size, LandscapeType lt);
    /// Clean up (free objects and reset world to uninitialized state)
    virtual void Unload();

    /// Return the type of the landscape
    LandscapeType GetLandscapeType() const { return lt; }

    /// Return the node at that point
    const MapNode& GetNode(const MapPoint pt) const;
    /// Return the neighboring node
    const MapNode& GetNeighbourNode(const MapPoint pt, Direction dir) const;

    void AddFigure(const MapPoint pt, noBase* fig);
    void RemoveFigure(const MapPoint pt, noBase* fig);
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
    void SetResource(const MapPoint pt, Resource newResource) { GetNodeInt(pt).resources = newResource; }
    void SetOwner(const MapPoint pt, const unsigned char newOwner) { GetNodeInt(pt).owner = newOwner; }
    void SetReserved(const MapPoint pt, const bool reserved);
    /// Sets the visibility and fires a Visibility Changed event if different
    /// fowTime is only used if visibility gets changed to FoW
    void SetVisibility(const MapPoint pt, unsigned char player, Visibility vis, unsigned fowTime = 0);

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
    template<typename T>
    T* GetSpecObj(const MapPoint pt)
    {
        return dynamic_cast<T*>(GetNode(pt).obj);
    }
    /// Return a specific object or NULL
    template<typename T>
    const T* GetSpecObj(const MapPoint pt) const
    {
        return dynamic_cast<const T*>(GetNode(pt).obj);
    }

    /// Return the terrain to the right when walking from the point in the given direction
    /// 0 = left upper triangle, 1 = triangle above, ..., 4 = triangle below
    TerrainType GetRightTerrain(const MapPoint pt, Direction dir) const;
    /// Return the terrain to the left when walking from the point in the given direction
    TerrainType GetLeftTerrain(const MapPoint pt, Direction dir) const;
    /// Create the FOW-objects, -streets, etc for a point and player
    void SaveFOWNode(const MapPoint pt, const unsigned player, unsigned curTime);
    unsigned GetNumSeas() const { return seas.size(); }
    // Return if all surrounding terrains match the given predicate
    bool IsOfTerrain(const MapPoint pt, bool(*terrainPredicate)(TerrainType)) const;
    // Return if a map point is fully surrounded by a given TerrainType
    bool IsOfTerrain(const MapPoint pt, const TerrainType t) const;
    /// Return whether a node is inside a (shippable) sea (surrounded by shippable water)
    bool IsSeaPoint(const MapPoint pt) const;
    /// Return true, if the point is surrounded by water
    bool IsWaterPoint(const MapPoint pt) const;
    // Return if any neighbour node match the given predicate
    bool HasTerrain(const MapPoint pt, bool(*terrainPredicate)(TerrainType)) const;
    // Return if a any neighbour node matches the given TerrainType
    bool HasTerrain(const MapPoint pt, const TerrainType t) const;

    unsigned GetSeaSize(const unsigned seaId) const;
    /// Return the id of the sea at which the coast in the given direction of the harbor lies. 0 = None
    unsigned short GetSeaId(const unsigned harborId, const Direction dir) const;
    /// Is the harbor at the given sea
    bool IsHarborAtSea(const unsigned harborId, const unsigned short seaId) const;
    /// Return the coast pt for a given harbor (where ships can land) if any
    MapPoint GetCoastalPoint(const unsigned harborId, const unsigned short seaId) const;
    /// Return the number of harbor points
    unsigned GetNumHarborPoints() const { return harbor_pos.size() - 1; }
    /// Return the coordinates for a given harbor point
    MapPoint GetHarborPoint(const unsigned harborId) const;
    /// Return the ID of the harbor point on that node or 0 if there is none
    unsigned GetHarborPointID(const MapPoint pt) const { return GetNode(pt).harborId; }
    const std::vector<HarborPos::Neighbor>& GetHarborNeighbors(const unsigned harborId, const ShipDirection& dir) const;
    /// Berechnet die Entfernung zwischen 2 Hafenpunkten
    unsigned CalcHarborDistance(unsigned habor_id1, unsigned harborId2) const;
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
    virtual void VisibilityChanged(const MapPoint pt, unsigned player, Visibility oldVis, Visibility newVis) = 0;
    /// Sets the road for the given (road) direction
    void SetRoad(const MapPoint pt, unsigned char roadDir, unsigned char type);
    BoundaryStones& GetBoundaryStones(const MapPoint pt) { return GetNodeInt(pt).boundary_stones; }
    /// Set the BQ at the point and return true if it was changed
    bool SetBQ(const MapPoint pt, BuildingQuality bq);

    /// Recalculates the shade of a point
    void RecalcShadow(const MapPoint pt);
};

//////////////////////////////////////////////////////////////////////////
// Implementation
//////////////////////////////////////////////////////////////////////////

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

#endif // World_h__
