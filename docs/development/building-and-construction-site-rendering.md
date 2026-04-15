# Building And Construction Site Rendering

This note summarizes how finished buildings and construction sites become
visible on screen in the runtime renderer.

## Rendering Split

Buildings and construction sites are normal map objects. They are not terrain
overlays.

- Terrain, borders, and roads are drawn first by `TerrainRenderer`.
- `GameWorldView` then walks the visible map rows and calls `DrawObject(...)`
  for the object on each visible node.
- Finished buildings and construction sites both derive from `noBaseBuilding`,
  so the final drawing behavior is selected by the object's virtual
  `Draw(...)` implementation.

That means buildings and construction sites follow the normal object draw
order. Figures, boundary stones, fog-of-war objects, construction aid, and GUI
markers are layered around them by `GameWorldView`, not by the terrain
renderer.

## Finished Building Rendering Pipeline

Finished buildings are `noBuilding` objects or subclasses of it. A construction
site becomes a finished building when `nofBuilder` detects that
`noBuildingSite::IsBuildingComplete()` is true. The builder removes the site
object, then calls `BuildingFactory::CreateBuilding(...)` to place the concrete
building subclass at the same map point.

The factory selects the runtime class from `BuildingType`:

- `nobHQ` for headquarters
- `nobStorehouse` for storehouses
- `nobHarborBuilding` for harbor buildings
- `nobMilitary` for barracks, guardhouses, watchtowers, and fortresses
- `nobShipYard` for shipyards
- `nobTemple` for temples
- `nobUsual` for the other production buildings

The common finished-building draw path is `noBuilding::DrawBaseBuilding(...)`.
It draws:

1. the cached building sprite from `noBaseBuilding::GetBuildingImage()`
2. the cached door sprite from `noBaseBuilding::GetDoorImage()` when the
   building door is open

Most concrete building classes call `DrawBaseBuilding(...)` first and then add
their own overlays:

- `nobUsual::Draw(...)` adds suspended-production signs, smoke for working
  buildings, and special props such as idle mill blades, donkey-breeder
  donkeys, idle catapults, pig-farm pigs, or Nubian mine fire.
- `nobMilitary::Draw(...)` adds garrison flags, frontier-distance flags, and a
  gold-disabled sign.
- `nobHQ::Draw(...)` draws either the headquarters tent skeleton or the normal
  building plus reserve-soldier flags.
- `nobHarborBuilding::Draw(...)` adds harbor fires and expedition wares while
  an expedition is active.
- `nobStorehouse::Draw(...)` just draws the base building.

Because those effects are added by the object draw method, they use normal
sprite ordering rather than terrain lighting. The building sprites are drawn
with their own cached shadows instead of sampling terrain vertex colors.

## Construction Site Rendering Pipeline

Construction sites are `noBuildingSite` objects. `GameWorld::SetBuildingSite(...)`
creates one after validating the building type, building quality, and nearby
military-building restrictions. Harbor expeditions create a harbor construction
site through the separate ship-founded colony path.

`noBuildingSite` has two visual states:

- `Planing` means the terrain is still being leveled.
- `Building` means the foundation and partial building are visible.

In the leveling state, `noBuildingSite::Draw(...)` draws only the nation-specific
construction sign and its shadow:

- nation image index `450`
- shadow image index `451`

In the building state, it draws:

1. the nation-specific foundation stone and shadow, image indices `455` and
   `456`
2. delivered board and stone stacks near the building door point
3. the partially completed skeleton sprite
4. the partially completed final building sprite

The partial draw is based on `build_progress`. Builders increase this value in
eight steps per consumed construction ware. The site splits progress into two
phases:

- the skeleton phase, based on the building's board cost
- the finished-building phase, based on the building's stone cost

For buildings that require no stones, the site splits the board-only work into
a 50:50 skeleton/building progression. The actual clipping is performed by
`glSmartBitmap::drawPercent(...)`, called on
`LOADER.building_cache[nation][type].skeleton` and
`LOADER.building_cache[nation][type].building`.

The construction-site door point comes from `noBaseBuilding::GetDoorPoint()`.
That same base class also creates or reuses the flag in front of the building
and the short road segment between the building point and its flag, so the
rendered site already participates in the normal road and flag visuals.

