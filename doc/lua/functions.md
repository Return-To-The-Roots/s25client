<!--
Copyright (C) 2005 - 2021 Settlers Freaks <sf-team at siedler25.org>

SPDX-License-Identifier: GPL-2.0-or-later
-->

# Lua objects and their methods

There are the following objects defined:

- [`rttr`: Main object](#Main-object-rttr)
- [`PlayerBase`: Player methods for settings and game mode](#PlayerBase)
- [`SettingsPlayer`: Player methods for settings mode](#SettingsPlayer)
- [`GamePlayer`: Player methods for game mode](#GamePlayer)
- [`World`: Represents the game world](#World)
- [`Serializer`: FIFO buffer for storing persistent data](#Serializer)

## Main object `rttr`

### Base functions (callable in settings and game mode)

Reference: [libs/libGamedata/lua/LuaInterfaceBase.cpp](../../libs/libGamedata/lua/LuaInterfaceBase.cpp)

**rttr:GetFeatureLevel()**  
Get the current feature level of the LUA interface.
Increases here indicate new features.
The current version is **6**.

**rttr:Log(message)**  
Log the message to console.

**rttr:IsHost()**  
Return true/false if the local player is the host.

**rttr:GetNumPlayers()**  
Return the number of players.

**rttr:GetLocalPlayerIdx()**  
Return 0-based index of the local player.

**rttr:MsgBox(title, message, isError = false)**  
Show a message box to the current user.
`isError` is used to show a red or green icon.
A scrollbar will be shown for long text.  
Note: You can have multiple message boxes opened.
First one will be on top until closed.

**rttr:MsgBoxEx(title, message, iconFile, iconIdx[, iconX, iconY])**  
Show a message box to the current user.
`iconIdx` is the index into `iconFile` (name w/o extension) of the icon to use.
The file must be loaded at this point!
(Safe to use "io").
The window is automatically adjusted to fit the image.
The text is moved, if possible, otherwise it is drawn over the image.
Place the image at the border if you don't want this.

Notes:

- You can have multiple message boxes opened.
First one will be on top till closed  
- You can not do any action until the window is closed  
- You should use an offset (`iconX`/`iconY`) as 0:0 leads to a graphic that is drawn in the upper left over the border (16,24 recommended)  

**rttr:RegisterTranslations(translations)**  
Register translations for multilanguage scripts, by passing a table containing languages as keys.
Example:

```lua
rttr:RegisterTranslations(
{
    en_GB =
    {
        World   = 'Hello World',
        Message = 'Have a nice day'
    },
    de =
    {
        World   = 'Hallo Welt',
        Message = 'Hab einen schönen Tag'
    }
}
```

In order to retrieve the Value, you simply use the following call:

```lua
_('World')
```

In this example,  

- If the client language is English, `Hello World` is returned  
- If the client language is German, `Hallo Welt` is returned  
- If the client language is Czech (or any other than english or german), the key is returned, in this case `World`  

[Back](#Lua-objects-and-their-methods)  

### Settings functions (callable in settings mode only)

Reference: [lua/LuaInterfaceSettings.h](../../libs/s25main/lua/LuaInterfaceSettings.h)  
**Important: All functions can only be used by the host!**

**rttr:GetPlayer(playerIdx)**  
Get the player with the given index, returns a [`SettingsPlayer`](#SettingsPlayer)

**rttr:SetAddon(addonId, value)**  
Change setting of an addon.
The addonId is the internal AddonID (see [const_addons.h](../../libs/s25main/addons/const_addons.h#L50)) with prefix ADDON_.

```lua
rttr:SetAddon(ADDON_FRONTIER_DISTANCE_REACHABLE, true)
```

For addons which can be enabled / disabled simply use true or false, for addons with different settings, add the number according to your setting.  

**rttr:ResetAddons()**  
Set all addons to S2 defaults

**rttr:ResetGameSettings()**  
Set all settings and addons to their defaults.
Note: Exploration is currently FoW.
Will be classic later.

**rttr:SetGameSettings(settings)**  
Set the settings with a table of keys:  

- speed: GS_VERYSLOW, GS_SLOW, GS_NORMAL, GS_FAST, GS_VERYFAST
- objective: GO_NONE, GO_CONQUER3_4, GO_TOTALDOMINATION
- startWares: SWR_VLOW, SWR_LOW, SWR_NORMAL, SWR_ALOT
- fow: EXP_DISABLED, EXP_CLASSIC, EXP_FOGOFWAR, EXP_FOGOFWAREXPLORED
- lockedTeams, teamView, randomStartPosition: true/false

[Back](#Lua-objects-and-their-methods)  

### Gamefunctions (callable in game mode only)

Reference: [lua/LuaInterfaceGame.h](../../libs/s25main/lua/LuaInterfaceGame.h)  
**Important: All state-changing functions should be executed by all players at the same time (GF), or there will be asyncs!**

**rttr:ClearResources()**  
Remove all workers and wares from all players warehouses.

**rttr:GetGF()**  
Return the current game frame number.

**rttr:FormatNumGFs(numGFs)**  
Return the real time duration for this number of game frames based on the current speed.
Output will be in `HH:MM:SS` format with hours omitted if zero.

**rttr:Chat(player, message)**  
Send message to player (-1 for all players).

**rttr:MissionStatement(player, title, message, imgIdx = IM_SWORDSMAN, pause = true)**  
Send mission statement to player, pauses the game in single player if pause is set.  
Possible images: `IM_NONE, IM_SWORDSMAN, IM_READER, IM_RIDER, IM_AVATAR1-IM_AVATAR12` (see [iwMissionStatement](../../libs/s25main/ingameWindows/iwMissionStatement.h#L28))

```lua
rttr:MissionStatement(0, "Diary", "Forth day\n\n\nThere are " .. rttr:GetNumPlayers() .. " players.")
```

Note: You can have multiple message boxes opened.
First one will be on top till closed.
_Not saved in savegame!_

**rttr:PostMessage(player, message)**  
Send post message to player.

**rttr:PostMessageWithLocation(player, message, x, y)**  
Send post message to player (with location x, y).

**rttr:GetPlayer(playerIdx)**  
Get the player with the given index, returns a [`GamePlayer`](#GamePlayer)

**rttr:GetWorld()**  
Get the world object, returns a [`World`](#World)

[Back](#Lua-objects-and-their-methods)  

## PlayerBase

Reference: [lua/LuaPlayerBase.h](../../libs/s25main/lua/LuaPlayerBase.h)  
This is the base object for [`GamePlayer`](#GamePlayer) and [`SettingsPlayer`](#SettingsPlayer).
Hence these methods are available in settings and game stage.

**GetName()**  
Return the players name.

**GetNation()**  
Return the nation, one of `NAT_AFRICANS, NAT_JAPANESE, NAT_ROMANS, NAT_VIKINGS, NAT_BABYLONIANS`

**GetTeam()**  
Return the players team, one of `TM_NONE, TM_RANDOM, TM_TEAM1, TM_TEAM2, TM_TEAM3, TM_TEAM4, TM_TEAM1TO2, TM_TEAM1TO3, TM_TEAM1TO4`  
See [TeamTypes](../../libs/s25main/gameTypes/TeamTypes.h) for enums.

**GetColor()**  
Return color index if found or player color
Color indices always have an alpha value.

**IsHuman()**  
Return true if the player is controlled by a human player.

**IsAI()**  
Return true if the player is controlled by an AI player.

**IsClosed()**  
Return true if the slot is closed.

**IsFree()**  
Return true if the slot is open and not controlled by a human or AI player.

**GetAILevel()**  
Return the AI difficulty level.

- -1: Not an AI
- 0: Dummy
- 1-3: Easy-Hard

[Back](#Lua-objects-and-their-methods)  

## SettingsPlayer

Reference: [lua/LuaServerPlayer.h](../../libs/s25main/lua/LuaServerPlayer.h)  
Available in settings mode only.

**SetNation(Nation)**  
Change the players nation.

**SetTeam(Team)**  
Change the players team.

**SetColor(color or colorIdx)**  
Sets the players color by index or directly if its alpha value is not zero.
Duplicate color values are possible, so you have to ensure unique colors if you want them!

**Close()**  
Closes a spot kicking any player or AI there.

**SetAI(level)**  
Add an AI or change its difficulty.

**SetName(name)**  
Change the player's name.

**SetPortrait(portraitIndex)**  
Change the player's portrait.

[Back](#Lua-objects-and-their-methods)  

## GamePlayer

Reference: [lua/LuaPlayer.h](../../libs/s25main/lua/LuaPlayer.h)  
This is the game state player object.

**player:EnableBuilding(type, notify)**  
Enable the building specified for the player.
If notify is true, the players gets a notification that the building is now available.  
_Not saved in savegame!_

**DisableBuilding(type)**  
Disable the building for the player.
_Not saved in savegame!_

**EnableAllBuildings()**
Enable all buildings for the player.
_Not saved in savegame!_  

**DisableAllBuildings()**  
Disable all buildings for the player.
_Not saved in savegame!_

**SetRestrictedArea(x1,y1, ...)**  
Restrict the area a player may have territory in.
If called without any x/y-values (e.g. `rttr:GetPlayer(1):SetRestrictedArea()`), the restrictions are lifted.
Otherwise, the coordinates specify one or multiple polygons the player may have territory in (replacing older restrictions).
A polygon inside a polygon will create a hole in it.
To specify multiple polygons, start with a 0,0-pair followed by the first polygon, another 0,0-pair, the second polygon etc. and a final 0,0-pair at the end.
For multiple polygons, each polygons first point has to be repeated at the end of it.  

Notes:

1. It is unspecified whether a point on the boundary of a polygon is inside or outside of it.
However if they are inside a polygon, then they are outside of a contained or containing polygon.
This allows creation of mutual exclusive areas like allowing an area only for a player and disallowing this area for another player.
2. RTTR/S2 uses a triangle-based grid.
This may cause surprises when using this function as you cannot easily describe a "real" rectangle.
Generally consider the border area as a "maybe" unless you tested it exactly.
3. The current implementation uses the crossing-count method described [here](http://geomalgorithms.com/a03-_inclusion.html).
But it assumes a rectangle grid, so every 2nd row is shifted by half a triangle for the means of the algorithm.  

**IsInRestrictedArea(x, y)**  
Return true if (x,y) is in the restricted area.
Restricted means, building is allowed.
The `SetRestrictedArea` function defines where you may build buildings and not where you are not allowed to.  

**ClearResources()**  
Remove all wares and workers from all warehouses

**AddWares(wareMap)**  
Add the specified goods to the players first warehouse.
Keys must be `GoodType`s and values unsigned numbers

**AddPeople(peopleMap)**  
Add the specified people to the player's first warehouse.
Keys must be `Job`s and values unsigned numbers

**GetNumBuildings(building_type)**  
Get number of buildings a player has of a given type.

**GetNumBuildingSites(building_type)**
Get number of building sites a player has of a given type.

**GetNumWares(ware_type)**  
Get number of wares a player has of a given type.

**GetNumPeople(job_type)**  
Get number of people a player has with a given job.

**GetStatisticsValue(statistic_type)**  
Get player's current statistic value of given type.

**AIConstructionOrder(x,y,buildingtype)**  
Order AI to build buildingtype at the given x,y location (AI will usually retry at a nearby location if the location is bad)
Returns true, if the order was submitted, false otherwise.  
Ignored, if player is not the host (returns always true)

**ModifyHQ(isTent)**  
Set whether the players HQ is shown as a tent (1) or not (0)

**IsDefeated()**  
Return true if the player is defeated

**IsAlly(otherPlayerIdx)**  
Return true if the player is an ally of this player

**IsAttackable(otherPlayerIdx)**  
Return true if the player can be attacked

**SuggestPact(otherPlayerIdx, PactType, duration)**  
Let the AI send a request to the other player for a new pact.
Ignored if the current player is not controlled by AI.

**CancelPact(PactType, otherPlayerIdx)**  
Let the AI send a request to the other player to cancel a pact.
Ignored if the current player is not controlled by AI.

**GetHQPos()**  
Return x,y of the players HQ

**Surrender(destroyBuildings)**  
Let the player give up either keeping its buildings or destroying them.
Can be called multiple times e.g. to destroy buildings later.

[Back](#Lua-objects-and-their-methods)  

## World  

Reference: [libs/s25main/lua/LuaWorld.h](../../libs/s25main/lua/LuaWorld.h)

**AddEnvObject(x, y, id, file = 0xFFFF)**  
Place a new environment object.
Only environment/static objects and empty space can be overwritten.

- `id`: id in file
- `file`: `0xFFFF` = map_?_z.lst, `0-5` = mis?bobs.lst

**AddStaticObject(x, y, id, file = 0xFFFF, size = 0)**  
Place a new static object.
Only environment/static objects and empty space can be overwritten.

- `id`: id in file
- `file`: `0xFFFF` = map_?_z.lst, `0-5` = mis?bobs.lst
- `size`: 0: Block nothing (destructible), 1: Block single spot, 2: Block spots like castle-sized buildings

**AddAnimal(x, y, species)**  
Adds an animal.
If the terrain is unsuitable the animal will die soon.
`species` must be a [SPEC_*](constants.md#Animals) constant.

```lua
rttr:GetWorld():AddAnimal(41, 44, SPEC_DUCK)
```

[Back](#Lua-objects-and-their-methods)  

**SetComputerBarrier(radius, x, y)**
Defines an area with a specified (inclusive) radius around map point {x, y} in which the AI cannot build military buildings.  
-- Added in LUA version 1.5

## Serializer

The following methods are available.
Remember that this is a FIFO structure, hence what is `Push`ed first needs to be `Pop`ped first.

**PushInt(value)**  
**PushBool(value)**  
**PushString(value)**  

**number PopInt()**  
**true/false PopBool()**  
**string PopString()**
