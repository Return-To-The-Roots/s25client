------------------------------------------------------------------------------
-- LUA-Script for MISS206.WLD (mission 7 of the original "Roman Campaign"   --
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

local requiredFeature = 4
function checkVersion()
    local featureLevel = rttr:GetFeatureLevel()
    if(featureLevel < requiredFeature) then
        rttr:MsgBox("LUA-Version Error", "Your Return to the Roots version is outdated. The required LUA-Feature level is " ..requiredFeature.. ", your version is "..featureLevel..". The script can possibly crash or run unexpectedly!\n\nPlease update the game!", true)
    end
end
-------------------------------- mission events and texts --------------------
-- Message-Window (mission statement and hints): 52 chars wide
eIdx = {1, 2, 3, 98, 99}

rttr:RegisterTranslations(
{
    cs =
    {
        Diary   = 'Deník',

        msg1    = 'Čtvrtý den 11. měsíce šestého roku.\n\nNaší skouti přinesli zprávy o tom že jsme na poloostrově, který je spojen se zbytkem pevniny dvěmy úzkýmy údolímy. Stopy naznačují, že se na jihu nalézá barbarská rasa Vikingů.',
        msgh1   = 'Vyhledejte bránu.',

        msg2    = 'Zajatý Viking nám řekl, že na poloostrově západně od nás žije další nesmírně bohatý kmen. Mají velké zásoby zlata. Přístup do jejich oblasti na západ od velké zátoky je silně opevněný. Přesnější podrobnosti se nám nepodařilo získat.',

        msg3    = '19. den čtvrtého měsíce sedmého roku\n\nVikingové se zdají být extrémně silní. Nějak musíme získat přístup do jejich pevnosti. Je zajímavé, že tento kmen má na malém poloostrově severně od jejich zlatých dolů hřbitov velryb. Zajímalo by mě, jestli se bojí také duchů velryb?',

        msg99   = 'Našli jsme bránu a obsadili ji.',
        msgh99  = 'Dokončil jsi tuto misi. Další kapitola na tebe čeká ...'
    },
    de =
    {
        Diary   = 'Tagebuch',

        msg1    = '4. im 11. des 6. Jahres.\n\nUnsere Halbinsel scheint nur durch zwei schmale\nTäler mit dem Rest des Kontinents verbunden zu sein.\nSpuren weisen auf ein barbarisches Wikingervolk im\nSüden hin.',
        msgh1   = 'Suchen Sie nach einem Tor.',

        msg2    = 'Ein gefangener Wikinger erzählte uns, daß auf der\nHalbinsel westlich der unseren noch ein weiterer\nStamm lebt, der sehr reich sei, da er über große\nGoldvorräte verfüge. Genaueres war nicht zu\nerfahren, außer, daß der Zugang zu ihrem Gebiet im\nWesten der großen Bucht stark befestigt ist.',

        msg3    = '19. im 4. des 7. Jahres\n\nDie Wikinger scheinen sehr stark zu sein. Wir müssen\ndiese Festung irgendwie knacken.\nInteressant ist, daß auch auch dieser Stamm einen\nWalfriedhof hat. Auf einer kleinen Halbinsel im\nNorden ihrer Goldminen. Ob auch sie Angst vor den\nWalgeistern haben?',

        msg99   = 'Wir haben das Tor gefunden und besetzt.',
        msgh99  = 'Sie haben diese Mission erfüllt. Das nächste Kapitel\nwartet auf Sie...'
    },
    en =
    { 
        Diary   = 'Diary',

        msg1    = 'The Fourth Day of the 11th Month of the Sixth Year.\n\nIt appears that our peninsula is linked to the rest\nof the continent only by two narrow valleys. Tracks\nsuggest there is a barbarian race of Vikings in the\nsouth.',
        msgh1   = 'Search for the gateway.',

        msg2    = 'A captured Viking told us that another extremely\nwealthy tribe lives on the peninsula to the west of\nus. They have large stocks of Gold. Access to their\nregion to the west of the large bay is heavily\nfortified. More precise details could not be\nobtained.',

        msg3    = '19th Day of the Fourth Month of the Seventh Year\n\nThe Vikings appear to be extremely strong. Somehow we\nmust gain access to their fortress.\nIt is interesting that this tribe also has a whale\ngraveyard on a small peninsula to the north of their\ngold mines. I wonder if they are afraid of the\nspirits of the whales as well?',

        msg99   = 'We have found the gateway and occupied it.',
        msgh99  = 'You have completed this mission. The next Chapter\nawaits you...'
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
    rttr:Log("-----------------------\n MISS206.lua loaded... \n-----------------------\n")
    rttr:ResetAddons()                          -- S2-settings
    rttr:SetAddon(ADDON_FRONTIER_DISTANCE_REACHABLE, true)
    rttr:SetGameSettings({
        ["fow"] = EXP_CLASSIC,
        ["teamView"] = false,
        ["lockedTeams"] = false
    })

    rttr:GetPlayer(0):SetNation(NAT_ROMANS)     -- nation
    rttr:GetPlayer(0):SetColor(0)               -- 0:blue, 1:red, 2:yellow,

    rttr:GetPlayer(1):SetAI(3)                  -- hard AI
    rttr:GetPlayer(1):SetNation(NAT_VIKINGS)    -- nation
    rttr:GetPlayer(1):SetColor(1)               -- yellow
    rttr:GetPlayer(1):SetName('Erik')           -- Enemy Name
    rttr:GetPlayer(1):SetTeam(TM_TEAM1)
    
    rttr:GetPlayer(2):SetAI(3)                  -- hard AI
    rttr:GetPlayer(2):SetNation(NAT_VIKINGS)    -- nation
    rttr:GetPlayer(2):SetColor(2)               -- red
    rttr:GetPlayer(2):SetName('Olof')           -- Enemy Name
    rttr:GetPlayer(2):SetTeam(TM_TEAM1)
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
    rttr:GetPlayer(p):DisableBuilding(BLD_SHIPYARD, false)
    rttr:GetPlayer(p):DisableBuilding(BLD_HARBORBUILDING, false)

    --!GLOBAL_SET_COMPUTER_BARRIER     10     51  22
    --!GLOBAL_SET_COMPUTER_BARRIER     10     36  23
    --!GLOBAL_SET_COMPUTER_BARRIER     08     50  60
    --!GLOBAL_SET_COMPUTER_BARRIER     08     49  64
    --!GLOBAL_SET_COMPUTER_BARRIER     08     47  68

    if not(p == 0) then
        rttr:GetPlayer(p):SetRestrictedArea(
            nil, nil,       -- enable the whole map
                  0,   0,
                  0, 127,
                143, 127,
                143,   0,
            nil, nil,       -- R=10,    X=51,   Y=22    V 
                56,  13,   -- R=10,    X=36,   Y=23
                61,  23,
                56,  33,
                48,  32,
                50,  26,
                26,  23,
                31,  13,
                56,  13,
            nil, nil,       -- R=08,    X=50,   Y=60    V   (->     R=6,    X=52,   Y=60)
                46,  60,    -- R=08,    X=49,   Y=64    V
                49,  54,    -- R=08,    X=47,   Y=68        (->     R=6,    X=47,   Y=68)
                55,  54,
                58,  60,
                53,  68,
                50,  74,
                44,  74,
                41,  68,
                46,  60,
            nil, nil
        )

        if(p == 2) then
            if onLoad then return end

            rttr:GetPlayer(p):AIConstructionOrder(43, 59, BLD_FORTRESS)
        end
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
            [GD_BEER      ] =  8,
            [GD_TONGS     ] =  1,
            [GD_HAMMER    ] =  2,
            [GD_AXE       ] =  1,
            [GD_SAW       ] =  1,
            [GD_PICKAXE   ] =  1,
            [GD_SHOVEL    ] =  1,
            [GD_CRUCIBLE  ] =  1,
            [GD_RODANDLINE] =  3,
            [GD_SCYTHE    ] =  3,
            [GD_WATER     ] = 20,
            [GD_CLEAVER   ] =  1,
            [GD_ROLLINGPIN] =  1,
            [GD_BOW       ] =  2,
            [GD_BOAT      ] = 13,
            [GD_SWORD     ] =  4,
            [GD_IRON      ] = 10,
            [GD_FLOUR     ] =  8,
            [GD_FISH      ] =  4,
            [GD_BREAD     ] =  6,
            [GD_SHIELD    ] =  4,
            [GD_WOOD      ] = 30,
            [GD_BOARDS    ] = 50,
            [GD_STONES    ] = 46,
            [GD_GRAIN     ] = 10,
            [GD_COINS     ] =  4,
            [GD_GOLD      ] = 15,
            [GD_IRONORE   ] = 15,
            [GD_COAL      ] = 30,
            [GD_MEAT      ] =  3,
            [GD_HAM       ] =  6
        })

        -- people
        rttr:GetPlayer(p):AddPeople({
            [JOB_HELPER             ] = 100,
            [JOB_WOODCUTTER         ] =   6,
            [JOB_FISHER             ] =   4,
            [JOB_FORESTER           ] =   2,
            [JOB_CARPENTER          ] =   2,
            [JOB_STONEMASON         ] =   4,
            [JOB_HUNTER             ] =   2,
            [JOB_FARMER             ] =   4,
            [JOB_MILLER             ] =   2,
            [JOB_BAKER              ] =   2,
            [JOB_BUTCHER            ] =   2,
            [JOB_MINER              ] =   6,
            [JOB_BREWER             ] =   2,
            [JOB_PIGBREEDER         ] =   2,
            [JOB_DONKEYBREEDER      ] =   1,
            [JOB_IRONFOUNDER        ] =   1,
            [JOB_MINTER             ] =   2,
            [JOB_METALWORKER        ] =   1,
            [JOB_ARMORER            ] =   1,
            [JOB_BUILDER            ] =   8,
            [JOB_PLANER             ] =   4,
            [JOB_GEOLOGIST          ] =   4,
            [JOB_PRIVATE            ] =  24,
            [JOB_PRIVATEFIRSTCLASS  ] =   4,
            [JOB_SERGEANT           ] =   3,
            [JOB_OFFICER            ] =   2,
            [JOB_GENERAL            ] =   1,
            [JOB_SCOUT              ] =   7,
            [JOB_SHIPWRIGHT         ] =   1,
            [JOB_PACKDONKEY         ] =   0,
            [JOB_CHARBURNER         ] =   0
        })

    elseif(p == 1) then
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
            [GD_IRON        ] = 10,
            [GD_FLOUR       ] =  0,
            [GD_FISH        ] = 20,
            [GD_BREAD       ] = 20,
            [GD_SHIELD      ] =  0,
            [GD_WOOD        ] =  0,
            [GD_BOARDS      ] = 90,
            [GD_STONES      ] = 90,
            [GD_GRAIN       ] =  0,
            [GD_COINS       ] =  4,
            [GD_GOLD        ] = 35,
            [GD_IRONORE     ] = 15,
            [GD_COAL        ] = 40,
            [GD_MEAT        ] = 20,
            [GD_HAM         ] =  0
        })

        -- people
        rttr:GetPlayer(p):AddPeople({
            [JOB_HELPER             ] = 50,
            [JOB_WOODCUTTER         ] = 10,
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
            [JOB_BREWER             ] =  5,
            [JOB_PIGBREEDER         ] = 10,
            [JOB_DONKEYBREEDER      ] = 10,
            [JOB_IRONFOUNDER        ] =  8,
            [JOB_MINTER             ] =  5,
            [JOB_METALWORKER        ] =  5,
            [JOB_ARMORER            ] = 10,
            [JOB_BUILDER            ] = 20,
            [JOB_PLANER             ] = 20,
            [JOB_GEOLOGIST          ] = 10,
            [JOB_PRIVATE            ] = 35,
            [JOB_PRIVATEFIRSTCLASS  ] = 20,
            [JOB_SERGEANT           ] = 10,
            [JOB_OFFICER            ] =  8,
            [JOB_GENERAL            ] =  3,
            [JOB_SCOUT              ] = 10,
            [JOB_SHIPWRIGHT         ] =  0,
            [JOB_PACKDONKEY         ] = 40,
            [JOB_CHARBURNER         ] =  0
        })

    elseif(p == 2) then
        -- goods
        rttr:GetPlayer(p):AddWares({
            [GD_BEER        ] =  20,
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
            [GD_FISH        ] =  20,
            [GD_BREAD       ] =  20,
            [GD_SHIELD      ] =   0,
            [GD_WOOD        ] =   0,
            [GD_BOARDS      ] =  90,
            [GD_STONES      ] = 120,
            [GD_GRAIN       ] =   0,
            [GD_COINS       ] =   0,
            [GD_GOLD        ] =   0,
            [GD_IRONORE     ] =   0,
            [GD_COAL        ] =  30,
            [GD_MEAT        ] =  20,
            [GD_HAM         ] =   0
        })

        -- people
        rttr:GetPlayer(p):AddPeople({
            [JOB_HELPER             ] = 50,
            [JOB_WOODCUTTER         ] = 10,
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
            [JOB_BREWER             ] =  5,
            [JOB_PIGBREEDER         ] = 10,
            [JOB_DONKEYBREEDER      ] = 10,
            [JOB_IRONFOUNDER        ] =  8,
            [JOB_MINTER             ] =  8,
            [JOB_METALWORKER        ] = 10,
            [JOB_ARMORER            ] = 10,
            [JOB_BUILDER            ] = 20,
            [JOB_PLANER             ] = 20,
            [JOB_GEOLOGIST          ] = 10,
            [JOB_PRIVATE            ] = 40,
            [JOB_PRIVATEFIRSTCLASS  ] =  9,
            [JOB_SERGEANT           ] =  3,
            [JOB_OFFICER            ] =  2,
            [JOB_GENERAL            ] =  1,
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

    if( (x == 13) and (y == 66) ) then MissionEvent(99)
    end

    if(not rttr:GetPlayer(1):IsInRestrictedArea(x, y)) then 
        MissionEvent(98) -- for lifting restrictions
    end
end

function onExplored(p, x, y, o)
    -- onContact events
    if( ((p == 0) and (o == 1)) or ((p == 1) and (o == 0)) ) then MissionEvent(2)
    end

    if(p ~= 0) then
        return
    end

    -- onExplored events
    if( (x == 43) and (y == 59) ) then MissionEvent(3)
    end
end

-- execute mission events, e == 1 is initial event, e == 99 is final event
function MissionEvent(e, onLoad)
    -- event e is inactive
    if(eState[e] <= 0) then
        return
    end

    if(e == 98) then
        rttr:GetPlayer(1):SetRestrictedArea()
        rttr:GetPlayer(2):SetRestrictedArea()

    -- call side effects for active events, check "eState[e] == 1" for multiple call events!
    elseif(e == 99) then
        -- TODO: EnableNextMissions()
        -- Show opened arc
        rttr:GetWorld():AddStaticObject(13, 66, 561, 0xFFFF, 2)
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