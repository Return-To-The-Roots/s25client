<!--
Copyright (C) 2018 - 2021 Settlers Freaks <sf-team at siedler25.org>

SPDX-License-Identifier: GPL-2.0-or-later
-->

# Resource loading

## Resources

"Resource" generally means graphics/image but there are also text resources or setting files.
The original Settlers 2 resources are organized in archives, files which contain multiple resources which don't need to be of the same type, just like a folder.
In fact we can treat folders like such an archive where each file is an entry in that archive.
Every resource is identified by the file it resides in and an index into that file.
Single files like BMPs are loaded as an archive too, which in this case contains only a single entry: The image itself.

Every filename is internally converted into an "resource identifier" or *resourceId*, which is the lower-cased filename with any extension removed.
This means that `Foo` and `FOO.LST` are treated as the same resource archive with the identifier `foo`.

In the code our main structure for resources is `libsiedler2::Archiv` and we basically load every file as an `Archiv`.
You can think of it as a `std::vector` with entries that can be one of many types.

## Loading

The resources are (mostly) not directly loaded.
Instead there are 2 loading events where a number of archives are loaded from disk:

1. At program start
2. At game start

On program start all archives with GUI resources are loaded and stay loaded during the entire lifetime of the program.
On game start map specific archives are loaded, such as terrain, nation images (buildings & units) and environment objects.
Those are reloaded on every game start according to the map specifications (e.g. summer vs. winter maps).

After those archives are loaded the resources are accessible via the *resourceId* and the index.
However for each resource it must be known at compile time what type (e.g. image) it is.
Mixing that up, e.g. by replacing archive entries with one of the wrong type, will lead to program errors or crashes.

When loading an archive it can be loaded from multiple locations at once and in this order:

1. The main location, e.g. inside the `S2/GFX` or `RTTR/assets/base` folder
2. Nation specific override files: `RTTR/assets/nations/<nation name>`
3. RttR specific override files: `RTTR/assets/overrides`
4. Addon specific files (when enabled): `RTTR/assets/addons/<hex number>`, e.g. `RTTR/assets/addons/0x000000A6`
5. User override files, e.g. `~/.s25rttr/LSTS` on Linux

So when loading an archive each of those folders is searched for a file which matches the *resourceId*.
Those are loaded individually into a final archive.
If the current archive contains a file, that is already present in the final archive, it is replaced, otherwise it is added.
This means that e.g. addons can replace some graphics or add new ones and the user can do that too by adding archives with matching *resourceId* to the user folder.

It is useful to remember that file extensions are not part of the *resourceId* and hence a `*.lst` can override a `*.bob` file and vice-versa.
And as folders are considered archives too one can simply replace graphics by creating a folder with the extensionless name of the archive to override and put `*.bmp`s into it.

Note: Due to the definition of the *resourceId* there can be multiple files for one *resourceId* in one folder.
The order in which those are loaded is unspecified and should be avoided.

Filenames inside folders undergo some parsing to determine some metadata about the file
Each filename is split at each dot into parts:

- The first part is treated as the index into the archive. Hexadecimal suffixes can be used, usually `0x` or `u+` (useful for font glyphs)
- The extension determines the general type:
  - `fon/fonx` for fonts (x for e**x**tended aka unicode fonts)
  - `bmp` for graphics
  - `bbm/act` for palettes
  - `txt/ger/eng` for text
- Other parts can specify subtypes:
  - `rle/player/shadow` for special graphics
  - `palette/paletteanims` for textual palette (animation) definitions
- draw offsets from the origin
  - `nx/ny<number>` for x and y offsets  (this allows to "move" e.g. a building such that the door is at the correct position)
  - Font glyphs use `dx/dy<number>` instead but that value is ignored

A special case are overrides for `*.bob` files:
To override such a file the name must be `<name>.bob.<ext>` for archive files and `<name>.bob` for folders.
This loads the file or folder normally (as an `<ext>` file in the file case) but then adjusts the loaded archive into the internal bob format.
Most importantly for the user the archive is searched for a text file containing overrides for the mapping definition of the bob file.
This allows to create new units and have them be correctly merged on runtime.
See the BOB file format of Settlers 2 for details.
