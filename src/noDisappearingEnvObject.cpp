// $Id: noDisappearingEnvObject.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
#include "noDisappearingEnvObject.h"

#include "EventManager.h"
#include "GameClient.h"
#include "GameWorld.h"
#include "SerializedGameData.h"
#include "Random.h"

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
noDisappearingEnvObject::noDisappearingEnvObject(const unsigned short x, const unsigned short y,
        const unsigned living_time, const unsigned add_var_living_time)
    : noCoordBase(NOP_ENVIRONMENT, x, y),
      disappearing(false), dead_event(em->AddEvent(this, living_time +
                                      Random::inst().Rand(__FILE__, __LINE__, obj_id, add_var_living_time)))
{
}

void noDisappearingEnvObject::Serialize_noDisappearingEnvObject(SerializedGameData* sgd) const
{
    Serialize_noCoordBase(sgd);

    sgd->PushBool(disappearing);
    sgd->PushObject(dead_event, true);
}

noDisappearingEnvObject::noDisappearingEnvObject(SerializedGameData* sgd, const unsigned obj_id) : noCoordBase(sgd, obj_id),
    disappearing(sgd->PopBool()),
    dead_event(sgd->PopObject<EventManager::Event>(GOT_EVENT))
{
}

/// Gibt Farbe zurück, mit der das Objekt gezeichnet werden soll
unsigned noDisappearingEnvObject::GetDrawColor() const
{
    if(disappearing)
    {
        unsigned int transparency = 0xFF - GAMECLIENT.Interpolate(0xFF, dead_event);
        return transparency | (transparency << 8) | (transparency << 16) | (transparency << 24);
    }
    else
        return 0xFFFFFFFF;
}

/// Gibt Farbe zurück, mit der der Schatten des Objekts gezeichnet werden soll
unsigned noDisappearingEnvObject::GetDrawShadowColor() const
{
    if(disappearing)
    {
        unsigned int transparency = 0x40 - GAMECLIENT.Interpolate(0x40, dead_event);
        return (transparency << 24);
    }
    else
        return COLOR_SHADOW;
}


///////////////////////////////////////////////////////////////////////////////
/**
 *  Benachrichtigen, wenn neuer GF erreicht wurde.
 *
 *  @author FloSoft
 */
void noDisappearingEnvObject::HandleEvent_noDisappearingEnvObject(const unsigned int id)
{
    if(id)
    {
        // endgültig vernichten
        em->AddToKillList(this);
        dead_event = 0;
    }
    else
    {
        // Jetzt verschwinden
        disappearing = true;
        // In ner bestimmten Zeit dann endgültig vernichten
        dead_event = em->AddEvent(this, 30, 1);
    }
}


///////////////////////////////////////////////////////////////////////////////
/**
 *  Räumt das Objekt auf.
 *
 *  @author FloSoft
 */
void noDisappearingEnvObject::Destroy_noDisappearingEnvObject(void)
{
    // Feld räumen, wenn ich sterbe
    gwg->SetNO(0, x, y);

    // ggf Event abmelden
    if(dead_event)
        em->RemoveEvent(dead_event);

    Destroy_noCoordBase();
}
