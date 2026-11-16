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
    sgd.addObject(this);
}

GameObject::GameObject(const GameObject& go) : objId(go.objId)
{
    // ein Objekt mehr
    ++objCounter_;
}

void GameObject::destroy() {}

GameObject::~GameObject()
{
    // RTTR_Assert(!world || !GetEvMgr().ObjectHasEvents(*this));
    RTTR_Assert(!world || !getEvMgr().isObjectInKillList(*this));
    // ein Objekt weniger
    --objCounter_;
}

EventManager& GameObject::getEvMgr()
{
    return world->getEvMgr();
}

void GameObject::sendPostMessage(unsigned player, std::unique_ptr<PostMsg> msg)
{
    world->getPostMgr().sendMsg(player, std::move(msg));
}

void GameObject::detachWorld(GameWorld* gameWorld)
{
    if(world == gameWorld)
        world = nullptr;
}

void GameObject::attachWorld(GameWorld* gameWorld)
{
    world = gameWorld;
}

std::string GameObject::toString() const
{
    return "GameObject(" + std::to_string(objId) + ")";
}
