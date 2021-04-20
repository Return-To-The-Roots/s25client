// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later
//

#pragma once

/// Fehler, die vom Client gemeldet werden
enum class ClientError
{
    IncompleteMessage,
    ServerFull,
    WrongPassword,
    ConnectionLost,
    InvalidServerType,
    MapTransmission,
    WrongVersion,
    InvalidMap
};

const char* ClientErrorToStr(ClientError error);
