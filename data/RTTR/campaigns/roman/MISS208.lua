------------------------------------------------------------------------------
-- LUA-Script for MISS208.WLD (mission 9 of the original "Roman Campaign")  --
--                                                                          --
-- Authors: CrazyL, Spikeone, ArthurMurray47                                --
------------------------------------------------------------------------------


-------------------------------- TODO -----------------------------------------
-- EnableNextMissions()
-- Set Portraits
-- Set AI Agression Level
-------------------------------------------------------------------------------


-------------------------------- Lua Version used ----------------------------
function getRequiredLuaVersion()
    return 1
end

function isMapPreviewEnabled()
    return false
end

local requiredFeature = 5
function checkVersion()
    local featureLevel = rttr:GetFeatureLevel()
    if(featureLevel < requiredFeature) then
        rttr:MsgBox("LUA-Version Error", "Your Return to the Roots version is outdated. The required LUA-Feature level is " ..requiredFeature.. ", your version is "..featureLevel..". The script can possibly crash or run unexpectedly!\n\nPlease update the game!", true)
    end
end
-------------------------------- mission events and texts ---------------------
-- Message-Window (mission statement and hints): 52 chars wide
eIdx = {1, 99}

rttr:RegisterTranslations(
{
    cs =
    {
        Diary   = 'Deník',

        msg1    = 'Opět se ocitáme ve světě, který se skládá z popela a lávy. Vzduchem se nese hnilobný zápach síry. Všechno pochází z nedaleké sopky, která chrlí svůj odporný obsah po celé zemi.\n\nMoji muži trvají na tom aby jsme brzy odešli; Mohu jen doufat, že bohové povedou naše kroky správnám směrem.',
        msgh1   = 'Najdi a obsaďt bránu! Zvaž vhodné příležitosti k vybudování přístavu.',

        msg99   = 'Našli jsme bránu a obsadili ji. Nezůstávejme tady déle, než je nezbytně nutné.',
        msgh99   = 'Dokončil jsi tuto misi. Další kapitola na tebe čeká ...'
    },
    de =
    {
        Diary   = 'Tagebuch',

        msg1    = 'Erneut fanden wir uns wieder in einer Welt aus\nAsche, Lava und dem stinkenden Atem der\nVulkanschlote, die das Land kreuz und quer\ndurchziehen.\n\nMeine Männer drängen auf baldigen Aufbruch; ich kann\nnur hoffen, daß die Götter unseren Weg lenken\nwerden.',
        msgh1   = 'Finden und besetzen sie das Tor! Achten Sie auf\nMöglichkeiten zum Hafenbau.',

        msg99   = 'Wir haben das Tor gefunden und besetzt. Laßt uns\nhier nicht länger bleiben als nötig.',
        msgh99  = 'Sie haben diese Mission erfüllt. Das letzte Kapitel\nwartet jetzt auf Sie...'
    },
    en =
    {
        Diary   = 'Diary',

        msg1    = 'Once again we find ourselves in a world that consists\nof ash and lava. The atmosphere carries the putrid\nsmell of sulphur. It all originates from the nearby\nvolcano which bellows out its vile contents across\nthe land.\n\nMy men are insisting we leave soon; I can only hope\nthat the gods will guide our steps.',
        msgh1   = 'Find and occupy the gateway! Consider opportunities\nto build a harbor.',

        msg99   = 'We have found the gateway and occupied it. Let us\nremain here no longer than is necessary.',
        msgh99   = 'You have completed this mission. The next Chapter\nawaits you...'
    },
    pl =
    {
        Diary   = 'Dziennik',

        msg1    = 'Znowu znajdujemy się w świecie, który składa się głównie z popiołów i lawy.\n\nAtmosfera niesie ze sobą zgniły i drażniący zapach siarki.\n\nWszystko to pochodzi z pobliskiego wulkanu, który wyrzuca swoje ohydne treści na powierzchnię ziemi.\n\nMoi ludzie nalegają, abyśmy jak najszybciej stąd odeszli - mogę jedynie mieć nadzieję, że bogowie poprowadzą nasze kroki.',
        msgh1   = 'Znajdź i zajmij wrota!\nRozważ możliwości budowy portu.',

        msg99   = 'Znaleźliśmy wrota i zajęliśmy je.\n\nNie pozostawajmy tu dłużej niż to konieczne.',
        msgh99   = 'Ukończyłeś tę misję.\nNastępny rozdział czeka na ciebie...'
    }
})

