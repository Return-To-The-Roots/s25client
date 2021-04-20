// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "GameObject.h"
#include "EventManager.h"
#include "SerializedGameData.h"
#include "postSystem/PostMsg.h"
#include "world/GameWorld.h"
#include <iostream>

/**
 *  Objekt-ID-Counter.
 */
unsigned GameObject::objIdCounter_ = 0;
unsigned GameObject::objCounter_ = 0;

GameWorld* GameObject::world = nullptr;

GameObject::GameObject() : objId(++objIdCounter_)
{
    // ein Objekt mehr
    ++objCounter_;
}

GameObject::GameObject(SerializedGameData& sgd, const unsigned obj_id) : objId(obj_id)
{
    // ein Objekt mehr
    ++objCounter_;
    sgd.AddObject(this);
}

GameObject::GameObject(const GameObject& go) : objId(go.objId)
{
    // ein Objekt mehr
    ++objCounter_;
}

void GameObject::Destroy() {}

GameObject::~GameObject()
{
    // RTTR_Assert(!world || !GetEvMgr().ObjectHasEvents(*this));
    RTTR_Assert(!world || !GetEvMgr().IsObjectInKillList(*this));
    // ein Objekt weniger
    --objCounter_;
}

EventManager& GameObject::GetEvMgr()
{
    return world->GetEvMgr();
}

void GameObject::SendPostMessage(unsigned player, std::unique_ptr<PostMsg> msg)
{
    world->GetPostMgr().SendMsg(player, std::move(msg));
}

void GameObject::DetachWorld(GameWorld* gameWorld)
{
    if(world == gameWorld)
        world = nullptr;
}

void GameObject::AttachWorld(GameWorld* gameWorld)
{
    world = gameWorld;
}

std::string GameObject::ToString() const
{
    return "GameObject(" + std::to_string(objId) + ")";
}
