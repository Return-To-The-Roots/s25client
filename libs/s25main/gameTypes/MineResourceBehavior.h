// Copyright (C) 2005 - 2026 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

class GlobalGameSettings;
enum class AddonId;
enum class BuildingType : unsigned char;

enum class MineResourceBehavior : unsigned
{
    Default,
    S4LikeExhaustion,
    Inexhaustible,
    WorkEverywhere
};

AddonId GetMineResourceBehaviorAddonId(BuildingType buildingType);
MineResourceBehavior GetConfiguredMineResourceBehavior(const GlobalGameSettings& settings, BuildingType buildingType);
MineResourceBehavior GetEffectiveMineResourceBehavior(const GlobalGameSettings& settings, BuildingType buildingType);
bool IsMineResourceDepletable(const GlobalGameSettings& settings, BuildingType buildingType);
