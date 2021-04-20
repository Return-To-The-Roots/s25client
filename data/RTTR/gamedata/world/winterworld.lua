-- Copyright (C) 2018 - 2021 Settlers Freaks <sf-team at siedler25.org>
--
-- SPDX-License-Identifier: GPL-2.0-or-later

texFile = "<RTTR_GAME>/GFX/TEXTURES/TEX7.LBM";

rttr:AddLandscape{
	name = __"winterworld",
	mapGfx = "<RTTR_GAME>/DATA/MAP_2_Z.LST",
	s2Id = 2,
	isWinter = true,
	roads = {
		normal = {
			texture = texFile,
			pos = {192, 0, 64, 16}
		},
		upgraded = {
			texture = texFile,
			pos = {192, 16, 64, 16}
		},
		boat = {
			texture = texFile,
			pos = {192, 32, 64, 16}
		},
		mountain = {
			texture = texFile,
			pos = {192, 160, 64, 16}
		}
	}
}

rttr:AddTerrainEdge{
	-- Name used to reference this
	name = "ww_snow",
	-- Landscape to which this applies by default
	landscape = "winterworld",
	-- Filename of the texture image
	texture = texFile,
	-- Position and size {x, y, w, h} in the image if it contains multiple textures
	-- Can be left out. A size of 0 (w and/or h) is interpreted as the remaining image
	pos = {192, 176, 64, 16}
}
rttr:AddTerrainEdge{
	name = "ww_mountain",
	landscape = "winterworld",
	texture = texFile,
	pos = {192, 192, 64, 16}
}
rttr:AddTerrainEdge{
	name = "ww_ice",
	landscape = "winterworld",
	texture = texFile,
	pos = {192, 208, 64, 16}
}
rttr:AddTerrainEdge{
	name = "ww_tundra",
	landscape = "winterworld",
	texture = texFile,
	pos = {192, 224, 64, 16}
}
rttr:AddTerrainEdge{
	name = "ww_water",
	landscape = "winterworld",
	texture = texFile,
	pos = {192, 240, 64, 16}
}

