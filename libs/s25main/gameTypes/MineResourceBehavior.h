// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "helpers/OptionalEnum.h"
#include "gameTypes/MapCoordinates.h"
#include <cstdint>

class GlobalGameSettings;
class GameWorld;
enum class AddonId;
enum class BuildingType : unsigned char;
enum class ResourceType : uint8_t;

enum class MineResourceBehavior
{
    Default,
    Inexhaustible,
    S4LikeExhaustion,
    WorkEverywhere
};
constexpr auto maxEnumValue(MineResourceBehavior)
{
    return MineResourceBehavior::WorkEverywhere;
}

AddonId GetMineResourceBehaviorAddonId(BuildingType buildingType);
ResourceType GetMineResourceType(BuildingType buildingType);
helpers::OptionalEnum<BuildingType> GetMineBuildingType(ResourceType resourceType);
unsigned GetRemainingMineResources(const GameWorld& world, MapPoint pos, ResourceType resourceType);
unsigned GetS4LikeMineProductionChance(unsigned remainingMatchingResources);
MineResourceBehavior GetMineResourceBehavior(const GlobalGameSettings& settings, BuildingType buildingType);
bool IsMineResourceDepletable(const GlobalGameSettings& settings, BuildingType buildingType);
