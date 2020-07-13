// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#include "GameObject.h"
#include "EventManager.h"
#include "SerializedGameData.h"
#include "postSystem/PostMsg.h"
#include "world/GameWorldGame.h"
#include <iostream>

/**
 *  Objekt-ID-Counter.
 */
unsigned GameObject::objIdCounter_ = 0;
unsigned GameObject::objCounter_ = 0;

GameWorldGame* GameObject::gwg = nullptr;

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

void GameObject::Serialize(SerializedGameData& /*sgd*/) const
{
    std::cout << "ERROR: GameObject::Serialize called." << std::endl; // qx
}

GameObject::~GameObject()
{
    // RTTR_Assert(!gwg || !GetEvMgr().ObjectHasEvents(*this));
    RTTR_Assert(!gwg || !GetEvMgr().IsObjectInKillList(*this));
    // ein Objekt weniger
    --objCounter_;
}

EventManager& GameObject::GetEvMgr()
{
    return gwg->GetEvMgr();
}

void GameObject::SendPostMessage(unsigned player, std::unique_ptr<PostMsg> msg)
{
    gwg->GetPostMgr().SendMsg(player, std::move(msg));
}

void GameObject::DetachWorld(GameWorldGame* gameWorld)
{
    if(gwg == gameWorld)
        gwg = nullptr;
}

void GameObject::AttachWorld(GameWorldGame* gameWorld)
{
    gwg = gameWorld;
}

std::string GameObject::ToString() const
{
    return "GameObject(" + std::to_string(objId) + ")";
}
