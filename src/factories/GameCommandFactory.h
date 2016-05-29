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

#ifndef GameCommandFactory_h__
#define GameCommandFactory_h__

#include "gameTypes/MapTypes.h"
#include "gameTypes/BuildingTypes.h"
#include "gameTypes/GoodTypes.h"
#include "gameTypes/JobTypes.h"
#include "gameTypes/ShipDirection.h"
#include "gameTypes/PactTypes.h"
#include "gameTypes/SettingsTypes.h"
#include "gameData/MilitaryConsts.h"
#include <vector>

namespace gc { class GameCommand; }
struct InventorySetting;

/// Factory class for creating game commands. Handling of them (storing, sending...) must be done in the derived class
class GameCommandFactory
{
public:
    /// Sets a flag on a spot
    bool SetFlag(const MapPoint pt);
    /// Destroys a flag on a spot
    bool DestroyFlag(const MapPoint pt);
    /// Builds a road from a starting point along a given route
    bool BuildRoad(const MapPoint pt, bool boat_road, const std::vector<unsigned char>& route);
    /// Destroys a road on a spot
    bool DestroyRoad(const MapPoint pt, unsigned char start_dir);
    bool UpgradeRoad(const MapPoint pt, unsigned char start_dir);
    /// Sets new distribution of goods
    bool ChangeDistribution(const Distributions& data);
    bool ChangeBuildOrder(bool useCustomBuildOrder, const BuildOrders& data);
    /// Sets a building site (new building)
    bool SetBuildingSite(const MapPoint pt, BuildingType bt);
    /// Destroys a building on a spot
    bool DestroyBuilding(const MapPoint pt);
    /// send out soldiers
    bool SendSoldiersHome(const MapPoint pt);
    /// order new soldiers
    bool OrderNewSoldiers(const MapPoint pt);
    bool ChangeTransport(const TransportOrders& data);
    /// Sets new military settings for the player (8 values)
    bool ChangeMilitary(const MilitarySettings& data);
    /// Sets new tool production settings
    bool ChangeTools(const ToolSettings& data, const signed char* order_delta = NULL);
    /// Calls a geologist to a flag
    bool CallGeologist(const MapPoint pt);
    bool CallScout(const MapPoint pt);
    /// Attacks an enemy building
    bool Attack(const MapPoint pt, unsigned soldiers_count, bool strong_soldiers);
    /// Sea-Attacks an enemy building
    bool SeaAttack(const MapPoint pt, unsigned soldiers_count, bool strong_soldiers);
    /// Toggles coin delivery on/off for a military building
    bool SetCoinsAllowed(const MapPoint pt, bool enabled);
    /// Stops/starts production of a producer
    bool SetProductionEnabled(const MapPoint pt, bool enabled);
    bool NotifyAlliesOfLocation(const MapPoint pt);
    /// Sets inventory settings for a warehouse
    bool SetInventorySetting(const MapPoint pt, bool isJob, unsigned char type, InventorySetting state);
    bool SetInventorySetting(const MapPoint pt, Job job, InventorySetting state);
    bool SetInventorySetting(const MapPoint pt, GoodType good, InventorySetting state);
    bool SetAllInventorySettings(const MapPoint pt, bool isJob, const std::vector<InventorySetting>& states);
    bool ChangeReserve(const MapPoint pt, unsigned char rank, unsigned char count);
    bool CheatArmageddon();
    /// Simply surrenders...
    bool Surrender();
    bool DestroyAll();
    bool SuggestPact(unsigned char player, PactType pt, unsigned duration);
    bool AcceptPact(bool accepted, unsigned id, PactType pt, unsigned char player);
    bool CancelPact(PactType pt, unsigned char player);
    /// Toggles the construction mode of the shipyard between boat and ship
    bool ToggleShipYardMode(const MapPoint pt);
    /// Starts Preparation of an sea expedition in a habor
    bool StartExpedition(const MapPoint pt);
    /// Lets a ship found a colony
    bool FoundColony(unsigned int shipID);
    /// Lets a ship travel to a new harbor spot in a given direction
    bool TravelToNextSpot(ShipDirection direction, unsigned int shipID);
    /// Cancels an expedition
    bool CancelExpedition(unsigned int shipID);
    bool StartExplorationExpedition(const MapPoint pt);
    bool TradeOverLand(const MapPoint pt, bool ware_figure, GoodType gt, Job job, unsigned count);

protected:
    virtual ~GameCommandFactory(){}
    /// Called for each created GC. Ownership over gc is passed!
    virtual bool AddGC(gc::GameCommand* gc) = 0;
};

#endif // GameMessageFactory_h__