-- format mission texts
function MissionText(e)
    local msg = _('msg' .. tostring(e))
    local msgh = _('msgh'.. tostring(e))

    if(msg ~= ('msg' .. tostring(e)) and msgh ~= ('msgh'.. tostring(e))) then
        rttr:MissionStatement(0, _('Diary'), msg .. '\n\n\n\n\n\n\n' ..msgh.. '\n\n\n\n\n\n\n', IM_SWORDSMAN, true)
        rttr:SetMissionGoal(0, msgh)
    elseif(msg ~= ('msg' .. tostring(e))) then
        rttr:MissionStatement(0, _('Diary'), msg .. '\n\n\n\n\n\n\n', IM_SWORDSMAN, true)
    else
        rttr:Log("Error: no Translation found: " .. _('msg' .. tostring(e)))
    end
end


-------------------------------- general settings -----------------------------
function onSettingsReady()
    checkVersion()
    rttr:Log("-----------------------\n MISS208.lua loaded... \n-----------------------\n")
    rttr:ResetAddons()
    rttr:SetAddon(ADDON_FRONTIER_DISTANCE_REACHABLE, true)
    rttr:SetGameSettings({
        ["fow"] = EXP_CLASSIC,
        ["teamView"] = false,
        ["lockedTeams"] = false
    })

    rttr:GetPlayer(0):SetNation(NAT_ROMANS)     -- nation
    rttr:GetPlayer(0):SetColor(0)               -- 0:blue, 1:read, 2:yellow, 
    rttr:GetPlayer(0):SetPortrait(0)

    rttr:GetPlayer(1):SetAI(3)                  -- hard AI
    rttr:GetPlayer(1):SetNation(NAT_JAPANESE)   -- nation
    rttr:GetPlayer(1):SetColor(1)               -- yellow
    rttr:GetPlayer(1):SetName('Yamauchi')       -- Enemy Name
    rttr:GetPlayer(1):SetPortrait(6)
    rttr:GetPlayer(1):SetTeam(TM_TEAM1)

    rttr:GetPlayer(2):SetAI(3)                  -- hard AI
    rttr:GetPlayer(2):SetNation(NAT_JAPANESE)   -- nation
    rttr:GetPlayer(2):SetColor(2)               -- red
    rttr:GetPlayer(2):SetName('Tsunami')        -- Enemy Name
    rttr:GetPlayer(2):SetPortrait(7)
    rttr:GetPlayer(2):SetTeam(TM_TEAM1)
end

-- start callback
function onStart(isFirstStart)
    for i = 0, (rttr:GetPlayerCount() - 1) do   -- set resources
        addPlayerRes(i, not isFirstStart)
        addPlayerBld(i, not isFirstStart)
    end

    eState = {}                                 -- enable all events (0: disable)
    for _, i in ipairs(eIdx) do
        eState[i] = 1
    end

    if not isFirstStart then                    -- initialize history of all called events, event counter at ["n"]
        for i = 1, eHist["n"] do
            MissionEvent(eHist[i], true)        -- call events without mission text to activate all achievements
        end

    else
        eHist = {["n"] = 0}
        MissionEvent(1)                         -- initial event / start screen
    end

    rttr:GetWorld():SetComputerBarrier(12, 112, 85)
    rttr:GetWorld():SetComputerBarrier(12, 103, 29)
end

