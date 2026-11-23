// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "EconomyModeHandler.h"
#include "buildings/nobBaseMilitary.h"
#include "enum_cast.hpp"
#include "helpers/OptionalEnum.h"
#include "lua/LuaInterfaceGame.h"
#include "notifications/NotificationManager.h"
#include "postSystem/PostManager.h"
#include "world/World.h"
#include "s25util/span.hpp"
#include <limits>
#include <memory>
#include <set>
#include <vector>

class ShipPathData;
class EventManager;
class FreePathFinder;
class GameInterface;
class GamePlayer;
class GlobalGameSettings;
class nobHarborBuilding;
class noBuildingSite;
class noFlag;
class nofPassiveSoldier;
class RoadPathFinder;
class SoundManager;

constexpr Direction getOppositeDir(const RoadDir roadDir) noexcept
{
    static_assert(rttr::enumCast(Direction::West) == rttr::enumCast(RoadDir::East)
                    && rttr::enumCast(Direction::NorthWest) == rttr::enumCast(RoadDir::SouthEast)
                    && rttr::enumCast(Direction::NorthEast) == rttr::enumCast(RoadDir::SouthWest),
                  "Opposite directions don't match");
    return Direction(rttr::enumCast(roadDir));
}

constexpr Direction toDirection(const RoadDir roadDir) noexcept
{
    static_assert(rttr::enumCast(Direction::East) == rttr::enumCast(RoadDir::East) + 3u
                    && rttr::enumCast(Direction::SouthEast) == rttr::enumCast(RoadDir::SouthEast) + 3u
                    && rttr::enumCast(Direction::SouthWest) == rttr::enumCast(RoadDir::SouthWest) + 3u,
                  "Directions don't match");
    return Direction(rttr::enumCast(roadDir) + 3u);
}

/// Grundlegende Klasse, die die Gamewelt darstellt, enth�lt nur deren Daten
class GameWorldBase : public World
{
    std::unique_ptr<RoadPathFinder> roadPathFinder;
    std::unique_ptr<FreePathFinder> freePathFinder;
    mutable std::unique_ptr<ShipPathData> shipPathData;
    PostManager postManager;
    mutable NotificationManager notifications;

    std::vector<GamePlayer> players;
    const GlobalGameSettings& gameSettings;
    EventManager& em;
    std::unique_ptr<SoundManager> soundManager;
    std::set<MapPoint, MapPointLess> ptsInsideComputerBarriers;
    LuaInterfaceGame* lua;

protected:
    /// Interface zum GUI
    GameInterface* gi;
    std::unique_ptr<EconomyModeHandler> econHandler;
    /// Replay minor version for backwards compatibility. Default is highest possible, i.e. current behavior.
    uint8_t replayCompatVersion_ = std::numeric_limits<uint8_t>::max();

public:
    GameWorldBase(std::vector<GamePlayer> players, const GlobalGameSettings& gameSettings, EventManager& em);
    ~GameWorldBase() override;

    // Grundlegende Initialisierungen
    void init(const MapExtent& mapSize, DescIdx<LandscapeDesc> lt = DescIdx<LandscapeDesc>(0)) override;
    // Remaining initialization after loading (BQ...)
    void initAfterLoad();

    GameInterface* getGameInterface() { return gi; }
    const GameInterface* getGameInterface() const { return gi; }
    void setGameInterface(GameInterface* const gi) { this->gi = gi; }

    /// Get the economy mode handler if set.
    /// TODO: Add const correct version, but iwEconomicProgress still needs to be able to call updateAmounts
    EconomyModeHandler* getEconHandler() const { return econHandler.get(); }

    /// Can a node be used for a road (no flag/bld, no other road, no danger...)
    /// Should only be used for the points between the 2 flags of a road
    bool isRoadAvailable(bool boat_road, MapPoint pt) const;
    /// Check if this road already exists completely
    bool roadAlreadyBuilt(bool boat_road, MapPoint start, const std::vector<Direction>& route);
    bool isOnRoad(const MapPoint& pt) const;
    /// Check if a flag is at a neighbour node
    bool isFlagAround(const MapPoint& pt) const;

    /// Checks if any of the neighboring nodes of a given map point are owned by any player.
    bool isAnyNeighborOwned(const MapPoint& pt) const;

