// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "noSkeleton.h"
#include "EventManager.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "random/Random.h"
#include "world/GameWorld.h"

noSkeleton::noSkeleton(const MapPoint pos) : noCoordBase(NodalObjectType::Environment, pos), type(0)
{
    current_event = getEvMgr().addEvent(this, 15000 + RANDOM_RAND(10000));
}

noSkeleton::~noSkeleton() = default;

void noSkeleton::destroy()
{
    // Remove from map if still present
    if(world->getNode(pos).obj == this)
        world->setNO(pos, nullptr);

    getEvMgr().removeEvent(current_event);

    noCoordBase::destroy();
}

void noSkeleton::Serialize(SerializedGameData& sgd) const
{
    noCoordBase::Serialize(sgd);

    sgd.PushUnsignedChar(type);
    sgd.pushEvent(current_event);
}

noSkeleton::noSkeleton(SerializedGameData& sgd, const unsigned obj_id)
    : noCoordBase(sgd, obj_id), type(sgd.PopUnsignedChar()), current_event(sgd.popEvent())
{}

void noSkeleton::draw(DrawPoint drawPt)
{
    LOADER.getMapTexture(547 + type)->drawFull(drawPt);
}

void noSkeleton::handleEvent(const unsigned /*id*/)
{
    if(!type)
    {
        // weiter verwesen, dann später sterben nach ner zufälligen Zeit
        type = 1;
        current_event = getEvMgr().addEvent(this, 10000 + RANDOM_RAND(10000));
    } else
    {
        // Remove from map and destroy
        RTTR_Assert(world->getNode(pos).obj == this);
        world->setNO(pos, nullptr);
        getEvMgr().addToKillList(this);
        current_event = nullptr;
    }
}
