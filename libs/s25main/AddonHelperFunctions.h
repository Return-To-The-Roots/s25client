// Copyright (C) 2005 - 2025 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameTypes/BuildingType.h"
#include "gameTypes/GoodTypes.h"
#include "gameTypes/JobTypes.h"
#include <functional>

class GlobalGameSettings;

std::function<bool(const BuildingType type)> makeIsUnusedBuilding(const GlobalGameSettings& ggs);
std::function<bool(const GoodType type)> makeIsUnusedWare(const GlobalGameSettings& ggs);
std::function<bool(const Job job)> makeIsUnusedJob(const GlobalGameSettings& ggs);

// Only used for deserialization, can be removed when breaking compatibility (GetGameDataVersion)
constexpr auto numLeatherAddonBuildings = 3u;
constexpr auto numWineAddonBuildings = 3u;
constexpr auto numWineAndLeatherAddonBuildings = numWineAddonBuildings + numLeatherAddonBuildings;

constexpr auto numLeatherAddonGoods = 3u;
constexpr auto numWineAddonGoods = 2u;
constexpr auto numWineAndLeatherAddonGoods = numWineAddonGoods + numLeatherAddonGoods;

constexpr auto numLeatherAddonJobs = 3u;
constexpr auto numWineAddonJobs = 3u;
constexpr auto numWineAndLeatherAddonJobs = numWineAddonJobs + numLeatherAddonJobs;

constexpr uint8_t transportPrioOfLeatherworks = 7;
