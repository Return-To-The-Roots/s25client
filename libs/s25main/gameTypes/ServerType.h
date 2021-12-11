// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

// Servertypen
enum class ServerType : uint16_t
{
    Lobby,
    Direct,
    Local,
    LAN
};
constexpr auto maxEnumValue(ServerType)
{
    return ServerType::LAN;
}