    /// Berechnet BQ bei einer gebauten Stra�e
    void recalcBQForRoad(MapPoint pt);
    /// Pr�ft, ob sich in unmittelbarer N�he (im Radius von 4) Milit�rgeb�ude befinden
    bool isMilitaryBuildingNearNode(MapPoint nPt, unsigned char player) const;
    /// Returns true if a Church can be built at `pt` by `player` (within placement radius of own HQ/Storehouse)
    bool canBuildChurchAt(MapPoint pt, unsigned char player) const;
    /// Return true if there is a military building or building site on the node
    /// If attackBldsOnly is true, then only troop buildings are returned
    /// Otherwise it includes e.g. HQ and harbour which cannot attack itself but hold land
    bool isMilitaryBuildingOnNode(MapPoint pt, bool attackBldsOnly) const;
    /// Erstellt eine Liste mit allen Milit�rgeb�uden in der Umgebung, radius bestimmt wie viele K�stchen nach einer
    /// Richtung im Umkreis
    sortedMilitaryBlds lookForMilitaryBuildings(MapPoint pt, unsigned short radius) const;

    /// Finds a path for figures. Returns first direction to walk in if found
    helpers::OptionalEnum<Direction> findHumanPath(MapPoint start, MapPoint dest, unsigned maxRoute = 0xFFFFFFFF,
                                                   bool randomRoute = false, unsigned* length = nullptr,
                                                   std::vector<Direction>* route = nullptr) const;
    /// Find path for ships to a specific harbor and see. Return true on success
    /// If starting point equals the coastal point of target harbor, set length 0, clear route and return true.
    bool findShipPathToHarbor(MapPoint start, HarborId harborId, SeaId seaId, std::vector<Direction>* route,
                              unsigned* length) const;
    /// Find path for ships with a limited distance. Return true on success
    bool findShipPath(MapPoint start, MapPoint dest, unsigned maxDistance, std::vector<Direction>* route,
                      unsigned* length) const;
    RoadPathFinder& getRoadPathFinder() const { return *roadPathFinder; }
    FreePathFinder& getFreePathFinder() const { return *freePathFinder; }
    ShipPathData& getShipPathData() const;

    /// Return flag that is on road at given point. dir will be set to the direction of the road from the returned flag
    /// prevDir (if set) will be skipped when searching for the road points
    noFlag* getRoadFlag(MapPoint pt, Direction& dir, helpers::OptionalEnum<Direction> prevDir = std::nullopt);
    const noFlag* getRoadFlag(MapPoint pt, Direction& dir,
                              helpers::OptionalEnum<Direction> prevDir = std::nullopt) const;

    /// Gets the (height adjusted) global coordinates of the node (e.g. for drawing)
    Position getNodePos(MapPoint pt) const;

    /// Berechnet Bauqualitäten an Punkt x;y und den ersten Kreis darum neu
    void recalcBQAroundPoint(MapPoint pt);
    /// Berechnet Bauqualitäten wie bei letzterer Funktion, bloß noch den 2. Kreis um x;y herum
    void recalcBQAroundPointBig(MapPoint pt);

    /// Ermittelt Sichtbarkeit eines Punktes auch unter Einbeziehung der Verbündeten des jeweiligen Spielers
    Visibility calcVisiblityWithAllies(MapPoint pt, unsigned char player) const;

    /// Ist es an dieser Stelle für einen Spieler möglich einen Hafen zu bauen
    bool isHarborPointFree(HarborId harborId, unsigned char player) const;
    /// Ermittelt, ob ein Punkt Küstenpunkt ist, d.h. Zugang zu einem schiffbaren Meer, an dem auch mindestens 1
    /// Hafenplatz liegt, hat und gibt ggf. die Meeres-ID zurück, ansonsten 0
    bool isCoastalPointToSeaWithHarbor(MapPoint pt) const;
    /// Sucht freie Hafenpunkte, also wo noch ein Hafen gebaut werden kann
    HarborId getNextFreeHarborPoint(MapPoint pt, HarborId originHarborId, const ShipDirection& dir,
                                    unsigned char player) const;
    /// Bestimmt für einen beliebigen Punkt auf der Karte die Entfernung zum nächsten Hafenpunkt
    unsigned calcDistanceToNearestHarbor(MapPoint pos) const;
    /// returns true when a harborpoint is in SEAATTACK_DISTANCE for figures!
    bool isAHarborInSeaAttackDistance(MapPoint pos) const;

