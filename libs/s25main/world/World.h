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

#pragma once

#include "enum_cast.hpp"
#include "world/MapBase.h"
#include "world/MilitarySquares.h"
#include "gameTypes/Direction.h"
#include "gameTypes/GO_Type.h"
#include "gameTypes/HarborPos.h"
#include "gameTypes/MapCoordinates.h"
#include "gameTypes/MapNode.h"
#include "gameTypes/MapTypes.h"
#include "gameData/DescIdx.h"
#include "gameData/WorldDescription.h"
#include <list>
#include <memory>
#include <vector>

struct LandscapeDesc;
class CatapultStone;
class noBase;
class noBuildingSite;
enum class ShipDirection : uint8_t;

struct WalkTerrain
{
    DescIdx<TerrainDesc> left, right;
};

/// Base class representing the world itself, no algorithms, handlers etc!
class World : public MapBase
{
    /// Informationen über die Weltmeere
    struct Sea
    {
        /// Anzahl der Knoten, welches sich in diesem Meer befinden
        unsigned nodes_count = 0;

        Sea() = default;
        Sea(unsigned nodes_count) : nodes_count(nodes_count) {}
    };

    friend class MapLoader;
    friend class MapSerializer;

    /// Landschafts-Typ
    DescIdx<LandscapeDesc> lt;

    /// Eigenschaften von einem Punkt auf der Map
    std::vector<MapNode> nodes;

    std::vector<Sea> seas;

    /// Alle Hafenpositionen
    std::vector<HarborPos> harbor_pos;

    WorldDescription description_;

    std::unique_ptr<noBase> noNodeObj;
    void Resize(const MapExtent& newSize) override final;

protected:
    /// harbor building sites created by ships
    std::list<noBuildingSite*> harbor_building_sites_from_sea;

public:
    /// Currently flying catapult stones
    std::list<CatapultStone*> catapult_stones;
    MilitarySquares militarySquares;

    World();
    virtual ~World();

    /// Initialize the world
    virtual void Init(const MapExtent& mapSize, DescIdx<LandscapeDesc> lt);
    /// Clean up (free objects and reset world to uninitialized state)
    void Unload();

    /// Return the type of the landscape
    DescIdx<LandscapeDesc> GetLandscapeType() const { return lt; }

    const WorldDescription& GetDescription() const { return description_; }
    WorldDescription& GetDescriptionWriteable() { return description_; }

    /// Return the node at that point
    const MapNode& GetNode(MapPoint pt) const;
    /// Return the neighboring node
    const MapNode& GetNeighbourNode(MapPoint pt, Direction dir) const;

    void AddFigure(MapPoint pt, noBase* fig);
    void RemoveFigure(MapPoint pt, noBase* fig);
    /// Return the NO from that point or a "nothing"-object if there is none
    noBase* GetNO(MapPoint pt);
    /// Return the NO from that point or a "nothing"-object if there is none
    const noBase* GetNO(MapPoint pt) const;
    /// Places a NO at a given position. If replace is true, the old object is replaced, else it is assumed as
    /// non-existent
    void SetNO(MapPoint pt, noBase* obj, bool replace = false);
    /// Destroys the object at the given node and removes it from the map. If checkExists is false than it is ok, if
    /// there is no obj
    void DestroyNO(MapPoint pt, bool checkExists = true);
    /// Return the game object type of the object at that point or GOT_NONE of there is none
    GO_Type GetGOT(MapPoint pt) const;
    void ReduceResource(MapPoint pt);
    void SetResource(const MapPoint pt, Resource newResource) { GetNodeInt(pt).resources = newResource; }
    void SetOwner(const MapPoint pt, unsigned char newOwner) { GetNodeInt(pt).owner = newOwner; }
    void SetReserved(MapPoint pt, bool reserved);
    /// Sets the visibility and fires a Visibility Changed event if different
    /// fowTime is only used if visibility gets changed to FoW
    void SetVisibility(MapPoint pt, unsigned char player, Visibility vis, unsigned fowTime = 0);

    void ChangeAltitude(MapPoint pt, unsigned char altitude);

    // Make whole map visible with no additional checks or notices
    void MakeWholeMapVisibleForAllPlayers();

    /// Checks if the point completely belongs to a player (if false but point itself belongs to player then it is a
    /// border) if owner is != 0 it checks if the points specific ownership
    bool IsPlayerTerritory(MapPoint pt, unsigned char owner = 0) const;

    /// Return the BQ for the given player at the point (including ownership constraints)
    BuildingQuality GetBQ(MapPoint pt, unsigned char player) const;
    /// Incorporates node ownership into the given BQ
    BuildingQuality AdjustBQ(MapPoint pt, unsigned char player, BuildingQuality nodeBQ) const;

    /// Return the figures currently on the node
    const std::list<noBase*>& GetFigures(const MapPoint pt) const { return GetNode(pt).figures; }

    /// Return a specific object or nullptr
    template<typename T>
    T* GetSpecObj(const MapPoint pt)
    {
        return dynamic_cast<T*>(GetNode(pt).obj);
    }
    /// Return a specific object or nullptr
    template<typename T>
    const T* GetSpecObj(MapPoint pt) const
    {
        return dynamic_cast<const T*>(GetNode(pt).obj);
    }

    /// Return the terrain to the right when walking from the point in the given direction
    /// 0 = left upper triangle, 1 = triangle above, ..., 4 = triangle below
    DescIdx<TerrainDesc> GetRightTerrain(MapPoint pt, Direction dir) const;
    /// Get left and right terrain from the point in the given direction
    WalkTerrain GetTerrain(MapPoint pt, Direction dir) const;
    helpers::EnumArray<DescIdx<TerrainDesc>, Direction> GetTerrainsAround(MapPoint pt) const;

