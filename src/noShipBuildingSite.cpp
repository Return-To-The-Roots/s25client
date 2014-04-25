// $Id: noShipBuildingSite.cpp 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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
#include "main.h"
#include "noShipBuildingSite.h"

#include "Loader.h"
#include "macros.h"
#include "GameWorld.h"
#include "EventManager.h"
#include "Random.h"
#include "SerializedGameData.h"
#include "noShip.h"
#include "GameClient.h"
#include "PostMsg.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

noShipBuildingSite::noShipBuildingSite(const unsigned short x, const unsigned short y, const unsigned char player)
    : noCoordBase(NOP_ENVIRONMENT, x, y),
      player(player), progress(0)
{
}

noShipBuildingSite::~noShipBuildingSite()
{
}

void noShipBuildingSite::Destroy()
{
    gwg->SetNO(NULL, x, y);

    Destroy_noCoordBase();
}

void noShipBuildingSite::Serialize(SerializedGameData* sgd) const
{
    Serialize_noCoordBase(sgd);

    sgd->PushUnsignedChar(player);
    sgd->PushUnsignedChar(progress);
}

noShipBuildingSite::noShipBuildingSite(SerializedGameData* sgd, const unsigned obj_id) : noCoordBase(sgd, obj_id),
    player(sgd->PopUnsignedChar()),
    progress(sgd->PopUnsignedChar())
{
}

/// Progress-Anteile für die 3 Baustufen
const unsigned PROGRESS_PARTS[3] =
{ 4, 2, 3};

//const unsigned TOTAL_PROGRESS = PROGRESS_PARTS[0] + PROGRESS_PARTS[1] + PROGRESS_PARTS[2];

void noShipBuildingSite::Draw(int x, int y)
{
    if(progress < PROGRESS_PARTS[0] + PROGRESS_PARTS[1])
    {
        glArchivItem_Bitmap* image = LOADER.GetImageN("boot_z", 24);
        unsigned height = min(image->getHeight() * unsigned(progress) / PROGRESS_PARTS[0],
                              unsigned(image->getHeight()));
        image->Draw(x, y + (image->getHeight() - height), 0, 0, 0, (image->getHeight() - height), 0, height);
        image =  LOADER.GetImageN("boot_z", 25);
        height = min(image->getHeight() * unsigned(progress) / PROGRESS_PARTS[0],
                     unsigned(image->getHeight()));
        image->Draw(x, y + (image->getHeight() - height), 0, 0, 0, (image->getHeight() - height), 0, height, COLOR_SHADOW);
    }
    if(progress > PROGRESS_PARTS[0])
    {
        unsigned real_progress = progress - PROGRESS_PARTS[0];
        glArchivItem_Bitmap* image =  LOADER.GetImageN("boot_z", 26);
        unsigned height = min(image->getHeight() * unsigned(real_progress) / PROGRESS_PARTS[1],
                              unsigned(image->getHeight()));
        image->Draw(x, y + (image->getHeight() - height), 0, 0, 0, (image->getHeight() - height), 0, height);
        image =  LOADER.GetImageN("boot_z", 27);
        height = min(image->getHeight() * unsigned(real_progress) / PROGRESS_PARTS[1],
                     unsigned(image->getHeight()));
        image->Draw(x, y + (image->getHeight() - height), 0, 0, 0, (image->getHeight() - height), 0, height, COLOR_SHADOW);
    }
    if(progress > PROGRESS_PARTS[0] + PROGRESS_PARTS[1])
    {
        unsigned real_progress = progress - PROGRESS_PARTS[0] - PROGRESS_PARTS[1];
        glArchivItem_Bitmap* image =  LOADER.GetImageN("boot_z", 28);
        unsigned height = image->getHeight() * unsigned(real_progress) / PROGRESS_PARTS[2];
        image->Draw(x, y + (image->getHeight() - height), 0, 0, 0, (image->getHeight() - height), 0, height);
        image =  LOADER.GetImageN("boot_z", 29);
        height = image->getHeight() * unsigned(real_progress) / PROGRESS_PARTS[2];
        image->Draw(x, y + (image->getHeight() - height), 0, 0, 0, (image->getHeight() - height), 0, height, COLOR_SHADOW);

    }
}



/// Das Schiff wird um eine Stufe weitergebaut
void noShipBuildingSite::MakeBuildStep()
{
    ++progress;

    // Schiff fertiggestellt?
    if(progress > PROGRESS_PARTS[0] + PROGRESS_PARTS[1] + PROGRESS_PARTS[2])
    {
        // Mich vernichten
        em->AddToKillList(this);
        gwg->SetNO(NULL, x, y);
        // ein fertiges Schiff stattdessen hinsetzen
        new noShip(x, y, player);
        // BQ neu berechnen, da Schiff nicht mehr blockiert
        gwg->RecalcBQAroundPointBig(x, y);

        // Spieler über Fertigstellung benachrichtigen
        if(GameClient::inst().GetPlayerID() == this->player)
            GAMECLIENT.SendPostMessage(new ShipPostMsg(_("A new ship is ready"), PMC_GENERAL, GAMECLIENT.GetPlayer(player)->nation, x, y));

        // KI Event senden
        GAMECLIENT.SendAIEvent(new AIEvent::Location(AIEvent::ShipBuilt, x, y), player);
    }

}
