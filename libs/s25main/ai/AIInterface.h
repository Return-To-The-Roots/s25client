// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "GameCommand.h"
#include "GamePlayer.h"
#include "NodalObjectTypes.h"
#include "ai/AIResource.h"
#include "factories/GameCommandFactory.h"
#include "helpers/OptionalEnum.h"
#include "world/GameWorldBase.h"
#include "gameTypes/ChatDestination.h"
#include "gameTypes/Direction.h"
#include <memory>
#include <vector>

class nobHQ;
class nobShipYard;
class RoadSegment;
class noBuilding;
class noBuildingSite;
class noFlag;
class noRoadNode;
class noShip;
class nobBaseWarehouse;
class nobHarborBuilding;
class nobMilitary;
class nobUsual;
struct Inventory;
class GameMessage_Chat;

class AIInterface : public GameCommandFactory
{
public:
    AIInterface(const GameWorldBase& gwb, std::vector<gc::GameCommandPtr>& gcs, unsigned char playerID);
    ~AIInterface();

    unsigned char GetPlayerId() const { return playerID_; }
    unsigned GetNumPlayers() const { return gwb.GetNumPlayers(); }
    const std::vector<unsigned>& getUsableHarbors() const { return usableHarbors_; }

    bool IsDefeated() const { return player_.IsDefeated(); }
    /// Return the resource buried on a given spot (gold, coal, ironore, granite (sub), fish, nothing)
    AISubSurfaceResource GetSubsurfaceResource(MapPoint pt) const;
    /// Return the resource on top on a given spot (wood, stones, nothing)
    AISurfaceResource GetSurfaceResource(MapPoint pt) const;
    /// Calculate the surface resource value on a given spot (wood/ stones/ farmland)
    /// when given a direction and lastvalue the calculation will be much faster O(n) vs O(n^2)
    int CalcResourceValue(MapPoint pt, AIResource res, helpers::OptionalEnum<Direction> direction = boost::none,
                          int lastval = 0xffff) const;
    /// Calculate the resource value for a given point
    int GetResourceRating(MapPoint pt, AIResource res) const;
    /// Test whether a given point is part of the border or not
    bool IsBorder(const MapPoint pt) const
    {
        return gwb.GetNode(pt).boundary_stones[BorderStonePos::OnPoint] == (playerID_ + 1);
    }
    /// Test whether a given point is part of own territory
    bool IsOwnTerritory(const MapPoint pt) const { return gwb.GetNode(pt).owner == (playerID_ + 1); }

    bool IsRoad(const MapPoint pt, Direction dir) { return gwb.GetPointRoad(pt, dir) != PointRoad::None; }
    /// Test whether there is a object of a certain type on a spot
    bool IsObjectTypeOnNode(const MapPoint pt, NodalObjectType objectType) const
    {
        return gwb.GetNO(pt)->GetType() == objectType;
    }
    /// Test whether there is specific building on a spot
    bool IsBuildingOnNode(const MapPoint pt, BuildingType bld) const
    {
        const noBase* no = gwb.GetNO(pt);
        const NodalObjectType noType = no->GetType();
        return (noType == NodalObjectType::Building || noType == NodalObjectType::Buildingsite)
               && (static_cast<const noBaseBuilding*>(no)->GetBuildingType() == bld);
    }
    /// Test whether the ai player can see a point
    bool IsVisible(const MapPoint pt) const
    {
        return gwb.CalcVisiblityWithAllies(pt, playerID_) == Visibility::Visible;
    }
    /// Return true when the building quality at the 2nd point is lower than the bq on the first point
    bool CalcBQSumDifference(MapPoint pt1, MapPoint pt2) const;
    /// Return building quality on a given spot
    BuildingQuality GetBuildingQuality(MapPoint pt) const;
    BuildingQuality GetBuildingQualityAnyOwner(MapPoint pt) const;
    /// Tries to find a free path for a road and return length and the route
    bool FindFreePathForNewRoad(MapPoint start, MapPoint target, std::vector<Direction>* route = nullptr,
                                unsigned* length = nullptr) const;
    /// Tries to find a route from start to target, returning length of that route if it exists
    bool FindPathOnRoads(const noRoadNode& start, const noRoadNode& target, unsigned* length = nullptr) const;
    /// Checks if it is allowed to build catapults
    bool CanBuildCatapult() const { return player_.CanBuildCatapult(); }
    /// checks if the player is allowed to build the building type (lua maybe later addon?)
    bool CanBuildBuildingtype(BuildingType bt) const { return player_.IsBuildingEnabled(bt); }
    /// Can the current player (AI) attack the other player?
    bool CanAttack(unsigned char otherPlayerId) const { return gwb.GetPlayer(GetPlayerId()).CanAttack(otherPlayerId); }
    /// player.FindWarehouse
    template<class T_IsWarehouseGood>
    nobBaseWarehouse* FindWarehouse(const noRoadNode& start, const T_IsWarehouseGood& isWarehouseGood, bool to_wh,
                                    bool use_boat_roads, unsigned* length = nullptr,
                                    const RoadSegment* forbidden = nullptr) const
    {
        return player_.FindWarehouse(start, isWarehouseGood, to_wh, use_boat_roads, length, forbidden);
    }
    /// Return the headquarter of the player (or null if destroyed)
    const nobHQ* GetHeadquarter() const;
    /// Return reference to the list of building sites
    const std::list<noBuildingSite*>& GetBuildingSites() const
    {
        return player_.GetBuildingRegister().GetBuildingSites();
    }
    const std::list<noBuildingSite*>& GetPlayerBuildingSites(unsigned playerId) const
    {
        return gwb.GetPlayer(playerId).GetBuildingRegister().GetBuildingSites();
    }
    /// Return a list to buildings of a given type
    const std::list<nobUsual*>& GetBuildings(const BuildingType type) const
    {
        return player_.GetBuildingRegister().GetBuildings(type);
    }
    const std::list<nobUsual*>& GetPlayerBuildings(const BuildingType type, unsigned playerId) const
    {
        return gwb.GetPlayer(playerId).GetBuildingRegister().GetBuildings(type);
    }
    // Return a list containing all military buildings
    const std::list<nobMilitary*>& GetMilitaryBuildings() const
    {
        return player_.GetBuildingRegister().GetMilitaryBuildings();
    }
    /// Return a list containing all harbors
    const std::list<nobHarborBuilding*>& GetHarbors() const { return player_.GetBuildingRegister().GetHarbors(); }
    /// Return a list containing all storehouses and harbors and the hq
    const std::list<nobBaseWarehouse*>& GetStorehouses() const
    {
        return player_.GetBuildingRegister().GetStorehouses();
    }

