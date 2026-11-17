// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "enum_cast.hpp"
#include "helpers/PtrSpan.h"
#include "helpers/StrongIdVector.h"
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

    helpers::StrongIdVector<Sea, SeaId> seas;

    /// Alle Hafenpositionen
    helpers::StrongIdVector<HarborPos, HarborId> harborData;

    WorldDescription description_;

    std::unique_ptr<noBase> noNodeObj;
    void resize(const MapExtent& newSize) override final;
    noBase& addFigureImpl(MapPoint pt, std::unique_ptr<noBase> fig);
    /// Implementation of RemoveFigure. Returned pointer must be wrapped in an owning pointer
    noBase* removeFigureImpl(MapPoint pt, noBase& fig);

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
    virtual void init(const MapExtent& mapSize, DescIdx<LandscapeDesc> lt);
    /// Clean up (free objects and reset world to uninitialized state)
    void unload();

    /// Return the type of the landscape
    DescIdx<LandscapeDesc> getLandscapeType() const { return lt; }

    const WorldDescription& getDescription() const { return description_; }
    WorldDescription& getDescriptionWriteable() { return description_; }

    /// Return the node at that point
    const MapNode& getNode(MapPoint pt) const;
    /// Return the neighboring node
    const MapNode& getNeighbourNode(MapPoint pt, Direction dir) const;

    // Add a figure to a node (taking ownership) and returns a reference to it
    template<typename T>
    T& addFigure(MapPoint pt, std::unique_ptr<T> fig)
    {
        return static_cast<T&>(addFigureImpl(pt, std::move(fig)));
    }
    template<typename T>
    std::unique_ptr<T> removeFigure(MapPoint pt, T& fig)
    {
        return std::unique_ptr<T>(static_cast<T*>(removeFigureImpl(pt, fig)));
    }
    template<typename T>
    std::unique_ptr<T> removeFigure(MapPoint pt, T*& fig) = delete;
    /// Return the NO from that point or a "nothing"-object if there is none
    noBase* getNO(MapPoint pt);
    /// Return the NO from that point or a "nothing"-object if there is none
    const noBase* getNO(MapPoint pt) const;
    /// Places a NO at a given position. If replace is true, the old object is replaced, else it is assumed as
    /// non-existent
    void setNO(MapPoint pt, noBase* obj, bool replace = false);
    /// Destroys the object at the given node and removes it from the map. If checkExists is false than it is ok, if
    /// there is no obj
    void destroyNO(MapPoint pt, bool checkExists = true);
    /// Return the game object type of the object at that point or GOT_NONE of there is none
    GO_Type getGOT(MapPoint pt) const;
    void reduceResource(MapPoint pt);
    void setResource(const MapPoint pt, Resource newResource) { getNodeInt(pt).resources = newResource; }
    void setOwner(const MapPoint pt, unsigned char newOwner) { getNodeInt(pt).owner = newOwner; }
    void setReserved(MapPoint pt, bool reserved);
    /// Sets the visibility and fires a Visibility Changed event if different
    /// fowTime is only used if visibility gets changed to FoW
    void setVisibility(MapPoint pt, unsigned char player, Visibility vis, unsigned fowTime = 0);

    void changeAltitude(MapPoint pt, unsigned char altitude);

    // Make whole map visible with no additional checks or notices
    void makeWholeMapVisibleForAllPlayers();

    /// Checks if the point completely belongs to a player (if false but point itself belongs to player then it is a
    /// border) if owner is != 0 it checks if the points specific ownership
    bool isPlayerTerritory(MapPoint pt, unsigned char owner = 0) const;

    /// Return the BQ for the given player at the point (including ownership constraints)
    BuildingQuality getBQ(MapPoint pt, unsigned char player) const;
    /// Incorporates node ownership into the given BQ
    BuildingQuality adjustBQ(MapPoint pt, unsigned char player, BuildingQuality nodeBQ) const;

    /// Return the figures currently on the node
    auto getFigures(const MapPoint pt) const { return helpers::nonNullPtrSpan(getNode(pt).figures); }
    bool hasFigureAt(MapPoint pt, const noBase& figure) const;

    /// Return a specific object or nullptr
    template<typename T>
    T* getSpecObj(const MapPoint pt)
    {
        return dynamic_cast<T*>(getNode(pt).obj);
    }
    /// Return a specific object or nullptr
    template<typename T>
    const T* getSpecObj(MapPoint pt) const
    {
        return dynamic_cast<const T*>(getNode(pt).obj);
    }

    /// Get left and right terrain from the point in the given direction
    WalkTerrain getTerrain(MapPoint pt, Direction dir) const;
    /// Return all terrains around the given point. The per-direction entry is the terrain to the right
    helpers::EnumArray<DescIdx<TerrainDesc>, Direction> getTerrainsAround(MapPoint pt) const;

    /// Create the FOW-objects, -streets, etc for a point and player
    void saveFOWNode(MapPoint pt, unsigned player, unsigned curTime);
    unsigned getNumSeas() const { return seas.size(); }
    /// Return whether a node is inside a (shippable) sea (surrounded by shippable water)
    bool isSeaPoint(MapPoint pt) const;
    /// Return true, if the point is surrounded by water
    bool isWaterPoint(MapPoint pt) const;
    /// Return true if all surrounding terrains match the given predicate
    template<class T_Predicate>
    bool isOfTerrain(MapPoint pt, T_Predicate predicate) const;
    /// Return true if any surrounding terrain (description) matches the predicate
    template<class T_Predicate>
    bool hasTerrain(MapPoint pt, T_Predicate predicate) const;

    unsigned getSeaSize(SeaId seaId) const;
    /// Return the id of the sea at which the coast in the given direction of the harbor lies. 0 = None
    SeaId getSeaId(HarborId harborId, Direction dir) const;
    /// Is the harbor at the given sea
    bool isHarborAtSea(HarborId harborId, SeaId seaId) const;
    /// Return the coast pt for a given harbor (where ships can land) if any
    MapPoint getCoastalPoint(HarborId harborId, SeaId seaId) const;
    /// Return the number of harbor points
    unsigned getNumHarborPoints() const { return harborData.size(); }
    /// Return the coordinates for a given harbor point
    MapPoint getHarborPoint(HarborId harborId) const;
    /// Return the ID of the harbor point on that node or 0 if there is none
    HarborId getHarborPointID(const MapPoint pt) const { return getNode(pt).harborId; }
    const std::vector<HarborPos::Neighbor>& getHarborNeighbors(HarborId harborId, const ShipDirection& dir) const;
    /// Return the minimum distance between 2 harbors
    /// Note: Harbors could be at 2 seas, so there might be multiple, different distances
    unsigned getMinHarborDistance(HarborId haborId1, HarborId harborId2) const;
    /// Return distance between harbors at given sea
    unsigned getHarborDistance(HarborId haborId1, HarborId harborId2, SeaId sea) const;
    /// Return the sea id if this is a point at a coast to a sea where ships can go. Else returns 0
    SeaId getSeaFromCoastalPoint(MapPoint pt) const;

    RoadDir toRoadDir(MapPoint& pt, const Direction dir) const
    {
        // Uses knowledge about Direction<->RoadDir to avoid switch, see static_asserts
        using rttr::enumCast;
        auto iDir = enumCast(dir);
        if(iDir >= enumCast(Direction::East))
        {
            static_assert(enumCast(Direction::East) - 3 == enumCast(RoadDir::East)
                            && enumCast(Direction::SouthEast) - 3 == enumCast(RoadDir::SouthEast)
                            && enumCast(Direction::SouthWest) - 3 == enumCast(RoadDir::SouthWest),
                          "Mismatch");
            iDir -= 3u; // Map East->East etc
        } else
        {
            // Will map iDir to opposite
            static_assert(enumCast(Direction::West) == enumCast(RoadDir::East)
                            && enumCast(Direction::NorthWest) == enumCast(RoadDir::SouthEast)
                            && enumCast(Direction::NorthEast) == enumCast(RoadDir::SouthWest),
                          "Mismatch");
            pt = getNeighbour(pt, dir);
        }
        return RoadDir(iDir);
    }

    /// Return the road type of this point in the given direction
    PointRoad getRoad(MapPoint pt, RoadDir dir) const;
    /// Return the road type from this point in the given direction
    PointRoad getPointRoad(MapPoint pt, Direction dir) const;
    /// Return the FOW road type for a player
    PointRoad getPointFOWRoad(MapPoint pt, Direction dir, unsigned char viewing_player) const;

    /// Adds a catapult stone currently flying
    void addCatapultStone(CatapultStone* cs);
    void removeCatapultStone(CatapultStone* cs);

