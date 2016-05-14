// Copyright (c) 2005 - 2015 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef SettingsConst_h__
#define SettingsConst_h__

#include <boost/array.hpp>

#define NUM_DISTRIBUTION 23
#define NUM_BUILD_ORDERS 32
#define NUM_TRANSPORT_ORDERS 14
#define NUM_TOOL_SETTINGS 12

/// Custom mapping of (Ware, Receiver)-tuple to percentage of wares distributed to that building
typedef boost::array<unsigned char, NUM_DISTRIBUTION> Distributions;
typedef boost::array<unsigned char, NUM_BUILD_ORDERS> BuildOrders;
/// Mapping transport priority -> standard transport priority of ware(group):
/// E.g. std prio of coins = 0 -> TransportOrders[0] = stdPrio[COINS] = 0
/// New prio of coints = 1 -> TransportOrders[1] = stdPrio[COINS] = 0
typedef boost::array<unsigned char, NUM_TRANSPORT_ORDERS> TransportOrders;
typedef boost::array<unsigned char, NUM_TOOL_SETTINGS> ToolSettings;

/// Anzahl an Militäreinstellungen
const unsigned MILITARY_SETTINGS_COUNT = 8;

/// Skalierung der einzelnen Militäreinstellungen (maximale Werte)
const boost::array<unsigned, MILITARY_SETTINGS_COUNT> SUPPRESS_UNUSED MILITARY_SETTINGS_SCALE =
{{
    10,
    5,
    5,
    5,
    8,
    8,
    8,
    8
}};

#endif // SettingsConst_h__