    /// Return the player with the given index
    GamePlayer& getPlayer(unsigned id);
    const GamePlayer& getPlayer(unsigned id) const;
    unsigned getNumPlayers() const;
    s25util::span<GamePlayer> getPlayers();
    s25util::span<const GamePlayer> getPlayers() const;
    bool isSinglePlayer() const;
    /// Return the game settings
    const GlobalGameSettings& getGGS() const { return gameSettings; }
    /// Get the replay (minor) version with which (backward) compatibility is enabled.
    uint8_t getReplayCompatVersion() const { return replayCompatVersion_; }
    /// Set the replay minor version to change the behavior of the game for backward compatibility.
    void setReplayCompatVersion(uint8_t version) { replayCompatVersion_ = version; }

    EventManager& getEvMgr() { return em; }
    const EventManager& getEvMgr() const { return em; }
    SoundManager& getSoundMgr() { return *soundManager; }
    PostManager& getPostMgr() { return postManager; }
    const PostManager& getPostMgr() const { return postManager; }
    NotificationManager& getNotifications() const
    {
        // We want to be abled to add notifications even on a const world
        // Hence return as non-const
        return this->notifications;
    }

    struct PotentialSeaAttacker
    {
        /// Comparator that compares only the soldier pointer
        struct CmpSoldier
        {
            nofPassiveSoldier* const search;
            CmpSoldier(nofPassiveSoldier* const search) : search(search) {}
            bool operator()(const PotentialSeaAttacker& other) const { return other.soldier == search; }
        };
        /// Soldat, der als Angreifer in Frage kommt
        nofPassiveSoldier* soldier;
        /// Hafen, den der Soldat zuerst ansteuern soll
        nobHarborBuilding* harbor;
        /// Entfernung Hafen-Hafen (entscheidende)
        unsigned distance;

        PotentialSeaAttacker(nofPassiveSoldier* soldier, nobHarborBuilding* harbor, unsigned distance)
            : soldier(soldier), harbor(harbor), distance(distance)
        {}
    };

    /// Liefert Hafenpunkte im Umkreis von einem bestimmten Milit�rgeb�ude
    std::vector<HarborId> getHarborPointsAroundMilitaryBuilding(MapPoint pt) const;
    /// Return all harbor Ids that can be used as a landing site for attacking the given point
    /// Sets all entries in @param use_seas to true, if the given sea can be used for attacking (seaId=1 -> Index 0 as
    /// seaId=0 is invalid sea)
    std::vector<HarborId> getUsableTargetHarborsForAttack(MapPoint targetPt, std::vector<bool>& use_seas,
                                                          unsigned char playerAttacker) const;
    /// Return all sea Ids from @param usableSeas that can be used for attacking the given point
    std::vector<SeaId> getFilteredSeaIDsForAttack(MapPoint targetPt, const std::vector<SeaId>& usableSeas,
                                                  unsigned char playerAttacker) const;
    /// Return all soldiers (in no specific order) that can be used to attack the given point via a sea.
    /// Checks all preconditions for a sea attack (addon, attackable...)
    std::vector<PotentialSeaAttacker> getSoldiersForSeaAttack(unsigned char playerAttacker, MapPoint pt) const;
    /// Return number or strength (summed ranks) of soldiers that can attack via the given sea
    unsigned getNumSoldiersForSeaAttackAtSea(unsigned char playerAttacker, SeaId sea, bool returnCount = true) const;

    /// Recalculates the BQ for the given point
    void recalcBQ(MapPoint pt);

    void setComputerBarrier(const MapPoint& pt, unsigned radius);
    bool isInsideComputerBarrier(const MapPoint& pt) const;

    bool hasLua() const { return lua != nullptr; }
    LuaInterfaceGame& getLua() const { return *lua; }
    void setLua(LuaInterfaceGame* newLua) { lua = newLua; }

protected:
    /// Called when the visibility of point changed for a player
    void visibilityChanged(MapPoint pt, unsigned player, Visibility oldVis, Visibility newVis) override;
    /// Called, when the altitude of a point was changed
    void altitudeChanged(MapPoint pt) override;

private:
    /// Returns the harbor ID of the next matching harbor in the given direction (0 = None)
    /// T_IsHarborOk must be a predicate taking a harbor Id and returning a bool if the harbor is valid to return
    template<typename T_IsHarborOk>
    HarborId getHarborInDir(MapPoint pt, HarborId originHarborId, const ShipDirection& dir,
                            T_IsHarborOk isHarborOk) const;
};
