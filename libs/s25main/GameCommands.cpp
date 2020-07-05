// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "GameCommands.h"
#include "GamePlayer.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobHarborBuilding.h"
#include "buildings/nobMilitary.h"
#include "buildings/nobShipYard.h"
#include "world/GameWorldGame.h"
#include "nodeObjs/noFlag.h"
#include "nodeObjs/noShip.h"

namespace gc {

void SetFlag::Execute(GameWorldGame& gwg, uint8_t playerId)
{
    gwg.SetFlag(pt_, playerId);
}

void DestroyFlag::Execute(GameWorldGame& gwg, uint8_t playerId)
{
    gwg.DestroyFlag(pt_, playerId);
}

void BuildRoad::Execute(GameWorldGame& gwg, uint8_t playerId)
{
    gwg.BuildRoad(playerId, boat_road, pt_, route);
}

void DestroyRoad::Execute(GameWorldGame& gwg, uint8_t playerId)
{
    auto* flag = gwg.GetSpecObj<noFlag>(pt_);
    if(flag && flag->GetPlayer() == playerId)
        flag->DestroyRoad(start_dir);
}

void UpgradeRoad::Execute(GameWorldGame& gwg, uint8_t playerId)
{
    auto* flag = gwg.GetSpecObj<noFlag>(pt_);
    if(flag && flag->GetPlayer() == playerId)
        flag->UpgradeRoad(start_dir);
}

void ChangeDistribution::Execute(GameWorldGame& gwg, uint8_t playerId)
{
    gwg.GetPlayer(playerId).ChangeDistribution(data);
}

void ChangeBuildOrder::Execute(GameWorldGame& gwg, uint8_t playerId)
{
    gwg.GetPlayer(playerId).ChangeBuildOrder(useCustomBuildOrder, data);
}

void SetBuildingSite::Execute(GameWorldGame& gwg, uint8_t playerId)
{
    gwg.SetBuildingSite(bt, pt_, playerId);
}

void DestroyBuilding::Execute(GameWorldGame& gwg, uint8_t playerId)
{
    gwg.DestroyBuilding(pt_, playerId);
}

void ChangeTransport::Execute(GameWorldGame& gwg, uint8_t playerId)
{
    gwg.GetPlayer(playerId).ConvertTransportData(data);
}

void ChangeMilitary::Execute(GameWorldGame& gwg, uint8_t playerId)
{
    gwg.GetPlayer(playerId).ChangeMilitarySettings(data);
}

void ChangeTools::Execute(GameWorldGame& gwg, uint8_t playerId)
{
    gwg.GetPlayer(playerId).ChangeToolsSettings(data, orders);
}

void CallSpecialist::Execute(GameWorldGame& gwg, uint8_t playerId)
{
    gwg.GetPlayer(playerId).CallFlagWorker(pt_, job);
}

void Attack::Execute(GameWorldGame& gwg, uint8_t playerId)
{
    gwg.Attack(playerId, pt_, soldiers_count, strong_soldiers);
}

void SeaAttack::Execute(GameWorldGame& gwg, uint8_t playerId)
{
    gwg.AttackViaSea(playerId, pt_, soldiers_count, strong_soldiers);
}

void SetCoinsAllowed::Execute(GameWorldGame& gwg, uint8_t playerId)
{
    auto* const bld = gwg.GetSpecObj<nobMilitary>(pt_);
    if(bld && bld->GetPlayer() == playerId)
        bld->SetCoinsAllowed(enabled);
}

void SendSoldiersHome::Execute(GameWorldGame& gwg, uint8_t playerId)
{
    auto* const bld = gwg.GetSpecObj<nobMilitary>(pt_);
    if(bld && bld->GetPlayer() == playerId)
        bld->SendSoldiersHome();
}

void OrderNewSoldiers::Execute(GameWorldGame& gwg, uint8_t playerId)
{
    auto* const bld = gwg.GetSpecObj<nobMilitary>(pt_);
    if(bld && bld->GetPlayer() == playerId)
        bld->OrderNewSoldiers();
}

void SetProductionEnabled::Execute(GameWorldGame& gwg, uint8_t playerId)
{
    auto* const bld = gwg.GetSpecObj<nobUsual>(pt_);
    if(bld && bld->GetPlayer() == playerId)
        bld->SetProductionEnabled(enabled);
}

void SetInventorySetting::Execute(GameWorldGame& gwg, uint8_t playerId)
{
    auto* const bld = gwg.GetSpecObj<nobBaseWarehouse>(pt_);
    if(bld && bld->GetPlayer() == playerId)
        bld->SetInventorySetting(isJob, type, state);
}

void SetAllInventorySettings::Execute(GameWorldGame& gwg, uint8_t playerId)
{
    auto* const bld = gwg.GetSpecObj<nobBaseWarehouse>(pt_);
    if(bld && bld->GetPlayer() == playerId)
        bld->SetAllInventorySettings(isJob, states);
}

void ChangeReserve::Execute(GameWorldGame& gwg, uint8_t playerId)
{
    auto* const bld = gwg.GetSpecObj<nobBaseWarehouse>(pt_);
    if(bld && bld->GetPlayer() == playerId)
        bld->SetRealReserve(rank, count);
}

void Surrender::Execute(GameWorldGame& gwg, uint8_t playerId)
{
    gwg.GetPlayer(playerId).Surrender();
}

void CheatArmageddon::Execute(GameWorldGame& gwg, unsigned char /*playerId*/)
{
    gwg.Armageddon();
}

void DestroyAll::Execute(GameWorldGame& gwg, uint8_t playerId)
{
    gwg.Armageddon(playerId);
}

void SuggestPact::Execute(GameWorldGame& gwg, uint8_t playerId)
{
    gwg.GetPlayer(playerId).SuggestPact(targetPlayer, pt, duration);
}

void AcceptPact::Execute(GameWorldGame& gwg, uint8_t playerId)
{
    gwg.GetPlayer(fromPlayer).AcceptPact(id, pt, playerId);
}

void CancelPact::Execute(GameWorldGame& gwg, uint8_t playerId)
{
    gwg.GetPlayer(playerId).CancelPact(pt, otherPlayer);
}

void NotifyAlliesOfLocation::Execute(GameWorldGame& gwg, uint8_t playerId)
{
    gwg.GetPlayer(playerId).NotifyAlliesOfLocation(pt_);
}

void SetShipYardMode::Execute(GameWorldGame& gwg, uint8_t playerId)
{
    auto* const bld = gwg.GetSpecObj<nobShipYard>(pt_);
    if(bld && bld->GetPlayer() == playerId)
        bld->SetMode(buildShips ? nobShipYard::SHIPS : nobShipYard::BOATS);
}

void StartStopExpedition::Execute(GameWorldGame& gwg, uint8_t playerId)
{
    auto* const bld = gwg.GetSpecObj<nobHarborBuilding>(pt_);
    if(bld && bld->GetPlayer() == playerId)
    {
        if(start)
            bld->StartExpedition();
        else
            bld->StopExpedition();
    }
}

void StartStopExplorationExpedition::Execute(GameWorldGame& gwg, uint8_t playerId)
{
    auto* const bld = gwg.GetSpecObj<nobHarborBuilding>(pt_);
    if(bld && bld->GetPlayer() == playerId)
    {
        if(start)
            bld->StartExplorationExpedition();
        else
            bld->StopExplorationExpedition();
    }
}

void ExpeditionCommand::Execute(GameWorldGame& gwg, uint8_t playerId)
{
    noShip* ship = gwg.GetPlayer(playerId).GetShipByID(this->ship_id);
    if(!ship)
        return;

    if(action == FOUNDCOLONY)
        ship->FoundColony();
    else if(action == CANCELEXPEDITION)
        ship->CancelExpedition();
    else
        ship->ContinueExpedition(ShipDirection(action - 2));
}

/// Fuehrt das GameCommand aus
void TradeOverLand::Execute(GameWorldGame& gwg, uint8_t playerId)
{
    auto* const bld = gwg.GetSpecObj<nobBaseWarehouse>(pt_);
    if(bld)
        gwg.GetPlayer(playerId).Trade(bld, what, count);
}

} // namespace gc
  // namespace gc
