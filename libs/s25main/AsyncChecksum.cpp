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

#include "AsyncChecksum.h"
#include "EventManager.h"
#include "FileChecksum.h"
#include "Game.h"
#include "GameObject.h"
#include "random/Random.h"
#include "s25util/Serializer.h"

AsyncChecksum::AsyncChecksum() : randChecksum(0), objCt(0), objIdCt(0), eventCt(0), evInstanceCt(0) {}

AsyncChecksum::AsyncChecksum(unsigned randChecksum, unsigned objCt, unsigned objIdCt, unsigned eventCt,
                             unsigned evInstanceCt)
    : randChecksum(randChecksum), objCt(objCt), objIdCt(objIdCt), eventCt(eventCt), evInstanceCt(evInstanceCt)
{}

void AsyncChecksum::Serialize(Serializer& ser) const
{
    ser.PushUnsignedInt(randChecksum);
    ser.PushUnsignedInt(objCt);
    ser.PushUnsignedInt(objIdCt);
    ser.PushUnsignedInt(eventCt);
    ser.PushUnsignedInt(evInstanceCt);
}

void AsyncChecksum::Deserialize(Serializer& ser)
{
    randChecksum = ser.PopUnsignedInt();
    objCt = ser.PopUnsignedInt();
    objIdCt = ser.PopUnsignedInt();
    eventCt = ser.PopUnsignedInt();
    evInstanceCt = ser.PopUnsignedInt();
}

unsigned AsyncChecksum::getHash() const
{
    Serializer ser;
    Serialize(ser);
    return CalcChecksumOfBuffer(ser.GetData(), ser.GetLength());
}

AsyncChecksum AsyncChecksum::create(const Game& game)
{
    return AsyncChecksum(RANDOM.GetChecksum(), GameObject::GetNumObjs(), GameObject::GetObjIDCounter(),
                         game.em_->GetNumActiveEvents(), game.em_->GetEventInstanceCtr());
}
