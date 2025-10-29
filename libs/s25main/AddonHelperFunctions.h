// Copyright (C) 2005 - 2025 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "GamePlayer.h"

auto isUnusedBuilding(GamePlayer const& player) -> std::function<bool(BuildingType const& type)>;
auto isUnusedWare(GamePlayer const& player) -> std::function<bool(GoodType const& type)>;
auto isUnusedJob(GamePlayer const& player) -> std::function<bool(Job const& job)>;