    /// Create the FOW-objects, -streets, etc for a point and player
    void SaveFOWNode(MapPoint pt, unsigned player, unsigned curTime);
    unsigned GetNumSeas() const { return seas.size(); }
    /// Return whether a node is inside a (shippable) sea (surrounded by shippable water)
    bool IsSeaPoint(MapPoint pt) const;
    /// Return true, if the point is surrounded by water
    bool IsWaterPoint(MapPoint pt) const;
    /// Return true if all surrounding terrains match the given predicate
    template<class T_Predicate>
    bool IsOfTerrain(MapPoint pt, T_Predicate predicate) const;
    /// Return true if any surrounding terrain (description) matches the predicate
    template<class T_Predicate>
    bool HasTerrain(MapPoint pt, T_Predicate predicate) const;

    unsigned GetSeaSize(unsigned seaId) const;
    /// Return the id of the sea at which the coast in the given direction of the harbor lies. 0 = None
    unsigned short GetSeaId(unsigned harborId, Direction dir) const;
    /// Is the harbor at the given sea
    bool IsHarborAtSea(unsigned harborId, unsigned short seaId) const;
    /// Return the coast pt for a given harbor (where ships can land) if any
    MapPoint GetCoastalPoint(unsigned harborId, unsigned short seaId) const;
    /// Return the number of harbor points
    unsigned GetNumHarborPoints() const { return harbor_pos.size() - 1; }
    /// Return the coordinates for a given harbor point
    MapPoint GetHarborPoint(unsigned harborId) const;
    /// Return the ID of the harbor point on that node or 0 if there is none
    unsigned GetHarborPointID(const MapPoint pt) const { return GetNode(pt).harborId; }
    const std::vector<HarborPos::Neighbor>& GetHarborNeighbors(unsigned harborId, const ShipDirection& dir) const;
    /// Berechnet die Entfernung zwischen 2 Hafenpunkten
    unsigned CalcHarborDistance(unsigned habor_id1, unsigned harborId2) const;
    /// Return the sea id if this is a point at a coast to a sea where ships can go. Else returns 0
    unsigned short GetSeaFromCoastalPoint(MapPoint pt) const;

    RoadDir toRoadDir(MapPoint& pt, const Direction dir) const
    {
        // Uses knowledge about Direction<->RoadDir to avoid switch, see static_asserts
        using rttr::enum_cast;
        auto iDir = enum_cast(dir);
        if(iDir >= enum_cast(Direction::East))
        {
            static_assert(enum_cast(Direction::East) - 3 == enum_cast(RoadDir::East)
                            && enum_cast(Direction::SouthEast) - 3 == enum_cast(RoadDir::SouthEast)
                            && enum_cast(Direction::SouthWest) - 3 == enum_cast(RoadDir::SouthWest),
                          "Mismatch");
            iDir -= 3u; // Map East->East etc
        } else
        {
            // Will map iDir to opposite
            static_assert(enum_cast(Direction::West) == enum_cast(RoadDir::East)
                            && enum_cast(Direction::NorthWest) == enum_cast(RoadDir::SouthEast)
                            && enum_cast(Direction::NorthEast) == enum_cast(RoadDir::SouthWest),
                          "Mismatch");
            pt = GetNeighbour(pt, dir);
        }
        return RoadDir(iDir);
    }

    /// Return the road type of this point in the given direction
    PointRoad GetRoad(MapPoint pt, RoadDir dir) const;
    /// Return the road type from this point in the given direction
    PointRoad GetPointRoad(MapPoint pt, Direction dir) const;
    /// Return the FOW road type for a player
    PointRoad GetPointFOWRoad(MapPoint pt, Direction dir, unsigned char viewing_player) const;

    /// Adds a catapult stone currently flying
    void AddCatapultStone(CatapultStone* cs);
    void RemoveCatapultStone(CatapultStone* cs);

protected:
    /// Internal method for access to nodes with write access
    MapNode& GetNodeInt(MapPoint pt);
    MapNode& GetNeighbourNodeInt(MapPoint pt, Direction dir);

    /// Notify derived classes of changed altitude
    virtual void AltitudeChanged(MapPoint pt) = 0;
    /// Notify derived classes of changed visibility
    virtual void VisibilityChanged(MapPoint pt, unsigned player, Visibility oldVis, Visibility newVis) = 0;
    /// Sets the road for the given (road) direction
    void SetRoad(MapPoint pt, RoadDir roadDir, PointRoad type);
    BoundaryStones& GetBoundaryStones(const MapPoint pt) { return GetNodeInt(pt).boundary_stones; }
    /// Set the BQ at the point and return true if it was changed
    bool SetBQ(MapPoint pt, BuildingQuality bq);

    /// Recalculates the shade of a point
    void RecalcShadow(MapPoint pt);
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

template<class T_Predicate>
inline bool World::IsOfTerrain(const MapPoint pt, T_Predicate predicate) const
{
    // NOTE: This is '!HasTerrain(pt, !predicate)'
    for(const DescIdx<TerrainDesc> tIdx : GetTerrainsAround(pt))
    {
        if(!predicate(GetDescription().get(tIdx)))
            return false;
    }
    return true;
}

template<class T_Predicate>
inline bool World::HasTerrain(const MapPoint pt, T_Predicate predicate) const
{
    for(const DescIdx<TerrainDesc> tIdx : GetTerrainsAround(pt))
    {
        if(predicate(GetDescription().get(tIdx)))
            return true;
    }
    return false;
}