protected:
    /// Internal method for access to nodes with write access
    MapNode& getNodeInt(MapPoint pt);
    MapNode& getNeighbourNodeInt(MapPoint pt, Direction dir);

    /// Notify derived classes of changed altitude
    virtual void altitudeChanged(MapPoint pt) = 0;
    /// Notify derived classes of changed visibility
    virtual void visibilityChanged(MapPoint pt, unsigned player, Visibility oldVis, Visibility newVis) = 0;
    /// Sets the road for the given (road) direction
    void setRoad(MapPoint pt, RoadDir roadDir, PointRoad type);
    BoundaryStones& getBoundaryStones(const MapPoint pt) { return getNodeInt(pt).boundary_stones; }
    /// Set the BQ at the point and return true if it was changed
    bool setBQ(MapPoint pt, BuildingQuality bq);

    /// Recalculates the shade of a point
    void recalcShadow(MapPoint pt);
};

//////////////////////////////////////////////////////////////////////////
// Implementation
//////////////////////////////////////////////////////////////////////////

inline const MapNode& World::getNode(const MapPoint pt) const
{
    return nodes[getIdx(pt)];
}

inline MapNode& World::getNodeInt(const MapPoint pt)
{
    return nodes[getIdx(pt)];
}

inline const MapNode& World::getNeighbourNode(const MapPoint pt, Direction dir) const
{
    return getNode(getNeighbour(pt, dir));
}

inline MapNode& World::getNeighbourNodeInt(const MapPoint pt, Direction dir)
{
    return getNodeInt(getNeighbour(pt, dir));
}

template<class T_Predicate>
inline bool World::isOfTerrain(const MapPoint pt, T_Predicate predicate) const
{
    // NOTE: This is '!HasTerrain(pt, !predicate)'
    for(const DescIdx<TerrainDesc> tIdx : getTerrainsAround(pt))
    {
        if(!predicate(getDescription().get(tIdx)))
            return false;
    }
    return true;
}

template<class T_Predicate>
inline bool World::hasTerrain(const MapPoint pt, T_Predicate predicate) const
{
    for(const DescIdx<TerrainDesc> tIdx : getTerrainsAround(pt))
    {
        if(predicate(getDescription().get(tIdx)))
            return true;
    }
    return false;
}
