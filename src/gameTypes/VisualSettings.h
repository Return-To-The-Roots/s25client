// Copyright (c) 2005 - 2016 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef VisualSettings_h__
#define VisualSettings_h__

#include "gameTypes/SettingsTypes.h"

struct VisualSettings
{
    /// Verteilung
    Distributions distribution;
    /// Art der Reihenfolge (0 = nach Auftraggebung, ansonsten nach build_order)
    unsigned char order_type;
    /// Baureihenfolge
    BuildOrders build_order;
    /// Transport-Reihenfolge
    TransportOrders transport_order;
    /// Militäreinstellungen (die vom Militärmenü)
    boost::array<unsigned char, MILITARY_SETTINGS_COUNT> military_settings;
    /// Werkzeugeinstellungen (in der Reihenfolge wie im Fenster!)
    ToolSettings tools_settings;
};

#endif // VisualSettings_h__