## Fog Of War And Visibility

Visible buildings and construction sites draw through their live objects:

- finished buildings call the concrete `noBuilding` subclass `Draw(...)`
- construction sites call `noBuildingSite::Draw(...)`

Fogged buildings and construction sites use saved `FOWObject` snapshots instead.
`World::SaveFOWNode(...)` stores `GetNO(pt)->CreateFOWObject()` in the
`FoWNode`. Later, `GameWorldView::Draw(...)` asks
`GameWorldViewer::GetYoungestFOWObject(...)` for the remembered object and draws
that instead of the live object.

The two building-related fog objects are:

- `fowBuilding`, created by `noBuilding::CreateFOWObject()`, which draws the
  finished building image with the fixed fog tint.
- `fowBuildingSite`, created by `noBuildingSite::CreateFOWObject()`, which
  remembers the leveling/building state and `build_progress`, then draws the
  same sign/foundation/partial-building logic with fog tint.

Fogged finished buildings intentionally draw only the base building image. Door
state, worker props, smoke, garrison flags, and other live overlays are not
reconstructed by `fowBuilding`.

## Sprite Asset Storage

The runtime rendering code mostly works with logical resource IDs and archive
indices, not hard-coded filesystem paths. The actual on-disk lookup follows the
normal resource-loading pipeline described in
`docs/architecture/resource-loading.md`.

For building and construction-site rendering, the important asset sources are:

- Finished building sprites:
  - loaded from the active nation building archive for each nation
  - fetched by `GetNationImage(...)` at `250 + 5 * BuildingType`
  - paired with the shadow at `250 + 5 * BuildingType + 1`
  - cached into `LOADER.building_cache[nation][type].building`
- Construction skeleton sprites:
  - normally fetched from `250 + 5 * BuildingType + 2`
  - paired with the shadow at `250 + 5 * BuildingType + 3`
  - cached into `LOADER.building_cache[nation][type].skeleton`
  - for headquarters, replaced by the `mis0bobs` tent graphics
- Door sprites:
  - fetched from `250 + 5 * BuildingType + 4`
  - cached into `LOADER.building_cache[nation][type].door`
- Charburner sprites:
  - loaded from the separate `charburner` archive
  - use winter-specific building and door variants when winter graphics are
    active
- Construction-site signs and foundation stones:
  - loaded from the active nation building archive at indices `450`, `451`,
    `455`, and `456`
- Building overlays:
  - smoke, suspended-production signs, military flags, animal props, fires, and
    ware stacks come from a mix of `map_new`, current map graphics, nation
    archives, `rom_bobs`, and ware-stack caches

In practical terms, those resources may come from several locations depending
on the installation and overrides:

- original Settlers II asset folders such as `S2/GFX`
- RTTR base assets under `RTTR/assets/base`
- nation override assets under `RTTR/assets/nations/<nation>`
- generic override assets under `RTTR/assets/overrides`
- addon asset folders under `RTTR/assets/addons/...`
- user override folders such as `~/.s25rttr/LSTS` on Linux

So when a building or construction-site sprite changes visually, the first
thing to check is not just the draw method, but also which archive and override
layer supplied the final bitmap entry.

## Name And Productivity Overlays

Building names and production information are drawn in a separate overlay pass,
not inside the building object.

After visible rows are drawn, `GameWorldView::DrawNameProductivityOverlay(...)`
iterates the visible nodes again and looks for `noBaseBuilding` objects. When
enabled, it draws:

- the localized building name
- grey construction-site progress for `GO_Type::Buildingsite`
- productivity or stopped/unoccupied text for usual production buildings
- troop count and extra military debug values for military buildings

This separation matters because the building sprite can be unchanged while the
text overlay changes with UI settings or debug instrumentation.

## Practical Summary

- Buildings and construction sites are object-pass sprites, not terrain
  overlays.
- Finished buildings draw from `LOADER.building_cache` and then add
  subclass-specific props.
- Construction sites draw either a leveling sign or foundation plus clipped
  skeleton/building sprites.
- Builders drive visible construction progress by incrementing
  `build_progress`.
- Fog-of-war buildings and construction sites come from remembered `FOWObject`
  snapshots.
- Asset changes can come from nation archives and override layers even when
  rendering code has not changed.
