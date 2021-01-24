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

#pragma once

#include "helpers/EnumArray.h"
#include "s25util/warningSuppression.h"

// Note: This enums are constructed for performance and easy conversion.
// AIResource must be contiguous and it is assumed that only valid enumerators are used

/// Resources stored on AI nodes. Starts with all values from AIResource in that order!
enum class AINodeResource
{
    Gold,
    Ironore,
    Coal,
    Granite,
    Fish,
    Wood,
    Stones,
    Plantspace,
    Borderland,
    // special:
    Multiple,
    Blocked,
    Nothing,
};

/// Resources handled by AI. Keep in sync with AINodeResource
enum class AIResource
{
    Gold,
    Ironore,
    Coal,
    Granite,
    Fish,
    Wood,
    Stones,
    Plantspace,
    Borderland,
};

constexpr auto maxEnumValue(AIResource)
{
    return AIResource::Borderland;
}

/// AI resources which can be above the surface
enum class AISurfaceResource
{
    Wood = static_cast<unsigned>(AINodeResource::Wood),
    Stones,
    Blocked = static_cast<unsigned>(AINodeResource::Blocked),
    Nothing = static_cast<unsigned>(AINodeResource::Nothing),
};

/// AI resources which can be above the surface
enum class AISubSurfaceResource
{
    Gold,
    Ironore,
    Coal,
    Granite,
    Fish,
    Nothing = static_cast<unsigned>(AINodeResource::Nothing),
};

/// Mapping functions, valid by construction of the enums
constexpr AINodeResource convertToNodeResource(AIResource res)
{
    return static_cast<AINodeResource>(static_cast<unsigned>(res));
}
constexpr AINodeResource convertToNodeResource(AISurfaceResource res)
{
    return static_cast<AINodeResource>(static_cast<unsigned>(res));
}
constexpr AINodeResource convertToNodeResource(AISubSurfaceResource res)
{
    return static_cast<AINodeResource>(static_cast<unsigned>(res));
}

constexpr bool operator==(AINodeResource lhs, AIResource rhs)
{
    return lhs == convertToNodeResource(rhs);
}

constexpr helpers::EnumArray<unsigned, AIResource> SUPPRESS_UNUSED RES_RADIUS = {
  2, // Gold
  2, // Ironore
  2, // Coal
  2, // Granite
  5, // Fish
  8, // Wood
  8, // Stones
  3, // Plantspace
  5, // Borderland
};
