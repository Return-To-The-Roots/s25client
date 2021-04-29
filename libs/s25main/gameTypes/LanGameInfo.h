// Copyright (C) 2005 - 2021 Settlers Freaks <sf-team at siedler25.org>
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <string>

class Serializer;

struct LanGameInfo
{
    std::string name;
    bool hasPwd;
    std::string map;
    uint8_t curNumPlayers, maxNumPlayers;
    uint16_t port;
    bool isIPv6;
    std::string version;
    std::string revision;

    bool Serialize(Serializer& serializer);
    bool Deserialize(Serializer& serializer);
};
