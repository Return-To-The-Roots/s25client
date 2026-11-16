// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "noDisappearingEnvObject.h"

#include "EventManager.h"
#include "SerializedGameData.h"
#include "network/GameClient.h"
#include "random/Random.h"
#include "world/GameWorld.h"
#include "s25util/colors.h"

noDisappearingEnvObject::noDisappearingEnvObject(const MapPoint pos, const unsigned living_time,
                                                 const unsigned add_var_living_time)
    : noCoordBase(NodalObjectType::Environment, pos), disappearing(false)
{
    dead_event = getEvMgr().addEvent(this, living_time + RANDOM_RAND(add_var_living_time));
}

void noDisappearingEnvObject::serialize(SerializedGameData& sgd) const
{
    noCoordBase::serialize(sgd);

    sgd.PushBool(disappearing);
    sgd.pushEvent(dead_event);
}

noDisappearingEnvObject::noDisappearingEnvObject(SerializedGameData& sgd, const unsigned obj_id)
    : noCoordBase(sgd, obj_id), disappearing(sgd.PopBool()), dead_event(sgd.popEvent())
{}

unsigned noDisappearingEnvObject::getDrawColor() const
{
    if(disappearing)
    {
        const unsigned opacity = GAMECLIENT.interpolate(0xFF, 0, dead_event);
        return opacity | (opacity << 8) | (opacity << 16) | (opacity << 24);
    } else
        return 0xFFFFFFFF;
}

unsigned noDisappearingEnvObject::getDrawShadowColor() const
{
    if(disappearing)
    {
        const unsigned opacity = GAMECLIENT.interpolate(0x40, 0, dead_event);
        return (opacity << 24);
    } else
        return COLOR_SHADOW;
}

void noDisappearingEnvObject::handleEvent(const unsigned /*id*/)
{
    if(disappearing)
    {
        RTTR_Assert(world->getNode(pos).obj == this);
        world->setNO(pos, nullptr);
        getEvMgr().addToKillList(this);
        dead_event = nullptr;
    } else
    {
        disappearing = true;
        // Remove after some time fading out
        dead_event = getEvMgr().addEvent(this, 30, 1);
    }
}

void noDisappearingEnvObject::destroy()
{
    // Remove from map if still present
    if(world->getNode(pos).obj == this)
        world->setNO(pos, nullptr);

    getEvMgr().removeEvent(dead_event);

    noCoordBase::destroy();
}
