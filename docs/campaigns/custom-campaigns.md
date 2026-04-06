<!--
Copyright (C) 2005 - 2023 Settlers Freaks <sf-team at siedler25.org>

SPDX-License-Identifier: GPL-2.0-or-later
-->

# Add a custom campaign

You can have a look into the already existing original Settlers 2 campaigns in the subfolders `RTTR/campaigns/roman` and `RTTR/campaigns/continent` for an example.
We will now create an example campaign `garden`.

## Location for adding a new campaign

To add a new custom campaign you have to create a new subfolder with your campaign name under `RTTR/campaigns/` in the installation directory or under `<RTTR_USERDATA>/campaigns/`.
`<RTTR_USERDATA>` is the placeholder for the RttR folder in your user directory, e.g. `~/.s25rttr` on Linux or "Saved Games" on Windows.  
This subfolder will be filled with all the needed data for the campaign.
We create the subfolder `RTTR/campaigns/garden` for our new example campaign.

## Lua campaign description file

First we have to add a `campaign.lua` file to our campaign folder `RTTR/campaigns/garden`. This file describes the settings and needed stuff for our new campaign `garden`.

```lua
function getRequiredLuaVersion()
    return 1
end

rttr:RegisterTranslations(
{
    en =
    {
        name = 'Garden campaign',
        shortDescription = 'A lot of flowers',
        longDescription= 'We will plant some sunflowers into our garden.'
    },
})

campaign = {
    version= 1,
    author= "Riese",
    name = _"name",
    shortDescription = _"shortDescription",
    longDescription = _"longDescription",
    image = "<RTTR_RTTR>/campaigns/garden/garden.bmp",
    maxHumanPlayers= 1,
    difficulty = "easy",
    mapFolder = "<RTTR_RTTR>/campaigns/garden",
    luaFolder = "<RTTR_RTTR>/campaigns/garden",
    maps = { "MISS01.WLD","MISS02.WLD"},
    selectionMap = {
        background = {"<RTTR_GAME>/campaigns/garden/mapscreen/background.bmp", 0},
        map = {"<RTTR_GAME>/campaigns/garden/mapscreen/map.bmp", 0},
        missionMapMask = {"<RTTR_GAME>/campaigns/garden/mapscreen/map_mask.bmp", 0},
        marker = {"<RTTR_GAME>/campaigns/garden/mapscreen/marker.bmp", 0},
        conquered = {"<RTTR_GAME>/campaigns/garden/mapscreen/conquered.bmp", 0},
        backgroundOffset = {0, 0},
        disabledColor = 0x70000000,
        missionSelectionInfos = {
            {0xffffff00, 100, 50},
            {0xffaf73cb, 200, 100}
        }
    }
}
```

## Explanation of the semantic of the campaign Lua file

The `rttr:RegisterTranslations` function is used for the possibility of providing translation support for the texts displayed in the RttR campaign selection screen.
If you do not want to provide any translation you can delete this function from the Lua file and write your text directly in the specific fields below.
The `campaign` dict describes your campaign. The `getRequiredLuaVersion` function returns the version of the Lua campaign interface.

### Versioning

The Lua campaign interface is versioned using a major version. Every time a feature is added, or a breaking change is made (e.g. a function is removed or changes behavior considerably) the major version is increased.

Every map script must have 1 function:
`getRequiredLuaVersion()`
You need to implement this and return the version your script works with. If it does not match the current version an error will be shown and the script will not be used.

### Explanation of the campaign table fields

If you want a field to be translated you have to add the translation as described above and set the variable to `_"<key>"`. The `_"..."` will translate the text during application execution depending on your language settings.

  1. `version`: Simple a number for versioning of the campaign
  2. `author`: Human readable string of the campaign creator
  3. `name`: The name of the campaign
  4. `shortDescription`: Short description of the campaign (like a headline to get a rough imagination of the campaign)
  5. `longDescription`: Extended description describing the campaign in detail. Will be shown in the campaign selection screen, when the campaign is selected.
  6. `image`: Path to an image displayed in the campaign selection screen. You can omit this if you do no want to provide an image.
  7. `maxHumanPlayers`: For now this is always 1 until we support multiplayer campaigns
  8. `difficulty`: Difficulty of the campaign. Should be one of the values easy, medium or hard.
  9. `mapFolder` and `luaFolder`: Path to the folder containing the campaign maps and associated Lua files. Usually your campaign folder or a subfolder of it.
  10. `maps`: List of the names of the files of the campaigns' mission maps
  11. `selectionMap`: Optional parameter. See [map selection screen](#selection-map) for detailed explanations.

Hints:

- To work on case-sensitive OS (like Linux) the file name of the Lua file must have the same case as the map file name. This applies to the map names in the campaign.lua file too.
For example: `MISS01.WLD, MISS01.lua` is correct and `MISS01.WLD, miss01.lua` will not work on Linux
- The Lua file of a map must have the same name as the map itself but with the extension `.lua` to be found.
- The Lua and the map file don't need to be in the same folder because the path can be specified separately.
- If `luaFolder` is not specified it defaults to the `mapFolder`, which defaults to an empty value.
- Both paths can start with placeholders like `<RTTR_GAME>`,  
  otherwise they must be (only) the name of a folder relative to the folder containing the campaign Lua file.
  I.e. multiple levels are not supported.  
  In particular an empty value refers to the folder containing the campaign Lua file itself.

### Optional map selection screen {#selection-map}

This parameter is optional and can be omitted in the Lua campaign file. If this parameter is specified the selection screen for the missions of a campaign is replaced by a selection map. Like the one used in the original settlers 2 world campaign.

We have the following parameters:

  1. `background` background image for the selection map
  2. `map` the map image itself
  3. `missionMapMask` this image is a mask that describes the mission areas of the `map` image. It must be the same size as the `map` image where the color of each pixel determines the mission it belongs to. Each mission must have a unique color (specified in the `missionSelectionInfos`). Any other color is treated as neutral area and ignored.
  4. `marker` the marker image shown when a mission is selected
  5. `conquered` the image shown when a mission is already finished
  6. `backgroundOffset` offset of the `map` image and `missionMapMask` image relative to the `background` image. Can be (0,0) if no offset exists.
  7. `disabledColor` color for drawing missions not playable yet. Usually this should be a partly transparent color
  8. `missionSelectionInfos` contains an entry for each mission and must be the same order as specified in `maps` Lua parameter. Each entry consists of three elements. The first is the `maskAreaColor` and the two following are the `ankerPos` x and y position. The `ankerPos` is the position the `conquered` image and the `cursor` image, if mission is selected, are displayed for this mission. The offset is always counted from the origin of the `map` image. The `maskAreaColor` is the color for the mission used in the `missionMapMask`.

Hint:  
All the images are described by the path to the image file and an index parameter. Usually the index parameter is zero.
For special image formats containing multiple images in an archive this is the index of the image to use.

## Final view of the example garden campaign folder

```sh
RTTR/campaigns/garden
RTTR/campaigns/garden/campaign.lua
RTTR/campaigns/garden/garden.bmp
RTTR/campaigns/garden/MISS01.lua
RTTR/campaigns/garden/MISS01.WLD
RTTR/campaigns/garden/MISS02.lua
RTTR/campaigns/garden/MISS02.WLD
RTTR/campaigns/garden/mapscreen/background.bmp
RTTR/campaigns/garden/mapscreen/map.bmp
RTTR/campaigns/garden/mapscreen/map_mask.bmp
RTTR/campaigns/garden/mapscreen/marker.bmp
RTTR/campaigns/garden/mapscreen/conquered.bmp
```
