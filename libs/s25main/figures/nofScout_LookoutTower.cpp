// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "nofScout_LookoutTower.h"
#include "EventManager.h"
#include "buildings/nobUsual.h"
#include "postSystem/PostMsgWithBuilding.h"
#include "world/GameWorld.h"
#include "gameData/MilitaryConsts.h"
class SerializedGameData;
class nobBaseWarehouse;

nofScout_LookoutTower::nofScout_LookoutTower(const MapPoint pos, const unsigned char player, nobUsual* workplace)
    : nofBuildingWorker(Job::Scout, pos, player, workplace)
{}

nofScout_LookoutTower::nofScout_LookoutTower(const MapPoint pos, const unsigned char player, nobBaseWarehouse* goalWh)
    : nofBuildingWorker(Job::Scout, pos, player, goalWh)
{}

nofScout_LookoutTower::nofScout_LookoutTower(SerializedGameData& sgd, const unsigned obj_id)
    : nofBuildingWorker(sgd, obj_id)
{}

void nofScout_LookoutTower::WalkedDerived() {}

void nofScout_LookoutTower::DrawWorking(DrawPoint /*drawPt*/) {}

void nofScout_LookoutTower::HandleDerivedEvent(const unsigned /*id*/) {}

void nofScout_LookoutTower::WorkAborted()
{
    // Im enstprechenden Radius alles neu berechnen
    world->RecalcVisibilitiesAroundPoint(pos, VISUALRANGE_LOOKOUTTOWER, player, workplace);
}

void nofScout_LookoutTower::WorkplaceReached()
{
    // Im enstprechenden Radius alles sichtbar machen
    world->MakeVisibleAroundPoint(pos, VISUALRANGE_LOOKOUTTOWER, player);

    // Und Post versenden
    SendPostMessage(player,
                    std::make_unique<PostMsgWithBuilding>(GetEvMgr().GetCurrentGF(), _("Lookout-tower occupied"),
                                                          PostCategory::Military, *workplace));
}

bool nofScout_LookoutTower::AreWaresAvailable() const
{
    // We never work!
    return false;
}
