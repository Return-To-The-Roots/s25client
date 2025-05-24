// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameData/TerrainDesc.h"

namespace rttr::mapGenerator {

uint8_t ByHumidity(const TerrainDesc& terrain);

bool IsWater(const TerrainDesc& terrain);

bool IsSwamp(const TerrainDesc& terrain);

bool IsLand(const TerrainDesc& terrain);

bool IsShipableWater(const TerrainDesc& terrain);

bool IsBuildableLand(const TerrainDesc& terrain);

bool IsCoastTerrain(const TerrainDesc& terrain);

bool IsBuildableCoast(const TerrainDesc& terrain);

bool IsBuildableMountain(const TerrainDesc& terrain);

bool IsMinableMountain(const TerrainDesc& terrain);

bool IsSnowOrLava(const TerrainDesc& terrain);

bool IsMountainOrSnowOrLava(const TerrainDesc& terrain);

} // namespace rttr::mapGenerator
