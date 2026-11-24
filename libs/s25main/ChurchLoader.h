// Copyright (C) 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameTypes/Nation.h"
#include <cstdint>

class GameWorldBase;
class GlobalGameSettings;
class ITexture;
class glArchivItem_Bitmap;

namespace churchaddon {

/// Placement radius around HQ/Storehouses where a church may be built (tiles)
constexpr unsigned CHURCH_PLACEMENT_RADIUS = 9u;
/// Maximum stored Blessings in a church
constexpr uint8_t MAX_CHURCH_BLESSINGS = 10u;
/// Game frames (~5 minutes at 50ms/GF) between church services
constexpr unsigned CHURCH_SERVICE_INTERVAL = 6000u;
/// Initial delay after church completion before the first service
constexpr unsigned CHURCH_FIRST_SERVICE_DELAY = 600u;
/// Duration of a church service in game frames once all visitors are inside
constexpr unsigned CHURCH_SERVICE_DURATION = 100u;
/// Safety timeout in game frames while waiting for invited visitors to arrive inside
constexpr unsigned CHURCH_MAX_GATHER_WAIT = 3000u;
/// Percentage of eligible workers/soldiers invited from each reachable building per service
constexpr unsigned CHURCH_ATTENDANCE_PERCENT = 20u;

/// Number of per-nation sprites in the "church" asset folder archive
constexpr unsigned NUM_SPRITES_PER_NATION = 6u;

/// Offset within each nation's sprite block inside the "church" asset folder archive
enum class NationSprite : unsigned
{
    Building = 0,
    BuildingShadow = 1,
    Skeleton = 2,
    SkeletonShadow = 3,
    Door = 4,
    Icon = 5,
};

/// Shared sprite indices in the "church" asset folder archive (after the 5 nation blocks)
enum class SharedSprite : unsigned
{
    Dove = 30,
    BlessingIcon = 31,
};

unsigned getNationSpriteId(Nation nation, NationSprite sprite);
int getDoveOffsetY(Nation nation);
ITexture* getIconTex(Nation nation);
glArchivItem_Bitmap* getIconBitmap(Nation nation);
ITexture* getDoveTex();
ITexture* getBlessingTex();
bool isAddonActive(const GlobalGameSettings& ggs);
bool isAddonActive(const GameWorldBase& gwb);

} // namespace churchaddon
