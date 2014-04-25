// $Id: noSign.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
#include "noSign.h"

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
noSign::noSign(const unsigned short x,
               const unsigned short y,
               const unsigned char type,
               const unsigned char quantity)
    : noDisappearingEnvObject(x, y, 8500, 500), type(type), quantity(quantity)
{
}

void noSign::Serialize_noSign(SerializedGameData* sgd) const
{
    Serialize_noDisappearingEnvObject(sgd);

    sgd->PushUnsignedChar(type);
    sgd->PushUnsignedChar(quantity);
}

noSign::noSign(SerializedGameData* sgd, const unsigned obj_id) : noDisappearingEnvObject(sgd, obj_id),
    type(sgd->PopUnsignedChar()),
    quantity(sgd->PopUnsignedChar())
{
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  An x,y zeichnen.
 *
 *  @author OLiver
 */
void noSign::Draw(int x, int y)
{
    // Wenns verschwindet, muss es immer transparenter werden
    unsigned color = GetDrawColor();

    // Schild selbst
    if(type != 5)
        LOADER.GetMapImageN(680 + type * 3 + quantity)->Draw(x, y, 0, 0, 0, 0, 0, 0, color);
    else
        // leeres Schild
        LOADER.GetMapImageN(695)->Draw(x, y, 0, 0, 0, 0, 0, 0, color);

    // Schatten des Schildes
    LOADER.GetMapImageN(700)->Draw(x, y, 0, 0, 0, 0, 0, 0, GetDrawShadowColor());
}

void noSign::HandleEvent(const unsigned int id)
{
    HandleEvent_noDisappearingEnvObject(id);
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Räumt das Objekt auf.
 *
 *  @author FloSoft
 */
void noSign::Destroy_noSign(void)
{
    Destroy_noDisappearingEnvObject();
}
