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

#include "ClientError.h"
#include "mygettext/mygettext.h"

const char* ClientErrorToStr(ClientError error)
{
    switch(error)
    {
        case ClientError::IncompleteMessage: return _("Too short Message received!");
        case ClientError::ServerFull: return _("This Server is full!");
        case ClientError::WrongPassword: return _("Wrong Password!");
        case ClientError::ConnectionLost: return _("Lost connection to server!");
        case ClientError::InvalidServerType: return _("Wrong Server Type!");
        case ClientError::MapTransmission: return _("Map transmission was corrupt!");
        case ClientError::WrongVersion: return _("Wrong client version");
        case ClientError::InvalidMap: return _("Map is invalid or failed to load properly!");
        default: return _("Unknown error!");
    }
}
