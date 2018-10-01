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

#include "rttrDefines.h" // IWYU pragma: keep
#include "noShipBuildingSite.h"

#include "EventManager.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "network/GameClient.h"
#include "noShip.h"
#include "notifications/ShipNote.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "postSystem/ShipPostMsg.h"
#include "world/GameWorldGame.h"

noShipBuildingSite::noShipBuildingSite(const MapPoint pos, const unsigned char player)
    : noCoordBase(NOP_ENVIRONMENT, pos), player(player), progress(0)
{}

noShipBuildingSite::~noShipBuildingSite() {}

void noShipBuildingSite::Destroy()
{
    gwg->SetNO(pos, NULL);

    Destroy_noCoordBase();
}

void noShipBuildingSite::Serialize(SerializedGameData& sgd) const
{
    Serialize_noCoordBase(sgd);

    sgd.PushUnsignedChar(player);
    sgd.PushUnsignedChar(progress);
}

noShipBuildingSite::noShipBuildingSite(SerializedGameData& sgd, const unsigned obj_id)
    : noCoordBase(sgd, obj_id), player(sgd.PopUnsignedChar()), progress(sgd.PopUnsignedChar())
{}

/// Progress-Anteile für die 3 Baustufen
const unsigned PROGRESS_PARTS[3] = {4, 2, 3};

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
        unsigned percentDone = (progress > PROGRESS_PARTS[1]) ? 100u : curProg * 100 / PROGRESS_PARTS[1];
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
        gwg->SetNO(pos, NULL);
        noShip* ship = new noShip(pos, player);
        gwg->AddFigure(pos, ship);

        // Schiff registrieren lassen
        gwg->GetPlayer(player).RegisterShip(ship);

        // BQ neu berechnen, da Schiff nicht mehr blockiert
        gwg->RecalcBQAroundPointBig(pos);

        // Spieler über Fertigstellung benachrichtigen
        SendPostMessage(player, new ShipPostMsg(GetEvMgr().GetCurrentGF(), _("A new ship is ready"), PostCategory::Economy, *ship));
        gwg->GetNotifications().publish(ShipNote(ShipNote::Constructed, player, pos));
    }
}
