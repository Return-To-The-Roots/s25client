// Copyright (c) 2005 - 2020 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

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
