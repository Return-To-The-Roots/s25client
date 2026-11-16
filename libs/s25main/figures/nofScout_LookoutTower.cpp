// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
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

nofScout_LookoutTower::nofScout_LookoutTower(SerializedGameData& sgd, const unsigned obj_id)
    : nofBuildingWorker(sgd, obj_id)
{}

void nofScout_LookoutTower::walkedDerived() {}

void nofScout_LookoutTower::drawWorking(DrawPoint /*drawPt*/) {}

void nofScout_LookoutTower::handleDerivedEvent(const unsigned /*id*/) {}

void nofScout_LookoutTower::workAborted()
{
    // Im enstprechenden Radius alles neu berechnen
    world->recalcVisibilitiesAroundPoint(pos, VISUALRANGE_LOOKOUTTOWER, player, workplace);
}

void nofScout_LookoutTower::workplaceReached()
{
    // Im enstprechenden Radius alles sichtbar machen
    world->makeVisibleAroundPoint(pos, VISUALRANGE_LOOKOUTTOWER, player);

    // Und Post versenden
    sendPostMessage(player,
                    std::make_unique<PostMsgWithBuilding>(getEvMgr().getCurrentGF(), _("Lookout-tower occupied"),
                                                          PostCategory::Military, *workplace));
}

bool nofScout_LookoutTower::areWaresAvailable() const
{
    // We never work!
    return false;
}
