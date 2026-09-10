<!--
Copyright (C) 2005 - 2026 Settlers Freaks <sf-team at siedler25.org>

SPDX-License-Identifier: GPL-2.0-or-later
-->

# Content

[Events](events.md)  
[Functions](functions.md)  
[Constants](constants.md)  

## Map scripts

LUA scripts are loaded from `path/to/map.lua` for a map at `path/to/map.swd`.
The game will look for a file ending in `'.lua'` instead of `'.swd'`.
Please be aware that case matters for some operating systems.
The file will be sent to all clients, overwriting files in the user's own rttr MAPS directory.  

## Interface overview

The new LUA interface is object oriented.
This means all RTTR entities are objects and hence functions must be invoked with `rttr:MyFunc()`.

LUA scripts (can) contain 2 parts:
One that is used during game creating (e.g. set resources, nations, fixed settings...)
and one that runs during the game and reacts on events.
This is important as some functions are only available during the 'Settings' state
and other only during the 'Game' state.
Calling an unavailable function will lead to aborting the script and stopping the game,
but you usually get a descriptive message in the console window and log.

Players are represented in 2 ways:
Either by their `playerIdx`, a zero-based index into an array of players.
Or by a player object which has methods to call.
The interface distinguishes between them by using naming conventions like `playerIdx` and `player`.

## Versioning

The lua interface is versioned using a main version and a feature level.
Everytime a feature is added the feature level is increased.
When a breaking change is made (e.g. a function is removed or changes behavior considerably) the main version is increased and the feature level reset to zero.

Every map script must have 1 function:  
**getRequiredLuaVersion()**  
You need to implement this and return the major/main version your script works with.
If it does not match the current version an error will be shown and the script will not be used.
See also `rttr:GetFeatureLevel()`.

### Feature level 7

- Allow relative paths for images referenced by campaign files.

## Example

```lua
    function getRequiredLuaVersion()
        return 1

    -- start callback, called after everything is set up
    function onStart(isFirstStart)
        if(not isFirstStart) then
            return
        end
        -- add 100 generals and 10 officers for player 0
        rttr:GetPlayer(0):AddPeople({[JOB_GENERAL] = 100, [JOB_OFFICER] = 10})
        -- disable barracks (building 1) for player 0
        rttr:GetPlayer(0):DisableBuilding(BLD_BARRACKS)

        -- restrictions for player 0
        rttr:GetPlayer(0):SetRestrictedArea(0,42, 255,42, 255,255, 0,255)
    end

    function onGameFrame(no)
        -- every 100 gf...
        if no % 100 == 0 then
            -- for all players...
            for player = 1, rttr:GetNumPlayers() do
                pl = rttr:GetPlayer(player - 1)
                rttr:Chat(-1, "[Player " .. pl:GetName() .. " @GF " .. rttr:GetGF() .. "] helpers: " ..
                               pl:GetNumPeople(JOB_HELPER) .. ", wells: " .. pl:GetNumBuildings(BLD_WELL) ..
                               ", water: " .. pl:GetNumWares(GD_WATER))
            end
        end
    end

    -- just output a message for now
    function onOccupied(player, x, y)
        rttr:Log("Point occupied: " .. player .. ": " .. x .. "," .. y)
    end

    -- just output a message for now
    function onExplored(player, x, y)
        rttr:Log("Point explored: " .. player .. ": " .. x .. "," .. y)
    end
```

For a more complete example usage of all functions have a look at [tests/testData/maps/LuaFunctions.lua](../../tests/testData/maps/LuaFunctions.lua)

[Back](#Content)
