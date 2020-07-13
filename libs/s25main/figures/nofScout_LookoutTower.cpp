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

#include "nofScout_LookoutTower.h"
#include "EventManager.h"
#include "buildings/nobUsual.h"
#include "postSystem/PostMsgWithBuilding.h"
#include "world/GameWorldGame.h"
#include "gameData/MilitaryConsts.h"
class SerializedGameData;
class nobBaseWarehouse;

nofScout_LookoutTower::nofScout_LookoutTower(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofBuildingWorker(JOB_SCOUT, pos, player, workplace)
{}

nofScout_LookoutTower::nofScout_LookoutTower(const MapPoint pos, const unsigned char player, nobBaseWarehouse* goalWh)
    : nofBuildingWorker(JOB_SCOUT, pos, player, goalWh)
{}

nofScout_LookoutTower::nofScout_LookoutTower(SerializedGameData& sgd, const unsigned obj_id) : nofBuildingWorker(sgd, obj_id) {}

void nofScout_LookoutTower::Serialize_nofScout_LookoutTower(SerializedGameData& sgd) const
{
    Serialize_nofBuildingWorker(sgd);
}

void nofScout_LookoutTower::WalkedDerived() {}

void nofScout_LookoutTower::DrawWorking(DrawPoint /*drawPt*/) {}

void nofScout_LookoutTower::HandleDerivedEvent(const unsigned /*id*/) {}

void nofScout_LookoutTower::WorkAborted()
{
    // Im enstprechenden Radius alles neu berechnen
    gwg->RecalcVisibilitiesAroundPoint(pos, VISUALRANGE_LOOKOUTTOWER, player, workplace);
}

void nofScout_LookoutTower::WorkplaceReached()
{
    // Im enstprechenden Radius alles sichtbar machen
    gwg->MakeVisibleAroundPoint(pos, VISUALRANGE_LOOKOUTTOWER, player);

    // Und Post versenden
    SendPostMessage(player, std::make_unique<PostMsgWithBuilding>(GetEvMgr().GetCurrentGF(), _("Lookout-tower occupied"),
                                                                  PostCategory::Military, *workplace));
}

bool nofScout_LookoutTower::AreWaresAvailable() const
{
    // We never work!
    return false;
}
