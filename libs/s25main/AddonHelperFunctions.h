// Copyright (C) 2005 - 2025 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "GamePlayer.h"
#include "GlobalGameSettings.h"
#include "world/GameWorld.h"
#include <LeatherLoader.h>
#include <WineLoader.h>
#include <addons/Addon.h>
#include <gameData/BuildingConsts.h>

auto isUnusedBuilding(GamePlayer const& player)
{
    return 
        [&player = player](BuildingType const& bld) {
        if(!wineaddon::isAddonActive(player.GetGameWorld()) && wineaddon::isWineAddonBuildingType(bld))
            return true;
        if(!player.GetGameWorld().GetGGS().isEnabled(AddonId::CHARBURNER) && bld == BuildingType::Charburner)
            return true;
        if(!leatheraddon::isAddonActive(player.GetGameWorld()) && leatheraddon::isLeatherAddonBuildingType(bld))
            return true;
        return false;
    };
}

auto isUnusedWare(GamePlayer const& player)
{
    return [&player = player](GoodType const& type) {
        if(!wineaddon::isAddonActive(player.GetGameWorld()) && wineaddon::isWineAddonGoodType(type))
            return true;
        if(!leatheraddon::isAddonActive(player.GetGameWorld()) && leatheraddon::isLeatherAddonGoodType(type))
            return true;
        return false;
    };
}

auto isUnusedJob(GamePlayer const& player)
{
    return [&](Job const& job) {
        if(!wineaddon::isAddonActive(player.GetGameWorld()) && wineaddon::isWineAddonJobType(job))
            return true;
        if(!player.GetGameWorld().GetGGS().isEnabled(AddonId::CHARBURNER) && job == Job::CharBurner)
            return true;
        if(!leatheraddon::isAddonActive(player.GetGameWorld()) && leatheraddon::isLeatherAddonJobType(job))
            return true;
        return false;
    };
}
