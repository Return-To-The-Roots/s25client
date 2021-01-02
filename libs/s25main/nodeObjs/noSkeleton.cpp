// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
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

#include "noSkeleton.h"
#include "EventManager.h"
#include "Loader.h"
#include "SerializedGameData.h"
#include "ogl/glArchivItem_Bitmap.h"
#include "random/Random.h"
#include "world/GameWorldGame.h"

noSkeleton::noSkeleton(const MapPoint pos) : noCoordBase(NodalObjectType::Environment, pos), type(0)
{
    current_event = GetEvMgr().AddEvent(this, 15000 + RANDOM.Rand(__FILE__, __LINE__, GetObjId(), 10000));
}

noSkeleton::~noSkeleton() = default;

void noSkeleton::Destroy_noSkeleton()
{
    gwg->SetNO(pos, nullptr);

    // ggf Event abmelden
    if(current_event)
        GetEvMgr().RemoveEvent(current_event);

    Destroy_noCoordBase();
}

void noSkeleton::Serialize_noSkeleton(SerializedGameData& sgd) const
{
    Serialize_noCoordBase(sgd);

    sgd.PushUnsignedChar(type);
    sgd.PushEvent(current_event);
}

noSkeleton::noSkeleton(SerializedGameData& sgd, const unsigned obj_id)
    : noCoordBase(sgd, obj_id), type(sgd.PopUnsignedChar()), current_event(sgd.PopEvent())
{}

void noSkeleton::Draw(DrawPoint drawPt)
{
    LOADER.GetMapImageN(547 + type)->DrawFull(drawPt);
}

void noSkeleton::HandleEvent(const unsigned /*id*/)
{
    if(!type)
    {
        // weiter verwesen, dann später sterben nach ner zufälligen Zeit
        type = 1;
        current_event = GetEvMgr().AddEvent(this, 10000 + RANDOM.Rand(__FILE__, __LINE__, GetObjId(), 10000));
    } else
    {
        // ganz weg damit
        current_event = nullptr;
        GetEvMgr().AddToKillList(this);
    }
}
