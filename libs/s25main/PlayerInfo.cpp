// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "PlayerInfo.h"
#include "s25util/Serializer.h"

PlayerInfo::PlayerInfo() : isHost(false), ping(0) {}

PlayerInfo::PlayerInfo(const BasePlayerInfo& baseInfo) : BasePlayerInfo(baseInfo), isHost(false), ping(0) {}

PlayerInfo::PlayerInfo(Serializer& ser, int basePlayerInfoVersion)
    : BasePlayerInfo(ser, basePlayerInfoVersion, false), isHost(ser.popBool()), ping(ser.popUnsignedInt())
{}

void PlayerInfo::serialize(Serializer& ser) const
{
    BasePlayerInfo::serialize(ser, false);
    ser.pushBool(isHost);
    ser.pushUnsignedInt(ping);
}