    /// Check if there is a building of the given type in a radius of at most maxDistance
    bool isBuildingNearby(BuildingType bldType, MapPoint pt, unsigned maxDistance) const;
    /// Check if there is a (useful) harbor spot in at most the given distance. onlyEmpty=True -> No harbor building
    /// there yet
    bool isHarborPosClose(MapPoint pt, unsigned maxDistance, bool onlyEmpty = false) const;

    /// Return the inventory of the AI player
    const Inventory& GetInventory() const { return player_.GetInventory(); }
    /// Return the number of ships
    unsigned GetNumShips() const { return player_.GetNumShips(); }
    /// Return the list of ships
    const std::vector<noShip*>& GetShips() const { return player_.GetShips(); }
    /// Return the ID of a given ship
    unsigned GetShipID(const noShip* ship) const { return player_.GetShipID(ship); }
    /// Test whether there is a possibility to start a expedition in a given direction from a given position, assuming a
    /// given starting harbor
    bool IsExplorationDirectionPossible(MapPoint pt, const nobHarborBuilding* originHarbor,
                                        ShipDirection direction) const;
    /// Test whether there is a possibility to start a expedition in a given direction from a given position, assuming a
    /// given starting harbor
    bool IsExplorationDirectionPossible(MapPoint pt, unsigned originHarborID, ShipDirection direction) const;
    Nation GetNation() const { return player_.nation; }

    bool SetCoinsAllowed(const nobMilitary* building, bool enabled);
    using GameCommandFactory::SetCoinsAllowed;

    bool StartStopExpedition(const nobHarborBuilding* hb, bool start);
    using GameCommandFactory::StartStopExpedition;

    bool FoundColony(const noShip* ship) { return FoundColony(GetShipID(ship)); }
    using GameCommandFactory::FoundColony;

    bool TravelToNextSpot(ShipDirection direction, const noShip* ship)
    {
        return TravelToNextSpot(direction, GetShipID(ship));
    }
    using GameCommandFactory::TravelToNextSpot;

    bool CancelExpedition(const noShip* ship) { return CancelExpedition(GetShipID(ship)); }
    using GameCommandFactory::CancelExpedition;

    bool SetShipYardMode(const nobShipYard* shipyard, bool buildShips);
    using GameCommandFactory::SetShipYardMode;

    bool DestroyBuilding(const noBuilding* building);
    using GameCommandFactory::DestroyBuilding;

    bool DestroyFlag(const noFlag* flag);
    using GameCommandFactory::DestroyFlag;

    bool CallSpecialist(const noFlag* flag, Job job);
    using GameCommandFactory::CallSpecialist;

    /// Sends a chat messsage to all players
    void Chat(const std::string& message, ChatDestination destination = ChatDestination::All);

    std::vector<std::unique_ptr<GameMessage_Chat>> FetchChatMessages();

    /// Pointer to GameWorld, containing all information about the world
    const GameWorldBase& gwb;

private:
    bool AddGC(gc::GameCommandPtr gc) override
    {
        gcs.push_back(gc);
        return true;
    }

    /// Pointer to this player, containing all information about his economy, buildings, etc.
    const GamePlayer& player_;
    /// Pointer to the game commands queue, to send commands to the game
    std::vector<gc::GameCommandPtr>& gcs;
    std::vector<std::unique_ptr<GameMessage_Chat>> pendingChatMsgs_;
    /// ID of AI player
    const unsigned char playerID_;
    /// Harbor ids which have at least one other harbor at the same sea
    std::vector<unsigned> usableHarbors_;
};
