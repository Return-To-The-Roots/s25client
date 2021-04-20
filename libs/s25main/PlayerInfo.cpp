// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "PlayerInfo.h"
#include "s25util/Serializer.h"

PlayerInfo::PlayerInfo() : isHost(false), ping(0) {}

PlayerInfo::PlayerInfo(const BasePlayerInfo& baseInfo) : BasePlayerInfo(baseInfo), isHost(false), ping(0) {}

PlayerInfo::PlayerInfo(Serializer& ser) : BasePlayerInfo(ser, false), isHost(ser.PopBool()), ping(ser.PopUnsignedInt())
{}

void PlayerInfo::Serialize(Serializer& ser) const
{
    BasePlayerInfo::Serialize(ser, false);
    ser.PushBool(isHost);
    ser.PushUnsignedInt(ping);
}
