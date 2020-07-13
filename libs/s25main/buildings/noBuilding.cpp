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

#include "noBuilding.h"
#include "GamePlayer.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glSmartBitmap.h"
#include "world/GameWorldGame.h"
#include "nodeObjs/noFire.h"
#include "s25util/Log.h"

noBuilding::noBuilding(const BuildingType type, const MapPoint pos, const unsigned char player, const Nation /*nation*/)
    : noBaseBuilding(NOP_BUILDING, type, pos, player), opendoor(0)
{}

void noBuilding::Destroy()
{
    // First we have to remove the building from the map and the player
    // Replace by fire (huts and mines become small fire, rest big)
    gwg->SetNO(pos, new noFire(pos, GetSize() != BQ_HUT && GetSize() != BQ_MINE), true);
    gwg->GetPlayer(player).RemoveBuilding(this, bldType_);
    // Destroy derived buildings
    DestroyBuilding();
    // Then go further down the chain
    Destroy_noBaseBuilding();
}

void noBuilding::Serialize_noBuilding(SerializedGameData& sgd) const
{
    Serialize_noBaseBuilding(sgd);

    sgd.PushSignedChar(opendoor);
}

noBuilding::noBuilding(SerializedGameData& sgd, const unsigned obj_id) : noBaseBuilding(sgd, obj_id), opendoor(sgd.PopSignedChar())
{
    if(opendoor < 0)
    {
        LOG.write("Bug detected: Door was closed to many times. Please report replay before this savegame/replay!");
        opendoor = 0;
    }
}

void noBuilding::DrawBaseBuilding(DrawPoint drawPt)
{
    LOADER.building_cache[nation][bldType_][0].draw(drawPt);
    DrawDoor(drawPt);
}

void noBuilding::DrawDoor(DrawPoint drawPt)
{
    if(!IsDoorOpen())
        return;
    glArchivItem_Bitmap* doorImg = GetDoorImage();
    if(doorImg)
        doorImg->DrawFull(drawPt);
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

FOWObject* noBuilding::CreateFOWObject() const
{
    return new fowBuilding(bldType_, nation);
}
