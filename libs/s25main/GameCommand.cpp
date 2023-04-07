// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GameCommand.h"
#include "GameCommands.h"
#include "helpers/toString.h"
#include <stdexcept>

namespace gc {

GameCommandPtr GameCommand::Deserialize(Serializer& ser)
{
    auto gcType = helpers::popEnum<GCType>(ser);
    GameCommand* gc;
    switch(gcType)
    {
        case GCType::SetFlag: gc = new SetFlag(ser); break;
        case GCType::DestroyFlag: gc = new DestroyFlag(ser); break;
        case GCType::BuildRoad: gc = new BuildRoad(ser); break;
        case GCType::DestroyRoad: gc = new DestroyRoad(ser); break;
        case GCType::ChangeDistribution: gc = new ChangeDistribution(ser); break;
        case GCType::ChangeBuildOrder: gc = new ChangeBuildOrder(ser); break;
        case GCType::SetBuildingsite: gc = new SetBuildingSite(ser); break;
        case GCType::DestroyBuilding: gc = new DestroyBuilding(ser); break;
        case GCType::ChangeTransport: gc = new ChangeTransport(ser); break;
        case GCType::ChangeMilitary: gc = new ChangeMilitary(ser); break;
        case GCType::ChangeTools: gc = new ChangeTools(ser); break;
        case GCType::CallSpecialist: gc = new CallSpecialist(ser); break;
        case GCType::Attack: gc = new Attack(ser); break;
        case GCType::SeaAttack: gc = new SeaAttack(ser); break;
        case GCType::SetCoinsAllowed: gc = new SetCoinsAllowed(ser); break;
        case GCType::SetProductionEnabled: gc = new SetProductionEnabled(ser); break;
        case GCType::SetInventorySetting: gc = new SetInventorySetting(ser); break;
        case GCType::SetAllInventorySettings: gc = new SetAllInventorySettings(ser); break;
        case GCType::ChangeReserve: gc = new ChangeReserve(ser); break;
        case GCType::SuggestPact: gc = new SuggestPact(ser); break;
        case GCType::AcceptPact: gc = new AcceptPact(ser); break;
        case GCType::CancelPact: gc = new CancelPact(ser); break;
        case GCType::SetShipyardMode: gc = new SetShipYardMode(ser); break;
        case GCType::StartStopExpedition: gc = new StartStopExpedition(ser); break;
        case GCType::StartStopExplorationExpedition: gc = new StartStopExplorationExpedition(ser); break;
        case GCType::ExpeditionCommand: gc = new ExpeditionCommand(ser); break;
        case GCType::Trade: gc = new TradeOverLand(ser); break;
        case GCType::Surrender: gc = new Surrender(ser); break;
        case GCType::CheatArmageddon: gc = new CheatArmageddon(ser); break;
        case GCType::DestroyAll: gc = new DestroyAll(ser); break;
        case GCType::UpgradeRoad: gc = new UpgradeRoad(ser); break;
        case GCType::OrderNewSoldiers: gc = new OrderNewSoldiers(ser); break;
        case GCType::SendSoldiersHome: gc = new SendSoldiersHome(ser); break;
        case GCType::SetDesiredTroops: gc = new SetDesiredTroops(ser); break;
        case GCType::NotifyAlliesOfLocation: gc = new NotifyAlliesOfLocation(ser); break;
        default: throw std::logic_error("Invalid GC Type: " + helpers::toString(rttr::enum_cast(gcType)));
    }
    RTTR_Assert(gc->gcType == gcType);
    return gc;
}

void GameCommand::Serialize(Serializer& ser) const
{
    helpers::pushEnum<uint8_t>(ser, gcType);
}

} // namespace gc
