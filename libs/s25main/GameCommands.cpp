// Copyright (C) 2005 - 2025 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GameCommands.h"
#include "AddonHelperFunctions.h"
#include "GamePlayer.h"
#include "LeatherLoader.h"
#include "WineLoader.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobHarborBuilding.h"
#include "buildings/nobMilitary.h"
#include "buildings/nobShipYard.h"
#include "buildings/nobTemple.h"
#include "enum_cast.hpp"
#include "helpers/MaxEnumValue.h"
#include "helpers/format.hpp"
#include "world/GameWorld.h"
#include "nodeObjs/noFlag.h"
#include "nodeObjs/noShip.h"
#include "gameData/SettingTypeConv.h"
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

ChangeDistribution::ChangeDistribution(Deserializer& ser) : GameCommand(GCType::ChangeDistribution)
{
    if(ser.getDataVersion() >= 2)
        helpers::popContainer(ser, data);
    else
    {
        const unsigned wineAddonAdditionalDistributions = 3;
        const unsigned leatherAddonAdditionalDistributions = 3;

        auto const numNotSavedDistributions =
          leatherAddonAdditionalDistributions + (ser.getDataVersion() < 1 ? wineAddonAdditionalDistributions : 0);

        std::vector<Distributions::value_type> tmpData(std::tuple_size_v<Distributions> - numNotSavedDistributions);

        auto getSkipBuildingAndDefault = [&](DistributionMapping const& mapping) {
            // Skipped and standard distribution in skipped case
            std::tuple<bool, unsigned int> result = {false, 0};
            if(ser.getDataVersion() < 1)
            {
                // skip over wine buildings
                std::get<0>(result) |= wineaddon::isWineAddonBuildingType(std::get<BuildingType>(mapping));
            }

            // skip over leather addon buildings and leather addon wares only
            std::get<0>(result) |= leatheraddon::isLeatherAddonBuildingType(std::get<BuildingType>(mapping));

            if(std::get<BuildingType>(mapping) == BuildingType::Slaughterhouse
               && (std::get<GoodType>(mapping) == GoodType::Ham))
                result = {true, 8};
            return result;
        };

        helpers::popContainer(ser, tmpData, true);
        size_t srcIdx = 0, tgtIdx = 0;
        for(const auto& mapping : distributionMap)
        {
            // skip over not stored buildings in tmpData
            const auto [skipped, defaultValue] = getSkipBuildingAndDefault(mapping);
            const auto setting = skipped ? defaultValue : tmpData[srcIdx++];
            data[tgtIdx++] = setting;
        }
    }
}

void ChangeDistribution::Execute(GameWorld& world, uint8_t playerId)
{
    world.GetPlayer(playerId).ChangeDistribution(data);
}

ChangeBuildOrder::ChangeBuildOrder(Deserializer& ser)
    : GameCommand(GCType::ChangeBuildOrder), useCustomBuildOrder(ser.PopBool())
{
    if(ser.getDataVersion() >= 2)
    {
        for(BuildingType& i : data)
            i = helpers::popEnum<BuildingType>(ser);
    } else
    {
        auto countOfNotAvailableBuildingsInSaveGame =
          ser.getDataVersion() < 1 ? numWineAndLeatherAddonBuildings : numLeatherAddonBuildings;
        std::vector<BuildingType> buildOrder(data.size() - countOfNotAvailableBuildingsInSaveGame);

        if(ser.getDataVersion() < 1)
            buildOrder.insert(buildOrder.end(), {BuildingType::Vineyard, BuildingType::Winery, BuildingType::Temple});

        if(ser.getDataVersion() < 2)
            buildOrder.insert(buildOrder.end(),
                              {BuildingType::Skinner, BuildingType::Tannery, BuildingType::LeatherWorks});

        std::generate(buildOrder.begin(), buildOrder.end() - countOfNotAvailableBuildingsInSaveGame,
                      [&]() { return helpers::popEnum<BuildingType>(ser); });

        std::copy(buildOrder.begin(), buildOrder.end(), data.begin());
    }
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

ChangeTransport::ChangeTransport(Deserializer& ser) : GameCommand(GCType::ChangeTransport)
{
    if(ser.getDataVersion() >= 2)
        helpers::popContainer(ser, data);
    else
    {
        const unsigned leatherAddonAdditionalTransportOrders = 1;
        std::vector<TransportOrders::value_type> tmpData(std::tuple_size<TransportOrders>::value
                                                         - leatherAddonAdditionalTransportOrders);

        helpers::popContainer(ser, tmpData, true);
        std::copy(tmpData.begin(), tmpData.end(), data.begin());
        // all transport prios greater equal transportPrioOfLeatherworks are increased by one because the new
        // leatherwork uses prio transportPrioOfLeatherworks
        std::transform(data.begin(), data.end() - leatherAddonAdditionalTransportOrders, data.begin(),
                       [](uint8_t& prio) { return prio < transportPrioOfLeatherworks ? prio : prio + 1; });
        data[std::tuple_size<TransportOrders>::value - leatherAddonAdditionalTransportOrders] =
          STD_TRANSPORT_PRIO[GoodType::Leather];
    }
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

void SetArmorAllowed::Execute(GameWorld& world, uint8_t playerId)
{
    auto* const bld = world.GetSpecObj<nobMilitary>(pt_);
    if(bld && bld->GetPlayer() == playerId)
        bld->SetArmorAllowed(enabled);
}

void SetTroopLimit::Execute(GameWorld& world, uint8_t playerId)
{
    auto* const bld = world.GetSpecObj<nobMilitary>(pt_);
    if(bld && bld->GetPlayer() == playerId)
        bld->SetTroopLimit(rank, count);
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

SetAllInventorySettings::SetAllInventorySettings(Deserializer& ser)
    : Coords(GCType::SetAllInventorySettings, ser), isJob(ser.PopBool())
{
    const uint32_t numStates = (isJob ? helpers::NumEnumValues_v<Job> : helpers::NumEnumValues_v<GoodType>);
    if(ser.getDataVersion() >= 2)
    {
        for(unsigned i = 0; i < numStates; i++)
            states.push_back(InventorySetting(ser.PopUnsignedChar()));
    } else
    {
        states.resize(numStates);
        if(isJob)
        {
            auto isJobSkipped = [&](Job const& job) {
                return (ser.getDataVersion() < 1 && wineaddon::isWineAddonJobType(job))
                       || leatheraddon::isLeatherAddonJobType(job);
            };

            size_t tgtIdx = 0;
            for(const auto i : helpers::enumRange<Job>())
            {
                // skip over not stored jobs
                if(!isJobSkipped(i))
                    states[tgtIdx] = InventorySetting(ser.PopUnsignedChar());
                tgtIdx++;
            }
        } else
        {
            auto isWareSkipped = [&](GoodType const& ware) {
                return (ser.getDataVersion() < 1 && wineaddon::isWineAddonGoodType(ware))
                       || leatheraddon::isLeatherAddonGoodType(ware);
            };

            size_t tgtIdx = 0;
            for(const auto i : helpers::enumRange<GoodType>())
            {
                // skip over not stored wares
                if(!isWareSkipped(i))
                    states[tgtIdx] = InventorySetting(ser.PopUnsignedChar());
                tgtIdx++;
            }
        }
    }
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

void SetTempleProductionMode::Execute(GameWorld& world, uint8_t playerId)
{
    auto* const bld = world.GetSpecObj<nobTemple>(pt_);
    if(bld && bld->GetPlayer() == playerId)
        bld->SetProductionMode(productionMode);
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
