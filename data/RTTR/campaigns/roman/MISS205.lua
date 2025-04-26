------------------------------------------------------------------------------
-- LUA-Script for MISS205.WLD (mission 6 of the original "Roman Campaign"   --
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
eIdx = {1, 2, 3, 99}

rttr:RegisterTranslations(
{
    cs =
    { 
        Diary   = 'Deník',

        msg1    = 'Poslední den pátého roku.\n\nSkalní rytiny naznačují, že na jihozápadě je ostrov. Zdá se, že tam jsou velká ložiska zlata.',
        msgh1   = 'Najděte a obsaďte bránu! porozhlédněte se po možnostech výstavby přístavu.',

        msg2    = 'Vikingové mají osadu za velkým pohořím na východě našeho ostrova. Z vrcholů hor je na severovýchodě vidět další velká pevnina.\n\n\n\nDosud jsme nenašli žádné stopy po bráně a váleční zajatci tvrdí, že o ničem takovém neslyšeli jsou to ale chabí lháři.',

        msg3    = 'Za hosrkým hřebenem jsme obsadili malou část země poblíž Vikingů. Zajali jsme další vězně, kteří nám řekli, kde je brána. Jak jsme již tušili, je to na dalekém severovýchodě. Kromě malé oblasti na jihu je pevnina zcela v rukou Vikingů. Musíme tam rychle připlout, postavit přístav a získat nové zásoby.',

        msg99   = 'Našli jsme bránu a obsadili ji.',
        msgh99  = 'Dokončil jsi tuto misi. Další kapitola na tebe čeká ...'
    },
    de =
    {
        Diary   = 'Tagebuch',

        msg1    = 'Letzter Tag im 5.Jahr.\n\nFelszeichnungen deuten auf eine Insel im Südwesten.\nDort scheint es große Goldvorkommen zu geben.',
        msgh1   = 'Finden und besetzen sie das Tor! Achten Sie auf\nMöglichkeiten zum Hafenbau.',

        msg2    = 'Wikinger. Sie siedeln hinter der großen Gebirgskette\nim Osten unserer Insel. Von den Gipfeln der Kette\nläßt sich eine weitere sehr große Landmasse im\nNordosten erahnen.\n\n\n\nWir haben noch keine Spur eines Tores gefunden und\nKriegsgefangene behaupten, so etwas nicht zu kennen.\nAber sie sind schlechte Lügner.',

        msg3    = 'Wir haben den kleinen Flecken Erde hinter dem\nGebirgszug von den Wikingern erobert.\nWir nahmen einen weiteren Gefangenen, der uns\nverriet, wo sich das Tor befindet. Wie wir schon\nvermutet haben, liegt es weit im Nordosten.\nAber bis auf einen kleinen Fleck im Süden der\nLandmasse ist sie bereits komplett in der Hand der\nWikinger. Wir müssen uns beeilen, dort einen Hafen\nerrichten und den Nachschub gut organisieren.',

        msg99   = 'Wir haben das Tor gefunden und besetzt.',
        msgh99  = 'Sie haben diese Mission erfüllt. Das nächste Kapitel\nwartet auf Sie...'
    },
    en =
    { 
        Diary   = 'Diary',

        msg1    = 'Last Day of the Fifth Year.\n\nRock carvings suggest there is an island in the\nsouthwest. It appears there are large gold deposits\nthere.',
        msgh1   = 'Find and occupy the gateway! Check out the\nopportunities for building a harbor.',

        msg2    = 'The Vikings have a settlement behind the large\nmountain range on the east of our island. From the\npeaks of the mountains, one can see another very\nlarge land mass in the northeast.\n\n\n\nWe have not yet found any trace of a gateway and the\nprisoners of war claim they know of no such thing,\nbut they are poor liars.',

        msg3    = 'We have captured the small area of land beyond the\nmountain chain from the Vikings.\nWe took further prisoners who told us where the\ngateway is. As we already suspected, it is in the far\nnortheast.\nApart from a small area in the south of the land\nmass, it is entirely in the hands of the Vikings. We\nmust quickly build a harbor there and acquire fresh\nsupplies.',

        msg99   = 'We have found the gateway and occupied it.',
        msgh99  = 'You have completed this mission. The next Chapter\nawaits you...'
    },
    pl =
    { 
        Diary   = 'Dziennik',

        msg1    = 'Ostatni Dzień Piątego Roku.\n\nRzeźby skalne sugerują, że na południowym zachodzie znajduje się wyspa.\n\nWygląda na to, że są tam znaczne złoża złota.',
        msgh1   = 'Znajdź i zajmij wrota!\nSprawdź możliwości budowy portu.',

        msg2    = 'Wikingowie mają osadę za dużym pasmem górskim na wschodzie naszej wyspy.\n\nZe szczytów gór można zobaczyć kolejny bardzo duży fragment lądu na północnym wschodzie.\n\nNie znaleźliśmy jeszcze żadnych śladów wrót, a jeńcy wojenni twierdzą, że nic o nich nie wiedzą... Są jednak kiepskimi kłamcami.',

        msg3    = 'Zdobyliśmy mały obszar ziemi za pasmem górskim od Wikingów.\n\nWzięliśmy kolejnych jeńców, którzy powiedzieli nam, gdzie znajdują się wrota.\n\nJak już podejrzewaliśmy, znajdują się one na dalekim północnym wschodzie.\n\nPoza małym obszarem na południu tej krainy, całość jest w rękach Wikingów.\n\nMusimy szybko zbudować tam port i zdobyć świeże zapasy.',

        msg99   = 'Znaleźliśmy wrota i zajęliśmy je.',
        msgh99  = 'Ukończyłeś tę misję.\nNastępny rozdział czeka na ciebie...'
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
    rttr:Log("-----------------------\n MISS205.lua loaded... \n-----------------------\n")
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
    for i = 0, 2 do                         -- set resources
        addPlayerRes(i, not isFirstStart)
        addPlayerBld(i, not isFirstStart)
    end

    eState = {}                             -- enable all events
    for _, i in ipairs(eIdx) do
        eState[i] = 1
    end

    if not isFirstStart then                -- initialize history of all called events, event counter at ["n"]
        for i = 1, eHist["n"] do
            MissionEvent(eHist[i], true)    -- call events without mission text to activate all achievements
        end

    else
        eHist = {["n"] = 0}
        MissionEvent(1)                     -- initial event / start screen
    end

    rttr:GetWorld():SetComputerBarrier(16, 162, 69)
    rttr:GetWorld():SetComputerBarrier(17, 148, 107)
    rttr:GetWorld():SetComputerBarrier(15, 131, 71)
    rttr:GetWorld():SetComputerBarrier(16, 103, 40)
    rttr:GetWorld():SetComputerBarrier(13, 124, 93)
    rttr:GetWorld():SetComputerBarrier(13, 103, 49)
    rttr:GetWorld():SetComputerBarrier(12, 108, 66)
    rttr:GetWorld():SetComputerBarrier(14, 61, 111)
    rttr:GetWorld():SetComputerBarrier(13, 48, 41)
    rttr:GetWorld():SetComputerBarrier(14, 40, 52)
    rttr:GetWorld():SetComputerBarrier(14, 25, 45)

    if isFirstStart then
        -- type 8 == 7 in rttr
        rttr:GetWorld():AddAnimal( 126,  48, SPEC_DUCK)
        rttr:GetWorld():AddAnimal( 126,  48, SPEC_DUCK)
        rttr:GetWorld():AddAnimal( 141,  54, SPEC_DUCK)
        rttr:GetWorld():AddAnimal( 141,  54, SPEC_DUCK)
        rttr:GetWorld():AddAnimal( 141,  54, SPEC_DUCK)
        rttr:GetWorld():AddAnimal( 153,  59, SPEC_RABBITWHITE)
        rttr:GetWorld():AddAnimal( 153,  59, SPEC_RABBITWHITE)
        rttr:GetWorld():AddAnimal( 154,  60, SPEC_RABBITGREY)
        rttr:GetWorld():AddAnimal( 154,  60, SPEC_RABBITGREY)
        rttr:GetWorld():AddAnimal(  72,  74, SPEC_RABBITGREY)
        rttr:GetWorld():AddAnimal(  72,  74, SPEC_RABBITWHITE)
        rttr:GetWorld():AddAnimal(  40,  33, SPEC_RABBITWHITE)
        rttr:GetWorld():AddAnimal(  40,  33, SPEC_RABBITGREY)
        rttr:GetWorld():AddAnimal(  92,  54, SPEC_RABBITWHITE)
        rttr:GetWorld():AddAnimal(  92,  54, SPEC_RABBITWHITE)
        rttr:GetWorld():AddAnimal(  82, 103, SPEC_RABBITGREY)
        rttr:GetWorld():AddAnimal(  82, 103, SPEC_RABBITGREY)
        rttr:GetWorld():AddAnimal(  82, 103, SPEC_RABBITWHITE)
        rttr:GetWorld():AddAnimal(  69,  81, SPEC_RABBITGREY)
        rttr:GetWorld():AddAnimal(  69,  81, SPEC_RABBITWHITE)
        rttr:GetWorld():AddAnimal(  69,  81, SPEC_RABBITWHITE)
        rttr:GetWorld():AddAnimal(  31,  82, SPEC_RABBITWHITE)
        rttr:GetWorld():AddAnimal(  31,  82, SPEC_RABBITGREY)
        rttr:GetWorld():AddAnimal(  31,  82, SPEC_RABBITGREY)
        rttr:GetWorld():AddAnimal(  31,  82, SPEC_RABBITGREY)
        rttr:GetWorld():AddAnimal(  31,  82, SPEC_RABBITGREY)
        rttr:GetWorld():AddAnimal(  31,  82, SPEC_RABBITGREY)
        rttr:GetWorld():AddAnimal(  31,  82, SPEC_RABBITGREY)
        rttr:GetWorld():AddAnimal(  31,  82, SPEC_RABBITGREY)
        rttr:GetWorld():AddAnimal(  31,  82, SPEC_RABBITGREY)
        rttr:GetWorld():AddAnimal(  31,  82, SPEC_RABBITGREY)
        rttr:GetWorld():AddAnimal(  31,  82, SPEC_RABBITGREY)
        rttr:GetWorld():AddAnimal(  31,  82, SPEC_RABBITGREY)
        rttr:GetWorld():AddAnimal(  31,  82, SPEC_RABBITGREY)
        rttr:GetWorld():AddAnimal(  31,  82, SPEC_RABBITGREY)
        rttr:GetWorld():AddAnimal(  31,  82, SPEC_RABBITGREY)
        rttr:GetWorld():AddAnimal(  31,  82, SPEC_RABBITGREY)
        rttr:GetWorld():AddAnimal(  31,  82, SPEC_RABBITGREY)
        rttr:GetWorld():AddAnimal(  31,  82, SPEC_RABBITGREY)
        rttr:GetWorld():AddAnimal(  31,  82, SPEC_RABBITGREY)
        rttr:GetWorld():AddAnimal(  31,  82, SPEC_RABBITGREY)
        rttr:GetWorld():AddAnimal(  31,  82, SPEC_RABBITGREY)
        rttr:GetWorld():AddAnimal(  31,  82, SPEC_RABBITGREY)
        rttr:GetWorld():AddAnimal(  31,  82, SPEC_RABBITGREY)
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
end

-------------------------------- set resources --------------------------------
-- Don't add goods/people onLoad!
function addPlayerRes(p, onLoad)
    if onLoad then return end

    rttr:GetPlayer(p):ClearResources()
    if(p == 0) then
        -- goods
        rttr:GetPlayer(p):AddWares({
            [GD_BEER      ] = 12,
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
            [GD_FISH      ] =  7,
            [GD_BREAD     ] =  7,
            [GD_SHIELD    ] =  0,
            [GD_WOOD      ] =  0,
            [GD_BOARDS    ] = 25,
            [GD_STONES    ] = 20,
            [GD_GRAIN     ] =  0,
            [GD_COINS     ] =  5,
            [GD_GOLD      ] = 15,
            [GD_IRONORE   ] = 10,
            [GD_COAL      ] = 35,
            [GD_MEAT      ] =  7,
            [GD_HAM       ] =  0
        })

        -- people
        rttr:GetPlayer(p):AddPeople({
            [JOB_HELPER             ] = 25,
            [JOB_WOODCUTTER         ] =  8,
            [JOB_FISHER             ] =  2,
            [JOB_FORESTER           ] =  3,
            [JOB_CARPENTER          ] =  4,
            [JOB_STONEMASON         ] =  3,
            [JOB_HUNTER             ] =  2,
            [JOB_FARMER             ] =  8,
            [JOB_MILLER             ] =  2,
            [JOB_BAKER              ] =  2,
            [JOB_BUTCHER            ] =  2,
            [JOB_MINER              ] =  6,
            [JOB_BREWER             ] =  2,
            [JOB_PIGBREEDER         ] =  4,
            [JOB_DONKEYBREEDER      ] =  1,
            [JOB_IRONFOUNDER        ] =  2,
            [JOB_MINTER             ] =  2,
            [JOB_METALWORKER        ] =  2,
            [JOB_ARMORER            ] =  4,
            [JOB_BUILDER            ] = 14,
            [JOB_PLANER             ] = 12,
            [JOB_GEOLOGIST          ] =  8,
            [JOB_PRIVATE            ] = 35,
            [JOB_PRIVATEFIRSTCLASS  ] = 10,
            [JOB_SERGEANT           ] =  5,
            [JOB_OFFICER            ] =  3,
            [JOB_GENERAL            ] =  1,
            [JOB_SCOUT              ] =  8,
            [JOB_SHIPWRIGHT         ] =  1,
            [JOB_PACKDONKEY         ] =  0,
            [JOB_CHARBURNER         ] =  0
        })

    elseif(p == 1) then
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
            [JOB_FARMER             ] = 10,
            [JOB_MILLER             ] = 10,
            [JOB_BAKER              ] =  5,
            [JOB_BUTCHER            ] = 10,
            [JOB_MINER              ] = 20,
            [JOB_BREWER             ] = 10,
            [JOB_PIGBREEDER         ] = 10,
            [JOB_DONKEYBREEDER      ] = 10,
            [JOB_IRONFOUNDER        ] =  8,
            [JOB_MINTER             ] =  5,
            [JOB_METALWORKER        ] =  5,
            [JOB_ARMORER            ] = 10,
            [JOB_BUILDER            ] = 20,
            [JOB_PLANER             ] = 20,
            [JOB_GEOLOGIST          ] = 10,
            [JOB_PRIVATE            ] = 50,
            [JOB_PRIVATEFIRSTCLASS  ] = 10,
            [JOB_SERGEANT           ] =  4,
            [JOB_OFFICER            ] =  2,
            [JOB_GENERAL            ] =  0,
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
            [GD_STONES      ] = 60,
            [GD_GRAIN       ] =  0,
            [GD_COINS       ] =  0,
            [GD_GOLD        ] =  0,
            [GD_IRONORE     ] =  0,
            [GD_COAL        ] = 30,
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
            [JOB_BREWER             ] = 10,
            [JOB_PIGBREEDER         ] = 10,
            [JOB_DONKEYBREEDER      ] = 10,
            [JOB_IRONFOUNDER        ] = 10,
            [JOB_MINTER             ] =  5,
            [JOB_METALWORKER        ] = 10,
            [JOB_ARMORER            ] = 10,
            [JOB_BUILDER            ] = 20,
            [JOB_PLANER             ] = 20,
            [JOB_GEOLOGIST          ] = 10,
            [JOB_PRIVATE            ] = 50,
            [JOB_PRIVATEFIRSTCLASS  ] = 10,
            [JOB_SERGEANT           ] =  2,
            [JOB_OFFICER            ] =  0,
            [JOB_GENERAL            ] =  0,
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

    if(     (x == 102) and (y == 50) ) then MissionEvent(3)
    elseif( (x == 148) and (y == 50) ) then MissionEvent(99)
    end
end

function onExplored(p, x, y, o)
    -- onContact events
    if( ((p == 0) and (o == 2)) or ((p == 2) and (o == 0)) ) then MissionEvent(2)
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
        rttr:GetWorld():AddStaticObject(148, 50, 561, 0xFFFF, 2)
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