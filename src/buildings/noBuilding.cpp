// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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
#include "defines.h" // IWYU pragma: keep
#include "noBuilding.h"
#include "world/GameWorldGame.h"
#include "nodeObjs/noFire.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "ogl/glSmartBitmap.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "libutil/src/Log.h"

// Include last!
#include "DebugNew.h" // IWYU pragma: keep
class noFigure;

noBuilding::noBuilding(const BuildingType type,
                       const MapPoint pos,
                       const unsigned char player,
                       const Nation  /*nation*/)
    : noBaseBuilding(NOP_BUILDING, type, pos, player),
      opendoor(0)
{
}

void noBuilding::Destroy_noBuilding()
{
    // Feuer erzeugen (bei Hütten und Bergwerken kleine Feuer, bei allen anderen große!)
    // Feuer setzen
    gwg->SetNO(pos, new noFire(pos, (GetSize() == BQ_HUT || GetSize() == BQ_MINE) ? 0 : 1), true);

    Destroy_noBaseBuilding();
}

void noBuilding::Serialize_noBuilding(SerializedGameData& sgd) const
{
    Serialize_noBaseBuilding(sgd);

    sgd.PushSignedChar(opendoor);
}

noBuilding::noBuilding(SerializedGameData& sgd, const unsigned obj_id) : noBaseBuilding(sgd, obj_id),
    opendoor(sgd.PopSignedChar())
{
    if(opendoor < 0){
        LOG.lprintf("Bug detected: Door was closed to many times. Please report replay before this savegame/replay!");
        opendoor = 0;
    }
}

void noBuilding::DrawBaseBuilding(int x, int y)
{
    LOADER.building_cache[nation][type_][0].draw(x, y);
    DrawDoor(x, y);
}

void noBuilding::DrawDoor(int x, int y)
{
    if(!opendoor)
        return;
    glArchivItem_Bitmap* doorImg = GetDoorImage();
    if(doorImg)
        doorImg->Draw(x, y, 0, 0, 0, 0, 0, 0);
}

void noBuilding::GotWorker(Job  /*job*/, noFigure*  /*worker*/)
{
}

FOWObject* noBuilding::CreateFOWObject() const
{
    return new fowBuilding(type_, nation);
}
