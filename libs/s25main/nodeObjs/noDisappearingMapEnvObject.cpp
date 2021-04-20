// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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

void noDisappearingMapEnvObject::Serialize(SerializedGameData& sgd) const
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
