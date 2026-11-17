// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
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

void noBuilding::destroy()
{
    // First we have to remove the building from the map and the player
    // Replace by fire (huts and mines become small fire, rest big)
    world->setNO(pos, new noFire(pos, getSize() != BuildingQuality::Hut && getSize() != BuildingQuality::Mine), true);
    world->getPlayer(player).removeBuilding(this, bldType_);
    // Destroy derived buildings
    DestroyBuilding();
    // Then go further down the chain
    noBaseBuilding::destroy();
}

void noBuilding::serialize(SerializedGameData& sgd) const
{
    noBaseBuilding::serialize(sgd);

    sgd.pushSignedChar(opendoor);
}

noBuilding::noBuilding(SerializedGameData& sgd, const unsigned obj_id)
    : noBaseBuilding(sgd, obj_id), opendoor(sgd.popSignedChar())
{
    if(opendoor < 0)
    {
        LOG.write("Bug detected: Door was closed to many times. Please report replay before this savegame/replay!");
        opendoor = 0;
    }
}

void noBuilding::drawBaseBuilding(DrawPoint drawPt) const
{
    getBuildingImage().drawFull(drawPt);
    drawDoor(drawPt);
}

void noBuilding::drawDoor(DrawPoint drawPt) const
{
    if(!isDoorOpen())
        return;
    getDoorImage().drawFull(drawPt);
}

void noBuilding::openDoor()
{
    ++opendoor;
}

void noBuilding::closeDoor()
{
    RTTR_Assert(isDoorOpen());
    --opendoor;
}

std::unique_ptr<FOWObject> noBuilding::createFOWObject() const
{
    return std::make_unique<fowBuilding>(bldType_, nation);
}
