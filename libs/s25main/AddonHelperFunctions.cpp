// Copyright (C) 2005 - 2025 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "AddonHelperFunctions.h"
#include "GlobalGameSettings.h"
#include <LeatherLoader.h>
#include <WineLoader.h>
#include <addons/Addon.h>

std::function<bool(const BuildingType type)> makeIsUnusedBuilding(const GlobalGameSettings& ggs)
{
    const bool wineAddonActive = ggs.isEnabled(AddonId::WINE);
    const bool charburnerAddonActive = ggs.isEnabled(AddonId::CHARBURNER);
    const bool leatherAddonActive = ggs.isEnabled(AddonId::LEATHER);

    return [wineAddonActive, charburnerAddonActive, leatherAddonActive](BuildingType const& bld) {
        if(!wineAddonActive && wineaddon::isWineAddonBuildingType(bld))
            return true;
        if(!charburnerAddonActive && bld == BuildingType::Charburner)
            return true;
        if(!leatherAddonActive && leatheraddon::isLeatherAddonBuildingType(bld))
            return true;
        return false;
    };
}

std::function<bool(const GoodType type)> makeIsUnusedWare(const GlobalGameSettings& ggs)
{
    const bool wineAddonActive = ggs.isEnabled(AddonId::WINE);
    const bool leatherAddonActive = ggs.isEnabled(AddonId::LEATHER);

    return [wineAddonActive, leatherAddonActive](GoodType const& type) {
        if(!wineAddonActive && wineaddon::isWineAddonGoodType(type))
            return true;
        if(!leatherAddonActive && leatheraddon::isLeatherAddonGoodType(type))
            return true;
        return false;
    };
}

std::function<bool(const Job job)> makeIsUnusedJob(const GlobalGameSettings& ggs)
{
    const bool wineAddonActive = ggs.isEnabled(AddonId::WINE);
    const bool charburnerAddonActive = ggs.isEnabled(AddonId::CHARBURNER);
    const bool leatherAddonActive = ggs.isEnabled(AddonId::LEATHER);

    return [wineAddonActive, charburnerAddonActive, leatherAddonActive](Job const& job) {
        if(!wineAddonActive && wineaddon::isWineAddonJobType(job))
            return true;
        if(!charburnerAddonActive && job == Job::CharBurner)
            return true;
        if(!leatherAddonActive && leatheraddon::isLeatherAddonJobType(job))
            return true;
        return false;
    };
}
