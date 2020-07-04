// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
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

#ifndef ShieldConsts_h__
#define ShieldConsts_h__

#include "gameTypes/GoodTypes.h"
#include "gameTypes/Nation.h"

/// Macht ggf. aus den verschiedenen Schilden der Nationen jeweils immer das römische normale Schild für
/// die Warensysteme usw
inline GoodType ConvertShields(const GoodType& good)
{
    return (good == GD_SHIELDVIKINGS || good == GD_SHIELDAFRICANS || good == GD_SHIELDJAPANESE) ? GD_SHIELDROMANS : good;
}

/// Umgekehrte Konvertierung: Gibt den Schildtyp für jede Nation an
const std::array<GoodType, NUM_NATIONS> SUPPRESS_UNUSED SHIELD_TYPES = {GD_SHIELDAFRICANS, GD_SHIELDJAPANESE, GD_SHIELDROMANS,
                                                                        GD_SHIELDVIKINGS, GD_SHIELDJAPANESE};

#endif // ShieldConsts_h__
