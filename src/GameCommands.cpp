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

    void SetFlag::ExecuteImp(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        gwg.SetFlag(pt, playerid);
    }

    void DestroyFlag::ExecuteImp(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        gwg.DestroyFlag(pt);
    }

    void BuildRoad::ExecuteImp(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        gwg.BuildRoad(playerid, boat_road, pt, route);
    }

    void DestroyRoad::ExecuteImp(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        gwg.DestroyRoad(pt, start_dir);
    }

    void UpgradeRoad::ExecuteImp(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        gwg.UpgradeRoad(pt, start_dir);
    }

    void ChangeDistribution::ExecuteImp(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        player.ChangeDistribution(data);
    }

    void ChangeBuildOrder::ExecuteImp(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        player.ChangeBuildOrder(order_type, data);
    }

    void SetBuildingSite::ExecuteImp(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        gwg.SetBuildingSite(bt, pt, playerid);
    }

    void DestroyBuilding::ExecuteImp(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        gwg.DestroyBuilding(pt, playerid);
    }

    void ChangeTransport::ExecuteImp(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        player.ConvertTransportData(data);
    }

    void ChangeMilitary::ExecuteImp(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        player.ChangeMilitarySettings(data);
    }

    void ChangeTools::ExecuteImp(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
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

    void CallGeologist::ExecuteImp(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        if(gwg.GetNO(pt)->GetGOT() == GOT_FLAG)
            player.CallFlagWorker(pt, JOB_GEOLOGIST);
    }

    void CallScout::ExecuteImp(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        if(gwg.GetNO(pt)->GetGOT() == GOT_FLAG)
            player.CallFlagWorker(pt, JOB_SCOUT);
    }

    void Attack::ExecuteImp(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        gwg.Attack(playerid, pt, soldiers_count, strong_soldiers);
    }

    void SeaAttack::ExecuteImp(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        gwg.AttackViaSea(playerid, pt, soldiers_count, strong_soldiers);
    }

    void SwitchPlayer::ExecuteImp(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        gwg.switchedPlayers.oldPlayer = playerid;
        gwg.switchedPlayers.newPlayer = new_player_id;
    }

    void ToggleCoins::ExecuteImp(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        if(gwg.GetNO(pt)->GetGOT() == GOT_NOB_MILITARY)
        {
            if(gwg.GetSpecObj<nobMilitary>(pt)->GetPlayer() == playerid)
                gwg.GetSpecObj<nobMilitary>(pt)->ToggleCoins();
        }
    }

    void SendSoldiersHome::ExecuteImp(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        if(gwg.GetNO(pt)->GetGOT() == GOT_NOB_MILITARY)
        {
            if(gwg.GetSpecObj<nobMilitary>(pt)->GetPlayer() == playerid)
                gwg.GetSpecObj<nobMilitary>(pt)->SendSoldiersHome();
        }
    }

    void OrderNewSoldiers::ExecuteImp(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        if(gwg.GetNO(pt)->GetGOT() == GOT_NOB_MILITARY)
        {
            if(gwg.GetSpecObj<nobMilitary>(pt)->GetPlayer() == playerid)
                gwg.GetSpecObj<nobMilitary>(pt)->OrderNewSoldiers();
        }
    }

    void ToggleProduction::ExecuteImp(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        if(gwg.GetNO(pt)->GetGOT() == GOT_NOB_USUAL || gwg.GetNO(pt)->GetGOT() == GOT_NOB_SHIPYARD)
            gwg.GetSpecObj<nobUsual>(pt)->ToggleProduction();
    }

    void ChangeInventorySetting::ExecuteImp(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        if(gwg.GetNO(pt)->GetType() == NOP_BUILDING)
            gwg.GetSpecObj<nobBaseWarehouse>(pt)
            ->ChangeRealInventorySetting(category, state, type);
    }

    void ChangeAllInventorySettings::ExecuteImp(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        if(gwg.GetNO(pt)->GetType() == NOP_BUILDING)
            gwg.GetSpecObj<nobBaseWarehouse>(pt)
            ->ChangeAllRealInventorySettings(category, state);
    }

    void ChangeReserve::ExecuteImp(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        if(gwg.GetNO(pt)->GetType() == NOP_BUILDING)
            gwg.GetSpecObj<nobBaseWarehouse>(pt)->SetRealReserve(rank, count);
    }

    void Surrender::ExecuteImp(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        player.Surrender();
    }

    void CheatArmageddon::ExecuteImp(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        gwg.Armageddon();
    }

    void DestroyAll::ExecuteImp(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        gwg.Armageddon(playerid);
    }

    void SuggestPact::ExecuteImp(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        player.SuggestPact(this->player, pt, duration);
    }

    void AcceptPact::ExecuteImp(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        ::GAMECLIENT.GetPlayer(this->player)->AcceptPact(id, pt, playerid);
    }

    void CancelPact::ExecuteImp(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        player.CancelPact(pt, this->player);
    }

    void NotifyAlliesOfLocation::ExecuteImp(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
	    player.NotifyAlliesOfLocation(pt, playerid);
    }

    void ToggleShipYardMode::ExecuteImp(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        if(gwg.GetNO(pt)->GetGOT() == GOT_NOB_SHIPYARD)
            gwg.GetSpecObj<nobShipYard>(pt)->ToggleMode();
    }

    void StartExpedition::ExecuteImp(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        if(gwg.GetNO(pt)->GetGOT() == GOT_NOB_HARBORBUILDING)
            gwg.GetSpecObj<nobHarborBuilding>(pt)->StartExpedition();
    }

    void StartExplorationExpedition::ExecuteImp(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        if(gwg.GetNO(pt)->GetGOT() == GOT_NOB_HARBORBUILDING)
            gwg.GetSpecObj<nobHarborBuilding>(pt)->StartExplorationExpedition();
    }

    void ExpeditionCommand::ExecuteImp(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
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
    void TradeOverLand::ExecuteImp(GameWorldGame& gwg, GameClientPlayer& player, const unsigned char playerid)
    {
        noBase* nob = gwg.GetNO(pt);
        if(nob->GetGOT() == GOT_NOB_HARBORBUILDING || nob->GetGOT() == GOT_NOB_HQ || nob->GetGOT() == GOT_NOB_STOREHOUSE)
            player.Trade(static_cast<nobBaseWarehouse*>(nob), gt, job, count);
    }

}
 // namespace gc
