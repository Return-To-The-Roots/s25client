<!--
Copyright (C) 2018 - 2021 Settlers Freaks <sf-team at siedler25.org>

SPDX-License-Identifier: GPL-2.0-or-later
-->

# Events

This lists all events, that is functions your script can implement and which are called at specific times,
e.g. when something happens.

- [Events during settings stage](#Settings)  
- [Events during game stage](#Game)  

## Settings

**onSettingsInit(isSinglePlayer, isSavegame)**  
Called when the game settings screen is entered.
Can be used to set up some dependent variables.  
**ONLY** settings event called for all players.
All other events are called only for host

**onSettingsReady()**  
Called when the settings screen is initialized and player manipulations are possible

**onPlayerJoined(playerIdx)**  
Called whenever a player joined.

**onPlayerLeft(playerIdx)**  
Called when a player has left.

**onPlayerReady(playerIdx)**  
Called when a player pressed "ready".
For the host player this means the game is starting (or host tries to).

**getAllowedChanges()**  
Should return a map with string keys and bool values.
If the bool is false, the given option cannot be changed.
If a key does not exist, a default value is used:  

- general(false): Settings like lock teams, team view, ...
- addonsAll(false): Cannot change any addon
- addonsSome(false): Can only change addons in list returned by `getAllowedAddons`
- swapping(false): Swap places
- playerState(false): Change player slots (add/change AI, ...)
- ownNation, ownColor, ownTeam (all true): Change values player
- aiNation, aiColor, aiTeam (all true): Change values of AI  

```lua
function getAllowedChanges()
    return { ["general"] = true}
end
```

**getAllowedAddons()**  
Only meaningfull if `addonsAll=false` and `addonsSome=true`:
Return a list with addonIds that the host can change.

## Game

**onStart(isFirstStart)**  
Called at the start of the game, after the headquarters are placed.
`isFirstStart` is true, if the game was just created and false for savegames.

**onSave(serializer)**  
Called when the game is being saved.
LUA data can be saved to the passed [`Serializer`](methods.md#Serializer) object.
Needs to return true on success or false on error.

**onLoad(serializer)**  
Called when the game is loaded (before onStart).
LUA data can be loaded from the [`Serializer`](methods.md#Serializer) object in the same order they are saved (FIFO).
Needs to return true on success or false on error.

**onOccupied(playerIdx, x, y)**  
Called every time a point on the map gets occupied by a player.

**onExplored(playerIdx, x, y, owner)**  
Called every time a point on the map becomes visible for a player.
The owner parameter contains the owner's player id, _nil_ means that there is no owner.

**onGameFrame(gameframeNumber)**  
Gets called every game frame.

**onResourceFound(playerIdx, x, y, type, quantity)**  
Given resource (RES_IRON, RES_GOLD, RES_COAL, RES_GRANITE or RES_WATER) was found at x,y.  

**onCancelPactRequest(pactType, fromPlayerIdx, toPlayerIdx)**  
Only called for AI.
Called when `fromPlayer` requests to cancel a pact.
Return false to decline.  

**onSuggestPact(PactType, suggestedByPlayerIdx, targetPlayerIdx, duration)**  
Only called for AI.
Player suggests a pact
Return true to accept or false to decline.

**onPactCanceled(PactType, canceledByPlayerIdx, targetPlayerIdx)**  
Called when a pact has been canceled.

**onPactCreated(PactType, suggestedByPlayerIdx, targetPlayerIdx, duration)**  
Called when a pact has been confirmed.
