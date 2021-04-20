-- Copyright (C) 2018 - 2021 Settlers Freaks <sf-team at siedler25.org>
--
-- SPDX-License-Identifier: GPL-2.0-or-later

texFile = "<RTTR_GAME>/GFX/TEXTURES/TEX6.LBM";

rttr:AddLandscape{
	name = __"wasteland",
	mapGfx = "<RTTR_GAME>/DATA/MAP_1_Z.LST",
	s2Id = 0x01,
	isWinter = false,
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
	name = "wl_stone",
	-- Landscape to which this applies by default
	landscape = "wasteland",
	-- Filename of the texture image
	texture = texFile,
	-- Position and size {x, y, w, h} in the image if it contains multiple textures
	-- Can be left out. A size of 0 (w and/or h) is interpreted as the remaining image
	pos = {192, 176, 64, 16}
}
rttr:AddTerrainEdge{
	name = "wl_moor",
	landscape = "wasteland",
	texture = texFile,
	pos = {192, 192, 64, 16}
}
rttr:AddTerrainEdge{
	name = "wl_wasteland",
	landscape = "wasteland",
	texture = texFile,
	pos = {192, 208, 64, 16}
}
rttr:AddTerrainEdge{
	name = "wl_mountain",
	landscape = "wasteland",
	texture = texFile,
	pos = {192, 224, 64, 16}
}

