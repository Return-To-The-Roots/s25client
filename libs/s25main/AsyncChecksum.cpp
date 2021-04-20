// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AsyncChecksum.h"
#include "EventManager.h"
#include "FileChecksum.h"
#include "Game.h"
#include "GameObject.h"
#include "random/Random.h"
#include "s25util/Serializer.h"
#include <ostream>

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

std::ostream& operator<<(std::ostream& os, const AsyncChecksum& checksum)
{
    return os << "RandCS = " << checksum.randChecksum << ",\tobjects/ID = " << checksum.objCt << "/" << checksum.objIdCt
              << ",\tevents/ID = " << checksum.eventCt << "/" << checksum.evInstanceCt;
}
