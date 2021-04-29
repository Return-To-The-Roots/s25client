-- Copyright (C) 2005 - 2021 Settlers Freaks <sf-team at siedler25.org>
--
-- SPDX-License-Identifier: GPL-2.0-or-later

gfCounter = 0
isSinglePlayer = false
isSavegame = false

rttr:Log("LUA: Test Log from Lua")

function getRequiredLuaVersion()
	return 1
end

function getAllowedAddons()
	return {ADDON_LIMIT_CATAPULTS, ADDON_CHARBURNER, ADDON_TRADE}
end

function getAllowedChanges()
	return {general=true, addonsAll=false, addonsSome=true, swapping=false, playerState = not isSinglePlayer, ownNation = true, ownColor=true, ownTeam=false, aiNation = false, aiColor=false, aiTeam=false}
end

function onSettingsInit(isSinglePlayerArg, isSavegameArg)
	isSinglePlayer = isSinglePlayerArg
	isSavegame = isSavegameArg
	return true
end

function onSettingsReady()
	rttr:Log("LUA: Starting a game with "..rttr:GetPlayerCount().." players")
	if(isSavegame) then
		return
	end
	
	assert(rttr:IsHost())
	assert(rttr:GetPlayerCount() == 3)
	
	-- Human
	player = rttr:GetPlayer(0)
	assert(player:IsHuman())
	assert(not player:IsAI())
	assert(not player:IsClosed())
	
	player:SetNation(NAT_BABYLONIANS)
	assert(player:GetNation() == NAT_BABYLONIANS)
	
	player:SetTeam(TM_TEAM2)
	assert(player:GetTeam() == TM_TEAM2)
	
	player:SetColor(5)
	assert(player:GetColor() == 5)
	
	-- AI
	player = rttr:GetPlayer(1)
	if(not isSinglePlayer) then
		assert(not player:IsAI())
		assert(player:IsFree())
		rttr:Log("LUA: Adding easy AI to slot 1")
		player:SetAI(1)
		assert(not rttr:GetPlayer(2):IsAI())
		assert(rttr:GetPlayer(2):IsFree())
		rttr:Log("LUA: Adding hard AI to slot 2")
		rttr:GetPlayer(2):SetAI(3)
	end
	
	assert(not player:IsHuman())
	assert(player:IsAI())
	assert(not player:IsClosed())
	rttr:Log("LUA: Closing slot 1")
	player:Close()
	assert(not player:IsAI())
	assert(player:IsClosed())
	rttr:Log("LUA: Adding medium AI to slot 1")
	player:SetAI(2) -- Medium
	assert(not player:IsHuman())
	assert(player:IsAI())
	assert(not player:IsClosed())
	assert(player:GetAILevel() == 2)
	
	rttr:ResetAddons()
	rttr:SetAddon(ADDON_MILITARY_AID, true);
	rttr:SetAddon(ADDON_MILITARY_HITPOINTS, true);
	
	settings = {speed=GS_VERYFAST, objective=GO_TOTALDOMINATION, startWares=SWR_VLOW, fow=EXP_FOGOFWAR, lockedTeams=true, teamView=true, randomStartPosition=false}
	rttr:SetGameSettings(settings)
	
	settingsStr = ""
	for key,value in pairs(settings) do
		settingsStr = settingsStr..key.."="..tostring(value)..", "
	end
	rttr:Log("LUA: Settings changed to: "..settingsStr)
	rttr:Log("LUA: Setting changes verified. Please check the other settings (Addons: Catapult, Charburner, Trade should be changeable, rest not)")
	if(not isSinglePlayer) then
		rttr:MsgBox("Lua status", "You should now test player joining by opening a spot and join from a 2nd instance")
	end
end

function onPlayerJoined(idx)
	assert(rttr:IsHost())
	assert(rttr:GetPlayer(idx):IsHuman())
	rttr:Log("LUA: Player "..idx.." joined")
end

function onPlayerLeft(idx)
	assert(rttr:IsHost())
	assert(not rttr:GetPlayer(idx):IsHuman())
	rttr:Log("LUA: Player "..idx.." left")
end

function onPlayerReady(idx)
	rttr:Log("LUA: Player "..idx.." "..rttr:GetPlayer(idx):GetName().." is ready")
	if(rttr:GetLocalPlayerIdx() == idx) then
		rttr:Log("LUA: Game is starting")
	end
end

