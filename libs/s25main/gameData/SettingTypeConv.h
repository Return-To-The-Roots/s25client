// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "helpers/EnumArray.h"
#include "gameTypes/GoodTypes.h"
#include "gameTypes/SettingsTypes.h"

/// Scaling (max values) of each military setting
extern const MilitarySettings MILITARY_SETTINGS_SCALE;
/// Standard priority of each ware
extern const TransportPriorities STD_TRANSPORT_PRIO;

/// Get the priority of a given good from the ordering of goods (good categories)
unsigned GetTransportPrioFromOrdering(const TransportOrders& ordering, GoodType good);
/// Converts the transport priorities to an odering of goods (good categories)
TransportOrders GetOrderingFromTransportPrio(const TransportPriorities& priorities);
