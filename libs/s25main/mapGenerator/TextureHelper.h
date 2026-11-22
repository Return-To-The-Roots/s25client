// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameData/TerrainDesc.h"

namespace rttr::mapGenerator {

uint8_t byHumidity(const TerrainDesc& terrain);

bool isWater(const TerrainDesc& terrain);

bool isSwamp(const TerrainDesc& terrain);

/// Terrain which can neither be entered by settlers nor by ships, e.g. swamp
bool isImpassable(const TerrainDesc& terrain);

bool isLand(const TerrainDesc& terrain);

bool isShipableWater(const TerrainDesc& terrain);

bool isBuildableLand(const TerrainDesc& terrain);

bool isCoastTerrain(const TerrainDesc& terrain);

bool isBuildableCoast(const TerrainDesc& terrain);

bool isBuildableMountain(const TerrainDesc& terrain);

bool isMinableMountain(const TerrainDesc& terrain);

bool isSnowOrLava(const TerrainDesc& terrain);

bool isMountainOrSnowOrLava(const TerrainDesc& terrain);

} // namespace rttr::mapGenerator
