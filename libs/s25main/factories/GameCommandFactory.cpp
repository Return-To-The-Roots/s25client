// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GameCommandFactory.h"
#include "GameCommands.h"
#include <stdexcept>

bool GameCommandFactory::SetFlag(const MapPoint pt)
{
    return addGC(new gc::SetFlag(pt));
}

bool GameCommandFactory::DestroyFlag(const MapPoint pt)
{
    return addGC(new gc::DestroyFlag(pt));
}

bool GameCommandFactory::BuildRoad(const MapPoint pt, bool boat_road, const std::vector<Direction>& route)
{
    return addGC(new gc::BuildRoad(pt, boat_road, route));
}

bool GameCommandFactory::DestroyRoad(const MapPoint pt, Direction start_dir)
{
    return addGC(new gc::DestroyRoad(pt, start_dir));
}

bool GameCommandFactory::UpgradeRoad(const MapPoint pt, Direction start_dir)
{
    return addGC(new gc::UpgradeRoad(pt, start_dir));
}

bool GameCommandFactory::ChangeDistribution(const Distributions& data)
{
    return addGC(new gc::ChangeDistribution(data));
}

bool GameCommandFactory::ChangeBuildOrder(bool useCustomBuildOrder, const BuildOrders& data)
{
    return addGC(new gc::ChangeBuildOrder(useCustomBuildOrder, data));
}

bool GameCommandFactory::SetBuildingSite(const MapPoint pt, BuildingType bt)
{
    return addGC(new gc::SetBuildingSite(pt, bt));
}

bool GameCommandFactory::DestroyBuilding(const MapPoint pt)
{
    return addGC(new gc::DestroyBuilding(pt));
}

bool GameCommandFactory::SetTroopLimit(const MapPoint pt, unsigned char rank, unsigned limit)
{
    return addGC(new gc::SetTroopLimit(pt, rank, limit));
}

bool GameCommandFactory::ChangeTransport(const TransportOrders& data)
{
    return addGC(new gc::ChangeTransport(data));
}

bool GameCommandFactory::ChangeMilitary(const MilitarySettings& data)
{
    return addGC(new gc::ChangeMilitary(data));
}

bool GameCommandFactory::ChangeTools(const ToolSettings& data, const int8_t* orderDelta)
{
    return addGC(new gc::ChangeTools(data, orderDelta));
}

bool GameCommandFactory::CallSpecialist(const MapPoint pt, Job job)
{
    RTTR_Assert(job == Job::Geologist || job == Job::Scout);
    return addGC(new gc::CallSpecialist(pt, job));
}

bool GameCommandFactory::Attack(const MapPoint pt, unsigned soldiers_count, bool strong_soldiers)
{
    return addGC(new gc::Attack(pt, soldiers_count, strong_soldiers));
}

bool GameCommandFactory::SeaAttack(const MapPoint pt, unsigned soldiers_count, bool strong_soldiers)
{
    return addGC(new gc::SeaAttack(pt, soldiers_count, strong_soldiers));
}

bool GameCommandFactory::SetCoinsAllowed(const MapPoint pt, bool enabled)
{
    return addGC(new gc::SetCoinsAllowed(pt, enabled));
}

bool GameCommandFactory::SetArmorAllowed(const MapPoint pt, bool enabled)
{
    return addGC(new gc::SetArmorAllowed(pt, enabled));
}

bool GameCommandFactory::SetProductionEnabled(const MapPoint pt, bool enabled)
{
    return addGC(new gc::SetProductionEnabled(pt, enabled));
}

bool GameCommandFactory::NotifyAlliesOfLocation(const MapPoint pt)
{
    return addGC(new gc::NotifyAlliesOfLocation(pt));
}

bool GameCommandFactory::SetInventorySetting(const MapPoint pt, const boost_variant2<GoodType, Job>& what,
                                             InventorySetting state)
{
    return addGC(new gc::SetInventorySetting(pt, what, state));
}

bool GameCommandFactory::SetAllInventorySettings(const MapPoint pt, bool isJob,
                                                 const std::vector<InventorySetting>& states)
{
    return addGC(new gc::SetAllInventorySettings(pt, isJob, states));
}

bool GameCommandFactory::ChangeReserve(const MapPoint pt, unsigned char rank, unsigned count)
{
    return addGC(new gc::ChangeReserve(pt, rank, count));
}

bool GameCommandFactory::CheatArmageddon()
{
    return addGC(new gc::CheatArmageddon());
}

bool GameCommandFactory::Surrender()
{
    return addGC(new gc::Surrender());
}

bool GameCommandFactory::DestroyAll()
{
    return addGC(new gc::DestroyAll());
}

bool GameCommandFactory::SuggestPact(unsigned char player, PactType pt, unsigned duration)
{
    return addGC(new gc::SuggestPact(player, pt, duration));
}

bool GameCommandFactory::AcceptPact(unsigned id, PactType pt, unsigned char player)
{
    return addGC(new gc::AcceptPact(id, pt, player));
}

bool GameCommandFactory::CancelPact(const PactType pt, unsigned char player)
{
    return addGC(new gc::CancelPact(pt, player));
}

bool GameCommandFactory::SetShipYardMode(const MapPoint pt, bool buildShips)
{
    return addGC(new gc::SetShipYardMode(pt, buildShips));
}

bool GameCommandFactory::SetTempleProductionMode(MapPoint pt, ProductionMode mode)
{
    return addGC(new gc::SetTempleProductionMode(pt, mode));
}

bool GameCommandFactory::StartStopExpedition(const MapPoint pt, bool start)
{
    return addGC(new gc::StartStopExpedition(pt, start));
}

bool GameCommandFactory::foundColony(unsigned shipID)
{
    return addGC(new gc::ExpeditionCommand(gc::ExpeditionCommand::Action::FoundColony, shipID));
}

bool GameCommandFactory::travelToNextSpot(ShipDirection direction, unsigned shipID)
{
    gc::ExpeditionCommand::Action action;
    switch(direction)
    {
        case ShipDirection::North: action = gc::ExpeditionCommand::Action::North; break;
        case ShipDirection::NorthEast: action = gc::ExpeditionCommand::Action::NorthEast; break;
        case ShipDirection::SouthEast: action = gc::ExpeditionCommand::Action::SouthEast; break;
        case ShipDirection::South: action = gc::ExpeditionCommand::Action::South; break;
        case ShipDirection::SouthWest: action = gc::ExpeditionCommand::Action::SouthWest; break;
        case ShipDirection::NorthWest: action = gc::ExpeditionCommand::Action::NorthWest; break;
    }
    return addGC(new gc::ExpeditionCommand(action, shipID));
}

bool GameCommandFactory::CancelExpedition(unsigned shipID)
{
    return addGC(new gc::ExpeditionCommand(gc::ExpeditionCommand::Action::CancelExpedition, shipID));
}

bool GameCommandFactory::StartStopExplorationExpedition(const MapPoint pt, bool start)
{
    return addGC(new gc::StartStopExplorationExpedition(pt, start));
}

bool GameCommandFactory::TradeOverLand(const MapPoint pt, const boost_variant2<GoodType, Job>& what, unsigned count)
{
    return addGC(new gc::TradeOverLand(pt, what, count));
}
