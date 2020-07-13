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

#include "noDisappearingEnvObject.h"

#include "EventManager.h"
#include "SerializedGameData.h"
#include "network/GameClient.h"
#include "random/Random.h"
#include "world/GameWorldGame.h"
#include "s25util/colors.h"

/**
 *  Konstruktor von @p noBase.
 *
 *  @param[in] x        X-Position
 *  @param[in] y        Y-Position
 *  @param[in] type     Typ der Ressource
 *  @param[in] quantity Menge der Ressource
 */
noDisappearingEnvObject::noDisappearingEnvObject(const MapPoint pos, const unsigned living_time, const unsigned add_var_living_time)
    : noCoordBase(NOP_ENVIRONMENT, pos), disappearing(false)
{
    dead_event = GetEvMgr().AddEvent(this, living_time + RANDOM.Rand(__FILE__, __LINE__, GetObjId(), add_var_living_time));
}

void noDisappearingEnvObject::Serialize(SerializedGameData& sgd) const
{
    Serialize_noCoordBase(sgd);

    sgd.PushBool(disappearing);
    sgd.PushEvent(dead_event);
}

noDisappearingEnvObject::noDisappearingEnvObject(SerializedGameData& sgd, const unsigned obj_id)
    : noCoordBase(sgd, obj_id), disappearing(sgd.PopBool()), dead_event(sgd.PopEvent())
{}

/// Gibt Farbe zurück, mit der das Objekt gezeichnet werden soll
unsigned noDisappearingEnvObject::GetDrawColor() const
{
    if(disappearing)
    {
        unsigned transparency = 0xFF - GAMECLIENT.Interpolate(0xFF, dead_event);
        return transparency | (transparency << 8) | (transparency << 16) | (transparency << 24);
    } else
        return 0xFFFFFFFF;
}

/// Gibt Farbe zurück, mit der der Schatten des Objekts gezeichnet werden soll
unsigned noDisappearingEnvObject::GetDrawShadowColor() const
{
    if(disappearing)
    {
        unsigned transparency = 0x40 - GAMECLIENT.Interpolate(0x40, dead_event);
        return (transparency << 24);
    } else
        return COLOR_SHADOW;
}

/**
 *  Benachrichtigen, wenn neuer GF erreicht wurde.
 */
void noDisappearingEnvObject::HandleEvent(const unsigned id)
{
    if(id)
    {
        // endgültig vernichten
        GetEvMgr().AddToKillList(this);
        dead_event = nullptr;
    } else
    {
        // Jetzt verschwinden
        disappearing = true;
        // In ner bestimmten Zeit dann endgültig vernichten
        dead_event = GetEvMgr().AddEvent(this, 30, 1);
    }
}

/**
 *  Räumt das Objekt auf.
 */
void noDisappearingEnvObject::Destroy()
{
    // Feld räumen, wenn ich sterbe
    gwg->SetNO(pos, nullptr);

    // ggf Event abmelden
    if(dead_event)
        GetEvMgr().RemoveEvent(dead_event);

    Destroy_noCoordBase();
}
