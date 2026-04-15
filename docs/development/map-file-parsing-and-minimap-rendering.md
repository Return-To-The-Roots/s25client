# Map File Parsing And Minimap Rendering

This note summarizes how RTTR reads classic Settlers II `.WLD` / `.SWD`
world maps and how it turns either raw map data or live world state into a
minimap texture.

It was cross-checked against:

- the external format description on Settlers2.net:
  `https://settlers2.net/documentation/world-map-file-format-wldswd/`
- the repository's older reference note:
  [external/s25edit/WLD_reference.md](../../external/s25edit/WLD_reference.md)

## Overview

In this repository, map handling is split into two distinct stages:

1. `external/libsiedler2` parses the bytes of a `.WLD` / `.SWD` file into a
   `libsiedler2::ArchivItem_Map`.
2. `libs/s25main/world/MapLoader.cpp` interprets selected layers from that
   parsed map and builds RTTR runtime state such as terrain, resources,
   decorative objects, animals, harbors, and headquarters.

The minimap also has two paths:

- `PreviewMinimap` renders directly from `ArchivItem_Map` data before the map
  is loaded into a live game world.
- `IngameMinimap` renders from the live `GameWorldViewer`, so it includes
  visibility, territory, live roads, and live buildings.

## Stage 1: Byte-Level `.WLD` / `.SWD` Parsing

The file entry point is `libsiedler2::loader::LoadMAP(...)` in
[external/libsiedler2/src/LoadMAP.cpp](../../external/libsiedler2/src/LoadMAP.cpp).
That allocates an `ArchivItem_Map` and calls `ArchivItem_Map::load(...)`.

### Header parsing

`ArchivItem_Map_Header::load(...)` in
[external/libsiedler2/src/ArchivItem_Map_Header.cpp](../../external/libsiedler2/src/ArchivItem_Map_Header.cpp)
expects the classic `WORLD_V1.0` signature and then reads:

- map name
- width and height placeholders embedded in the title area
- graphics set / landscape id
- player count
- author
- 7 HQ X/Y positions
- validation byte
- 7 player-face bytes
- 250 packed area records (`areaInfos`)
- the secondary header marker `0x2711`
- the actual width and height used for the map blocks

Two implementation details matter:

- The parser supports the two common title layouts by checking whether the
  early width/height placeholders match the later width/height values and then
  deciding whether the title is effectively 20 or 24 bytes long.
- It contains a compatibility workaround for a known malformed map
  (`"Das Dreieck"`) that has an extra word before the first block.

### Data blocks

`ArchivItem_Map::load(...)` in
[external/libsiedler2/src/ArchivItem_Map.cpp](../../external/libsiedler2/src/ArchivItem_Map.cpp)
then reads exactly 14 block headers and 14 block payloads. Each block header
must match the fixed structure described by the external docs:

- magic `0x2710`
- zeroed unknown field
- width
- height
- multiplier
- block length

The repo maps those blocks to `MapLayer` in
[external/libsiedler2/include/libsiedler2/ArchivItem_Map.h](../../external/libsiedler2/include/libsiedler2/ArchivItem_Map.h):

- `Altitude`
- `Terrain1`
- `Terrain2`
- `RoadsOld`
- `ObjectIndex`
- `ObjectType`
- `Animals`
- `Unknown7`
- `BuildingQuality`
- `Unknown9`
- `Unknown10`
- `Resources`
- `Shadows`
- `Lakes`

`Reservations` and `Owner` exist in the enum as runtime-only extensions and are
not read from `.WLD` / `.SWD`.

### Footer

After the 14 fixed blocks, `ArchivItem_Map::load(...)` reads an optional extra
animal list until byte `0xFF`. The parser preserves that footer in
`ArchivItem_Map::extraInfo`.

One notable detail: the parser preserves this extra animal footer, but the main
world loader in `s25main` does not currently consume it.

## Stage 2: Semantic Import Into RTTR World State

The gameplay loader lives in
[libs/s25main/world/MapLoader.cpp](../../libs/s25main/world/MapLoader.cpp).
`MapLoader::Load(path)` first calls `libsiedler2::loader::LoadMAP(...)`, then
passes the resulting `ArchivItem_Map` into `MapLoader::Load(map, exploration)`.

### World initialization

`MapLoader::Load(...)`:

- loads game data descriptions
- selects the landscape from `map.getHeader().getGfxSet()`
- initializes the world with the parsed width and height
- imports node data
- places objects
- places animals
- computes harbors and seas
- recomputes shadows
- optionally seeds fog-of-war memory

### Layers that RTTR actively imports

`MapLoader::InitNodes(...)` uses these file layers directly:

- `Altitude` -> `MapNode::altitude`
- `Terrain1` / `Terrain2` -> terrain descriptors for the two triangles of a
  node
- `Resources` -> water, fish, coal, iron, gold, granite

Important details:

- RTTR masks terrain values with `0x3F` before terrain lookup, so special
  high-bit flags are stripped before matching terrain definitions.
- Harbor markers are detected from the harbor bit in `Terrain1`.
- Fog-of-war visibility is initialized from the game settings, not from file
  data.

### Objects and HQ positions

`MapLoader::PlaceObjects(...)` interprets the `ObjectType` and `ObjectIndex`
pair. In practice this is where RTTR reads:

- player start positions (`ObjectType == 0x80`, player index in `ObjectIndex`)
- trees
- granite
- decorative map objects

