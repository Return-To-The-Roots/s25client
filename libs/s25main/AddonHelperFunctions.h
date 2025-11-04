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
constexpr int leatherAddonBuildings = 3;
constexpr int wineAddonBuildings = 3;
constexpr int wineAndLeatherAddonBuildings = wineAddonBuildings + leatherAddonBuildings;

constexpr int leatherAddonGoods = 3;
constexpr int wineAddonGoods = 2;
constexpr int wineAndLeatherAddonGoods = wineAddonGoods + leatherAddonGoods;

constexpr int leatherAddonJobs = 3;
constexpr int wineAddonJobs = 3;
constexpr int wineAndLeatherAddonJobs = wineAddonJobs + leatherAddonJobs;

constexpr uint8_t transportPrioOfLeatherworks = 7;
