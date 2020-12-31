// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "GameCommand.h"
#include "gameTypes/BuildingType.h"
#include "gameTypes/Direction.h"
#include "gameTypes/GoodTypes.h"
#include "gameTypes/JobTypes.h"
#include "gameTypes/MapCoordinates.h"
#include "gameTypes/PactTypes.h"
#include "gameTypes/SettingsTypes.h"
#include "gameTypes/ShipDirection.h"
#include <boost/variant.hpp>
#include <vector>

struct InventorySetting;

/// Factory class for creating game commands. Handling of them (storing, sending...) must be done in the derived class
class GameCommandFactory
{
public:
    /// Sets a flag on a spot
    bool SetFlag(MapPoint pt);
    /// Destroys a flag on a spot
    bool DestroyFlag(MapPoint pt);
    /// Builds a road from a starting point along a given route
    bool BuildRoad(MapPoint pt, bool boat_road, const std::vector<Direction>& route);
    /// Destroys a road on a spot
    bool DestroyRoad(MapPoint pt, Direction start_dir);
    bool UpgradeRoad(MapPoint pt, Direction start_dir);
    /// Sets new distribution of goods
    bool ChangeDistribution(const Distributions& data);
    bool ChangeBuildOrder(bool useCustomBuildOrder, const BuildOrders& data);
    /// Sets a building site (new building)
    bool SetBuildingSite(MapPoint pt, BuildingType bt);
    /// Destroys a building on a spot
    bool DestroyBuilding(MapPoint pt);
    /// send out soldiers
    bool SendSoldiersHome(MapPoint pt);
    /// order new soldiers
    bool OrderNewSoldiers(MapPoint pt);
    bool ChangeTransport(const TransportOrders& data);
    /// Sets new military settings for the player (8 values)
    bool ChangeMilitary(const MilitarySettings& data);
    /// Sets new tool production settings
    bool ChangeTools(const ToolSettings& data, const int8_t* order_delta = nullptr);
    /// Calls a specialist to a flag
    bool CallSpecialist(MapPoint pt, Job job);
    /// Attacks an enemy building
    bool Attack(MapPoint pt, unsigned soldiers_count, bool strong_soldiers);
    /// Sea-Attacks an enemy building
    bool SeaAttack(MapPoint pt, unsigned soldiers_count, bool strong_soldiers);
    /// Toggles coin delivery on/off for a military building
    bool SetCoinsAllowed(MapPoint pt, bool enabled);
    /// Stops/starts production of a producer
    bool SetProductionEnabled(MapPoint pt, bool enabled);
    bool NotifyAlliesOfLocation(MapPoint pt);
    /// Sets inventory settings for a warehouse
    bool SetInventorySetting(MapPoint pt, const boost::variant<GoodType, Job>& what, InventorySetting state);
    bool SetAllInventorySettings(MapPoint pt, bool isJob, const std::vector<InventorySetting>& states);
    bool ChangeReserve(MapPoint pt, unsigned char rank, unsigned count);
    bool CheatArmageddon();
    /// Simply surrenders...
    bool Surrender();
    bool DestroyAll();
    bool SuggestPact(unsigned char player, PactType pt, unsigned duration);
    bool AcceptPact(unsigned id, PactType pt, unsigned char player);
    bool CancelPact(PactType pt, unsigned char player);
    /// Toggles the construction mode of the shipyard between boat and ship
    bool SetShipYardMode(MapPoint pt, bool buildShips);
    /// Starts Preparation of an sea expedition in a habor
    bool StartStopExpedition(MapPoint pt, bool start);
    /// Lets a ship found a colony
    bool FoundColony(unsigned shipID);
    /// Lets a ship travel to a new harbor spot in a given direction
    bool TravelToNextSpot(ShipDirection direction, unsigned shipID);
    /// Cancels an expedition
    bool CancelExpedition(unsigned shipID);
    bool StartStopExplorationExpedition(MapPoint pt, bool start);
    bool TradeOverLand(MapPoint pt, const boost::variant<GoodType, Job>& what, unsigned count);

protected:
    virtual ~GameCommandFactory() = default;
    /// Called for each created GC. Return true iff this is going to be executed
    virtual bool AddGC(gc::GameCommandPtr gc) = 0;
};
