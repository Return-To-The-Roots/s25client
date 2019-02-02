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

#ifndef ToolNote_h__
#define ToolNote_h__

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

#endif // ToolNote_h__
