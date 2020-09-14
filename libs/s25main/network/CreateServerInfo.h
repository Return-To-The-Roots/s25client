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
        : type(type), port(port), gameName(std::move(gameName)), password(std::move(password)), ipv6(ipv6), use_upnp(useUpnp)
    {}
};
