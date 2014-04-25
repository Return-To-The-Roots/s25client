// $Id: noDisappearingMapEnvObject.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
#include "noDisappearingMapEnvObject.h"

#include "EventManager.h"
#include "GameClient.h"
#include "GameWorld.h"
#include "SerializedGameData.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p noBase.
 *
 *  @param[in] x        X-Position
 *  @param[in] y        Y-Position
 *  @param[in] type     Typ der Ressource
 *  @param[in] quantity Menge der Ressource
 *
 *  @author OLiver
 */
noDisappearingMapEnvObject::noDisappearingMapEnvObject(const unsigned short x,
        const unsigned short y,
        const unsigned short map_id)
    : noDisappearingEnvObject(x, y, 4000, 1000), map_id(map_id)
{
}

void noDisappearingMapEnvObject::Serialize_noDisappearingMapEnvObject(SerializedGameData* sgd) const
{
    Serialize_noDisappearingEnvObject(sgd);

    sgd->PushUnsignedShort(map_id);
}

noDisappearingMapEnvObject::noDisappearingMapEnvObject(SerializedGameData* sgd, const unsigned obj_id)
    : noDisappearingEnvObject(sgd, obj_id), map_id(sgd->PopUnsignedShort())
{
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  An x,y zeichnen.
 *
 *  @author OLiver
 */
void noDisappearingMapEnvObject::Draw(int x, int y)
{
    // Bild
    LOADER.GetMapImageN(map_id)->Draw(x, y, 0, 0, 0, 0, 0, 0, GetDrawColor());
    // Schatten
    LOADER.GetMapImageN(map_id + 100)->Draw(x, y, 0, 0, 0, 0, 0, 0, GetDrawShadowColor());
}

void noDisappearingMapEnvObject::HandleEvent(const unsigned int id)
{
    HandleEvent_noDisappearingEnvObject(id);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Räumt das Objekt auf.
 *
 *  @author FloSoft
 */
void noDisappearingMapEnvObject::Destroy_noDisappearingMapEnvObject(void)
{
    Destroy_noDisappearingEnvObject();
}
