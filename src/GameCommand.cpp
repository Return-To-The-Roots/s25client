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

#include "defines.h"
#include "GameCommand.h"
#include "GameCommands.h"
#include "helpers/converters.h"

#include <stdexcept>

using namespace gc;

GameCommand* GameCommand::Deserialize(const Type gst, Serializer& ser)
{
    switch(gst)
    {
    case SETFLAG: return new SetFlag(ser);
    case DESTROYFLAG: return new DestroyFlag(ser);
    case BUILDROAD: return new BuildRoad(ser);
    case DESTROYROAD: return new DestroyRoad(ser);
    case CHANGEDISTRIBUTION: return new ChangeDistribution(ser);
    case CHANGEBUILDORDER: return new ChangeBuildOrder(ser);
    case SETBUILDINGSITE: return new SetBuildingSite(ser);
    case DESTROYBUILDING: return new DestroyBuilding(ser);
    case CHANGETRANSPORT: return new ChangeTransport(ser);
    case CHANGEMILITARY: return new ChangeMilitary(ser);
    case CHANGETOOLS: return new ChangeTools(ser);
    case CALLGEOLOGIST: return new CallGeologist(ser);
    case CALLSCOUT: return new CallScout(ser);
    case ATTACK: return new Attack(ser);
    case SEAATTACK: return new SeaAttack(ser);
    case TOGGLECOINS: return new ToggleCoins(ser);
    case TOGGLEPRODUCTION: return new ToggleProduction(ser);
    case SET_INVENTORY_SETTING: return new SetInventorySetting(ser);
    case SET_ALL_INVENTORY_SETTINGS: return new SetAllInventorySettings(ser);
    case CHANGERESERVE: return new ChangeReserve(ser);
    case SUGGESTPACT: return new SuggestPact(ser);
    case ACCEPTPACT: return new AcceptPact(ser);
    case CANCELPACT: return new CancelPact(ser);
    case TOGGLESHIPYARDMODE: return new ToggleShipYardMode(ser);
    case STARTEXPEDITION: return new StartExpedition(ser);
    case STARTEXPLORATIONEXPEDITION: return new StartExplorationExpedition(ser);
    case EXPEDITION_COMMAND: return new ExpeditionCommand(ser);
    case TRADEOVERLAND: return new TradeOverLand(ser);
    case SURRENDER: return new Surrender(ser);
    case CHEAT_ARMAGEDDON: return new CheatArmageddon(ser);
    case DESTROYALL: return new DestroyAll(ser);
    case UPGRADEROAD: return new UpgradeRoad(ser);
    case ORDERNEWSOLDIERS: return new OrderNewSoldiers(ser);
    case SENDSOLDIERSHOME: return new SendSoldiersHome(ser);
    case NOTIFYALLIESOFLOCATION: return new NotifyAlliesOfLocation(ser);
    default: break;
    }

    throw std::logic_error("Invalid GC Type: " + helpers::toString(gst));
}
