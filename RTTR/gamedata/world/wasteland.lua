rttr:AddTerrainEdge{
	-- Name used to reference this
	name = "wl_stone",
	-- Landscape to which this applies by default
	landscape = "wasteland",
	-- Filename of the texture image
	texture = "TEX6.LBM",
	-- Position and size {x, y, w, h} in the image if it contains multiple textures
	-- Can be left out. A size of 0 (w and/or h) is interpreted as the remaining image
	pos = {192, 176, 64, 16}
}
rttr:AddTerrainEdge{
	name = "wl_moor",
	landscape = "wasteland",
	texture = "TEX6.LBM",
	pos = {192, 192, 64, 16}
}
rttr:AddTerrainEdge{
	name = "wl_wasteland",
	landscape = "wasteland",
	texture = "TEX6.LBM",
	pos = {192, 208, 64, 16}
}
rttr:AddTerrainEdge{
	name = "wl_mountain",
	landscape = "wasteland",
	texture = "TEX6.LBM",
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
	s2Id = 2,
	-- If this is higher than the neighbours edgePriority then it draws over the neighbour
	-- Valid = [-128, 127], defaults to 0
	edgePriority = 49,
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
	texture = "TEX6.LBM",
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
	s2Id = 4,
	edgePriority = 48,
	kind = "land",
	property = "walkable",
	humidity = 0,
	texture = "TEX6.LBM",
	pos = {48, 0, 48, 48},
	color = 0xFF9c7c64
}
rttr:AddTerrain{
	name = "wl_lavaManyStone",
	landscape = "wasteland",
	edgeType = "wl_stone",
	s2Id = 3,
	edgePriority = 36,
	kind = "lava",
	property = "unwalkable",
	texture = "TEX6.LBM",
	pos = {96, 0, 37, 31},
	color = 0xFF001820
}
rttr:AddTerrain{
	name = "wl_flowerPasture",
	landscape = "wasteland",
	edgeType = "wl_mountain",
	s2Id = 0xF,
	edgePriority = 37,
	kind = "land",
	texture = "TEX6.LBM",
	pos = {144, 0, 48, 48},
	color = 0xFF444850
}
rttr:AddTerrain{
	name = "wl_mountain1",
	landscape = "wasteland",
	edgeType = "wl_mountain",
	s2Id = 1,
	edgePriority = 45,
	kind = "mountain",
	texture = "TEX6.LBM",
	pos = {0, 48, 48, 48},
	color = 0xFF706c54
}
rttr:AddTerrain{
	name = "wl_mountain2",
	landscape = "wasteland",
	edgeType = "wl_mountain",
	s2Id = 0XB,
	edgePriority = 44,
	kind = "mountain",
	texture = "TEX6.LBM",
	pos = {48, 48, 48, 48},
	color = 0xFF706454
}
rttr:AddTerrain{
	name = "wl_mountain3",
	landscape = "wasteland",
	edgeType = "wl_mountain",
	s2Id = 0XC,
	edgePriority = 43,
	kind = "mountain",
	texture = "TEX6.LBM",
	pos = {96, 48, 48, 48},
	color = 0xFF684c24
}
rttr:AddTerrain{
	name = "wl_mountain4",
	landscape = "wasteland",
	edgeType = "wl_mountain",
	s2Id = 0xD,
	edgePriority = 13,
	kind = "mountain",
	texture = "TEX6.LBM",
	pos = {144, 48, 48, 48},
	color = 0xFF684c24
}
rttr:AddTerrain{
	name = "wl_dSteppe",
	landscape = "wasteland",
	edgeType = "wl_mountain",
	s2Id = 0,
	edgePriority = 38,
	kind = "land",
	humidity = 60,
	texture = "TEX6.LBM",
	pos = {0, 96, 48, 48},
	color = 0xFF444850
}
rttr:AddTerrain{
	name = "wl_pasture1",
	landscape = "wasteland",
	edgeType = "wl_mountain",
	s2Id = 8,
	edgePriority = 41,
	kind = "land",
	humidity = 95,
	texture = "TEX6.LBM",
	pos = {48, 96, 48, 48},
	color = 0xFF5c5840
}
rttr:AddTerrain{
	name = "wl_pasture2",
	landscape = "wasteland",
	edgeType = "wl_mountain",
	s2Id = 9,
	edgePriority = 40,
	kind = "land",
	texture = "TEX6.LBM",
	pos = {96, 96, 48, 48},
	color = 0xFF646048
}
rttr:AddTerrain{
	name = "wl_pasture3",
	landscape = "wasteland",
	edgeType = "wl_mountain",
	s2Id = 0xA,
	edgePriority = 39,
	kind = "land",
	texture = "TEX6.LBM",
	pos = {144, 96, 48, 48},
	color = 0xFF646048
}
rttr:AddTerrain{
	name = "wl_lSteppe",
	landscape = "wasteland",
	edgeType = "wl_wasteland",
	s2Id = 0xE,
	edgePriority = 46,
	kind = "land",
	humidity = 30,
	texture = "TEX6.LBM",
	pos = {0, 144, 48, 48},
	color = 0xFF88b028
}
rttr:AddTerrain{
	name = "wl_alpPasture",
	landscape = "wasteland",
	edgeType = "wl_stone",
	s2Id = 0x12,
	edgePriority = 47,
	kind = "mountain",
	property = "buildable",
	texture = "TEX6.LBM",
	pos = {48, 144, 37, 31},
	color = 0xFF001820
}
rttr:AddTerrain{
	name = "wl_moor",
	landscape = "wasteland",
	edgeType = "wl_moor",
	s2Id = 5,
	edgePriority = 35,
	kind = "water",
	texture = "TEX6.LBM",
	pos = {192, 48, 55, 56},
	palAnimIdx = 10,
	color = 0xFF454520
}
rttr:AddTerrain{
	name = "wl_lava",
	landscape = "wasteland",
	edgeType = "none",
	s2Id = 0x10,
	kind = "lava",
	texture = "TEX6.LBM",
	pos = {192, 104, 55, 56},
	palAnimIdx = 11,
	color = 0xFFC32020
}
rttr:AddTerrain{
	name = "wl_reefMoor",
	landscape = "wasteland",
	edgeType = "wl_moor",
	s2Id = 19,
	edgePriority = 50,
	kind = "water",
	property = "unwalkable",
	texture = "TEX6.LBM",
	pos = {192, 48, 55, 56},
	palAnimIdx = 10,
	color = 0xFF454520
}
rttr:AddTerrain{
	name = "wl_shallowMoor",
	landscape = "wasteland",
	edgeType = "wl_moor",
	s2Id = 6,
	edgePriority = 50,
	kind = "water",
	property = "buildable",
	texture = "TEX6.LBM",
	pos = {192, 48, 55, 56},
	palAnimIdx = 10,
	color = 0xFF454520
}
rttr:AddTerrain{
	name = "wl_flatMountain",
	landscape = "wasteland",
	edgeType = "wl_mountain",
	s2Id = 0x22,
	edgePriority = 42,
	kind = "mountain",
	property = "buildable",
	texture = "TEX6.LBM",
	pos = {48, 48, 48, 48},
	color = 0xFF706454
}
rttr:AddTerrain{
	name = "wl_lava2",
	landscape = "wasteland",
	edgeType = "none",
	s2Id = 0x14,
	kind = "lava",
	texture = "TEX6.LBM",
	pos = {66, 223, 31, 32},
	palAnimIdx = 11,
	color = 0xFFC32020
}
rttr:AddTerrain{
	name = "wl_lava3",
	landscape = "wasteland",
	edgeType = "none",
	s2Id = 0x15,
	kind = "lava",
	texture = "TEX6.LBM",
	pos = {99, 223, 31, 32},
	palAnimIdx = 11,
	color = 0xFFC32020
}
rttr:AddTerrain{
	name = "wl_lava4",
	landscape = "wasteland",
	edgeType = "none",
	s2Id = 0x16,
	kind = "lava",
	texture = "TEX6.LBM",
	pos = {132, 223, 31, 32},
	palAnimIdx = 11,
	color = 0xFFC32020
}