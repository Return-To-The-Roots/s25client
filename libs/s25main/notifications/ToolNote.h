// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "notifications/notifications.h"

struct ToolNote
{
    ENABLE_NOTIFICATION(ToolNote);

    enum Type
    {
        OrderPlaced,    // New order was placed
        OrderCompleted, // An ordered tool was produced
        SettingsChanged // Tool settings (production priority) has changed
    };

    ToolNote(Type type, unsigned player) : type(type), player(player) {}

    const Type type;
    const unsigned player;
};
