// $Id: GameObject.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
unsigned int GameObject::obj_id_counter = 1;
unsigned int GameObject::obj_counter = 0;

GameWorldGame* GameObject::gwg = NULL;
EventManager* GameObject::em = NULL;
GameClientPlayerList* GameObject::players = NULL;

///////////////////////////////////////////////////////////////////////////////
/**
 *  Konstruktor von @p GameObject.
 *
 *  @param[in] nop ObjektTyp
 *
 *  @author OLiver
 */
GameObject::GameObject(void) : obj_id(obj_id_counter++)
{
    // ein Objekt mehr
    ++obj_counter;
}

GameObject::GameObject(SerializedGameData* sgd, const unsigned obj_id) : obj_id(obj_id)
{
    // ein Objekt mehr
    ++obj_counter;
    sgd->AddObject(this);
}

GameObject::GameObject(const GameObject& go) : obj_id(go.obj_id)
{
    // ein Objekt mehr
    ++obj_counter;
}

void GameObject::Destroy()
{
}

void GameObject::Serialize(SerializedGameData* sgd) const
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
    // ein Objekt weniger
    --obj_counter;

    /*
    if (em)
    {
        // only for debugging purposes
        em->RemoveAllEventsOfObject(this);
    }
    */
}
