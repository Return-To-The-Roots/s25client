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
#include "gameTypes/InventorySetting.h"
#include "gameData/MilitaryConsts.h"
#include <vector>

namespace gc { class GameCommand; }

/// GameCommandFactory that uses static inheritance via CRTP
/// to call the derived class' AddGC method whenever a GC is constructed
template<class T_Handler>
class GameCommandFactory{
public:
    /// Sets a flag on a spot
    bool SetFlag(const MapPoint pt);
    /// Destroys a flag on a spot
    bool DestroyFlag(const MapPoint pt);
    /// Builds a road from a starting point along a given route
    bool BuildRoad(const MapPoint pt, const bool boat_road, const std::vector<unsigned char>& route);
    /// Destroys a road on a spot
    bool DestroyRoad(const MapPoint pt, const unsigned char start_dir);
    bool UpgradeRoad(const MapPoint pt, const unsigned char start_dir);
    /// Sets new distribution of goods
    bool ChangeDistribution(const Distributions& data);
    bool ChangeBuildOrder(const unsigned char order_type, const BuildOrders& data);
    /// Sets a building site (new building)
    bool SetBuildingSite(const MapPoint pt, const BuildingType bt);
    /// Destroys a building on a spot
    bool DestroyBuilding(const MapPoint pt);
    /// send out soldiers
    bool SendSoldiersHome(const MapPoint pt);
    /// order new soldiers
    bool OrderNewSoldiers(const MapPoint pt);
    bool ChangeTransport(const TransportOrders& data);
    /// Sets new military settings for the player (8 values)
    bool ChangeMilitary(const boost::array<unsigned char, MILITARY_SETTINGS_COUNT>& data);
    /// Sets new tool production settings
    bool ChangeTools(const ToolSettings& data, signed char* order_delta = NULL);
    /// Calls a geologist to a flag
    bool CallGeologist(const MapPoint pt);
    bool CallScout(const MapPoint pt);
    /// Attacks an enemy building
    bool Attack(const MapPoint pt, const unsigned soldiers_count, const bool strong_soldiers);
    /// Sea-Attacks an enemy building
    bool SeaAttack(const MapPoint pt, const unsigned soldiers_count, const bool strong_soldiers);
    bool SwitchPlayer(const unsigned char new_player_id);
    /// Toggles coin delivery on/off for a military building
    bool ToggleCoins(const MapPoint pt);
    /// Stops/starts production of a producer
    bool ToggleProduction(const MapPoint pt);
    bool NotifyAlliesOfLocation(const MapPoint pt);
    /// changes inventory settings for a warehouse by XOR with old settings (self fixing stupid settings)
    bool ChangeInventorySetting(const MapPoint pt, const unsigned char category, const InventorySetting state, const unsigned char type);
    bool ChangeAllInventorySettings(const MapPoint pt, const unsigned char category, const InventorySetting state);
    bool ChangeReserve(const MapPoint pt, const unsigned char rank, const unsigned char count);
    bool CheatArmageddon();
    /// Simply surrenders...
    bool Surrender();
    bool DestroyAll();
    bool SuggestPact(const unsigned char player, const PactType pt, const unsigned duration);
    bool AcceptPact(const bool accepted, const unsigned id, const PactType pt, const unsigned char player);
    bool CancelPact(const PactType pt, const unsigned char player);
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
    bool TradeOverLand(const MapPoint pt, const bool ware_figure, const GoodType gt, const Job job, const unsigned count);

private:
    bool AddGC_Virt(gc::GameCommand* gc)
    {
        return static_cast<T_Handler*>(this)->AddGC(gc);
    }
};

#endif // GameMessageFactory_h__