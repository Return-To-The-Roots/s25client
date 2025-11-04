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
