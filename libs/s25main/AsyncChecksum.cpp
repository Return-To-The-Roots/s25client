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

void AsyncChecksum::serialize(Serializer& ser) const
{
    ser.pushUnsignedInt(randChecksum);
    ser.pushUnsignedInt(objCt);
    ser.pushUnsignedInt(objIdCt);
    ser.pushUnsignedInt(eventCt);
    ser.pushUnsignedInt(evInstanceCt);
}

void AsyncChecksum::deserialize(Serializer& ser)
{
    randChecksum = ser.popUnsignedInt();
    objCt = ser.popUnsignedInt();
    objIdCt = ser.popUnsignedInt();
    eventCt = ser.popUnsignedInt();
    evInstanceCt = ser.popUnsignedInt();
}

unsigned AsyncChecksum::getHash() const
{
    Serializer ser;
    serialize(ser);
    return calcChecksumOfBuffer(ser.getData(), ser.getLength());
}

AsyncChecksum AsyncChecksum::create(const Game& game)
{
    return AsyncChecksum(RANDOM.getChecksum(), GameObject::getNumObjs(), GameObject::getObjIDCounter(),
                         game.em_->getNumActiveEvents(), game.em_->getEventInstanceCtr());
}

std::ostream& operator<<(std::ostream& os, const AsyncChecksum& checksum)
{
    return os << "RandCS = " << checksum.randChecksum << ",\tobjects/ID = " << checksum.objCt << "/" << checksum.objIdCt
              << ",\tevents/ID = " << checksum.eventCt << "/" << checksum.evInstanceCt;
}
