// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
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
    dead_event = GetEvMgr().AddEvent(this, living_time + RANDOM_RAND(add_var_living_time));
}

void noDisappearingEnvObject::Serialize(SerializedGameData& sgd) const
{
    noCoordBase::Serialize(sgd);

    sgd.PushBool(disappearing);
    sgd.PushEvent(dead_event);
}

noDisappearingEnvObject::noDisappearingEnvObject(SerializedGameData& sgd, const unsigned obj_id)
    : noCoordBase(sgd, obj_id), disappearing(sgd.PopBool()), dead_event(sgd.PopEvent())
{}

unsigned noDisappearingEnvObject::GetDrawColor() const
{
    if(disappearing)
    {
        unsigned transparency = 0xFF - GAMECLIENT.Interpolate(0xFF, dead_event);
        return transparency | (transparency << 8) | (transparency << 16) | (transparency << 24);
    } else
        return 0xFFFFFFFF;
}

unsigned noDisappearingEnvObject::GetDrawShadowColor() const
{
    if(disappearing)
    {
        unsigned transparency = 0x40 - GAMECLIENT.Interpolate(0x40, dead_event);
        return (transparency << 24);
    } else
        return COLOR_SHADOW;
}

void noDisappearingEnvObject::HandleEvent(const unsigned id)
{
    if(id)
    {
        RTTR_Assert(world->GetNode(pos).obj == this);
        world->SetNO(pos, nullptr);
        GetEvMgr().AddToKillList(this);
        dead_event = nullptr;
    } else
    {
        disappearing = true;
        // Remove after some time fading out
        dead_event = GetEvMgr().AddEvent(this, 30, 1);
    }
}

void noDisappearingEnvObject::Destroy()
{
    // Remove from map if still present
    if(world->GetNode(pos).obj == this)
        world->SetNO(pos, nullptr);

    GetEvMgr().RemoveEvent(dead_event);

    noCoordBase::Destroy();
}
