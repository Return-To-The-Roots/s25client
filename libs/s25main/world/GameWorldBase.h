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

#ifndef GameWorldBase_h__
#define GameWorldBase_h__

#include "buildings/nobBaseMilitary.h"
#include "lua/LuaInterfaceGame.h"
#include "notifications/NotificationManager.h"
#include "postSystem/PostManager.h"
#include "world/World.h"
#include "libutil/unique_ptr.h"
#include <vector>

class EventManager;
class FreePathFinder;
class GamePlayer;
class GameInterface;
class GlobalGameSettings;
class LuaInterfaceGame;
class noBase;
class noBuildingSite;
class noFlag;
class nobHarborBuilding;
class nofPassiveSoldier;
class RoadPathFinder;

/// Grundlegende Klasse, die die Gamewelt darstellt, enth�lt nur deren Daten
class GameWorldBase : public World
{
    libutil::unique_ptr<RoadPathFinder> roadPathFinder;
    libutil::unique_ptr<FreePathFinder> freePathFinder;
    PostManager postManager;
    NotificationManager notifications;

    std::vector<GamePlayer> players;
    const GlobalGameSettings& gameSettings;
    EventManager& em;
    libutil::unique_ptr<LuaInterfaceGame> lua;

protected:
    /// Interface zum GUI
    GameInterface* gi;
    /// harbor building sites created by ships
    std::list<noBuildingSite*> harbor_building_sites_from_sea;

public:
    GameWorldBase(const std::vector<GamePlayer>& players, const GlobalGameSettings& gameSettings, EventManager& em);
    ~GameWorldBase() override;

    // Grundlegende Initialisierungen
    void Init(const MapExtent& mapSize, DescIdx<LandscapeDesc> lt = DescIdx<LandscapeDesc>(0)) override;
    // Remaining initialization after loading (BQ...)
    void InitAfterLoad();

    /// Setzt GameInterface
    void SetGameInterface(GameInterface* const gi) { this->gi = gi; }

    /// Can a node be used for a road (no flag/bld, no other road, no danger...)
    /// Should only be used for the points between the 2 flags of a road
    bool IsRoadAvailable(const bool boat_road, const MapPoint pt) const;
    /// Check if this road already exists completely
    bool RoadAlreadyBuilt(const bool boat_road, const MapPoint start, const std::vector<Direction>& route);
    bool IsOnRoad(const MapPoint& pt) const;
    /// Check if a flag is at a neighbour node
    bool IsFlagAround(const MapPoint& pt) const;

    /// Berechnet BQ bei einer gebauten Stra�e
    void RecalcBQForRoad(const MapPoint pt);
    /// Pr�ft, ob sich in unmittelbarer N�he (im Radius von 4) Milit�rgeb�ude befinden
    bool IsMilitaryBuildingNearNode(const MapPoint nPt, const unsigned char player) const;
    /// Return true if there is a military building or building site on the node
    /// If attackBldsOnly is true, then only troop buildings are returned
    /// Otherwise it includes e.g. HQ and harbour which cannot attack itself but hold land
    bool IsMilitaryBuildingOnNode(const MapPoint pt, bool attackBldsOnly) const;
    /// Erstellt eine Liste mit allen Milit�rgeb�uden in der Umgebung, radius bestimmt wie viele K�stchen nach einer Richtung im Umkreis
    sortedMilitaryBlds LookForMilitaryBuildings(const MapPoint pt, unsigned short radius) const;

    /// Finds a path for figures. Returns 0xFF if none found
    unsigned char FindHumanPath(const MapPoint start, const MapPoint dest, unsigned max_route = 0xFFFFFFFF, bool random_route = false,
                                unsigned* length = NULL, std::vector<Direction>* route = NULL) const;
    /// Find path for ships to a specific harbor and see. Return true on success
    bool FindShipPathToHarbor(const MapPoint start, unsigned harborId, unsigned seaId, std::vector<Direction>* route, unsigned* length);
    /// Find path for ships with a limited distance. Return true on success
    bool FindShipPath(const MapPoint start, const MapPoint dest, unsigned maxDistance, std::vector<Direction>* route, unsigned* length);
    RoadPathFinder& GetRoadPathFinder() const { return *roadPathFinder; }
    FreePathFinder& GetFreePathFinder() const { return *freePathFinder; }

    /// Return flag that is on road at given point. dir will be set to the direction of the road from the returned flag
    /// prevDir (if set) will be skipped when searching for the road points
    noFlag* GetRoadFlag(MapPoint pt, Direction& dir, unsigned prevDir = 255);
    const noFlag* GetRoadFlag(MapPoint pt, Direction& dir, unsigned prevDir = 255) const;

    /// Erzeugt eine GUI-ID für die Fenster von Map-Objekten
    unsigned CreateGUIID(const MapPoint pt) const { return 1000 + GetIdx(pt); }

    /// Gets the (height adjusted) global coordinates of the node (e.g. for drawing)
    Position GetNodePos(const MapPoint pt) const;

