// $Id: iwDirectIPCreate.h 9357 2014-04-25 15:35:25Z FloSoft $
//
// Copyright (c) 2005 - 2011 Settlers Freaks (sf-team at siedler25.org)
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
#ifndef iwDIRECTIPCREATE_H_INCLUDED
#define iwDIRECTIPCREATE_H_INCLUDED

#pragma once

#include "IngameWindow.h"
#include "LobbyInterface.h"

/// Struktur zur Weitergabe der Spiel-Eröffnungsdaten
struct CreateServerInfo
{
    unsigned char type;    ///< Typ des Servers.
    unsigned short port;   ///< Port des Servers
    std::string gamename;  ///< Name des Servers.
    std::string password;  ///< Passwort des Servers.
    bool ipv6;             ///< Soll IPv6 verwendet werden?
    bool use_upnp;
};

class iwDirectIPCreate : public IngameWindow, public LobbyInterface
{
    public:
        iwDirectIPCreate(unsigned int server_type);

        void LC_Status_Error(std::string error);

    protected:
        void Msg_EditChange(const unsigned int ctrl_id);
        void Msg_EditEnter(const unsigned int ctrl_id);
        void Msg_ButtonClick(const unsigned int ctrl_id);
        void Msg_OptionGroupChange(const unsigned int ctrl_id, const unsigned short selection);

    private:
        void SetText(const std::string& text, unsigned int color, bool button);

    private:
        unsigned int server_type;
};

#endif // !iwDIRECTIPCREATE_H_INCLUDED
