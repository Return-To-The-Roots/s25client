// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "notifications/notifications.h"
#include "gameTypes/MapCoordinates.h"

struct NodeNote
{
    ENABLE_NOTIFICATION(NodeNote);

    enum Type
    {
        Altitude, // Nodes altitude was changed
        BQ,       // Building quality
        Owner,
    };

    NodeNote(Type type, const MapPoint& pt) : type(type), pos(pt) {}

    const Type type;
    const MapPoint pos;
};
