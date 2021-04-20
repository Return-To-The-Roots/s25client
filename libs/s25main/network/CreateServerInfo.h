// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameTypes/ServerType.h"
#include <string>
#include <utility>

/// Data struct for game creation
struct CreateServerInfo
{
    const ServerType type;
    const uint16_t port;
    const std::string gameName;
    const std::string password;
    const bool ipv6; // IPv6 or IPv4
    const bool use_upnp;
    CreateServerInfo(ServerType type, uint16_t port, std::string gameName, std::string password = "", bool ipv6 = false,
                     bool useUpnp = false)
        : type(type), port(port), gameName(std::move(gameName)), password(std::move(password)), ipv6(ipv6),
          use_upnp(useUpnp)
    {}
};
