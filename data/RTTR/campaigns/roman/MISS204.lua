------------------------------------------------------------------------------
-- LUA-Script for MISS204.WLD (mission 5 of the original "Roman Campaign")  --
--                                                                          --
-- Authors: CrazyL, Spikeone, ArthurMurray47, kubaau                        --
------------------------------------------------------------------------------


-------------------------------- TODO -----------------------------------------
-- EnableNextMissions()
-- Set Portraits
-------------------------------------------------------------------------------


-------------------------------- Lua Version used -----------------------------
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
-------------------------------- mission events and texts ---------------------
-- Message-Window (mission statement and hints): 52 chars wide
eIdx = {1, 2, 3, 99}

rttr:RegisterTranslations(
{
    cs =
    { 
        Diary   = 'Deník',

        msg1    = 'Jsme obklopeni pustou divočinou. Zdá se,že tento svět obklopují neproniknutelné hory nebo lávové proudy.\n\nJe zde nemožné sledovat čas. Vše halí věčný soumrak, není zde sluneční svit ani světlo hvězd.\n\nJaké asi zázraky nebo nebezpečí tady na nás čekají?',
        msgh1   = 'Stále hledejte bránu. Pro budovy je zde omezený prostor; pečlivě zvaž, jak využiješ své zdroje.',

        msg2    = 'Potkali jsme lidi mnohem podivnějšího vzhledu než je náš vikingský přítel Erik. Říkají si \"Synové Nipponu.\" Jsou malí a hubení, ale zjevně zruční a houževnatí. Jejich nejpodivnějším rysem je barva pleti. Je to žlutý okr a jejich oči jsou jen malé štěrbinky. Chovají se přátelsky, i když nás kvůli smému vzhledu znempokojují.',

        msg3    = '\"Synové Nipponu\" žijící na jihu mají na severu příbuzné. Jejich vzájemný vztah vypadá velmi přátelský. Válka s jednou rasou znamená válku s druhou. Případný konflikt musíme pečlivě zvážit ...',

        msg99   = 'Našli jsme bránu a obsadili ji. Můžeme skrze ní kdykolv projít!',
        msgh99  = 'Dokončil jsi tuto misi. Další kapitola na tebe čeká ...'
    },
    de =
    {
        Diary   = 'Tagebuch',

        msg1    = 'Um uns herum nur öde Wildnis. Die Ränder dieser Welt\nscheinen unüberwindliche Gebirge oder Lavaströme zu\nsein.\n\nEs ist unmöglich, hier die Zeit zu bestimmen. Ein\ndiffuses, immerwährendes Zwielicht liegt über allem\nals gäbe es weder Sonne noch Gestirne.\n\nWelche Wunder und Gefahren liegen hier verborgen?',
        msgh1   = 'Halten Sie Ausschau nach dem Tor. Der\nPlatz zum Bauen ist begrenzt; überlegen Sie gut, wie\nSie Ihre Resourcen ausnutzen.',

        msg2    = 'Wir haben Menschen getroffen, die noch viel\nseltsamer ausschauen, als die blonden Riesen, denen\nunser Schiffsbauer angehört.\nSie nennen sich \"Söhne Nippons\", sind klein und\nschmal, aber offensichtlich von großer Gewandheit\nund Zähigkeit. Das Seltsame an ihnen ist aber ihre\nHautfarbe. Sie ist ockergelb und ihre Augen sind nur\nkleine Schlitze. Sie wirken freundlich, obwohl ihr\nBlick sehr beunruhigend ist, da man die Augen nicht\nsieht.',

        msg3    = 'Die \"Söhne Nippons\" im Süden haben noch Verwandte im\nNorden. Ihre Beziehungen untereinander sind\nanscheinend sehr freundschaftlich. Krieg mit dem\neinen Volk bedeutet auch Krieg mit dem anderen. Wir\nmüssen uns etwas einfallen lassen...',

        msg99   = 'Wir haben das Tor gefunden und besetzt. Wir können\nes betreten, wann immer wir wollen!',
        msgh99  = 'Sie haben diese Mission erfüllt. Das nächste Kapitel\nwartet schon auf Sie... '
    },
    en =
    {
        Diary   = 'Diary',

        msg1    = 'We are surrounded by nothing but\nbarren wilderness. The edges of this world seem to\nbe either impenetrable mountains or lava flows.\n\nIt is impossible to keep track of time here. A dim,\nperpetual twilight hangs over everything and there\nis neither sunlight nor starlight.\n\nWhat wonders and dangers lie hidden here?',
        msgh1   = 'Keep looking for the gateway. There is limited space\nfor buildings; carefully consider how to exploit\nyour resources.',

        msg2    = 'We have met humans of much stranger appearance than\nour Viking shipwright.\nThey call themselves \"Sons of Nippon.\" They are\nshort and thin but obviously skilled and tenacious.\nTheir strangest feature is their skin color. It is\nyellow ochre and their eyes are just small slits.\nThey act in a friendly manner although they are\nunsettling to behold because of their strange\nappearance.',

        msg3    = 'The \"Sons of Nippon\" in the south have relatives in\nthe north. Their relationship with each other\nappears very friendly. War with one race means war\nwith the other. We must consider the prospect of war\ncarefully...',

        msg99   = 'We have found the gateway and occupied it. We can go\nthrough it whenever we want!',
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
    rttr:Log("-----------------------\n MISS204.lua loaded... \n-----------------------\n")
    rttr:ResetAddons()
    rttr:SetAddon(ADDON_CATAPULTS_ATTACK_ALLIES, true)
    rttr:SetAddon(ADDON_FRONTIER_DISTANCE_REACHABLE, true)
    rttr:SetGameSettings({
        ["fow"] = EXP_CLASSIC,
        ["teamView"] = false,
        ["lockedTeams"] = false
    })

    rttr:GetPlayer(0):SetNation(NAT_ROMANS)     -- nation
    rttr:GetPlayer(0):SetColor(0)               -- 0:blue, 1:read, 2:yellow, 

    rttr:GetPlayer(1):SetAI(3)                  -- hard AI
    rttr:GetPlayer(1):SetNation(NAT_JAPANESE)   -- nation
    rttr:GetPlayer(1):SetColor(1)               -- yellow
    rttr:GetPlayer(1):SetName('Hakirawashi')    -- Enemy Name

    rttr:GetPlayer(2):SetAI(3)                  -- hard AI
    rttr:GetPlayer(2):SetNation(NAT_JAPANESE)   -- nation
    rttr:GetPlayer(2):SetColor(2)               -- red
    rttr:GetPlayer(2):SetName('Tsunami')        -- Enemy Name
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
    if isFirstStart then
        rttr:GetPlayer(2):MakeOneSidedAllianceTo(1) -- !GLOBAL_SET_COMPUTER_ALLIANCE  2 1
        rttr:GetPlayer(1):MakeOneSidedAllianceTo(2) -- !GLOBAL_SET_COMPUTER_ALLIANCE  1 2
        rttr:GetPlayer(1):MakeOneSidedAllianceTo(0) -- !GLOBAL_SET_COMPUTER_ALLIANCE  1 0
        rttr:GetPlayer(2):MakeOneSidedAllianceTo(0) -- !GLOBAL_SET_COMPUTER_ALLIANCE  2 0
    end

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
    -- buildings (all players)
    rttr:GetPlayer(p):EnableAllBuildings()
    rttr:GetPlayer(p):DisableBuilding(BLD_SHIPYARD, false)
    rttr:GetPlayer(p):DisableBuilding(BLD_HARBORBUILDING, false)
end

-------------------------------- set resources --------------------------------
-- Don't add goods/people onLoad!
function addPlayerRes(p, onLoad)
    if onLoad then return end

    rttr:GetPlayer(p):ClearResources()
    if(p == 0) then
        -- goods
        rttr:GetPlayer(p):AddWares({
            [GD_BEER        ] =  0,
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
            [GD_BOARDS      ] = 50,
            [GD_STONES      ] = 50,
            [GD_GRAIN       ] =  0,
            [GD_COINS       ] =  0,
            [GD_GOLD        ] =  0,
            [GD_IRONORE     ] =  0,
            [GD_COAL        ] = 10,
            [GD_MEAT        ] = 20,
            [GD_HAM         ] =  0
        })

        -- people
        rttr:GetPlayer(p):AddPeople({
            [JOB_HELPER             ] = 50,
            [JOB_WOODCUTTER         ] = 15,
            [JOB_FISHER             ] =  8,
            [JOB_FORESTER           ] = 10,
            [JOB_CARPENTER          ] =  5,
            [JOB_STONEMASON         ] =  2,
            [JOB_HUNTER             ] =  3,
            [JOB_FARMER             ] =  8,
            [JOB_MILLER             ] =  5,
            [JOB_BAKER              ] =  4,
            [JOB_BUTCHER            ] =  3,
            [JOB_MINER              ] =  8,
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
            [JOB_SHIPWRIGHT         ] =  0,
            [JOB_PACKDONKEY         ] =  0,
            [JOB_CHARBURNER         ] =  0
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
            [GD_BOAT        ] =  15,
            [GD_SWORD       ] =   0,
            [GD_IRON        ] =   0,
            [GD_FLOUR       ] =   0,
            [GD_FISH        ] =  40,
            [GD_BREAD       ] =  20,
            [GD_SHIELD      ] =   0,
            [GD_WOOD        ] =  50,    --0 -> 50 (no trees at start)
            [GD_BOARDS      ] = 100,
            [GD_STONES      ] =  50,
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
            [GD_BOAT        ] =  15,
            [GD_SWORD       ] =   0,
            [GD_IRON        ] =   0,
            [GD_FLOUR       ] =   0,
            [GD_FISH        ] =  40,
            [GD_BREAD       ] =  20,
            [GD_SHIELD      ] =   0,
            [GD_WOOD        ] =   0,
            [GD_BOARDS      ] = 100,
            [GD_STONES      ] =  50,
            [GD_GRAIN       ] =   0,
            [GD_COINS       ] =   0,
            [GD_GOLD        ] =   0,
            [GD_IRONORE     ] =   0,
            [GD_COAL        ] =  10,
            [GD_MEAT        ] =  50,
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

    if( (x == 19) and (y == 37) ) then MissionEvent(99)
    end
end

function onExplored(p, x, y, o)
    -- onContact events
    if(     ((p == 0) and (o == 1)) or ((p == 1) and (o == 0)) ) then MissionEvent(2)
    elseif( ((p == 0) and (o == 2)) or ((p == 2) and (o == 0)) ) then MissionEvent(3)
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
        rttr:GetWorld():AddStaticObject(19, 37, 561, 0xFFFF, 2)
    end

    -- update event state
    eState[e] = eState[e] - 1

    -- no history update or mission texts while loading
    if not onLoad then
        rttr:Log(">> Event: " .. e .. ",\teState[" .. e .. "] = " .. eState[e])         -- TEST
        eHist["n"] = eHist["n"] + 1
        eHist[eHist["n"]] = e
        MissionText(e)
    end
end