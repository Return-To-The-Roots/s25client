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

// Include last!
#include "DebugNew.h" // IWYU pragma: keep
class noFigure;

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
noBuilding::noBuilding(const BuildingType type,
                       const MapPoint pos,
                       const unsigned char player,
                       const Nation nation)
    : noBaseBuilding(NOP_BUILDING, type, pos, player),
      opendoor(0)
{
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void noBuilding::Destroy_noBuilding()
{
    // Feuer erzeugen (bei Hütten und Bergwerken kleine Feuer, bei allen anderen große!)
    // Feuer setzen
    gwg->SetNO(pos, new noFire(pos, (GetSize() == BQ_HUT || GetSize() == BQ_MINE) ? 0 : 1), true);

    Destroy_noBaseBuilding();
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void noBuilding::Serialize_noBuilding(SerializedGameData& sgd) const
{
    Serialize_noBaseBuilding(sgd);

    sgd.PushUnsignedChar(opendoor);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
noBuilding::noBuilding(SerializedGameData& sgd, const unsigned obj_id) : noBaseBuilding(sgd, obj_id),
    opendoor(sgd.PopUnsignedChar())
{
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void noBuilding::DrawBaseBuilding(int x, int y)
{
    LOADER.building_cache[nation][type_][0].draw(x, y);

    if (opendoor && GetDoorImage())
    {
        GetDoorImage()->Draw(x, y, 0, 0, 0, 0, 0, 0);
    }
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
void noBuilding::GotWorker(Job job, noFigure* worker)
{
}

///////////////////////////////////////////////////////////////////////////////
/**
 *
 *
 *  @author OLiver
 */
FOWObject* noBuilding::CreateFOWObject() const
{
    return new fowBuilding(type_, nation);
}
