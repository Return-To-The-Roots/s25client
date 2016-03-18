gfCounter=0

rttr:Log("Test Log from Lua")

function addPlayerRes(pl)
	wares = {GD_HAMMER=8,GD_AXE=6,GD_SAW=3,GD_PICKAXE=6,GD_RODANDLINE=2,GD_SCYTHE=6,GD_CLEAVER=1,GD_ROLLINGPIN=1,GD_BOW=2,GD_BOAT=20,GD_SWORD=0,GD_SHIELDROMANS=0,GD_WOOD=20,GD_BOARDS=40,GD_STONES=40,GD_GRAIN=0,GD_COINS=0,GD_IRONORE=18,GD_COAL=36,GD_FISH=8,GD_BREAD=8}
	people = {JOB_HELPER=30,JOB_WOODCUTTER=6,JOB_FISHER=0,JOB_FORESTER=2,JOB_CARPENTER=2,JOB_STONEMASON=4,JOB_HUNTER=1,JOB_MINER=10,JOB_BREWER=1,JOB_IRONFOUNDER=2,JOB_MINTER=1,JOB_METALWORKER=1,JOB_ARMORER=2,JOB_BUILDER=14,JOB_PLANER=8,JOB_PRIVATE=10,JOB_PRIVATEFIRSTCLASS=1,JOB_SERGEANT=1,JOB_OFFICER=1,JOB_GENERAL=20,JOB_GEOLOGIST=4,JOB_SHIPWRIGHT=0,JOB_SCOUT=4}
	pl.AddWares(wares)
	for gd,ct in pairs(wares) do
		assert(player.GetWareCount(gd) == ct)
	end
	pl.AddPeople(people)
	for job,ct in pairs(people) do
		assert(player.GetWareCount(job) == ct)
	end
end

function testPlayerFuncs(pl)
	pl.DisableAllBuildings()
	pl.EnableAllBuildings()
	pl.DisableBuilding(BLD_FORESTER)
	pl.DisableBuilding(BLD_WOODCUTTER)
	pl.EnableBuilding(BLD_WOODCUTTER)
	pl.ClearResources()
	addPlayerRes(pl)
	pl.setRestrictedArea(5,2, 5,10, 10,10, 10,2)
	assert(pl.GetBuildingCount(BLD_WINDMILL) == 0)
	pl.ModifyHQ(true)
end

function onStart()
	assert(gfCounter == 0)
	assert(rttr:GetGF() == 0)
	rttr:ClearResources()
	assert(rttr:GetPlayer(0):GetWareCount(GD_BEER) == 0)
	addPlayerRes(rttr:GetPlayer(0))
	addPlayerRes(rttr:GetPlayer(1))
	assert(rttr:GetPlayerCount() > 1)
	for i=0,rttr:GetPlayerCount() do
		testPlayerFuncs(rttr:GetPlayer(i))
	end
	assert(rttr:GetPlayer(0):AIConstructionOrder(10,10, BLD_WOODCUTTER) == false) -- Human
	assert(rttr:GetPlayer(0):AIConstructionOrder(10,10, BLD_WOODCUTTER) == true)  -- AI	
	
	assert(rttr:GetWorld():AddEnvObject(23,72,12,1))
	assert(rttr:GetWorld():AddStaticbject(23,50,12,1,2))
	world = rttr:GetWorld()
	ASO = world.AddStaticObject
	if(pcall(ASO, world, 1,1,12,1,5)) then -- Fail due to invalid size
		assert(false)
	end
end

function onGF()
	gfCounter = gfCounter + 1
	assert(gfCounter == rttr:GetGF())
	if(gfCounter == 10) then
		rttr:Log("Showing mission statement!")
		rttr:MissionStatement(0, "Test Mission", "If you see this, all is ok")
	end
	if(gfCounter == 20) then
		rttr:Log("Send 2 post messages")
		rttr:PostMessage(0, "Post message working")
		x,y = rttr:GetPlayer(0):GetHQPos()
		rttr:PostMessageWithLocation(0, "This should be a message at your HQ", x,y)
	end
	if(gfCounter == 30) then
		rttr:Log("\n\nYou can now close the game")
	end
end

function onExplored(pIdx, x, y)
	assert(pIdx >= 0 and pIdx < rttr:GetPlayerCount())
end

function onOccupied(pIdx, x, y)
	assert(pIdx >= 0 and pIdx < rttr:GetPlayerCount())
end
