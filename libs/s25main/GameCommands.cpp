// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GameCommands.h"
#include "GamePlayer.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobHarborBuilding.h"
#include "buildings/nobMilitary.h"
#include "buildings/nobShipYard.h"
#include "enum_cast.hpp"
#include "helpers/MaxEnumValue.h"
#include "helpers/format.hpp"
#include "world/GameWorld.h"
#include "nodeObjs/noFlag.h"
#include "nodeObjs/noShip.h"
#include <algorithm>
#include <stdexcept>

namespace gc {

void SetFlag::Execute(GameWorld& world, uint8_t playerId)
{
    world.SetFlag(pt_, playerId);
}

void DestroyFlag::Execute(GameWorld& world, uint8_t playerId)
{
    world.DestroyFlag(pt_, playerId);
}

BuildRoad::BuildRoad(Serializer& ser)
    : Coords(GCType::BuildRoad, ser), boat_road(ser.PopBool()), route(ser.PopUnsignedInt())
{
    for(Direction& i : route)
        i = helpers::popEnum<Direction>(ser);
}

void BuildRoad::Serialize(Serializer& ser) const
{
    Coords::Serialize(ser);

    ser.PushBool(boat_road);
    ser.PushUnsignedInt(route.size());
    for(auto i : route)
        helpers::pushEnum<uint8_t>(ser, i);
}

void BuildRoad::Execute(GameWorld& world, uint8_t playerId)
{
    world.BuildRoad(playerId, boat_road, pt_, route);
}

DestroyRoad::DestroyRoad(Serializer& ser)
    : Coords(GCType::DestroyRoad, ser), start_dir(helpers::popEnum<Direction>(ser))
{}

void DestroyRoad::Serialize(Serializer& ser) const
{
    Coords::Serialize(ser);

    helpers::pushEnum<uint8_t>(ser, start_dir);
}

void DestroyRoad::Execute(GameWorld& world, uint8_t playerId)
{
    auto* flag = world.GetSpecObj<noFlag>(pt_);
    if(flag && flag->GetPlayer() == playerId)
        flag->DestroyRoad(start_dir);
}

UpgradeRoad::UpgradeRoad(Serializer& ser)
    : Coords(GCType::UpgradeRoad, ser), start_dir(helpers::popEnum<Direction>(ser))
{}

void UpgradeRoad::Serialize(Serializer& ser) const
{
    Coords::Serialize(ser);
    helpers::pushEnum<uint8_t>(ser, start_dir);
}

void UpgradeRoad::Execute(GameWorld& world, uint8_t playerId)
{
    auto* flag = world.GetSpecObj<noFlag>(pt_);
    if(flag && flag->GetPlayer() == playerId)
        flag->UpgradeRoad(start_dir);
}

void ChangeDistribution::Execute(GameWorld& world, uint8_t playerId)
{
    world.GetPlayer(playerId).ChangeDistribution(data);
}

void ChangeBuildOrder::Execute(GameWorld& world, uint8_t playerId)
{
    world.GetPlayer(playerId).ChangeBuildOrder(useCustomBuildOrder, data);
}

void SetBuildingSite::Execute(GameWorld& world, uint8_t playerId)
{
    world.SetBuildingSite(bt, pt_, playerId);
}

void DestroyBuilding::Execute(GameWorld& world, uint8_t playerId)
{
    world.DestroyBuilding(pt_, playerId);
}

void ChangeTransport::Execute(GameWorld& world, uint8_t playerId)
{
    world.GetPlayer(playerId).ConvertTransportData(data);
}

void ChangeMilitary::Execute(GameWorld& world, uint8_t playerId)
{
    world.GetPlayer(playerId).ChangeMilitarySettings(data);
}

ChangeTools::ChangeTools(const ToolSettings& data, const int8_t* order_delta)
    : GameCommand(GCType::ChangeTools), data(data), orders()
{
    if(order_delta != nullptr)
        std::copy_n(order_delta, orders.size(), orders.begin());
}

void ChangeTools::Execute(GameWorld& world, uint8_t playerId)
{
    world.GetPlayer(playerId).ChangeToolsSettings(data, orders);
}

void CallSpecialist::Execute(GameWorld& world, uint8_t playerId)
{
    world.GetPlayer(playerId).CallFlagWorker(pt_, job);
}

void Attack::Execute(GameWorld& world, uint8_t playerId)
{
    world.Attack(playerId, pt_, soldiers_count, strong_soldiers);
}

void SeaAttack::Execute(GameWorld& world, uint8_t playerId)
{
    world.AttackViaSea(playerId, pt_, soldiers_count, strong_soldiers);
}

void SetCoinsAllowed::Execute(GameWorld& world, uint8_t playerId)
{
    auto* const bld = world.GetSpecObj<nobMilitary>(pt_);
    if(bld && bld->GetPlayer() == playerId)
        bld->SetCoinsAllowed(enabled);
}

void SetDesiredTroops::Execute(GameWorld& world, uint8_t playerId)
{
    auto* const bld = world.GetSpecObj<nobMilitary>(pt_);
    if(bld && bld->GetPlayer() == playerId)
        bld->SetDesiredTroops(rank, count);
}

void SetProductionEnabled::Execute(GameWorld& world, uint8_t playerId)
{
    auto* const bld = world.GetSpecObj<nobUsual>(pt_);
    if(bld && bld->GetPlayer() == playerId)
        bld->SetProductionEnabled(enabled);
}

void SetInventorySetting::Execute(GameWorld& world, uint8_t playerId)
{
    auto* const bld = world.GetSpecObj<nobBaseWarehouse>(pt_);
    if(bld && bld->GetPlayer() == playerId)
        bld->SetInventorySetting(what, state);
}

void SetAllInventorySettings::Execute(GameWorld& world, uint8_t playerId)
{
    auto* const bld = world.GetSpecObj<nobBaseWarehouse>(pt_);
    if(bld && bld->GetPlayer() == playerId)
        bld->SetAllInventorySettings(isJob, states);
}

void ChangeReserve::Execute(GameWorld& world, uint8_t playerId)
{
    auto* const bld = world.GetSpecObj<nobBaseWarehouse>(pt_);
    if(bld && bld->GetPlayer() == playerId)
        bld->SetRealReserve(rank, count);
}

void Surrender::Execute(GameWorld& world, uint8_t playerId)
{
    world.GetPlayer(playerId).Surrender();
}

void CheatArmageddon::Execute(GameWorld& world, unsigned char /*playerId*/)
{
    world.Armageddon();
}

void DestroyAll::Execute(GameWorld& world, uint8_t playerId)
{
    world.Armageddon(playerId);
}

void SuggestPact::Execute(GameWorld& world, uint8_t playerId)
{
    world.GetPlayer(playerId).SuggestPact(targetPlayer, pt, duration);
}

void AcceptPact::Execute(GameWorld& world, uint8_t playerId)
{
    world.GetPlayer(fromPlayer).AcceptPact(id, pt, playerId);
}

void CancelPact::Execute(GameWorld& world, uint8_t playerId)
{
    world.GetPlayer(playerId).CancelPact(pt, otherPlayer);
}

void NotifyAlliesOfLocation::Execute(GameWorld& world, uint8_t playerId)
{
    world.GetPlayer(playerId).NotifyAlliesOfLocation(pt_);
}

void SetShipYardMode::Execute(GameWorld& world, uint8_t playerId)
{
    auto* const bld = world.GetSpecObj<nobShipYard>(pt_);
    if(bld && bld->GetPlayer() == playerId)
        bld->SetMode(buildShips ? nobShipYard::Mode::Ships : nobShipYard::Mode::Boats);
}

void StartStopExpedition::Execute(GameWorld& world, uint8_t playerId)
{
    auto* const bld = world.GetSpecObj<nobHarborBuilding>(pt_);
    if(bld && bld->GetPlayer() == playerId)
    {
        if(start)
            bld->StartExpedition();
        else
            bld->StopExpedition();
    }
}

void StartStopExplorationExpedition::Execute(GameWorld& world, uint8_t playerId)
{
    auto* const bld = world.GetSpecObj<nobHarborBuilding>(pt_);
    if(bld && bld->GetPlayer() == playerId)
    {
        if(start)
            bld->StartExplorationExpedition();
        else
            bld->StopExplorationExpedition();
    }
}

void ExpeditionCommand::Execute(GameWorld& world, uint8_t playerId)
{
    noShip* ship = world.GetPlayer(playerId).GetShipByID(this->ship_id);
    if(!ship)
        return;

    switch(action)
    {
        case Action::FoundColony: ship->FoundColony(); break;
        case Action::CancelExpedition: ship->CancelExpedition(); break;
        case Action::North: ship->ContinueExpedition(ShipDirection::North); break;
        case Action::NorthEast: ship->ContinueExpedition(ShipDirection::NorthEast); break;
        case Action::SouthEast: ship->ContinueExpedition(ShipDirection::SouthEast); break;
        case Action::South: ship->ContinueExpedition(ShipDirection::South); break;
        case Action::SouthWest: ship->ContinueExpedition(ShipDirection::SouthWest); break;
        case Action::NorthWest: ship->ContinueExpedition(ShipDirection::NorthWest); break;
    }
}

/// Fuehrt das GameCommand aus
void TradeOverLand::Execute(GameWorld& world, uint8_t playerId)
{
    auto* const bld = world.GetSpecObj<nobBaseWarehouse>(pt_);
    if(bld)
        world.GetPlayer(playerId).Trade(bld, what, count);
}

} // namespace gc