function addPlayerRes(pl)
	wares = {[GD_HAMMER]=8,[GD_AXE]=6,[GD_SAW]=3,[GD_PICKAXE]=6,[GD_RODANDLINE]=2,[GD_SCYTHE]=6,[GD_CLEAVER]=1,[GD_ROLLINGPIN]=1,[GD_BOW]=2,[GD_BOAT]=20,[GD_SWORD]=0,[GD_SHIELD]=0,[GD_WOOD]=20,[GD_BOARDS]=40,[GD_STONES]=40,[GD_GRAIN]=0,[GD_COINS]=0,[GD_IRONORE]=18,[GD_COAL]=36,[GD_FISH]=8,[GD_BREAD]=8}
	people = {[JOB_HELPER]=30,[JOB_WOODCUTTER]=6,[JOB_FISHER]=0,[JOB_FORESTER]=2,[JOB_CARPENTER]=2,[JOB_STONEMASON]=4,[JOB_HUNTER]=1,[JOB_MINER]=10,[JOB_BREWER]=1,[JOB_IRONFOUNDER]=2,[JOB_MINTER]=1,[JOB_METALWORKER]=1,[JOB_ARMORER]=2,[JOB_BUILDER]=14,[JOB_PLANER]=8,[JOB_PRIVATE]=10,[JOB_PRIVATEFIRSTCLASS]=1,[JOB_SERGEANT]=1,[JOB_OFFICER]=1,[JOB_GENERAL]=20,[JOB_GEOLOGIST]=4,[JOB_SHIPWRIGHT]=0,[JOB_SCOUT]=4}
	pl:AddWares(wares)
	for gd,ct in pairs(wares) do
		assert(pl:GetWareCount(gd) == ct, "Ware count of " .. gd .. "!=" .. ct)
	end
	pl:AddPeople(people)
	for job,ct in pairs(people) do
		assert(pl:GetPeopleCount(job) == ct, "Job count of " .. job .. "!=" .. ct)
	end
end

function testPlayerFuncs(pl)
	pl:DisableAllBuildings()
	pl:EnableAllBuildings()
	pl:DisableBuilding(BLD_FORESTER)
	pl:DisableBuilding(BLD_WOODCUTTER)
	pl:EnableBuilding(BLD_WOODCUTTER)
	pl:ClearResources()
	assert(pl:GetWareCount(GD_BEER) == 0)
	addPlayerRes(pl)
	pl:SetRestrictedArea(5,2, 5,10, 10,10, 10,2)
	assert(pl:GetBuildingCount(BLD_MILL) == 0)
	pl:ModifyHQ(true)
end

function onSave(saveGame)
	saveGame:PushBool(true)
	saveGame:PushInt(42)
	saveGame:PushString("Hello RttR!")
	return true
end

isLoaded = false

function onLoad(saveGame)
	assert(isLoaded == false)
	isLoaded = true
	assert(saveGame:PopBool() == true)
	assert(saveGame:PopInt() == 42)
	assert(saveGame:PopString() == "Hello RttR!")
	rttr:Log("LUA: Lua state loaded!")
	return true
end

function onStart(isFirstStart)
	assert(gfCounter == 0)
	if(not isFirstStart) then
		gfCounter = rttr:GetGF()
	end
	assert(rttr:GetGF() == gfCounter)
	assert(isLoaded ~= isFirstStart)
	assert(rttr:GetPlayerCount() > 1)
	if(isFirstStart) then
		for i=1,rttr:GetPlayerCount() do
			testPlayerFuncs(rttr:GetPlayer(i - 1))
		end
	end
end

function onGameFrame()
	gfCounter = gfCounter + 1
	assert(gfCounter == rttr:GetGF())
	if(gfCounter == 50) then
		if(rttr:IsHost()) then
			assert(rttr:GetPlayer(0):AIConstructionOrder(10,10, BLD_WOODCUTTER) == false) -- Human
			assert(rttr:GetPlayer(1):AIConstructionOrder(10,10, BLD_WOODCUTTER) == true)  -- AI
		end
		
		rttr:Log("Adding wiking at 49:29")
		assert(rttr:GetWorld():AddEnvObject(49,29,0,3))
		rttr:Log("Adding broken castle at 46:37")
		assert(rttr:GetWorld():AddStaticObject(46,37,6,2,2))
		world = rttr:GetWorld()
		ASO = world.AddStaticObject
		if(pcall(ASO, world, 46,37,6,2,5)) then -- Fail due to invalid size
			assert(false)
		end
		rttr:Log("LUA: Showing mission statement!")
		rttr:MissionStatement(rttr:GetLocalPlayerIdx(), "Test Mission", "Mission statement seems to be working")
	end
	if(gfCounter == 100) then
		rttr:Log("LUA: Send 2 post messages")
		rttr:PostMessage(rttr:GetLocalPlayerIdx(), "Post message working")
		x,y = rttr:GetPlayer(rttr:GetLocalPlayerIdx()):GetHQPos()
		rttr:PostMessageWithLocation(rttr:GetLocalPlayerIdx(), "This should be a message at your HQ", x,y)
		rttr:MsgBoxEx("Wise guy", "This guy tells you to check the log for any lua errors or assertion failures!", "io", 259)
	end
	if(gfCounter == 500) then
		rttr:Log("\n\nLUA: You can now close the game")
		rttr:MsgBoxEx("Wise guy", "And now he is on the right O.o", "io", 259, 420 - 34, 20 + 23)
	end
end

function onExplored(pIdx, x, y)
	assert(pIdx >= 0 and pIdx < rttr:GetPlayerCount())
end

function onOccupied(pIdx, x, y)
	assert(pIdx >= 0 and pIdx < rttr:GetPlayerCount())
end
