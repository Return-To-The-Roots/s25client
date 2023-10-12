<!--
Copyright (C) 2005 - 2023 Settlers Freaks <sf-team at siedler25.org>

SPDX-License-Identifier: GPL-2.0-or-later
-->

# Add a custom campaign

You can have a look into the already existing original Settlers 2 campaigns in the subfolders `RTTR/CAMPAIGNS/roman` and `RTTR/CAMPAIGNS/continent` for an example.
We will now create an example campaign `garden`.

## Location for adding a new campaign

To add a new custom campaign you have to create a new subfolder with your campaign name under `RTTR/CAMPAIGNS/` in the install directory or under `<RTTR_USERDATA>/CAMPAIGNS/`.
`<RTTR_USERDATA>` is the placeholder for the RttR folder in your user dir, e.g. `~/.s25rttr` on Linux or "Saved Games" on Windows.  
This subfolder will be filled with all the needed data for the campaign.
We create the subfolder `RTTR/CAMPAIGNS/garden` for our new example campaign.


## Lua campaign description file

First we have to add a `campaign.lua` file to our campaign folder `RTTR/CAMPAIGNS/garden`. This file describes the settings and needed stuff for our new campaign `garden`.

```
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
    image = "<RTTR_RTTR>/CAMPAIGNS/garden/garden.bmp",
    maxHumanPlayers= 1,
    difficulty = "easy",
    mapFolder = "<RTTR_RTTR>/CAMPAIGNS/garden",
    luaFolder = "<RTTR_RTTR>/CAMPAIGNS/garden",
    maps = { "MISS01.WLD","MISS02.WLD"}
}
```

## Explaination of the semantic of the campaign.lua file

The `rttr:RegisterTranslations` function is used for the possibility of providing translation support for the texts displayed in the rttr campaign selection screen. If you do not want to provide any translation you can delete this function from the lua file and write your text directly in the specific fields below.
The `campaign` dict describes your campaign. The `getRequiredLuaVersion` function returns the version of the lua campaign interface.

### Versioning

The lua campaign interface is versioned using a major version. Everytime a feature is added or a breaking change is made (e.g. a function is removed or changes behavior considerably) the major version is increased.

Every map script must have 1 function:
getRequiredLuaVersion()
You need to implement this and return the version your script works with. If it does not match the current version an error will be shown and the script will not be used.

### Explanation of the campaign table fields

If you want a field to be translated you have to add the translation as described above and set the variable to _"<key>". The _"..." will translate the text during application execution depending on your language settings.

  1. `version` simple a number for versioning of the campaign
  2. `author` human readable string of the campaign creator
  3. `name` the name of the campaign
  4. `shortDescription` Short description of the campaign (like a head line to get a rough imagination of the campaign)
  5. `longDescription` Extended description describing the campaign in detail. Will be shown in the campaign selection screen, when the campaign is selected.
  6. `image` Path to an image displayed in the campaign selection screen. This can also be `image=""` if you do no want to provide an image.
  7. `maxHumanPlayers` for now this is always 1 until we support multiplayer campaigns
  8. `difficulty` difficulty of the campaign. Should be one of the valus easy, medium or hard.
  9. `mapFolder` and `luaFolder` Path to the folder containing the campaign maps and associated lua files. Usually your campaign folder or a subfolder of it
  10. `maps` List of the names of the files of the campaigns mission maps

Hints:
- The lua file of a map must have the same name as the map it self but with the extension `.lua` to be found correctly. The lua and the map file must not be in the same folder because the path can be specified differently.
- All paths can contain placeholders like `<RTTR_RTTR>, ...`

## Final view of the example garden campaign folder
```
RTTR/CAMPAIGNS/garden
RTTR/CAMPAIGNS/garden/campaign.lua
RTTR/CAMPAIGNS/garden/garden.bmp
RTTR/CAMPAIGNS/garden/MISS01.lua
RTTR/CAMPAIGNS/garden/MISS01.WLD
RTTR/CAMPAIGNS/garden/MISS02.lua
RTTR/CAMPAIGNS/garden/MISS02.WLD

 ```
