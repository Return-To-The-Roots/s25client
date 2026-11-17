// Copyright (C) 2005 - 2021 Settlers Freaks <sf-team at siedler25.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "LanGameInfo.h"
#include "s25util/Serializer.h"

bool LanGameInfo::serialize(Serializer& serializer)
{
    if(name.size() > 64)
        name.resize(64);
    if(map.size() > 64)
        map.resize(64);
    if(version.size() > 16)
        version.resize(16);
    serializer.pushString(name);
    serializer.pushBool(hasPwd);
    serializer.pushString(map);
    serializer.pushUnsignedChar(curNumPlayers);
    serializer.pushUnsignedChar(maxNumPlayers);
    serializer.pushUnsignedShort(port);
    serializer.pushBool(isIPv6);
    serializer.pushString(version);
    serializer.pushString(revision);
    return true;
}

bool LanGameInfo::deserialize(Serializer& serializer)
{
    name = serializer.popString();
    hasPwd = serializer.popBool();
    map = serializer.popString();
    curNumPlayers = serializer.popUnsignedChar();
    maxNumPlayers = serializer.popUnsignedChar();
    port = serializer.popUnsignedShort();
    isIPv6 = serializer.popBool();
    version = serializer.popString();
    revision = serializer.popString();
    return true;
}
