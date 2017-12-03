rttr:AddTerrainEdge{
	-- Name used to reference this
	name = "gl_snow",
	-- Landscape to which this applies by default
	landscape = "greenland",
	-- Filename of the texure image
	texture = "TEX5.LBM",
	-- Position and size {x, y, w, h} in the image if it contains multiple textures
	-- Can be left out. A size of 0 (w and/or h) is interpreted as the remaining image
	pos = {192, 176, 64, 16}
}
rttr:AddTerrainEdge{
	name = "gl_mountain",
	landscape = "greenland",
	texture = "TEX5.LBM",
	pos = {192, 192, 64, 16}
}
rttr:AddTerrainEdge{
	name = "gl_desert",
	landscape = "greenland",
	texture = "TEX5.LBM",
	pos = {192, 208, 64, 16}
}
rttr:AddTerrainEdge{
	name = "gl_meadow",
	landscape = "greenland",
	texture = "TEX5.LBM",
	pos = {192, 224, 64, 16}
}
rttr:AddTerrainEdge{
	name = "gl_water",
	landscape = "greenland",
	texture = "TEX5.LBM",
	pos = {192, 240, 64, 16}
}

rttr:AddTerrain{
	-- Name used to reference this
	name = "gl_snow",
	-- Landscape to which this applies by default
	landscape = "greenland",
	-- Name of the edge drawn over neighbouring terrain or "none"
	edgeType = "gl_snow",
	-- Id used in the original S2. Defaults to 0xFF (not in S2)
	s2Id = 2,
	-- If this is higher than the neighbours edgePriority then it draws over the neighbour
	-- Valid = [-128, 127], defaults to 0
	edgePriority = 49,
	-- What kind of terrain is this? (Used for animals, ships, etc)
	-- Valid = land (default), water, lava, snow, mountain
	kind = "snow",
	-- Property for this terrain. 
	-- Valid = buildable   (allows buildings, includes walkable), default for land
	--	      mineable    (allows mines, includes walkable), default for mountain
	--	      walkable    (allows flags, people, animals)
	--        shippable   (allows ships only), default for water
	--        unwalkable  (can't walk on, but near)
	--        unreachable (dangerous, can't go near), default for snow, lava
	property = "unreachable",
	-- Filename of the texure image
	texture = "TEX5.LBM",
	-- Position and size {x, y, w, h} in the image if it contains multiple textures
	-- Can be left out. A size of 0 (w and/or h) is interpreted as the remaining image
	pos = {0, 0, 30, 30},
	-- Index of the palette animation in the file, default -1 for no animation
	palAnimIdx = -1,
	-- Color used to display this on the minimap
	color = 0xFFFFFFFF
}
rttr:AddTerrain{
	name = "gl_desert",
	landscape = "greenland",
	edgeType = "gl_desert",
	s2Id = 4,
	edgePriority = 48,
	kind = "land",
	property = "walkable",
	texture = "TEX5.LBM",
	pos = {48, 0, 48, 48},
	color = 0xFFc09c7c
}
rttr:AddTerrain{
	name = "gl_swamp",
	landscape = "greenland",
	edgeType = "gl_meadow",
	s2Id = 3,
	edgePriority = 36,
	kind = "land",
	property = "unwalkable",
	texture = "TEX5.LBM",
	pos = {96, 0, 30, 30},
	color = 0xFF649014
}
rttr:AddTerrain{
	name = "gl_meadowFlowers",
	landscape = "greenland",
	edgeType = "gl_meadow",
	s2Id = 0xF,
	edgePriority = 37,
	kind = "land",
	texture = "TEX5.LBM",
	pos = {144, 0, 42, 42},
	color = 0xFF48780c
}
rttr:AddTerrain{
	name = "gl_mountain1",
	landscape = "greenland",
	edgeType = "gl_mountain",
	s2Id = 1,
	edgePriority = 45,
	kind = "mountain",
	texture = "TEX5.LBM",
	pos = {0, 48, 48, 48},
	color = 0xFF9c8058
}
rttr:AddTerrain{
	name = "gl_mountain2",
	landscape = "greenland",
	edgeType = "gl_mountain",
	s2Id = 0XB,
	edgePriority = 44,
	kind = "mountain",
	texture = "TEX5.LBM",
	pos = {48, 48, 48, 48},
	color = 0xFF9c8058
}
rttr:AddTerrain{
	name = "gl_mountain3",
	landscape = "greenland",
	edgeType = "gl_mountain",
	s2Id = 0XC,
	edgePriority = 43,
	kind = "mountain",
	texture = "TEX5.LBM",
	pos = {96, 48, 48, 48},
	color = 0xFF9c8058
}
rttr:AddTerrain{
	name = "gl_mountain4",
	landscape = "greenland",
	edgeType = "gl_mountain",
	s2Id = 0xD,
	edgePriority = 13,
	kind = "mountain",
	texture = "TEX5.LBM",
	pos = {144, 48, 48, 48},
	color = 0xFF9c8058
}
rttr:AddTerrain{
	name = "gl_savannah",
	landscape = "greenland",
	edgeType = "gl_desert",
	s2Id = 0,
	edgePriority = 38,
	kind = "land",
	texture = "TEX5.LBM",
	pos = {0, 96, 48, 48},
	color = 0xFF649014
}
rttr:AddTerrain{
	name = "gl_meadow1",
	landscape = "greenland",
	edgeType = "gl_meadow",
	s2Id = 8,
	edgePriority = 41,
	kind = "land",
	texture = "TEX5.LBM",
	pos = {48, 96, 48, 48},
	color = 0xFF48780c
}
rttr:AddTerrain{
	name = "gl_meadow2",
	landscape = "greenland",
	edgeType = "gl_meadow",
	s2Id = 9,
	edgePriority = 40,
	kind = "land",
	texture = "TEX5.LBM",
	pos = {96, 96, 48, 48},
	color = 0xFF649014
}
rttr:AddTerrain{
	name = "gl_meadow3",
	landscape = "greenland",
	edgeType = "gl_meadow",
	s2Id = 0xA,
	edgePriority = 39,
	kind = "land",
	texture = "TEX5.LBM",
	pos = {144, 96, 48, 48},
	color = 0xFF407008
}
rttr:AddTerrain{
	name = "gl_steppe",
	landscape = "greenland",
	edgeType = "gl_desert",
	s2Id = 0xE,
	edgePriority = 46,
	kind = "land",
	texture = "TEX5.LBM",
	pos = {0, 144, 40, 40},
	color = 0xFF88b028
}
rttr:AddTerrain{
	name = "gl_mountainMeadow",
	landscape = "greenland",
	edgeType = "gl_mountain",
	s2Id = 0x12,
	edgePriority = 47,
	kind = "mountain",
	property = "buildable",
	texture = "TEX5.LBM",
	pos = {48, 144, 30, 30},
	color = 0xFF9c8058
}
rttr:AddTerrain{
	name = "gl_water",
	landscape = "greenland",
	edgeType = "gl_water",
	s2Id = 5,
	edgePriority = 35,
	kind = "water",
	texture = "TEX5.LBM",
	pos = {192, 48, 55, 56},
	palAnimIdx = 10,
	color = 0xFF1038a4
}
rttr:AddTerrain{
	name = "gl_lava",
	landscape = "greenland",
	edgeType = "none",
	s2Id = 0x10,
	kind = "lava",
	texture = "TEX5.LBM",
	pos = {192, 104, 55, 56},
	palAnimIdx = 11,
	color = 0xFFc02020
}
rttr:AddTerrain{
	name = "gl_reefWater",
	landscape = "greenland",
	edgeType = "gl_water",
	s2Id = 19,
	edgePriority = 50,
	kind = "water",
	property = "unwalkable",
	texture = "TEX5.LBM",
	pos = {192, 48, 55, 56},
	palAnimIdx = 10,
	color = 0xFF1038a4
}
rttr:AddTerrain{
	name = "gl_shallowWater",
	landscape = "greenland",
	edgeType = "gl_water",
	s2Id = 6,
	edgePriority = 50,
	kind = "water",
	property = "buildable",
	texture = "TEX5.LBM",
	pos = {192, 48, 55, 56},
	palAnimIdx = 10,
	color = 0xFF1038a4
}
rttr:AddTerrain{
	name = "gl_flatMountain",
	landscape = "greenland",
	edgeType = "gl_mountain",
	s2Id = 0x22,
	edgePriority = 42,
	kind = "mountain",
	property = "buildable",
	texture = "TEX5.LBM",
	pos = {48, 48, 48, 48},
	color = 0xFF9c8058
}
rttr:AddTerrain{
	name = "gl_lava2",
	landscape = "greenland",
	edgeType = "none",
	s2Id = 0x14,
	kind = "lava",
	texture = "TEX5.LBM",
	pos = {66, 223, 31, 32},
	palAnimIdx = 11,
	color = 0xFFc02020
}
rttr:AddTerrain{
	name = "gl_lava3",
	landscape = "greenland",
	edgeType = "none",
	s2Id = 0x15,
	kind = "lava",
	texture = "TEX5.LBM",
	pos = {99, 223, 31, 32},
	palAnimIdx = 11,
	color = 0xFFc02020
}
rttr:AddTerrain{
	name = "gl_lava4",
	landscape = "greenland",
	edgeType = "none",
	s2Id = 0x16,
	kind = "lava",
	texture = "TEX5.LBM",
	pos = {132, 223, 31, 32},
	palAnimIdx = 11,
	color = 0xFFc02020
}