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
    current_event = GetEvMgr().AddEvent(this, 15000 + RANDOM_RAND(10000));
}

noSkeleton::~noSkeleton() = default;

void noSkeleton::Destroy()
{
    world->SetNO(pos, nullptr);

    // ggf Event abmelden
    if(current_event)
        GetEvMgr().RemoveEvent(current_event);

    noCoordBase::Destroy();
}

void noSkeleton::Serialize(SerializedGameData& sgd) const
{
    noCoordBase::Serialize(sgd);

    sgd.PushUnsignedChar(type);
    sgd.PushEvent(current_event);
}

noSkeleton::noSkeleton(SerializedGameData& sgd, const unsigned obj_id)
    : noCoordBase(sgd, obj_id), type(sgd.PopUnsignedChar()), current_event(sgd.PopEvent())
{}

void noSkeleton::Draw(DrawPoint drawPt)
{
    LOADER.GetMapTexture(547 + type)->DrawFull(drawPt);
}

void noSkeleton::HandleEvent(const unsigned /*id*/)
{
    if(!type)
    {
        // weiter verwesen, dann später sterben nach ner zufälligen Zeit
        type = 1;
        current_event = GetEvMgr().AddEvent(this, 10000 + RANDOM_RAND(10000));
    } else
    {
        // ganz weg damit
        current_event = nullptr;
        GetEvMgr().AddToKillList(this);
    }
}
