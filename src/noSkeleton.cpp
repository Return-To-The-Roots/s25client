// $Id: noSkeleton.cpp 9357 2014-04-25 15:35:25Z FloSoft $
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
#include "noSkeleton.h"

#include "Loader.h"
#include "macros.h"
#include "GameWorld.h"
#include "EventManager.h"
#include "Random.h"
#include "SerializedGameData.h"

///////////////////////////////////////////////////////////////////////////////
// Makros / Defines
#if defined _WIN32 && defined _DEBUG && defined _MSC_VER
#define new new(_NORMAL_BLOCK, THIS_FILE, __LINE__)
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

noSkeleton::noSkeleton(const unsigned short x, const unsigned short y)
    : noCoordBase(NOP_ENVIRONMENT, x, y),
      type(0), current_event(em->AddEvent(this, 15000 + RANDOM.Rand(__FILE__, __LINE__, obj_id, 10000)))
{
}

noSkeleton::~noSkeleton()
{
}

void noSkeleton::Destroy_noSkeleton()
{
    gwg->SetNO(NULL, x, y);

    // ggf Event abmelden
    if(current_event)
        em->RemoveEvent(current_event);

    Destroy_noCoordBase();
}

void noSkeleton::Serialize_noSkeleton(SerializedGameData* sgd) const
{
    Serialize_noCoordBase(sgd);

    sgd->PushUnsignedChar(type);
    sgd->PushObject(current_event, true);
}

noSkeleton::noSkeleton(SerializedGameData* sgd, const unsigned obj_id) : noCoordBase(sgd, obj_id),
    type(sgd->PopUnsignedChar()),
    current_event(sgd->PopObject<EventManager::Event>(GOT_EVENT))
{

}

void noSkeleton::Draw(int x, int y)
{
    LOADER.GetMapImageN(547 + type)->Draw(x, y, 0, 0, 0, 0, 0, 0);
}

void noSkeleton::HandleEvent(const unsigned int id)
{
    if(!type)
    {
        // weiter verwesen, dann später sterben nach ner zufälligen Zeit
        type = 1;
        current_event = em->AddEvent(this, 10000 + RANDOM.Rand(__FILE__, __LINE__, obj_id, 10000));
    }
    else
    {
        // ganz weg damit
        current_event = 0;
        em->AddToKillList(this);
    }
}