    /// Berechnet Bauqualitäten an Punkt x;y und den ersten Kreis darum neu
    void RecalcBQAroundPoint(const MapPoint pt);
    /// Berechnet Bauqualitäten wie bei letzterer Funktion, bloß noch den 2. Kreis um x;y herum
    void RecalcBQAroundPointBig(const MapPoint pt);

    /// Ermittelt Sichtbarkeit eines Punktes auch unter Einbeziehung der Verbündeten des jeweiligen Spielers
    Visibility CalcVisiblityWithAllies(const MapPoint pt, const unsigned char player) const;

    /// Ist es an dieser Stelle für einen Spieler möglich einen Hafen zu bauen
    bool IsHarborPointFree(const unsigned harborId, const unsigned char player) const;
    /// Ermittelt, ob ein Punkt Küstenpunkt ist, d.h. Zugang zu einem schiffbaren Meer, an dem auch mindestens 1 Hafenplatz liegt, hat
    /// und gibt ggf. die Meeres-ID zurück, ansonsten 0
    bool IsCoastalPointToSeaWithHarbor(const MapPoint pt) const;
    /// Sucht freie Hafenpunkte, also wo noch ein Hafen gebaut werden kann
    unsigned GetNextFreeHarborPoint(const MapPoint pt, const unsigned origin_harborId, const ShipDirection& dir,
                                    const unsigned char player) const;
    /// Bestimmt für einen beliebigen Punkt auf der Karte die Entfernung zum nächsten Hafenpunkt
    unsigned CalcDistanceToNearestHarbor(const MapPoint pos) const;
    /// returns true when a harborpoint is in SEAATTACK_DISTANCE for figures!
    bool IsAHarborInSeaAttackDistance(const MapPoint pos) const;

    /// Return the player with the given index
    GamePlayer& GetPlayer(const unsigned id);
    const GamePlayer& GetPlayer(const unsigned id) const;
    unsigned GetNumPlayers() const;
    bool IsSinglePlayer() const;
    /// Return the game settings
    const GlobalGameSettings& GetGGS() const { return gameSettings; }
    EventManager& GetEvMgr() { return em; }
    const EventManager& GetEvMgr() const { return em; }
    PostManager& GetPostMgr() { return postManager; }
    const PostManager& GetPostMgr() const { return postManager; }
    NotificationManager& GetNotifications() const
    {
        return const_cast<GameWorldBase*>(this)->notifications;
    } // We want to be abled to add notifications even on a const world

    struct PotentialSeaAttacker
    {
        /// Comparator that compares only the soldier pointer
        struct CmpSoldier
        {
            nofPassiveSoldier* const search;
            CmpSoldier(nofPassiveSoldier* const search) : search(search) {}
            bool operator()(const PotentialSeaAttacker& other) { return other.soldier == search; }
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
    std::vector<unsigned> GetHarborPointsAroundMilitaryBuilding(const MapPoint pt) const;
    /// Return all harbor Ids that can be used as a landing site for attacking the given point
    /// Sets all entries in @param use_seas to true, if the given sea can be used for attacking (seaId=1 -> Index 0 as seaId=0 is invalid
    /// sea)
    std::vector<unsigned> GetUsableTargetHarborsForAttack(const MapPoint targetPt, std::vector<bool>& use_seas,
                                                          const unsigned char player_attacker) const;
    /// Return all sea Ids from @param usableSeas that can be used for attacking the given point
    std::vector<unsigned short> GetFilteredSeaIDsForAttack(const MapPoint targetPt, const std::vector<unsigned short>& usableSeas,
                                                           const unsigned char player_attacker) const;
    /// Return all soldiers (in no specific order) that can be used to attack the given point via a sea.
    /// Checks all preconditions for a sea attack (addon, attackable...)
    std::vector<PotentialSeaAttacker> GetSoldiersForSeaAttack(const unsigned char player_attacker, const MapPoint targetPt) const;
    /// Return number or strength (summed ranks) of soldiers that can attack via the given sea
    unsigned GetNumSoldiersForSeaAttackAtSea(const unsigned char player_attacker, unsigned short seaid, bool returnCount = true) const;

    /// Recalculates the BQ for the given point
    void RecalcBQ(const MapPoint pt);

    bool HasLua() const { return lua.get() != NULL; }
    LuaInterfaceGame& GetLua() const { return *lua.get(); }
    void SetLua(LuaInterfaceGame* newLua) { lua.reset(newLua); }

protected:
    /// Called when the visibility of point changed for a player
    void VisibilityChanged(const MapPoint pt, unsigned player, Visibility oldVis, Visibility newVis) override;
    /// Called, when the altitude of a point was changed
    void AltitudeChanged(const MapPoint pt) override;

private:
    /// Returns the harbor ID of the next matching harbor in the given direction (0 = None)
    /// T_IsHarborOk must be a predicate taking a harbor Id and returning a bool if the harbor is valid to return
    template<typename T_IsHarborOk>
    unsigned GetHarborInDir(const MapPoint pt, const unsigned origin_harborId, const ShipDirection& dir, T_IsHarborOk isHarborOk) const;
};

#endif // GameWorldBase_h__
