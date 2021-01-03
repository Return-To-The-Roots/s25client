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
