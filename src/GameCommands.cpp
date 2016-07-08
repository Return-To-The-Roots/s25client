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

#include "defines.h" // IWYU pragma: keep
#include "GameCommands.h"
#include "GamePlayer.h"
#include "buildings/nobShipYard.h"
#include "buildings/nobMilitary.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobHarborBuilding.h"
#include "nodeObjs/noShip.h"
#include "nodeObjs/noFlag.h"
#include "world/GameWorldGame.h"
#include <iostream>

namespace gc{

    void SetFlag::Execute(GameWorldGame& gwg, unsigned char playerId)
    {
        gwg.SetFlag(pt_, playerId);
    }

    void DestroyFlag::Execute(GameWorldGame& gwg, unsigned char playerId)
    {
        gwg.DestroyFlag(pt_, playerId);
    }

    void BuildRoad::Execute(GameWorldGame& gwg, unsigned char playerId)
    {
        gwg.BuildRoad(playerId, boat_road, pt_, route);
    }

    void DestroyRoad::Execute(GameWorldGame& gwg, unsigned char playerId)
    {
        noFlag* flag = gwg.GetSpecObj<noFlag>(pt_);
        if(flag && flag->GetPlayer() == playerId)
            flag->DestroyRoad(start_dir);
    }

    void UpgradeRoad::Execute(GameWorldGame& gwg, unsigned char playerId)
    {
        noFlag* flag = gwg.GetSpecObj<noFlag>(pt_);
        if(flag && flag->GetPlayer() == playerId)
            flag->UpgradeRoad(start_dir);
    }

    void ChangeDistribution::Execute(GameWorldGame& gwg, unsigned char playerId)
    {
        gwg.GetPlayer(playerId).ChangeDistribution(data);
    }

    void ChangeBuildOrder::Execute(GameWorldGame& gwg, unsigned char playerId)
    {
        gwg.GetPlayer(playerId).ChangeBuildOrder(useCustomBuildOrder, data);
    }

    void SetBuildingSite::Execute(GameWorldGame& gwg, unsigned char playerId)
    {
        gwg.SetBuildingSite(bt, pt_, playerId);
    }

    void DestroyBuilding::Execute(GameWorldGame& gwg, unsigned char playerId)
    {
        gwg.DestroyBuilding(pt_, playerId);
    }

    void ChangeTransport::Execute(GameWorldGame& gwg, unsigned char playerId)
    {
        gwg.GetPlayer(playerId).ConvertTransportData(data);
    }

    void ChangeMilitary::Execute(GameWorldGame& gwg, unsigned char playerId)
    {
        gwg.GetPlayer(playerId).ChangeMilitarySettings(data);
    }

    void ChangeTools::Execute(GameWorldGame& gwg, unsigned char playerId)
    {
        gwg.GetPlayer(playerId).ChangeToolsSettings(data, orders);
    }

    void CallGeologist::Execute(GameWorldGame& gwg, unsigned char playerId)
    {
        gwg.GetPlayer(playerId).CallFlagWorker(pt_, JOB_GEOLOGIST);
    }

    void CallScout::Execute(GameWorldGame& gwg, unsigned char playerId)
    {
        gwg.GetPlayer(playerId).CallFlagWorker(pt_, JOB_SCOUT);
    }

    void Attack::Execute(GameWorldGame& gwg, unsigned char playerId)
    {
        gwg.Attack(playerId, pt_, soldiers_count, strong_soldiers);
    }

    void SeaAttack::Execute(GameWorldGame& gwg, unsigned char playerId)
    {
        gwg.AttackViaSea(playerId, pt_, soldiers_count, strong_soldiers);
    }

    void SetCoinsAllowed::Execute(GameWorldGame& gwg, unsigned char playerId)
    {
        nobMilitary* const bld = gwg.GetSpecObj<nobMilitary>(pt_);
        if(bld && bld->GetPlayer() == playerId)
            bld->SetCoinsAllowed(enabled);
    }

    void SendSoldiersHome::Execute(GameWorldGame& gwg, unsigned char playerId)
    {
        nobMilitary* const bld = gwg.GetSpecObj<nobMilitary>(pt_);
        if(bld && bld->GetPlayer() == playerId)
            bld->SendSoldiersHome();
    }

