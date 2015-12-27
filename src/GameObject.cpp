// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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
#include "defines.h"
#include "GameObject.h"
#include "SerializedGameData.h"
#include "EventManager.h"

#include <iostream>

#include "GamePlayerList.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

///////////////////////////////////////////////////////////////////////////////
/**
 *  Objekt-ID-Counter.
 *
 *  @author OLiver
 */
unsigned int GameObject::objIdCounter_ = 1;
unsigned int GameObject::objCounter_ = 0;

GameWorldGame* GameObject::gwg = NULL;
EventManager* GameObject::em = NULL;

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p GameObject.
 *
 *  @param[in] nop ObjektTyp
 *
 *  @author OLiver
 */
GameObject::GameObject(void) : objId(objIdCounter_++)
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

GameObject& GameObject::operator=(const GameObject& obj)
{
    if(this == &obj)
        return *this;
    objId = obj.objId;
    ++objCounter_;
    return *this;
}

void GameObject::Destroy()
{
}

void GameObject::Serialize(SerializedGameData& sgd) const
{
    std::cout << "ERROR: GameObject::Serialize called." << std::endl; // qx
}

///////////////////////////////////////////////////////////////////////////////
/**
 *  Destruktor von @p GameObject.
 *
 *  @author OLiver
 */
GameObject::~GameObject()
{
    assert(!em || !em->ObjectHasEvents(this));
    assert(!em || !em->ObjectIsInKillList(this));
    // ein Objekt weniger
    --objCounter_;
}
