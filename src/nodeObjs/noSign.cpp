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

#include "defines.h" // IWYU pragma: keep
#include "noSign.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "ogl/glArchivItem_Bitmap_Player.h"

/**
 *  Konstruktor von @p noBase.
 *
 *  @param[in] x        X-Position
 *  @param[in] y        Y-Position
 *  @param[in] type     Typ der Ressource
 *  @param[in] quantity Menge der Ressource
 */
noSign::noSign(const MapPoint pos, const unsigned char type, const unsigned char quantity)
    : noDisappearingEnvObject(pos, 8500, 500), type(type), quantity(quantity)
{
}

void noSign::Serialize_noSign(SerializedGameData& sgd) const
{
    noDisappearingEnvObject::Serialize(sgd);

    sgd.PushUnsignedChar(type);
    sgd.PushUnsignedChar(quantity);
}

noSign::noSign(SerializedGameData& sgd, const unsigned obj_id)
    : noDisappearingEnvObject(sgd, obj_id), type(sgd.PopUnsignedChar()), quantity(sgd.PopUnsignedChar())
{
}

/**
 *  An x,y zeichnen.
 */
void noSign::Draw(DrawPoint drawPt)
{
    // Wenns verschwindet, muss es immer transparenter werden
    unsigned color = GetDrawColor();

    // Schild selbst
    if(type != 5)
        LOADER.GetMapPlayerImage(680 + type * 3 + quantity)->DrawFull(drawPt, color);
    else
        // leeres Schild
        LOADER.GetMapPlayerImage(695)->DrawFull(drawPt, color);

    // Schatten des Schildes
    LOADER.GetMapImageN(700)->DrawFull(drawPt, GetDrawShadowColor());
}