function getAllowedChanges()
    return {
        ["addonsAll"]   = false,
        ["ownNation"]   = false,
        ["ownColor"]    = false,
        ["ownTeam"]     = false,
        ["ownPortrait"] = false,
        ["aiNation"]    = false,
        ["aiColor"]     = false,
        ["aiTeam"]      = false,
        ["aiPortrait"]  = false
    }
end

-- save callback
function onSave(saveGame)
    saveGame:PushInt(eHist["n"])
    for i = 1, eHist["n"] do
        saveGame:PushInt(eHist[i])
    end
    return true
end

-- load callback
function onLoad(saveGame)
    eHist = {["n"] = saveGame:PopInt()}
    for i = 1, eHist["n"] do
        eHist[i] = saveGame:PopInt()
    end
    return true
end

-------------------------------- set buildings --------------------------------
function addPlayerBld(p, onLoad)
    -- set buildings for all players
    rttr:GetPlayer(p):EnableAllBuildings()

    if not(p == 0) then
        rttr:GetPlayer(p):DisableBuilding(BLD_SHIPYARD)
        rttr:GetPlayer(p):DisableBuilding(BLD_HARBORBUILDING)
    end
end

-------------------------------- set resources --------------------------------
-- Don't add goods/people onLoad!
function addPlayerRes(p, onLoad)
    if onLoad then return end

    rttr:GetPlayer(p):ClearResources()
    if(p == 0) then
        -- goods
        rttr:GetPlayer(p):AddWares({
            [GD_BEER      ] = 20,
            [GD_TONGS     ] =  0,
            [GD_HAMMER    ] =  0,
            [GD_AXE       ] =  0,
            [GD_SAW       ] =  0,
            [GD_PICKAXE   ] =  0,
            [GD_SHOVEL    ] =  0,
            [GD_CRUCIBLE  ] =  0,
            [GD_RODANDLINE] =  0,
            [GD_SCYTHE    ] =  0,
            [GD_WATER     ] =  0,
            [GD_CLEAVER   ] =  0,
            [GD_ROLLINGPIN] =  0,
            [GD_BOW       ] =  0,
            [GD_BOAT      ] =  0,
            [GD_SWORD     ] =  0,
            [GD_IRON      ] = 10,
            [GD_FLOUR     ] =  0,
            [GD_FISH      ] = 20,
            [GD_BREAD     ] = 20,
            [GD_SHIELD    ] =  0,
            [GD_WOOD      ] =  0,
            [GD_BOARDS    ] = 50,
            [GD_STONES    ] = 50,
            [GD_GRAIN     ] =  0,
            [GD_COINS     ] =  0,
            [GD_GOLD      ] =  0,
            [GD_IRONORE   ] = 15,
            [GD_COAL      ] = 30,
            [GD_MEAT      ] = 20,
            [GD_HAM       ] =  0
        })

        -- people
        rttr:GetPlayer(p):AddPeople({
            [JOB_HELPER             ] = 50,
            [JOB_WOODCUTTER         ] = 15,
            [JOB_FISHER             ] =  8,
            [JOB_FORESTER           ] = 10,
            [JOB_CARPENTER          ] =  5,
            [JOB_STONEMASON         ] =  5,
            [JOB_HUNTER             ] =  3,
            [JOB_FARMER             ] =  8,
            [JOB_MILLER             ] =  5,
            [JOB_BAKER              ] =  4,
            [JOB_BUTCHER            ] =  3,
            [JOB_MINER              ] = 10,
            [JOB_BREWER             ] =  1,
            [JOB_PIGBREEDER         ] =  5,
            [JOB_DONKEYBREEDER      ] =  2,
            [JOB_IRONFOUNDER        ] =  3,
            [JOB_MINTER             ] =  2,
            [JOB_METALWORKER        ] =  3,
            [JOB_ARMORER            ] =  3,
            [JOB_BUILDER            ] = 12,
            [JOB_PLANER             ] =  6,
            [JOB_GEOLOGIST          ] =  4,
            [JOB_PRIVATE            ] = 30,
            [JOB_PRIVATEFIRSTCLASS  ] =  0,
            [JOB_SERGEANT           ] =  0,
            [JOB_OFFICER            ] =  0,
            [JOB_GENERAL            ] =  0,
            [JOB_SCOUT              ] =  5,
            [JOB_SHIPWRIGHT         ] =  1,
            [JOB_PACKDONKEY         ] =  0,
            [JOB_CHARBURNER     ] =  0
        })

    elseif(p == 1) then
        -- goods
        rttr:GetPlayer(p):AddWares({
            [GD_BEER        ] =  40,
            [GD_TONGS       ] =   0,
            [GD_HAMMER      ] =   0,
            [GD_AXE         ] =   0,
            [GD_SAW         ] =   0,
            [GD_PICKAXE     ] =   0,
            [GD_SHOVEL      ] =   0,
            [GD_CRUCIBLE    ] =   0,
            [GD_RODANDLINE  ] =   0,
            [GD_SCYTHE      ] =   0,
            [GD_WATER       ] =   0,
            [GD_CLEAVER     ] =   0,
            [GD_ROLLINGPIN  ] =   0,
            [GD_BOW         ] =   0,
            [GD_BOAT        ] =   0,
            [GD_SWORD       ] =   0,
            [GD_IRON        ] =   0,
            [GD_FLOUR       ] =   0,
            [GD_FISH        ] =  40,
            [GD_BREAD       ] =   0,
            [GD_SHIELD      ] =   0,
            [GD_WOOD        ] =   0,
            [GD_BOARDS      ] = 100,
            [GD_STONES      ] =  80,
            [GD_GRAIN       ] =   0,
            [GD_COINS       ] =   0,
            [GD_GOLD        ] =   0,
            [GD_IRONORE     ] =   0,
            [GD_COAL        ] =  10,
            [GD_MEAT        ] =  60,
            [GD_HAM         ] =   0
        })

        -- people
        rttr:GetPlayer(p):AddPeople({
            [JOB_HELPER             ] = 50,
            [JOB_WOODCUTTER         ] = 15,
            [JOB_FISHER             ] = 10,
            [JOB_FORESTER           ] = 10,
            [JOB_CARPENTER          ] = 10,
            [JOB_STONEMASON         ] = 10,
            [JOB_HUNTER             ] =  5,
            [JOB_FARMER             ] = 10,
            [JOB_MILLER             ] =  5,
            [JOB_BAKER              ] =  5,
            [JOB_BUTCHER            ] =  5,
            [JOB_MINER              ] = 20,
            [JOB_BREWER             ] =  5,
            [JOB_PIGBREEDER         ] =  5,
            [JOB_DONKEYBREEDER      ] =  5,
            [JOB_IRONFOUNDER        ] = 10,
            [JOB_MINTER             ] = 15,
            [JOB_METALWORKER        ] =  5,
            [JOB_ARMORER            ] = 10,
            [JOB_BUILDER            ] = 20,
            [JOB_PLANER             ] = 15,
            [JOB_GEOLOGIST          ] =  5,
            [JOB_PRIVATE            ] = 30,
            [JOB_PRIVATEFIRSTCLASS  ] =  4,
            [JOB_SERGEANT           ] =  2,
            [JOB_OFFICER            ] =  0,
            [JOB_GENERAL            ] =  0,
            [JOB_SCOUT              ] = 10,
            [JOB_SHIPWRIGHT         ] =  0,
            [JOB_PACKDONKEY         ] = 20,
            [JOB_CHARBURNER         ] =  0
        })

    elseif(p == 2) then
        -- goods
        rttr:GetPlayer(p):AddWares({
            [GD_BEER      ] =  40,
            [GD_TONGS     ] =   0,
            [GD_HAMMER    ] =   0,
            [GD_AXE       ] =   0,
            [GD_SAW       ] =   0,
            [GD_PICKAXE   ] =   0,
            [GD_SHOVEL    ] =   0,
            [GD_CRUCIBLE  ] =   0,
            [GD_RODANDLINE] =   0,
            [GD_SCYTHE    ] =   0,
            [GD_WATER     ] =   0,
            [GD_CLEAVER   ] =   0,
            [GD_ROLLINGPIN] =   0,
            [GD_BOW       ] =   0,
            [GD_BOAT      ] =   0,
            [GD_SWORD     ] =   0,
            [GD_IRON      ] =   0,
            [GD_FLOUR     ] =   0,
            [GD_FISH      ] =  40,
            [GD_BREAD     ] =   0,
            [GD_SHIELD    ] =   0,
            [GD_WOOD      ] =   0,
            [GD_BOARDS    ] = 100,
            [GD_STONES    ] =  80,
            [GD_GRAIN     ] =   0,
            [GD_COINS     ] =   0,
            [GD_GOLD      ] =   0,
            [GD_IRONORE   ] =   0,
            [GD_COAL      ] =  10,
            [GD_MEAT      ] =  40,
            [GD_HAM       ] =   0
        })

        -- people
        rttr:GetPlayer(p):AddPeople({
            [JOB_HELPER             ] = 50,
            [JOB_WOODCUTTER         ] = 15,
            [JOB_FISHER             ] = 10,
            [JOB_FORESTER           ] = 10,
            [JOB_CARPENTER          ] = 10,
            [JOB_STONEMASON         ] = 10,
            [JOB_HUNTER             ] =  5,
            [JOB_FARMER             ] = 10,
            [JOB_MILLER             ] =  5,
            [JOB_BAKER              ] =  5,
            [JOB_BUTCHER            ] =  5,
            [JOB_MINER              ] = 20,
            [JOB_BREWER             ] =  5,
            [JOB_PIGBREEDER         ] =  5,
            [JOB_DONKEYBREEDER      ] =  5,
            [JOB_IRONFOUNDER        ] = 10,
            [JOB_MINTER             ] = 10,
            [JOB_METALWORKER        ] =  5,
            [JOB_ARMORER            ] = 10,
            [JOB_BUILDER            ] = 20,
            [JOB_PLANER             ] = 15,
            [JOB_GEOLOGIST          ] =  5,
            [JOB_PRIVATE            ] = 30,
            [JOB_PRIVATEFIRSTCLASS  ] =  1,
            [JOB_SERGEANT           ] =  1,
            [JOB_OFFICER            ] =  1,
            [JOB_GENERAL            ] =  1,
            [JOB_SCOUT              ] = 10,
            [JOB_SHIPWRIGHT         ] =  0,
            [JOB_PACKDONKEY         ] = 20,
            [JOB_CHARBURNER         ] =  0
        })
    end
end


-------------------------------- mission events -------------------------------
function onOccupied(p, x, y)
    -- only check human player
    if(p ~= 0) then
        return
    end

    if( (x == 127) and (y == 48) ) then MissionEvent(99)
    end
end

-- execute mission events, e == 1 is initial event, e == 99 is final event
function MissionEvent(e, onLoad)
    -- event e is inactive
    if(eState[e] <= 0) then
        return
    end

    -- call side effects for active events, check "eState[e] == 1" for multiple call events!
    if(e == 99) then
        -- TODO: EnableNextMissions()
        -- Show opened arc - Done
        rttr:GetWorld():AddStaticObject(127, 48, 561, 0xFFFF, 2)
    end

    -- update event state
    eState[e] = eState[e] - 1

    -- no history update or mission texts while loading
    if not onLoad then
        rttr:Log(">> Event: " .. e .. ",\teState[" .. e .. "] = " .. eState[e])
        eHist["n"] = eHist["n"] + 1
        eHist[eHist["n"]] = e
        MissionText(e)
    end
end