// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "noBuilding.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glSmartBitmap.h"
#include "world/GameWorld.h"
#include "nodeObjs/noFire.h"
#include "s25util/Log.h"

noBuilding::noBuilding(const BuildingType type, const MapPoint pos, const unsigned char player, const Nation /*nation*/)
    : noBaseBuilding(NodalObjectType::Building, type, pos, player), opendoor(0)
{}

void noBuilding::Destroy()
{
    // First we have to remove the building from the map and the player
    // Replace by fire (huts and mines become small fire, rest big)
    world->SetNO(pos, new noFire(pos, GetSize() != BuildingQuality::Hut && GetSize() != BuildingQuality::Mine), true);
    world->GetPlayer(player).RemoveBuilding(this, bldType_);
    // Destroy derived buildings
    DestroyBuilding();
    // Then go further down the chain
    noBaseBuilding::Destroy();
}

void noBuilding::Serialize(SerializedGameData& sgd) const
{
    noBaseBuilding::Serialize(sgd);

    sgd.PushSignedChar(opendoor);
}

noBuilding::noBuilding(SerializedGameData& sgd, const unsigned obj_id)
    : noBaseBuilding(sgd, obj_id), opendoor(sgd.PopSignedChar())
{
    if(opendoor < 0)
    {
        LOG.write("Bug detected: Door was closed to many times. Please report replay before this savegame/replay!");
        opendoor = 0;
    }
}

void noBuilding::DrawBaseBuilding(DrawPoint drawPt) const
{
    GetBuildingImage().DrawFull(drawPt);
    DrawDoor(drawPt);
}

void noBuilding::DrawDoor(DrawPoint drawPt) const
{
    if(!IsDoorOpen())
        return;
    GetDoorImage().DrawFull(drawPt);
}

void noBuilding::OpenDoor()
{
    ++opendoor;
}

void noBuilding::CloseDoor()
{
    RTTR_Assert(IsDoorOpen());
    --opendoor;
}

std::unique_ptr<FOWObject> noBuilding::CreateFOWObject() const
{
    return std::make_unique<fowBuilding>(bldType_, nation);
}
