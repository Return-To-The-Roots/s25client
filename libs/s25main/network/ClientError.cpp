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
        case CE_INCOMPLETE_MESSAGE: return _("Too short Message received!");
        case CE_SERVER_FULL: return _("This Server is full!");
        case CE_WRONG_PW: return _("Wrong Password!");
        case CE_CONNECTION_LOST: return _("Lost connection to server!");
        case CE_INVALID_SERVERTYPE: return _("Wrong Server Type!");
        case CE_MAP_TRANSMISSION: return _("Map transmission was corrupt!");
        case CE_WRONG_VERSION: return _("Wrong client version");
        case CE_INVALID_MAP: return _("Map is invalid or failed to load properly!");
        default: return _("Unknown error!");
    }
}
