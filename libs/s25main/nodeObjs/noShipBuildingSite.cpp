// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "noShipBuildingSite.h"

#include "EventManager.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "noShip.h"
#include "notifications/ShipNote.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "postSystem/ShipPostMsg.h"
#include "world/GameWorld.h"

noShipBuildingSite::noShipBuildingSite(const MapPoint pos, const unsigned char player)
    : noCoordBase(NodalObjectType::Environment, pos), player(player), progress(0)
{}

noShipBuildingSite::~noShipBuildingSite() = default;

void noShipBuildingSite::destroy()
{
    if(world->getNode(pos).obj == this)
        world->setNO(pos, nullptr);

    noCoordBase::destroy();
}

void noShipBuildingSite::serialize(SerializedGameData& sgd) const
{
    noCoordBase::serialize(sgd);

    sgd.pushUnsignedChar(player);
    sgd.pushUnsignedChar(progress);
}

noShipBuildingSite::noShipBuildingSite(SerializedGameData& sgd, const unsigned obj_id)
    : noCoordBase(sgd, obj_id), player(sgd.popUnsignedChar()), progress(sgd.popUnsignedChar())
{}

/// Progress-Anteile für die 3 Baustufen
constexpr std::array<unsigned, 3> PROGRESS_PARTS = {4, 2, 3};

void noShipBuildingSite::draw(DrawPoint drawPt)
{
    if(progress < PROGRESS_PARTS[0] + PROGRESS_PARTS[1])
    {
        glArchivItem_Bitmap* image = LOADER.getImageN("boot_z", 24);
        unsigned percentDone = (progress > PROGRESS_PARTS[0]) ? 100u : progress * 100 / PROGRESS_PARTS[0];
        image->drawPercent(drawPt, percentDone);
        image = LOADER.getImageN("boot_z", 25);
        image->drawPercent(drawPt, percentDone, COLOR_SHADOW);
    }
    if(progress > PROGRESS_PARTS[0])
    {
        unsigned curProg = progress - PROGRESS_PARTS[0];
        unsigned percentDone = (curProg > PROGRESS_PARTS[1]) ? 100u : curProg * 100 / PROGRESS_PARTS[1];
        glArchivItem_Bitmap* image = LOADER.getImageN("boot_z", 26);
        image->drawPercent(drawPt, percentDone);
        image = LOADER.getImageN("boot_z", 27);
        image->drawPercent(drawPt, percentDone, COLOR_SHADOW);
    }
    if(progress > PROGRESS_PARTS[0] + PROGRESS_PARTS[1])
    {
        unsigned percentDone = (progress - PROGRESS_PARTS[0] - PROGRESS_PARTS[1]) * 100 / PROGRESS_PARTS[2];
        glArchivItem_Bitmap* image = LOADER.getImageN("boot_z", 28);
        image->drawPercent(drawPt, percentDone);
        image = LOADER.getImageN("boot_z", 29);
        image->drawPercent(drawPt, percentDone, COLOR_SHADOW);
    }
}

/// Das Schiff wird um eine Stufe weitergebaut
void noShipBuildingSite::makeBuildStep()
{
    ++progress;

    // Schiff fertiggestellt?
    if(progress > PROGRESS_PARTS[0] + PROGRESS_PARTS[1] + PROGRESS_PARTS[2])
    {
        // Replace me by ship
        world->setNO(pos, nullptr);
        getEvMgr().addToKillList(this);
        auto& ship = world->addFigure(pos, std::make_unique<noShip>(pos, player));

        // Schiff registrieren lassen
        world->getPlayer(player).registerShip(ship);

        // BQ neu berechnen, da Schiff nicht mehr blockiert
        world->recalcBQAroundPointBig(pos);

        // Spieler über Fertigstellung benachrichtigen
        sendPostMessage(player, std::make_unique<ShipPostMsg>(getEvMgr().getCurrentGF(), _("A new ship is ready"),
                                                              PostCategory::Economy, ship));
        world->getNotifications().publish(ShipNote(ShipNote::Constructed, player, pos));
    }
}
