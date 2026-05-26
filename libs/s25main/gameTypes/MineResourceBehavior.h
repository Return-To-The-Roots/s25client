// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "helpers/OptionalEnum.h"
#include <cstdint>

class GlobalGameSettings;
enum class AddonId;
enum class BuildingType : unsigned char;
enum class ResourceType : uint8_t;

enum class MineResourceBehavior : unsigned
{
    Default = 0,
    Inexhaustible = 1,
    S4LikeExhaustion = 2,
    WorkEverywhere = 3
};

AddonId GetMineResourceBehaviorAddonId(BuildingType buildingType);
ResourceType GetMineResourceType(BuildingType buildingType);
helpers::OptionalEnum<BuildingType> GetMineBuildingType(ResourceType resourceType);
/// Remaining matching resources at which S4-like mines reach full productivity.
unsigned GetS4LikeMineFullProductivityResourceAmount();
unsigned GetS4LikeMineProductionChance(unsigned remainingMatchingResources);
MineResourceBehavior GetMineResourceBehavior(const GlobalGameSettings& settings, BuildingType buildingType);
bool IsMineResourceDepletable(const GlobalGameSettings& settings, BuildingType buildingType);