    void OrderNewSoldiers::Execute(GameWorldGame& gwg, unsigned char playerId)
    {
        nobMilitary* const bld = gwg.GetSpecObj<nobMilitary>(pt_);
        if(bld && bld->GetPlayer() == playerId)
            bld->OrderNewSoldiers();
    }

    void SetProductionEnabled::Execute(GameWorldGame& gwg, unsigned char playerId)
    {
        nobUsual* const bld = gwg.GetSpecObj<nobUsual>(pt_);
        if(bld && bld->GetPlayer() == playerId)
            bld->SetProductionEnabled(enabled);
    }

    void SetInventorySetting::Execute(GameWorldGame& gwg, unsigned char playerId)
    {
        nobBaseWarehouse* const bld = gwg.GetSpecObj<nobBaseWarehouse>(pt_);
        if(bld && bld->GetPlayer() == playerId)
            bld->SetInventorySetting(isJob, type, state);
    }

    void SetAllInventorySettings::Execute(GameWorldGame& gwg, unsigned char playerId)
    {
        nobBaseWarehouse* const bld = gwg.GetSpecObj<nobBaseWarehouse>(pt_);
        if(bld && bld->GetPlayer() == playerId)
            bld->SetAllInventorySettings(isJob, states);
    }

    void ChangeReserve::Execute(GameWorldGame& gwg, unsigned char playerId)
    {
        nobBaseWarehouse* const bld = gwg.GetSpecObj<nobBaseWarehouse>(pt_);
        if(bld && bld->GetPlayer() == playerId)
            bld->SetRealReserve(rank, count);
    }

    void Surrender::Execute(GameWorldGame& gwg, unsigned char playerId)
    {
        gwg.GetPlayer(playerId).Surrender();
    }

    void CheatArmageddon::Execute(GameWorldGame& gwg, unsigned char  /*playerId*/)
    {
        gwg.Armageddon();
    }

    void DestroyAll::Execute(GameWorldGame& gwg, unsigned char playerId)
    {
        gwg.Armageddon(playerId);
    }

    void SuggestPact::Execute(GameWorldGame& gwg, unsigned char playerId)
    {
        gwg.GetPlayer(playerId).SuggestPact(targetPlayer, pt, duration);
    }

    void AcceptPact::Execute(GameWorldGame& gwg, unsigned char playerId)
    {
        gwg.GetPlayer(fromPlayer).AcceptPact(id, pt, playerId);
    }

    void CancelPact::Execute(GameWorldGame& gwg, unsigned char playerId)
    {
        gwg.GetPlayer(playerId).CancelPact(pt, player);
    }

    void NotifyAlliesOfLocation::Execute(GameWorldGame& gwg, unsigned char playerId)
    {
        gwg.GetPlayer(playerId).NotifyAlliesOfLocation(pt_);
    }

    void ToggleShipYardMode::Execute(GameWorldGame& gwg, unsigned char playerId)
    {
        nobShipYard* const bld = gwg.GetSpecObj<nobShipYard>(pt_);
        if(bld && bld->GetPlayer() == playerId)
            bld->ToggleMode();
    }

    void StartExpedition::Execute(GameWorldGame& gwg, unsigned char playerId)
    {
        nobHarborBuilding* const bld = gwg.GetSpecObj<nobHarborBuilding>(pt_);
        if(bld && bld->GetPlayer() == playerId)
            bld->StartExpedition();
    }

    void StartExplorationExpedition::Execute(GameWorldGame& gwg, unsigned char playerId)
    {
        nobHarborBuilding* const bld = gwg.GetSpecObj<nobHarborBuilding>(pt_);
        if(bld && bld->GetPlayer() == playerId)
            bld->StartExplorationExpedition();
    }

    void ExpeditionCommand::Execute(GameWorldGame& gwg, unsigned char playerId)
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
    void TradeOverLand::Execute(GameWorldGame& gwg, unsigned char playerId)
    {
        nobBaseWarehouse* const bld = gwg.GetSpecObj<nobBaseWarehouse>(pt_);
        if(bld)
            gwg.GetPlayer(playerId).Trade(bld, gt, job, count);
    }

}
 // namespace gc
