// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameTypes/SettingsTypes.h"

struct VisualSettings
{
    /// Distribution of wares
    Distributions distribution;
    /// Use custom build order? (False = order of build issue time)
    bool useCustomBuildOrder;
    /// Custom build order
    BuildOrders build_order;
    /// Transport-Order
    TransportOrders transport_order;
    /// Military settings (in the menu)
    MilitarySettings military_settings;
    /// Priority of each tool
    ToolSettings tools_settings;
};
