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

void noShipBuildingSite::Destroy()
{
    world->SetNO(pos, nullptr);

    noCoordBase::Destroy();
}

void noShipBuildingSite::Serialize(SerializedGameData& sgd) const
{
    noCoordBase::Serialize(sgd);

    sgd.PushUnsignedChar(player);
    sgd.PushUnsignedChar(progress);
}

noShipBuildingSite::noShipBuildingSite(SerializedGameData& sgd, const unsigned obj_id)
    : noCoordBase(sgd, obj_id), player(sgd.PopUnsignedChar()), progress(sgd.PopUnsignedChar())
{}

/// Progress-Anteile für die 3 Baustufen
const std::array<unsigned, 3> PROGRESS_PARTS = {4, 2, 3};

// const unsigned TOTAL_PROGRESS = PROGRESS_PARTS[0] + PROGRESS_PARTS[1] + PROGRESS_PARTS[2];

void noShipBuildingSite::Draw(DrawPoint drawPt)
{
    if(progress < PROGRESS_PARTS[0] + PROGRESS_PARTS[1])
    {
        glArchivItem_Bitmap* image = LOADER.GetImageN("boot_z", 24);
        unsigned percentDone = (progress > PROGRESS_PARTS[0]) ? 100u : progress * 100 / PROGRESS_PARTS[0];
        image->DrawPercent(drawPt, percentDone);
        image = LOADER.GetImageN("boot_z", 25);
        image->DrawPercent(drawPt, percentDone, COLOR_SHADOW);
    }
    if(progress > PROGRESS_PARTS[0])
    {
        unsigned curProg = progress - PROGRESS_PARTS[0];
        unsigned percentDone = (curProg > PROGRESS_PARTS[1]) ? 100u : curProg * 100 / PROGRESS_PARTS[1];
        glArchivItem_Bitmap* image = LOADER.GetImageN("boot_z", 26);
        image->DrawPercent(drawPt, percentDone);
        image = LOADER.GetImageN("boot_z", 27);
        image->DrawPercent(drawPt, percentDone, COLOR_SHADOW);
    }
    if(progress > PROGRESS_PARTS[0] + PROGRESS_PARTS[1])
    {
        unsigned percentDone = (progress - PROGRESS_PARTS[0] - PROGRESS_PARTS[1]) * 100 / PROGRESS_PARTS[2];
        glArchivItem_Bitmap* image = LOADER.GetImageN("boot_z", 28);
        image->DrawPercent(drawPt, percentDone);
        image = LOADER.GetImageN("boot_z", 29);
        image->DrawPercent(drawPt, percentDone, COLOR_SHADOW);
    }
}

/// Das Schiff wird um eine Stufe weitergebaut
void noShipBuildingSite::MakeBuildStep()
{
    ++progress;

    // Schiff fertiggestellt?
    if(progress > PROGRESS_PARTS[0] + PROGRESS_PARTS[1] + PROGRESS_PARTS[2])
    {
        // Replace me by ship
        GetEvMgr().AddToKillList(this);
        world->SetNO(pos, nullptr);
        auto& ship = world->AddFigure(pos, std::make_unique<noShip>(pos, player));

        // Schiff registrieren lassen
        world->GetPlayer(player).RegisterShip(ship);

        // BQ neu berechnen, da Schiff nicht mehr blockiert
        world->RecalcBQAroundPointBig(pos);

        // Spieler über Fertigstellung benachrichtigen
        SendPostMessage(player, std::make_unique<ShipPostMsg>(GetEvMgr().GetCurrentGF(), _("A new ship is ready"),
                                                              PostCategory::Economy, ship));
        world->GetNotifications().publish(ShipNote(ShipNote::Constructed, player, pos));
    }
}
