// Copyright (C) 2016 - 2021 Settlers Freaks <sf-team at siedler25.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "LanGameInfo.h"
#include "s25util/Serializer.h"

bool LanGameInfo::Serialize(Serializer& serializer)
{
    if(name.size() > 64)
        name.resize(64);
    if(map.size() > 64)
        map.resize(64);
    if(version.size() > 16)
        version.resize(16);
    serializer.PushString(name);
    serializer.PushBool(hasPwd);
    serializer.PushString(map);
    serializer.PushUnsignedChar(curNumPlayers);
    serializer.PushUnsignedChar(maxNumPlayers);
    serializer.PushUnsignedShort(port);
    serializer.PushBool(isIPv6);
    serializer.PushString(version);
    serializer.PushString(revision);
    return true;
}

bool LanGameInfo::Deserialize(Serializer& serializer)
{
    name = serializer.PopString();
    hasPwd = serializer.PopBool();
    map = serializer.PopString();
    curNumPlayers = serializer.PopUnsignedChar();
    maxNumPlayers = serializer.PopUnsignedChar();
    port = serializer.PopUnsignedShort();
    isIPv6 = serializer.PopBool();
    version = serializer.PopString();
    revision = serializer.PopString();
    return true;
}
