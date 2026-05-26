// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "MineResourceBehavior.h"
#include "GlobalGameSettings.h"
#include "addons/const_addons.h"
#include "gameTypes/BuildingType.h"
#include "gameTypes/Resource.h"
#include <algorithm>

namespace {
constexpr unsigned MAX_PRODUCTION_PERCENT = 100;
/// S4-like mines scale output chance against a 20-resource full-productivity reference capacity.
constexpr unsigned S4LIKE_RESOURCE_AMOUNT_FOR_FULL_PRODUCTION = 20;
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

unsigned GetS4LikeMineProductionChance(const unsigned resourceAmount)
{
    return std::min(MAX_PRODUCTION_PERCENT,
                    resourceAmount * MAX_PRODUCTION_PERCENT / S4LIKE_RESOURCE_AMOUNT_FOR_FULL_PRODUCTION);
}

MineResourceBehavior GetMineResourceBehavior(const GlobalGameSettings& settings, const BuildingType buildingType)
{
    switch(static_cast<MineResourceBehavior>(settings.getSelection(GetMineResourceBehaviorAddonId(buildingType))))
    {
        case MineResourceBehavior::Inexhaustible: return MineResourceBehavior::Inexhaustible;
        case MineResourceBehavior::S4LikeExhaustion: return MineResourceBehavior::S4LikeExhaustion;
        case MineResourceBehavior::WorkEverywhere: return MineResourceBehavior::WorkEverywhere;
        default: return MineResourceBehavior::Default;
    }
}

bool IsMineResourceDepletable(const GlobalGameSettings& settings, const BuildingType buildingType)
{
    const MineResourceBehavior behavior = GetMineResourceBehavior(settings, buildingType);
    return behavior == MineResourceBehavior::Default || behavior == MineResourceBehavior::S4LikeExhaustion;
}
