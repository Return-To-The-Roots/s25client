// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GameCommandFactory.h"
#include "GameCommands.h"
#include <stdexcept>

bool GameCommandFactory::SetFlag(const MapPoint pt)
{
    return AddGC(new gc::SetFlag(pt));
}

bool GameCommandFactory::DestroyFlag(const MapPoint pt)
{
    return AddGC(new gc::DestroyFlag(pt));
}

bool GameCommandFactory::BuildRoad(const MapPoint pt, bool boat_road, const std::vector<Direction>& route)
{
    return AddGC(new gc::BuildRoad(pt, boat_road, route));
}

bool GameCommandFactory::DestroyRoad(const MapPoint pt, Direction start_dir)
{
    return AddGC(new gc::DestroyRoad(pt, start_dir));
}

bool GameCommandFactory::UpgradeRoad(const MapPoint pt, Direction start_dir)
{
    return AddGC(new gc::UpgradeRoad(pt, start_dir));
}

bool GameCommandFactory::ChangeDistribution(const Distributions& data)
{
    return AddGC(new gc::ChangeDistribution(data));
}

bool GameCommandFactory::ChangeBuildOrder(bool useCustomBuildOrder, const BuildOrders& data)
{
    return AddGC(new gc::ChangeBuildOrder(useCustomBuildOrder, data));
}

bool GameCommandFactory::SetBuildingSite(const MapPoint pt, BuildingType bt)
{
    return AddGC(new gc::SetBuildingSite(pt, bt));
}

bool GameCommandFactory::DestroyBuilding(const MapPoint pt)
{
    return AddGC(new gc::DestroyBuilding(pt));
}

bool GameCommandFactory::SetDesiredTroops(const MapPoint pt, unsigned char rank, unsigned count)
{
    return AddGC(new gc::SetDesiredTroops(pt, rank, count));
}

bool GameCommandFactory::ChangeTransport(const TransportOrders& data)
{
    return AddGC(new gc::ChangeTransport(data));
}

bool GameCommandFactory::ChangeMilitary(const MilitarySettings& data)
{
    return AddGC(new gc::ChangeMilitary(data));
}

bool GameCommandFactory::ChangeTools(const ToolSettings& data, const int8_t* order_delta)
{
    return AddGC(new gc::ChangeTools(data, order_delta));
}

bool GameCommandFactory::CallSpecialist(const MapPoint pt, Job job)
{
    RTTR_Assert(job == Job::Geologist || job == Job::Scout);
    return AddGC(new gc::CallSpecialist(pt, job));
}

bool GameCommandFactory::Attack(const MapPoint pt, unsigned soldiers_count, bool strong_soldiers)
{
    return AddGC(new gc::Attack(pt, soldiers_count, strong_soldiers));
}

bool GameCommandFactory::SeaAttack(const MapPoint pt, unsigned soldiers_count, bool strong_soldiers)
{
    return AddGC(new gc::SeaAttack(pt, soldiers_count, strong_soldiers));
}

bool GameCommandFactory::SetCoinsAllowed(const MapPoint pt, bool enabled)
{
    return AddGC(new gc::SetCoinsAllowed(pt, enabled));
}

bool GameCommandFactory::SetProductionEnabled(const MapPoint pt, bool enabled)
{
    return AddGC(new gc::SetProductionEnabled(pt, enabled));
}

bool GameCommandFactory::NotifyAlliesOfLocation(const MapPoint pt)
{
    return AddGC(new gc::NotifyAlliesOfLocation(pt));
}

bool GameCommandFactory::SetInventorySetting(const MapPoint pt, const boost::variant<GoodType, Job>& what,
                                             InventorySetting state)
{
    return AddGC(new gc::SetInventorySetting(pt, what, state));
}

bool GameCommandFactory::SetAllInventorySettings(const MapPoint pt, bool isJob,
                                                 const std::vector<InventorySetting>& states)
{
    return AddGC(new gc::SetAllInventorySettings(pt, isJob, states));
}

bool GameCommandFactory::ChangeReserve(const MapPoint pt, unsigned char rank, unsigned count)
{
    return AddGC(new gc::ChangeReserve(pt, rank, count));
}

bool GameCommandFactory::CheatArmageddon()
{
    return AddGC(new gc::CheatArmageddon());
}

bool GameCommandFactory::Surrender()
{
    return AddGC(new gc::Surrender());
}

bool GameCommandFactory::DestroyAll()
{
    return AddGC(new gc::DestroyAll());
}

bool GameCommandFactory::SuggestPact(unsigned char player, PactType pt, unsigned duration)
{
    return AddGC(new gc::SuggestPact(player, pt, duration));
}

bool GameCommandFactory::AcceptPact(unsigned id, PactType pt, unsigned char player)
{
    return AddGC(new gc::AcceptPact(id, pt, player));
}

bool GameCommandFactory::CancelPact(const PactType pt, unsigned char player)
{
    return AddGC(new gc::CancelPact(pt, player));
}

bool GameCommandFactory::SetShipYardMode(const MapPoint pt, bool buildShips)
{
    return AddGC(new gc::SetShipYardMode(pt, buildShips));
}

bool GameCommandFactory::StartStopExpedition(const MapPoint pt, bool start)
{
    return AddGC(new gc::StartStopExpedition(pt, start));
}

bool GameCommandFactory::FoundColony(unsigned shipID)
{
    return AddGC(new gc::ExpeditionCommand(gc::ExpeditionCommand::Action::FoundColony, shipID));
}

bool GameCommandFactory::TravelToNextSpot(ShipDirection direction, unsigned shipID)
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
    return AddGC(new gc::ExpeditionCommand(action, shipID));
}

bool GameCommandFactory::CancelExpedition(unsigned shipID)
{
    return AddGC(new gc::ExpeditionCommand(gc::ExpeditionCommand::Action::CancelExpedition, shipID));
}

bool GameCommandFactory::StartStopExplorationExpedition(const MapPoint pt, bool start)
{
    return AddGC(new gc::StartStopExplorationExpedition(pt, start));
}

bool GameCommandFactory::TradeOverLand(const MapPoint pt, const boost::variant<GoodType, Job>& what, unsigned count)
{
    return AddGC(new gc::TradeOverLand(pt, what, count));
}