rttr:AddTerrain{
	-- Name used to reference this
	name = "ww_iceFloe",
	-- Landscape to which this applies by default
	landscape = "winterworld",
	-- Name of the edge drawn over neighbouring terrain or "none"
	edgeType = "ww_water",
	-- Id used in the original S2. Defaults to 0xFF (not in S2)
	s2Id = 2,
	-- If this is higher than the neighbours edgePriority then it draws over the neighbour
	-- Valid = [-128, 127], defaults to 0
	edgePriority = 73,
	-- What kind of terrain is this? (Used for animals, ships, etc)
	-- Valid = land (default), water, lava, snow, mountain
	kind = "land",
	-- Property for this terrain. 
	-- Valid = buildable   (allows buildings, includes walkable), default for land
	--	      mineable    (allows mines, includes walkable), default for mountain
	--	      walkable    (allows flags, people, animals)
	--        shippable   (allows ships only), default for water
	--        unwalkable  (can't walk on, but near)
	--        unreachable (dangerous, can't go near), default for snow, lava
	property = "unreachable",
	-- Humidity in percent (0..100) which determinate how much water can be on this terrain
	-- Defaults to 0 for lava, snow, mountain, 100 otherwise
	humidity = 0,
	-- Filename of the texture image
	texture = texFile,
	-- Position and size {x, y, w, h} in the image if it contains multiple textures
	-- Can be left out. A size of 0 (w and/or h) is interpreted as the remaining image
	pos = {0, 0, 30, 30},
	-- Index of the palette animation in the file, default -1 for no animation
	palAnimIdx = -1,
	-- Color used to display this on the minimap
	color = 0xFF00286C
}
rttr:AddTerrain{
	name = "ww_ice1",
	landscape = "winterworld",
	edgeType = "ww_ice",
	s2Id = 4,
	edgePriority = 43,
	kind = "land",
	property = "walkable",
	humidity = 0,
	texture = texFile,
	pos = {48, 0, 32, 31},
	color = 0xFF0070b0
}
rttr:AddTerrain{
	name = "ww_iceFloes",
	landscape = "winterworld",
	edgeType = "ww_water",
	s2Id = 3,
	edgePriority = 83,
	kind = "water",
	property = "unwalkable",
	texture = texFile,
	pos = {96, 0, 32, 31},
	color = 0xFF00286c
}
rttr:AddTerrain{
	name = "ww_tundraFlower",
	landscape = "winterworld",
	edgeType = "ww_tundra",
	s2Id = 0xF,
	edgePriority = 13,
	kind = "land",
	texture = texFile,
	pos = {144, 0, 32, 31},
	color = 0xFF7c84ac
}
rttr:AddTerrain{
	name = "ww_mountain1",
	landscape = "winterworld",
	edgeType = "ww_mountain",
	s2Id = 1,
	edgePriority = 48,
	kind = "mountain",
	texture = texFile,
	pos = {0, 48, 32, 31},
	color = 0xFF54586c
}
rttr:AddTerrain{
	name = "ww_mountain2",
	landscape = "winterworld",
	edgeType = "ww_mountain",
	s2Id = 0XB,
	edgePriority = 63,
	kind = "mountain",
	texture = texFile,
	pos = {48, 48, 32, 31},
	color = 0xFF60607c
}
rttr:AddTerrain{
	name = "ww_mountain3",
	landscape = "winterworld",
	edgeType = "ww_mountain",
	s2Id = 0XC,
	edgePriority = 58,
	kind = "mountain",
	texture = texFile,
	pos = {96, 48, 32, 31},
	color = 0xFF686c8c
}
rttr:AddTerrain{
	name = "ww_mountain4",
	landscape = "winterworld",
	edgeType = "ww_mountain",
	s2Id = 0xD,
	edgePriority = 53,
	kind = "mountain",
	texture = texFile,
	pos = {144, 48, 32, 31},
	color = 0xFF686c8c
}
rttr:AddTerrain{
	name = "ww_taiga",
	landscape = "winterworld",
	edgeType = "ww_tundra",
	s2Id = 0,
	edgePriority = 18,
	kind = "land",
	humidity = 60,
	texture = texFile,
	pos = {0, 96, 32, 31},
	color = 0xFFa0accc
}
rttr:AddTerrain{
	name = "ww_tundra1",
	landscape = "winterworld",
	edgeType = "ww_tundra",
	s2Id = 8,
	edgePriority = 23,
	kind = "land",
	humidity = 95,
	texture = texFile,
	pos = {48, 96, 32, 31},
	color = 0xFFb0a494
}
rttr:AddTerrain{
	name = "ww_tundra2",
	landscape = "winterworld",
	edgeType = "ww_tundra",
	s2Id = 9,
	edgePriority = 28,
	kind = "land",
	texture = texFile,
	pos = {96, 96, 32, 31},
	color = 0xFF88a874
}
rttr:AddTerrain{
	name = "ww_tundra3",
	landscape = "winterworld",
	edgeType = "ww_tundra",
	s2Id = 0xA,
	edgePriority = 33,
	kind = "land",
	texture = texFile,
	pos = {144, 96, 32, 31},
	color = 0xFFa0accc
}
rttr:AddTerrain{
	name = "ww_tundra4",
	landscape = "winterworld",
	edgeType = "ww_tundra",
	s2Id = 0xE,
	edgePriority = 8,
	kind = "land",
	humidity = 30,
	texture = texFile,
	pos = {0, 144, 32, 31},
	color = 0xFF88b15e
}
rttr:AddTerrain{
	name = "ww_snow",
	landscape = "winterworld",
	edgeType = "ww_snow",
	s2Id = 0x12,
	edgePriority = 68,
	kind = "snow",
	property = "buildable",
	texture = texFile,
	pos = {48, 144, 32, 31},
	color = 0xFF94a0c0
}
rttr:AddTerrain{
	name = "ww_water",
	landscape = "winterworld",
	edgeType = "ww_water",
	s2Id = 5,
	edgePriority = 78,
	kind = "water",
	texture = texFile,
	pos = {193, 49, 53, 54},
	texType = "rotated",
	palAnimIdx = 10,
	color = 0xFF1038a4
}
rttr:AddTerrain{
	name = "ww_lava",
	landscape = "winterworld",
	edgeType = "none",
	s2Id = 0x10,
	kind = "lava",
	texture = texFile,
	pos = {193, 105, 53, 54},
	texType = "rotated",
	palAnimIdx = 11,
	color = 0xFFc02020
}
rttr:AddTerrain{
	name = "ww_reefWater",
	landscape = "winterworld",
	edgeType = "ww_water",
	s2Id = 19,
	edgePriority = 78,
	kind = "water",
	property = "unwalkable",
	texture = texFile,
	pos = {193, 49, 53, 54},
	texType = "rotated",
	palAnimIdx = 10,
	color = 0xFF1038a4
}
rttr:AddTerrain{
	name = "ww_shallowWater",
	landscape = "winterworld",
	edgeType = "ww_water",
	s2Id = 6,
	edgePriority = 78,
	kind = "water",
	property = "buildable",
	texture = texFile,
	pos = {193, 49, 53, 54},
	texType = "rotated",
	palAnimIdx = 10,
	color = 0xFF1038a4
}
rttr:AddTerrain{
	name = "ww_flatMountain",
	landscape = "winterworld",
	edgeType = "ww_mountain",
	s2Id = 0x22,
	edgePriority = 38,
	kind = "mountain",
	property = "buildable",
	texture = texFile,
	pos = {48, 48, 32, 31},
	color = 0xFF60607c
}
rttr:AddTerrain{
	name = "ww_lava2",
	landscape = "winterworld",
	edgeType = "none",
	s2Id = 0x14,
	kind = "lava",
	texture = texFile,
	pos = {66, 222, 31, 33},
	texType = "stacked",
	palAnimIdx = 11,
	color = 0xFFc02020
}
rttr:AddTerrain{
	name = "ww_lava3",
	landscape = "winterworld",
	edgeType = "none",
	s2Id = 0x15,
	kind = "lava",
	texture = texFile,
	pos = {99, 222, 31, 33},
	texType = "stacked",
	palAnimIdx = 11,
	color = 0xFFc02020
}
rttr:AddTerrain{
	name = "ww_lava4",
	landscape = "winterworld",
	edgeType = "none",
	s2Id = 0x16,
	kind = "lava",
	texture = texFile,
	pos = {132, 222, 31, 33},
	texType = "stacked",
	palAnimIdx = 11,
	color = 0xFFc02020
}
rttr:AddTerrain{
	name = "ww_ice2",
	landscape = "winterworld",
	edgeType = "ww_ice",
	s2Id = 7,
	edgePriority = 43,
	kind = "land",
	property = "walkable",
	humidity = 0,
	texture = texFile,
	pos = {48, 0, 32, 31},
	color = 0xFF0070b0
}
