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

#ifndef CreateServerInfo_h__
#define CreateServerInfo_h__

#include "gameTypes/ServerType.h"
#include <string>

/// Struktur zur Weitergabe der Spiel-Eröffnungsdaten
struct CreateServerInfo
{
    ServerType type;      /// Typ des Servers.
    unsigned short port;  /// Port des Servers
    std::string gamename; /// Name des Servers.
    std::string password; /// Passwort des Servers.
    bool ipv6;            /// Soll IPv6 verwendet werden?
    bool use_upnp;
};

#endif // CreateServerInfo_h__
