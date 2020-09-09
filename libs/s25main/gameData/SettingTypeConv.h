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

#ifndef SettingTypeConv_h__
#define SettingTypeConv_h__

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

#endif // SettingTypeConv_h__
