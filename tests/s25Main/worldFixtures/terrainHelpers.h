// Copyright (C) 2005 - 2024 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "gameData/DescIdx.h"
#include "gameData/TerrainDesc.h"

struct WorldDescription;

// Get unwalkable, but shippable water terrain
DescIdx<TerrainDesc> GetWaterTerrain(const WorldDescription& desc);
// Get land terrain with given additional property
DescIdx<TerrainDesc> GetLandTerrain(const WorldDescription& desc, ETerrain property = ETerrain::Walkable);
