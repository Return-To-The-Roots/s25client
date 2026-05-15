// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "MineResourceBehavior.h"
#include "GlobalGameSettings.h"
#include "addons/const_addons.h"
#include "gameTypes/BuildingType.h"

AddonId GetMineResourceBehaviorAddonId(const BuildingType buildingType)
{
    switch(buildingType)
    {
        case BuildingType::GoldMine: return AddonId::GOLDMINE_RESOURCE_BEHAVIOR;
        case BuildingType::IronMine: return AddonId::IRONMINE_RESOURCE_BEHAVIOR;
        case BuildingType::CoalMine: return AddonId::COALMINE_RESOURCE_BEHAVIOR;
        default: return AddonId::GRANITEMINE_RESOURCE_BEHAVIOR;
    }
}

MineResourceBehavior GetConfiguredMineResourceBehavior(const GlobalGameSettings& settings,
                                                       const BuildingType buildingType)
{
    switch(static_cast<MineResourceBehavior>(settings.getSelection(GetMineResourceBehaviorAddonId(buildingType))))
    {
        case MineResourceBehavior::S4LikeExhaustion: return MineResourceBehavior::S4LikeExhaustion;
        case MineResourceBehavior::Inexhaustible: return MineResourceBehavior::Inexhaustible;
        case MineResourceBehavior::WorkEverywhere: return MineResourceBehavior::WorkEverywhere;
        default: return MineResourceBehavior::Default;
    }
}

MineResourceBehavior GetEffectiveMineResourceBehavior(const GlobalGameSettings& settings,
                                                      const BuildingType buildingType)
{
    const MineResourceBehavior configuredBehavior = GetConfiguredMineResourceBehavior(settings, buildingType);
    if(configuredBehavior != MineResourceBehavior::Default)
        return configuredBehavior;

    if(buildingType == BuildingType::GraniteMine && settings.isEnabled(AddonId::GRANITEMINES_WORK_EVERYWHERE))
        return MineResourceBehavior::WorkEverywhere;

    if(settings.isEnabled(AddonId::INEXHAUSTIBLE_MINES))
        return MineResourceBehavior::Inexhaustible;

    return MineResourceBehavior::Default;
}

bool IsMineResourceDepletable(const GlobalGameSettings& settings, const BuildingType buildingType)
{
    const MineResourceBehavior configuredBehavior = GetConfiguredMineResourceBehavior(settings, buildingType);
    const MineResourceBehavior effectiveBehavior = GetEffectiveMineResourceBehavior(settings, buildingType);

    if(effectiveBehavior == MineResourceBehavior::Inexhaustible)
        return false;

    if(configuredBehavior == MineResourceBehavior::Default && settings.isEnabled(AddonId::INEXHAUSTIBLE_MINES))
        return false;

    if(configuredBehavior == MineResourceBehavior::Default && buildingType == BuildingType::GraniteMine
       && settings.isEnabled(AddonId::INEXHAUSTIBLE_GRANITEMINES))
        return false;

    return true;
}
