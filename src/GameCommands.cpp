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

///////////////////////////////////////////////////////////////////////////////
// Header
#include "defines.h"
#include "GameCommands.h"
#include "GameClient.h"
#include "GameClientPlayer.h"
#include "GameWorld.h"
#include "buildings/nobShipYard.h"
#include "buildings/nobMilitary.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobHarborBuilding.h"
#include "nodeObjs/noShip.h"

#include <cstdlib>

namespace gc{

    void SetFlag::Execute(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        gwg.SetFlag(pt, playerid);
    }

    void DestroyFlag::Execute(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        gwg.DestroyFlag(pt);
    }

    void BuildRoad::Execute(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        gwg.BuildRoad(playerid, boat_road, pt, route);
    }

    void DestroyRoad::Execute(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        gwg.DestroyRoad(pt, start_dir);
    }

    void UpgradeRoad::Execute(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        gwg.UpgradeRoad(pt, start_dir);
    }

    void ChangeDistribution::Execute(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        player.ChangeDistribution(data);
    }

    void ChangeBuildOrder::Execute(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        player.ChangeBuildOrder(order_type, data);
    }

    void SetBuildingSite::Execute(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        gwg.SetBuildingSite(bt, pt, playerid);
    }

    void DestroyBuilding::Execute(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        gwg.DestroyBuilding(pt, playerid);
    }

    void ChangeTransport::Execute(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        player.ConvertTransportData(data);
    }

    void ChangeMilitary::Execute(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        player.ChangeMilitarySettings(data);
    }

    void ChangeTools::Execute(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        player.ChangeToolsSettings(data);

        for (unsigned i = 0; i < TOOL_COUNT; ++i)
        {
            player.tools_ordered[i] = std::max(std::min( player.tools_ordered[i] + orders[i], 99 ), 0);
            player.tools_ordered_delta[i] -= orders[i];

            if (orders[i] != 0)
                std::cout << ">> Committing an order of " << (int)orders[i] << " for tool #" << i << std::endl;
        }
    }

    void CallGeologist::Execute(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        if(gwg.GetNO(pt)->GetGOT() == GOT_FLAG)
            player.CallFlagWorker(pt, JOB_GEOLOGIST);
    }

    void CallScout::Execute(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        if(gwg.GetNO(pt)->GetGOT() == GOT_FLAG)
            player.CallFlagWorker(pt, JOB_SCOUT);
    }

    void Attack::Execute(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        gwg.Attack(playerid, pt, soldiers_count, strong_soldiers);
    }

    void SeaAttack::Execute(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        gwg.AttackViaSea(playerid, pt, soldiers_count, strong_soldiers);
    }

    void SwitchPlayer::Execute(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        gwg.switchedPlayers.oldPlayer = playerid;
        gwg.switchedPlayers.newPlayer = new_player_id;
    }

    void ToggleCoins::Execute(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        if(gwg.GetNO(pt)->GetGOT() == GOT_NOB_MILITARY)
        {
            if(gwg.GetSpecObj<nobMilitary>(pt)->GetPlayer() == playerid)
                gwg.GetSpecObj<nobMilitary>(pt)->ToggleCoins();
        }
    }

    void SendSoldiersHome::Execute(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        if(gwg.GetNO(pt)->GetGOT() == GOT_NOB_MILITARY)
        {
            if(gwg.GetSpecObj<nobMilitary>(pt)->GetPlayer() == playerid)
                gwg.GetSpecObj<nobMilitary>(pt)->SendSoldiersHome();
        }
    }

    void OrderNewSoldiers::Execute(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        if(gwg.GetNO(pt)->GetGOT() == GOT_NOB_MILITARY)
        {
            if(gwg.GetSpecObj<nobMilitary>(pt)->GetPlayer() == playerid)
                gwg.GetSpecObj<nobMilitary>(pt)->OrderNewSoldiers();
        }
    }

    void ToggleProduction::Execute(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        if(gwg.GetNO(pt)->GetGOT() == GOT_NOB_USUAL || gwg.GetNO(pt)->GetGOT() == GOT_NOB_SHIPYARD)
            gwg.GetSpecObj<nobUsual>(pt)->ToggleProduction();
    }

    void ChangeInventorySetting::Execute(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        if(gwg.GetNO(pt)->GetType() == NOP_BUILDING)
            gwg.GetSpecObj<nobBaseWarehouse>(pt)
            ->ChangeRealInventorySetting(category, state, type);
    }

    void ChangeAllInventorySettings::Execute(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        if(gwg.GetNO(pt)->GetType() == NOP_BUILDING)
            gwg.GetSpecObj<nobBaseWarehouse>(pt)
            ->ChangeAllRealInventorySettings(category, state);
    }

    void ChangeReserve::Execute(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        if(gwg.GetNO(pt)->GetType() == NOP_BUILDING)
            gwg.GetSpecObj<nobBaseWarehouse>(pt)->SetRealReserve(rank, count);
    }

    void Surrender::Execute(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        player.Surrender();
    }

    void CheatArmageddon::Execute(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        gwg.Armageddon();
    }

    void DestroyAll::Execute(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        gwg.Armageddon(playerid);
    }

    void SuggestPact::Execute(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        player.SuggestPact(this->player, pt, duration);
    }

    void AcceptPact::Execute(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        ::GAMECLIENT.GetPlayer(this->player)->AcceptPact(id, pt, playerid);
    }

    void CancelPact::Execute(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        player.CancelPact(pt, this->player);
    }

    void NotifyAlliesOfLocation::Execute(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
	    player.NotifyAlliesOfLocation(pt, playerid);
    }

    void ToggleShipYardMode::Execute(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        if(gwg.GetNO(pt)->GetGOT() == GOT_NOB_SHIPYARD)
            gwg.GetSpecObj<nobShipYard>(pt)->ToggleMode();
    }

    void StartExpedition::Execute(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        if(gwg.GetNO(pt)->GetGOT() == GOT_NOB_HARBORBUILDING)
            gwg.GetSpecObj<nobHarborBuilding>(pt)->StartExpedition();
    }

    void StartExplorationExpedition::Execute(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        if(gwg.GetNO(pt)->GetGOT() == GOT_NOB_HARBORBUILDING)
            gwg.GetSpecObj<nobHarborBuilding>(pt)->StartExplorationExpedition();
    }

    void ExpeditionCommand::Execute(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        noShip* ship = player.GetShipByID(this->ship_id);
        if(!ship)
            return;

        if(this->action == FOUNDCOLONY)
            ship->FoundColony();
        else if(this->action == CANCELEXPEDITION)
            ship->CancelExpedition();
        else
            ship->ContinueExpedition(action - 2);
    }

    /// Fuehrt das GameCommand aus
    void TradeOverLand::Execute(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        noBase* nob = gwg.GetNO(pt);
        if(nob->GetGOT() == GOT_NOB_HARBORBUILDING || nob->GetGOT() == GOT_NOB_HQ || nob->GetGOT() == GOT_NOB_STOREHOUSE)
            player.Trade(static_cast<nobBaseWarehouse*>(nob), gt, job, count);
    }

}
 // namespace gc
