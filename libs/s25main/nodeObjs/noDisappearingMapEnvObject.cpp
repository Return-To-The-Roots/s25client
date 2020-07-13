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

#include "noDisappearingMapEnvObject.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "ogl/glArchivItem_Bitmap.h"

/**
 *  Konstruktor von @p noBase.
 *
 *  @param[in] x        X-Position
 *  @param[in] y        Y-Position
 *  @param[in] type     Typ der Ressource
 *  @param[in] quantity Menge der Ressource
 */
noDisappearingMapEnvObject::noDisappearingMapEnvObject(const MapPoint pos, const unsigned short map_id)
    : noDisappearingEnvObject(pos, 4000, 1000), map_id(map_id)
{}

void noDisappearingMapEnvObject::Serialize_noDisappearingMapEnvObject(SerializedGameData& sgd) const
{
    noDisappearingEnvObject::Serialize(sgd);

    sgd.PushUnsignedShort(map_id);
}

noDisappearingMapEnvObject::noDisappearingMapEnvObject(SerializedGameData& sgd, const unsigned obj_id)
    : noDisappearingEnvObject(sgd, obj_id), map_id(sgd.PopUnsignedShort())
{}

/**
 *  An x,y zeichnen.
 */
void noDisappearingMapEnvObject::Draw(DrawPoint drawPt)
{
    // Bild
    LOADER.GetMapImageN(map_id)->DrawFull(drawPt, GetDrawColor());
    // Schatten
    LOADER.GetMapImageN(map_id + 100)->DrawFull(drawPt, GetDrawShadowColor());
}
