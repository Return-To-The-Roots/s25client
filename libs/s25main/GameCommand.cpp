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
    auto gcType = static_cast<Type>(ser.PopUnsignedChar());
    GameCommand* gc;
    switch(gcType)
    {
        case SET_FLAG: gc = new SetFlag(ser); break;
        case DESTROY_FLAG: gc = new DestroyFlag(ser); break;
        case BUILD_ROAD: gc = new BuildRoad(ser); break;
        case DESTROY_ROAD: gc = new DestroyRoad(ser); break;
        case CHANGE_DISTRIBUTION: gc = new ChangeDistribution(ser); break;
        case CHANGE_BUILDORDER: gc = new ChangeBuildOrder(ser); break;
        case SET_BUILDINGSITE: gc = new SetBuildingSite(ser); break;
        case DESTROY_BUILDING: gc = new DestroyBuilding(ser); break;
        case CHANGE_TRANSPORT: gc = new ChangeTransport(ser); break;
        case CHANGE_MILITARY: gc = new ChangeMilitary(ser); break;
        case CHANGE_TOOLS: gc = new ChangeTools(ser); break;
        case CALL_SPECIALIST: gc = new CallSpecialist(ser); break;
        case ATTACK: gc = new Attack(ser); break;
        case SEA_ATTACK: gc = new SeaAttack(ser); break;
        case SET_COINS_ALLOWED: gc = new SetCoinsAllowed(ser); break;
        case SET_PRODUCTION_ENABLED: gc = new SetProductionEnabled(ser); break;
        case SET_INVENTORY_SETTING: gc = new SetInventorySetting(ser); break;
        case SET_ALL_INVENTORY_SETTINGS: gc = new SetAllInventorySettings(ser); break;
        case CHANGE_RESERVE: gc = new ChangeReserve(ser); break;
        case SUGGEST_PACT: gc = new SuggestPact(ser); break;
        case ACCEPT_PACT: gc = new AcceptPact(ser); break;
        case CANCEL_PACT: gc = new CancelPact(ser); break;
        case SET_SHIPYARD_MODE: gc = new SetShipYardMode(ser); break;
        case START_STOP_EXPEDITION: gc = new StartStopExpedition(ser); break;
        case START_STOP_EXPLORATION_EXPEDITION: gc = new StartStopExplorationExpedition(ser); break;
        case EXPEDITION_COMMAND: gc = new ExpeditionCommand(ser); break;
        case TRADE: gc = new TradeOverLand(ser); break;
        case SURRENDER: gc = new Surrender(ser); break;
        case CHEAT_ARMAGEDDON: gc = new CheatArmageddon(ser); break;
        case DESTROY_ALL: gc = new DestroyAll(ser); break;
        case UPGRADE_ROAD: gc = new UpgradeRoad(ser); break;
        case ORDER_NEW_SOLDIERS: gc = new OrderNewSoldiers(ser); break;
        case SEND_SOLDIERS_HOME: gc = new SendSoldiersHome(ser); break;
        case NOTIFY_ALLIES_OF_LOCATION: gc = new NotifyAlliesOfLocation(ser); break;
        default: gc = nullptr; throw std::logic_error("Invalid GC Type: " + helpers::toString(gcType));
    }
    RTTR_Assert(gc->gcType == gcType);
    return gc;
}

void GameCommand::Serialize(Serializer& ser) const
{
    ser.PushUnsignedChar(static_cast<uint8_t>(gcType));
}

} // namespace gc
