------------------------------------------------------------------------------
-- LUA-Script for MISS207.WLD (mission 8 of the original "Roman Campaign")  --
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
-------------------------------- mission events and texts --------------------
-- Message-Window (mission statement and hints): 52 chars wide
eIdx = {1, 99}

rttr:RegisterTranslations(
{
    cs =
    {
        Diary   = 'deník',

        msg1    = 'Skauti mi řekli o mocném nepříteli na západě. Musíme se tu pokusit zůstat co nejdéle. V případě nouze můžeme vždy uniknout na východ nebo po moři ...',
        msgh1   = 'Vyhledej bránu. Zvaž vhodné příležitosti k vybudování přístavu.',

        msg99   = 'Našli jsme bránu a obsadili ji. Kdy dorazíme do cíle?',
        msgh99   = 'Dokončil jsi tuto misi. Další kapitola na tebe čeká ...'
    },
    de =
    {
        Diary   = 'Tagebuch',

        msg1    = 'Späher berichteten mir von einem übermächtigem Feind\nim Westen. Wir müssen versuchen, uns hier solange\nwie möglich zu halten. Aber im Notfall bliebe uns\nimmer noch die Flucht nach Osten oder übers Meer...',
        msgh1   = 'Suchen Sie nach einem Tor. Achten Sie auf eventuell\nvorhandene Möglichkeiten zum Hafenbau.',

        msg99   = 'Wir haben das Tor gefunden und besetzt. Wann werden\nwir unser Ziel wohl erreichen?',
        msgh99   = 'Sie haben diese Mission erfüllt. Das nächste Kapitel\nwartet auf Sie...'
    },
    en =
    { 
        Diary   = 'Diary',

        msg1    = 'Scouts have told me of a mighty enemy in the west. We\nmust try and stay here as long as possible. In an\nemergency we can always escape to the east or by\nsea...',
        msgh1   = 'Search for a gateway. Consider opportunities to build\na harbor.',

        msg99   = 'We have found the gateway and occupied it. When will\nwe reach our destination?',
        msg99   = 'You have completed this mission. The next Chapter\nawaits you...'
    },
    pl =
    { 
        Diary   = 'Dziennik',

        msg1    = 'Zwiadowcy powiedzieli mi o potężnym wrogu na zachodzie.\n\nMusimy spróbować zostać tutaj tak długo, jak to możliwe.\n\nW nagłym wypadku możemy zawsze uciec na wschód lub morzem...',
        msgh1   = 'Szukaj bramy.\nRozważ możliwości budowy portu.',

        msg99   = 'Znaleźliśmy bramę i zajęliśmy ją.\n\nKiedy dotrzemy do naszego celu?',
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
    rttr:Log("-----------------------\n MISS207.lua loaded... \n-----------------------\n")
    rttr:ResetAddons()
    rttr:SetAddon(ADDON_FRONTIER_DISTANCE_REACHABLE, true)
    rttr:SetGameSettings({
        ["fow"] = EXP_CLASSIC,
        ["teamView"] = false,
        ["lockedTeams"] = false
    })

    rttr:GetPlayer(0):SetNation(NAT_ROMANS)     -- nation
    rttr:GetPlayer(0):SetColor(0)               -- 0:blue, 1:read, 2:yellow, 

    rttr:GetPlayer(1):SetAI(3)                  -- hard AI
    rttr:GetPlayer(1):SetNation(NAT_AFRICANS)   -- nation
    rttr:GetPlayer(1):SetColor(1)               -- yellow
    rttr:GetPlayer(1):SetName('Mnga Tscha')     -- Enemy Name
    rttr:GetPlayer(1):SetTeam(TM_TEAM1)

    rttr:GetPlayer(2):SetAI(3)                  -- hard AI
    rttr:GetPlayer(2):SetNation(NAT_AFRICANS)   -- nation
    rttr:GetPlayer(2):SetColor(2)               -- red
    rttr:GetPlayer(2):SetName('Todo')           -- Enemy Name
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

    rttr:GetWorld():SetComputerBarrier(10, 41, 122)
    rttr:GetWorld():SetComputerBarrier(10, 72, 111)
    rttr:GetWorld():SetComputerBarrier(15, 72, 97)
    rttr:GetWorld():SetComputerBarrier(10, 19, 100)
end

function getAllowedChanges()
    return {
        ["addonsAll"]   = false,
        ["ownNation"]   = false,
        ["ownColor"]    = false,
        ["ownTeam"]     = false,
        ["aiNation"]    = false, 
        ["aiColor"]     = false,
        ["aiTeam"]      = false
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
        rttr:GetPlayer(p):DisableBuilding(BLD_SHIPYARD, false)
        rttr:GetPlayer(p):DisableBuilding(BLD_HARBORBUILDING, false)
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
            [GD_BEER        ] = 10,
            [GD_TONGS       ] =  2,
            [GD_HAMMER      ] =  8,
            [GD_AXE         ] =  2,
            [GD_SAW         ] =  2,
            [GD_PICKAXE     ] =  5,
            [GD_SHOVEL      ] =  4,
            [GD_CRUCIBLE    ] =  4,
            [GD_RODANDLINE  ] =  8,
            [GD_SCYTHE      ] =  4,
            [GD_WATER       ] = 20,
            [GD_CLEAVER     ] =  3,
            [GD_ROLLINGPIN  ] =  2,
            [GD_BOW         ] =  4,
            [GD_BOAT        ] =  0,
            [GD_SWORD       ] =  9,
            [GD_IRON        ] = 10,
            [GD_FLOUR       ] =  8,
            [GD_FISH        ] = 14,
            [GD_BREAD       ] = 16,
            [GD_SHIELD      ] =  9,
            [GD_WOOD        ] = 20,
            [GD_BOARDS      ] = 50,
            [GD_STONES      ] = 50,
            [GD_GRAIN       ] = 10,
            [GD_COINS       ] =  2,
            [GD_GOLD        ] = 15,
            [GD_IRONORE     ] = 15,
            [GD_COAL        ] = 30,
            [GD_MEAT        ] = 13,
            [GD_HAM         ] = 12
        })

        -- people
        rttr:GetPlayer(p):AddPeople({
            [JOB_HELPER             ] = 100,
            [JOB_WOODCUTTER         ] =   4,
            [JOB_FISHER             ] =   4,
            [JOB_FORESTER           ] =   3,
            [JOB_CARPENTER          ] =   3,
            [JOB_STONEMASON         ] =   4,
            [JOB_HUNTER             ] =   2,
            [JOB_FARMER             ] =   2,
            [JOB_MILLER             ] =   1,
            [JOB_BAKER              ] =   1,
            [JOB_BUTCHER            ] =   1,
            [JOB_MINER              ] =   6,
            [JOB_BREWER             ] =   3,
            [JOB_PIGBREEDER         ] =   1,
            [JOB_DONKEYBREEDER      ] =   0,
            [JOB_IRONFOUNDER        ] =   2,
            [JOB_MINTER             ] =   2,
            [JOB_METALWORKER        ] =   2,
            [JOB_ARMORER            ] =   4,
            [JOB_BUILDER            ] =   8,
            [JOB_PLANER             ] =   4,
            [JOB_GEOLOGIST          ] =   4,
            [JOB_PRIVATE            ] =  20,
            [JOB_PRIVATEFIRSTCLASS  ] =   6,
            [JOB_SERGEANT           ] =   4,
            [JOB_OFFICER            ] =   2,
            [JOB_GENERAL            ] =   1,
            [JOB_SCOUT              ] =   7,
            [JOB_SHIPWRIGHT         ] =   1,
            [JOB_PACKDONKEY         ] =  15,
            [JOB_CHARBURNER         ] =   0
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
            [GD_BREAD       ] =  40,
            [GD_SHIELD      ] =   0,
            [GD_WOOD        ] =   0,
            [GD_BOARDS      ] = 120,
            [GD_STONES      ] = 220,
            [GD_GRAIN       ] =   0,
            [GD_COINS       ] =   0,
            [GD_GOLD        ] =  25,
            [GD_IRONORE     ] =   0,
            [GD_COAL        ] =  40,
            [GD_MEAT        ] =  40,
            [GD_HAM         ] =   0
        })

        -- people
        rttr:GetPlayer(p):AddPeople({
            [JOB_HELPER             ] = 50,
            [JOB_WOODCUTTER         ] = 20,
            [JOB_FISHER             ] = 10,
            [JOB_FORESTER           ] = 10,
            [JOB_CARPENTER          ] = 10,
            [JOB_STONEMASON         ] = 10,
            [JOB_HUNTER             ] = 10,
            [JOB_FARMER             ] = 20,
            [JOB_MILLER             ] = 10,
            [JOB_BAKER              ] = 10,
            [JOB_BUTCHER            ] = 10,
            [JOB_MINER              ] = 20,
            [JOB_BREWER             ] = 10,
            [JOB_PIGBREEDER         ] =  8,
            [JOB_DONKEYBREEDER      ] =  2,
            [JOB_IRONFOUNDER        ] = 10,
            [JOB_MINTER             ] =  6,
            [JOB_METALWORKER        ] =  5,
            [JOB_ARMORER            ] = 10,
            [JOB_BUILDER            ] = 20,
            [JOB_PLANER             ] = 20,
            [JOB_GEOLOGIST          ] = 10,
            [JOB_PRIVATE            ] = 30,
            [JOB_PRIVATEFIRSTCLASS  ] = 16,
            [JOB_SERGEANT           ] =  5,
            [JOB_OFFICER            ] =  4,
            [JOB_GENERAL            ] =  3,
            [JOB_SCOUT              ] = 10,
            [JOB_SHIPWRIGHT         ] =  0,
            [JOB_PACKDONKEY         ] = 40,
            [JOB_CHARBURNER         ] =  0
        })

    elseif(p == 2) then
        -- goods
        rttr:GetPlayer(p):AddWares({
            [GD_BEER        ] = 20,
            [GD_TONGS       ] =  0,
            [GD_HAMMER      ] =  0,
            [GD_AXE         ] =  0,
            [GD_SAW         ] =  0,
            [GD_PICKAXE     ] =  0,
            [GD_SHOVEL      ] =  0,
            [GD_CRUCIBLE    ] =  0,
            [GD_RODANDLINE  ] =  0,
            [GD_SCYTHE      ] =  0,
            [GD_WATER       ] =  0,
            [GD_CLEAVER     ] =  0,
            [GD_ROLLINGPIN  ] =  0,
            [GD_BOW         ] =  0,
            [GD_BOAT        ] =  0,
            [GD_SWORD       ] =  0,
            [GD_IRON        ] =  0,
            [GD_FLOUR       ] =  0,
            [GD_FISH        ] = 20,
            [GD_BREAD       ] = 20,
            [GD_SHIELD      ] =  0,
            [GD_WOOD        ] =  0,
            [GD_BOARDS      ] = 90,
            [GD_STONES      ] = 70,
            [GD_GRAIN       ] =  0,
            [GD_COINS       ] =  0,
            [GD_GOLD        ] =  0,
            [GD_IRONORE     ] =  0,
            [GD_COAL        ] = 20,
            [GD_MEAT        ] = 20,
            [GD_HAM         ] =  0
        })

        -- people
        rttr:GetPlayer(p):AddPeople({
            [JOB_HELPER             ] = 50,
            [JOB_WOODCUTTER         ] = 20,
            [JOB_FISHER             ] = 10,
            [JOB_FORESTER           ] = 10,
            [JOB_CARPENTER          ] = 10,
            [JOB_STONEMASON         ] = 10,
            [JOB_HUNTER             ] = 10,
            [JOB_FARMER             ] = 10,
            [JOB_MILLER             ] = 10,
            [JOB_BAKER              ] = 10,
            [JOB_BUTCHER            ] = 10,
            [JOB_MINER              ] = 20,
            [JOB_BREWER             ] = 10,
            [JOB_PIGBREEDER         ] =  5,
            [JOB_DONKEYBREEDER      ] =  2,
            [JOB_IRONFOUNDER        ] =  8,
            [JOB_MINTER             ] =  5,
            [JOB_METALWORKER        ] =  5,
            [JOB_ARMORER            ] = 15,
            [JOB_BUILDER            ] = 20,
            [JOB_PLANER             ] = 20,
            [JOB_GEOLOGIST          ] = 10,
            [JOB_PRIVATE            ] = 20,
            [JOB_PRIVATEFIRSTCLASS  ] = 16,
            [JOB_SERGEANT           ] =  5,
            [JOB_OFFICER            ] =  4,
            [JOB_GENERAL            ] =  3,
            [JOB_SCOUT              ] = 10,
            [JOB_SHIPWRIGHT         ] =  0,
            [JOB_PACKDONKEY         ] = 40,
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

    if( (x == 11) and (y == 125) ) then MissionEvent(99)
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
        -- Show opened arc
        rttr:GetWorld():AddStaticObject(11, 125, 561, 0xFFFF, 2)
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