// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include <cstdint>

// Targets for ingame chat
enum class ChatDestination : uint8_t
{
    System,
    All,
    Allies,
    Enemies
};

constexpr auto maxEnumValue(ChatDestination)
{
    return ChatDestination::Enemies;
}