This matters because the live loader does not use the header HQ coordinate
arrays. Headquarters are placed later from the start positions collected out of
the object layers.

### Animals

`MapLoader::PlaceAnimals(...)` reads only the `Animals` layer and maps known
ids to RTTR animal species.

The optional footer animal list parsed by `libsiedler2` is not used here.

### Recomputed instead of trusted

Several pieces of file data are parsed or preserved but not directly trusted by
the gameplay loader:

- `Shadows` is ignored for live world state; RTTR recomputes shadows by calling
  `world.RecalcShadow(pt)` for every node.
- Header `areaInfos` is parsed and preserved, but seafaring areas are rebuilt by
  `InitSeasAndHarbors(...)`.
- `Lakes` is not used for runtime sea ownership; RTTR flood-fills water and
  computes harbor neighbors itself.
- `RoadsOld` is not imported as live roads.
- `BuildingQuality` is not imported as the final buildability state; RTTR
  recalculates build quality from runtime terrain rules.

So the repo is best understood as "parse the original format, but rebuild the
simulation-facing derived data."

## Minimap Base Representation

The shared base class is [libs/s25main/Minimap.cpp](../../libs/s25main/Minimap.cpp).

`Minimap::CreateMapTexture()` creates a texture of size:

- width = `mapWidth * 2`
- height = `mapHeight`

Each logical map node contributes two minimap pixels, one for each terrain
triangle. The x-coordinate is staggered like this:

```cpp
x = (pt.x * 2 + triangle + (pt.y & 1)) % (mapWidth * 2)
```

That encoding is the core reason the minimap looks like a compacted hex/triangle
projection instead of a square grid.

`ctrlMinimap` then rescales that texture for the UI, using
`MINIMAP_SCALE_X ~= 2 / sqrt(3)` from
[libs/s25main/gameData/MinimapConsts.h](../../libs/s25main/gameData/MinimapConsts.h)
to preserve the hex geometry aspect ratio.

## Preview Minimap

The map-selection preview path lives in
[libs/s25main/PreviewMinimap.cpp](../../libs/s25main/PreviewMinimap.cpp).

`PreviewMinimap::SetMap(...)` pulls raw layers from `ArchivItem_Map`:

- `ObjectType`
- `Terrain1`
- `Terrain2`
- `Shadows`, if present

If the file does not provide the shadow layer, RTTR recomputes it from
altitudes using the same 4-neighbor formula described by the external docs.

### Preview coloring rules

`PreviewMinimap::CalcPixelColor(...)` applies a simple priority:

1. trees -> fixed green with brightness variation
2. granite -> fixed gray with brightness variation
3. otherwise -> terrain minimap color from game data, then shading

Terrain colors do not come from hard-coded file ids. RTTR loads the active
`WorldDescription`, finds the terrain descriptors for the map's `gfxSet`, and
uses each terrain's `minimapColor`.

`ctrlPreviewMinimap` then adds a UI overlay for HQ positions by scanning the
raw `ObjectType` / `ObjectIndex` layers again and drawing small colored
rectangles for players.

## Ingame Minimap

The live gameplay minimap is implemented in
[libs/s25main/IngameMinimap.cpp](../../libs/s25main/IngameMinimap.cpp).

Unlike the preview minimap, it does not read raw file layers. It renders from
`GameWorldViewer`, so it reflects:

- current fog-of-war / invisibility
- current ownership
- current roads
- current buildings and construction sites
- current live shadows from the runtime world

### Ingame coloring rules

`IngameMinimap::CalcPixelColor(...)` uses this effective priority:

1. fully invisible tile -> solid black
2. tree / granite -> dedicated colors, optionally blended with owner color
3. owned building or construction site -> white if house overlay is enabled
4. owned road -> gray if road overlay is enabled
5. owned terrain -> terrain color blended with player color if territory is enabled
6. otherwise -> pure terrain color

If the tile is in fog-of-war instead of fully visible, the final color is
darkened by halving RGB components.

Terrain colors still come from terrain descriptors, but the shading source is
the live node shadow value in the world, not the raw file shadow block.

### Incremental updates

The ingame minimap supports incremental refresh:

- `UpdateNode(pt)` marks a node dirty
- `BeforeDrawing()` applies the queued updates just before drawing
- if at least half the map is dirty, it rebuilds the whole texture
- otherwise it updates only the changed pixels in-place

That avoids recreating the full minimap texture for every small road, border,
or building update.

### UI behavior

`ctrlIngameMinimap` draws the minimap texture and overlays the current viewport
marker. Dragging or clicking on the minimap recenters the `GameWorldView`.

The same object also exposes the runtime toggles for:

- territory tint
- house markers
- road markers

Those toggles selectively recompute only the affected minimap pixel classes.

## Practical Takeaways

- RTTR parses classic `.WLD` / `.SWD` files faithfully enough to preserve the
  original 14-layer structure and optional animal footer.
- The actual simulation loader imports only a subset of those layers and
  recomputes several derived systems, especially shadows, buildability, and
  seafaring topology.
- The preview minimap is a lightweight file-based renderer.
- The ingame minimap is a live world-state renderer with visibility and owner
  overlays.
- When investigating map bugs, first determine whether the issue is in:
  1. byte-level file parsing in `external/libsiedler2`
  2. semantic import in `MapLoader`
  3. preview minimap rendering
  4. ingame minimap rendering