rttr:AddTerrain{
	-- Name used to reference this
	name = "wl_lavaFewStone",
	-- Landscape to which this applies by default
	landscape = "wasteland",
	-- Name of the edge drawn over neighbouring terrain or "none"
	edgeType = "none",
	-- Id used in the original S2. Defaults to 0xFF (not in S2)
	s2Id = 0x02,
	-- If this is higher than the neighbours edgePriority then it draws over the neighbour
	-- Valid = [-128, 127], defaults to 0
	edgePriority = 20,
	-- What kind of terrain is this? (Used for animals, ships, etc)
	-- Valid = land (default), water, lava, snow, mountain
	kind = "lava",
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
	palAnimIdx = 11,
	-- Color used to display this on the minimap
	color = 0xFF860000
}
rttr:AddTerrain{
	name = "wl_wasteland1",
	landscape = "wasteland",
	edgeType = "wl_wasteland",
	s2Id = 0x04,
	edgePriority = 50,
	kind = "land",
	property = "walkable",
	humidity = 0,
	texture = texFile,
	pos = {48, 0, 32, 31},
	color = 0xFF9c7c64
}
rttr:AddTerrain{
	name = "wl_lavaManyStone",
	landscape = "wasteland",
	edgeType = "wl_stone",
	s2Id = 0x03,
	edgePriority = 80,
	kind = "lava",
	property = "unwalkable",
	texture = texFile,
	pos = {96, 0, 37, 31},
	color = 0xFF001820
}
rttr:AddTerrain{
	name = "wl_flowerPasture",
	landscape = "wasteland",
	edgeType = "wl_mountain",
	s2Id = 0x0F,
	edgePriority = 40,
	kind = "land",
	texture = texFile,
	pos = {144, 0, 32, 31},
	color = 0xFF444850
}
rttr:AddTerrain{
	name = "wl_mountain1",
	landscape = "wasteland",
	edgeType = "wl_mountain",
	s2Id = 0x01,
	edgePriority = 30,
	kind = "mountain",
	texture = texFile,
	pos = {0, 48, 32, 31},
	color = 0xFF706c54
}
rttr:AddTerrain{
	name = "wl_mountain2",
	landscape = "wasteland",
	edgeType = "wl_mountain",
	s2Id = 0x0B,
	edgePriority = 30,
	kind = "mountain",
	texture = texFile,
	pos = {48, 48, 32, 31},
	color = 0xFF706454
}
rttr:AddTerrain{
	name = "wl_mountain3",
	landscape = "wasteland",
	edgeType = "wl_mountain",
	s2Id = 0x0C,
	edgePriority = 30,
	kind = "mountain",
	texture = texFile,
	pos = {96, 48, 32, 31},
	color = 0xFF684c24
}
rttr:AddTerrain{
	name = "wl_mountain4",
	landscape = "wasteland",
	edgeType = "wl_mountain",
	s2Id = 0x0D,
	edgePriority = 30,
	kind = "mountain",
	texture = texFile,
	pos = {144, 48, 32, 31},
	color = 0xFF684c24
}
rttr:AddTerrain{
	name = "wl_dSteppe",
	landscape = "wasteland",
	edgeType = "wl_mountain",
	s2Id = 0x00,
	edgePriority = 40,
	kind = "land",
	humidity = 60,
	texture = texFile,
	pos = {0, 96, 32, 31},
	color = 0xFF444850
}
rttr:AddTerrain{
	name = "wl_pasture1",
	landscape = "wasteland",
	edgeType = "wl_mountain",
	s2Id = 0x08,
	edgePriority = 40,
	kind = "land",
	humidity = 95,
	texture = texFile,
	pos = {48, 96, 32, 31},
	color = 0xFF5c5840
}
rttr:AddTerrain{
	name = "wl_pasture2",
	landscape = "wasteland",
	edgeType = "wl_mountain",
	s2Id = 0x09,
	edgePriority = 40,
	kind = "land",
	texture = texFile,
	pos = {96, 96, 32, 31},
	color = 0xFF646048
}
rttr:AddTerrain{
	name = "wl_pasture3",
	landscape = "wasteland",
	edgeType = "wl_mountain",
	s2Id = 0x0A,
	edgePriority = 40,
	kind = "land",
	texture = texFile,
	pos = {144, 96, 32, 31},
	color = 0xFF646048
}
rttr:AddTerrain{
	name = "wl_lSteppe",
	landscape = "wasteland",
	edgeType = "wl_wasteland",
	s2Id = 0x0E,
	edgePriority = 60,
	kind = "land",
	humidity = 30,
	texture = texFile,
	pos = {0, 144, 32, 31},
	color = 0xFF88b028
}
rttr:AddTerrain{
	name = "wl_alpPasture",
	landscape = "wasteland",
	edgeType = "wl_stone",
	s2Id = 0x12,
	edgePriority = 90,
	kind = "mountain",
	property = "buildable",
	texture = texFile,
	pos = {48, 144, 32, 31},
	color = 0xFF001820
}
rttr:AddTerrain{
	name = "wl_moor",
	landscape = "wasteland",
	edgeType = "wl_moor",
	s2Id = 0x05,
	edgePriority = 70,
	kind = "water",
	texture = texFile,
	pos = {193, 49, 53, 54},
	texType = "rotated",
	palAnimIdx = 10,
	color = 0xFF454520
}
rttr:AddTerrain{
	name = "wl_lava",
	landscape = "wasteland",
	edgeType = "none",
	s2Id = 0x10,
	edgePriority = 10,
	kind = "lava",
	texture = texFile,
	pos = {193, 105, 53, 54},
	texType = "rotated",
	palAnimIdx = 11,
	color = 0xFFC32020
}
rttr:AddTerrain{
	name = "wl_reefMoor",
	landscape = "wasteland",
	edgeType = "wl_moor",
	s2Id = 0x13,
	edgePriority = 70,
	kind = "water",
	property = "unwalkable",
	texture = texFile,
	pos = {193, 49, 53, 54},
	texType = "rotated",
	palAnimIdx = 10,
	color = 0xFF454520
}
rttr:AddTerrain{
	name = "wl_shallowMoor",
	landscape = "wasteland",
	edgeType = "wl_moor",
	s2Id = 0x06,
	edgePriority = 70,
	kind = "water",
	property = "buildable",
	texture = texFile,
	pos = {193, 49, 53, 54},
	texType = "rotated",
	palAnimIdx = 10,
	color = 0xFF454520
}
rttr:AddTerrain{
	name = "wl_flatMountain",
	landscape = "wasteland",
	edgeType = "wl_mountain",
	s2Id = 0x22,
	edgePriority = 30,
	kind = "mountain",
	property = "buildable",
	texture = texFile,
	pos = {48, 48, 32, 31},
	color = 0xFF706454
}
rttr:AddTerrain{
	name = "wl_lava2",
	landscape = "wasteland",
	edgeType = "none",
	s2Id = 0x14,
	edgePriority = 10,
	kind = "lava",
	texture = texFile,
	pos = {66, 222, 31, 33},
	texType = "stacked",
	palAnimIdx = 11,
	color = 0xFFC32020
}
rttr:AddTerrain{
	name = "wl_lava3",
	landscape = "wasteland",
	edgeType = "none",
	s2Id = 0x15,
	edgePriority = 10,
	kind = "lava",
	texture = texFile,
	pos = {99, 222, 31, 33},
	texType = "stacked",
	palAnimIdx = 11,
	color = 0xFFC32020
}
rttr:AddTerrain{
	name = "wl_lava4",
	landscape = "wasteland",
	edgeType = "none",
	s2Id = 0x16,
	edgePriority = 10,
	kind = "lava",
	texture = texFile,
	pos = {132, 222, 31, 33},
	texType = "stacked",
	palAnimIdx = 11,
	color = 0xFFC32020
}
rttr:AddTerrain{
	name = "wl_wasteland2",
	landscape = "wasteland",
	edgeType = "wl_wasteland",
	s2Id = 0x07,
	edgePriority = 50,
	kind = "land",
	property = "walkable",
	humidity = 0,
	texture = texFile,
	pos = {48, 0, 32, 31},
	color = 0xFF9c7c64
}
rttr:AddTerrain{
	name = "wl_lava1",
	landscape = "wasteland",
	edgeType = "none",
	s2Id = 0x11,
	edgePriority = 10,
	kind = "lava",
	texture = texFile,
	pos = {193, 105, 53, 54},
	texType = "rotated",
	palAnimIdx = 11,
	color = 0xFFC32020
}