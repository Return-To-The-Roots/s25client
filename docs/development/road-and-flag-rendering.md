# Road And Flag Rendering

This note summarizes how roads and flags become visible on screen in the
runtime renderer.

## Rendering Split

Roads and flags are not rendered by the same subsystem.

- Roads are terrain overlays. They are prepared and drawn inside
  `TerrainRenderer`, before nodal objects are drawn.
- Flags are normal map objects. `GameWorldView` iterates visible nodes after
  terrain rendering and calls `obj->Draw(...)` for each object on that node.

That split matters because roads inherit terrain shading and visibility, while
flags follow normal object draw order and sprite animation rules.

## Road Rendering Pipeline

Road state is stored in the world as `PointRoad` values, with only three
canonical directions persisted per node: `East`, `SouthEast`, and
`SouthWest`. At draw time, `TerrainRenderer::PrepareWaysPoint(...)` asks the
viewer for the road visible at each of those directions.

The viewer layer can return either:

- the real road from the world state, or
- a temporary visual overlay from `GameWorldViewer::visualNodes`.

That is why road previews during road building can be rendered without
changing the committed game state.

For each visible road edge, the terrain renderer:

1. Computes start and end screen positions from the two node vertices.
2. Corrects the segment when it crosses the wrapped map border.
3. Chooses a road texture variant.
4. Batches the segment into `sorted_roads`.

The texture choice is terrain-aware. For each landscape, the world description
provides four road texture classes:

- `normal`
- `upgraded`
- `boat`
- `mountain`

The runtime selects them like this:

- `PointRoad::Boat` uses the `boat` texture.
- Land roads touching mountain terrain use the `mountain` texture.
- `PointRoad::Donkey` uses the `upgraded` texture.
- Otherwise the `normal` texture is used.

The actual draw step happens in `TerrainRenderer::DrawWays(...)`. Each road
segment is emitted as one textured quad, with a direction-specific quad shape
so the strip lines up with the isometric grid. Vertex colors at both ends come
from terrain lighting, so the road brightness follows the same shading and
visibility darkening as the ground below it.

## Flag Rendering Pipeline

Flags are `noFlag` objects. After the terrain pass, `GameWorldView::Draw(...)`
walks the visible map rows and calls `DrawObject(...)`, which forwards to
`noFlag::Draw(...)` when a flag occupies the node.

`noFlag::Draw(...)` is sprite-based:

- it picks an animation frame with `GAMECLIENT.GetGlobalAnimation(...)`
- it draws a cached flag sprite from `LOADER.flag_cache`
- it tints the sprite with the owning player's color
- it then draws ware-stack sprites on top of the flag at fixed local offsets

The cache is built in `Loader::fillCaches()`. For every nation, flag type, and
animation step, the loader composes a `glSmartBitmap` from:

- a player-colored flag bitmap, and
- a matching nation-specific shadow bitmap

Flag appearance depends on `FlagType`:

- `Normal` is the default land flag.
- `Large` is used after donkey-road upgrades.
- `Water` is used when the flag node touches water terrain.

Because flags are drawn in the object pass, figures can be layered around or
over them according to the world-view figure ordering. Roads do not participate
in that ordering because they were already drawn in the terrain pass.

## Fog Of War And Visibility

Roads and flags handle fog of war differently, but both use viewer-mediated
state.

- Roads: `GameWorldViewer::GetVisibleRoad(...)` returns the real road for
  visible nodes, the remembered road from the youngest `FoWNode` for fogged
  nodes, and `None` for invisible nodes.
- Flags: a visible flag draws normally through `noFlag::Draw(...)`; a fogged
  flag draws through `fowFlag::Draw(...)`, which reuses the flag cache with a
  fixed frame and fog tint.

Terrain visibility also affects road brightness indirectly, because the road
quad colors are sampled from terrain vertex colors, and those colors are
reduced for fogged or invisible nodes.

## Sprite Asset Storage

The runtime rendering code mostly works with logical resource IDs and archive
indices, not hard-coded filesystem paths. The actual on-disk lookup follows the
normal resource-loading pipeline described in
`docs/architecture/resource-loading.md`.

For road and flag rendering, the important asset sources are:

- Road textures:
  - resolved from the current landscape description
  - loaded from the current map graphics archive referenced by `mapGfxPath`
  - exposed through `LandscapeDesc::roadTexDesc` as `normal`, `upgraded`,
    `boat`, and `mountain`
- Flag sprites:
  - loaded from the active nation building archive for each nation
  - fetched by numeric indices via `GetNationPlayerImage(...)` and
    `GetNationImage(...)`
  - cached into `LOADER.flag_cache`
- Carrier and boat-carrier sprites:
  - loaded at game start from archives such as `rom_bobs`, `carrier`, `jobs`,
    `boat`, and `wine_bobs`
  - cached into `bob_jobs_cache`, `carrier_cache`, `fat_carrier_cache`, and
    `boat_cache`
- Road-building helper markers and other map overlay icons:
  - loaded from the `map_new` archive

In practical terms, those resources may come from several locations depending
on the installation and overrides:

- original Settlers II asset folders such as `S2/GFX`
- RTTR base assets under `RTTR/assets/base`
- nation override assets under `RTTR/assets/nations/<nation>`
- generic override assets under `RTTR/assets/overrides`
- addon asset folders under `RTTR/assets/addons/...`
- user override folders such as `~/.s25rttr/LSTS` on Linux

So when a road or flag sprite changes visually, the first thing to check is not
just the rendering code, but also which archive and override layer supplied the
final bitmap entry.

## Road Build Preview

Interactive road building uses a visual overlay instead of mutating world road
state on every mouse move.

`dskGameInterface` writes preview segments into
`GameWorldViewer::SetVisiblePointRoad(...)`. The terrain renderer then sees
those preview roads through `GetVisibleRoad(...)` and draws them using the same
pipeline as real roads. When the command succeeds or fails, the viewer removes
the overlay after the corresponding `RoadNote`.

`GameWorldView::DrawGUI(...)` adds the extra UI on top of that preview:

- cursor markers
- current route point highlight
- revert marker
- directional/height helper icons
- target-flag highlight

So the visible road-building experience is a combination of:

- real road-style preview geometry from the terrain renderer, and
- explicit GUI markers from the world-view GUI pass.

## Practical Summary

- Roads are textured terrain quads selected by landscape and road class.
- Flags are animated object sprites loaded per nation and recolored per player.
- Fog-of-war roads come from remembered `FoWNode` road data.
- Fog-of-war flags come from `fowFlag`.
- Road previews are viewer overlays, not world mutations.
