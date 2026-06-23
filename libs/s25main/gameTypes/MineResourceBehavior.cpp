// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "MineResourceBehavior.h"
#include "GlobalGameSettings.h"
#include "addons/const_addons.h"
#include "world/GameWorld.h"
#include "gameTypes/BuildingType.h"
#include "gameTypes/MapCoordinates.h"
#include "gameTypes/Resource.h"
#include "gameData/GameConsts.h"
#include <algorithm>

namespace {
constexpr unsigned MAX_PRODUCTION_PERCENT = 100;
constexpr unsigned S4LIKE_FULL_PRODUCTIVITY_RESOURCE_AMOUNT = 20;
} // namespace

AddonId GetMineResourceBehaviorAddonId(const BuildingType buildingType)
{
    switch(buildingType)
    {
        case BuildingType::GoldMine: return AddonId::GOLDMINE_RESOURCE_BEHAVIOR;
        case BuildingType::IronMine: return AddonId::IRONMINE_RESOURCE_BEHAVIOR;
        case BuildingType::CoalMine: return AddonId::COALMINE_RESOURCE_BEHAVIOR;
        default: return AddonId::INEXHAUSTIBLE_GRANITEMINES;
    }
}

ResourceType GetMineResourceType(const BuildingType buildingType)
{
    switch(buildingType)
    {
        case BuildingType::GoldMine: return ResourceType::Gold;
        case BuildingType::IronMine: return ResourceType::Iron;
        case BuildingType::CoalMine: return ResourceType::Coal;
        default: return ResourceType::Granite;
    }
}

helpers::OptionalEnum<BuildingType> GetMineBuildingType(const ResourceType resourceType)
{
    switch(resourceType)
    {
        case ResourceType::Gold: return BuildingType::GoldMine;
        case ResourceType::Iron: return BuildingType::IronMine;
        case ResourceType::Coal: return BuildingType::CoalMine;
        case ResourceType::Granite: return BuildingType::GraniteMine;
        default: return boost::none;
    }
}

unsigned GetRemainingMineResources(const GameWorld& world, const MapPoint pos, const ResourceType resourceType)
{
    unsigned resourceAmount = 0;
    const auto resourcePts = world.GetMatchingPointsInRadius(
      pos, MINER_RADIUS,
      [&world, resourceType](const MapPoint pt) { return world.GetNode(pt).resources.has(resourceType); }, true);
    for(const MapPoint pt : resourcePts)
        resourceAmount += world.GetNode(pt).resources.getAmount();
    return resourceAmount;
}

unsigned GetS4LikeMineFullProductivityResourceAmount()
{
    return S4LIKE_FULL_PRODUCTIVITY_RESOURCE_AMOUNT;
}

unsigned GetS4LikeMineProductionChance(const unsigned remainingMatchingResources)
{
    // S4-like productivity is intentionally based on a 20-resource reference amount, not the theoretical maximum
    // resources in the mine radius. Below that amount, the production chance degrades linearly.
    const unsigned chancePercent =
      remainingMatchingResources * MAX_PRODUCTION_PERCENT / GetS4LikeMineFullProductivityResourceAmount();
    return std::min(MAX_PRODUCTION_PERCENT, chancePercent);
}

MineResourceBehavior GetMineResourceBehavior(const GlobalGameSettings& settings, const BuildingType buildingType)
{
    const unsigned selection = settings.getSelection(GetMineResourceBehaviorAddonId(buildingType));
    if(!helpers::isValidEnumValue<MineResourceBehavior>(selection))
        return MineResourceBehavior::Default;
    return static_cast<MineResourceBehavior>(selection);
}

bool IsMineResourceDepletable(const GlobalGameSettings& settings, const BuildingType buildingType)
{
    const MineResourceBehavior behavior = GetMineResourceBehavior(settings, buildingType);
    return behavior == MineResourceBehavior::Default || behavior == MineResourceBehavior::S4LikeExhaustion;
}
