// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "ClientError.h"
#include "mygettext/mygettext.h"

const char* ClientErrorToStr(ClientError error)
{
    switch(error)
    {
        case ClientError::InvalidMessage: return _("Server sent an invalid message!");
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
