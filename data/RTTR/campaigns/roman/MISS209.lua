------------------------------------------------------------------------------
-- LUA-Script for MISS209.WLD (mission 10 of the original "Roman Campaign") --
--                                                                          --
-- Authors: CrazyL, Spikeone, ArthurMurray47                                --
------------------------------------------------------------------------------


-------------------------------- TODO -----------------------------------------
-- Set Portraits
-- Set AI Agression Level
-------------------------------------------------------------------------------


-------------------------------- Lua Version used -----------------------------
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
eIdx = {1, 2, 99}

rttr:RegisterTranslations(
{
    cs =
    { 
        Diary   = 'Deník',

        msg1    = 'Když se schiluje k poslední a nejnebezpečnější kapitole naší dlouhé cesty domů, vzduch je cítit očekávání a dokonce i malá neochota. Uvidíme ještě někdy Řím? Musíme zmobilizovat všechny své síly, protože tento ostrov se zdá být naší nejlepší šancí ...',
        msgh1   = 'Buď opatrný. Neútoč, dokud si nebudeš jist zda můžeš porazit oba nepřátele.',

        msg2    = '17. den šestého měsíce 10. roku.\n\nVe skutečnosti to jsou římané kdo nám stojí v cestě k desáté bráně. Nejsou vůbec přátelští. Možná to je druhá strana mince, nepřátelský bratr Remus. Možná je jejich brána ta, která nás konečně zavede zpět do našeho milovaného Říma. Jak říká stará legenda: \"Kvůli Římu bude bratr bojovat proti bratrovi.\" Téměř deset let neustále sníme o své vlasti. Teď se nevzdáme.',
        msgh2   = 'Shromážděte všechny své síly, abyste je mohli prorazit! Zajměte bránu!',

        msg99   = 'Poslední den 10. roku. Poslední záznam Oktaviána v tomto deníku\n\nUspěli jsme. Zítra se vracíme do Říma přesně deset let poté, co jsme se stali trosečníky na tomto ostrově. Deset let, během nichž jsme cestovali ve stopách našich předků. Deset let, během nichž jsme se dozvěděli, co dělalo Řím tak skvělým. Nabídneme bohům velkou hostinu a znovu uvidíme naše rodiny a přátele. Už se nemůžu dočkat.',
        msgh99  = 'Uspěl jsi!'
    },
    de =
    {
        Diary   = 'Tagebuch',

        msg1    = 'Die Aura der Entscheidung liegt über diesem Ort.\nWerden wir Rom jemals wiedersehen? Wir müssen alle\nunsere Kraft zusammennehmen, denn diese Insel\nscheint unsere große Chance zu sein...',
        msgh1   = 'Seien Sie vorsichtig. Sie sollten den Feind erst\nangreifen, wenn sie es mit beiden Gegnern aufnehmen\nkönnen.',

        msg2    = '17.Tag im 6.Monat des 10.Jahres\n\nRömer. Vor dem zehnten Tor stehen tatsächlich Römer.\nAber sie sind uns keineswegs freundlich gesinnt.\nVielleicht sind sie die andere Seite derselben\nMünze, der verfeindete Bruder Remus. Vielleicht ist\nihr Tor dasjenige, welches uns in unser geliebtes\nRom zurückbringt.\nEs ist wie die alte Sage:\nUm Roms Willen müssen wir den Bruderkampf aufnehmen.\nSeit fast zehn Jahren träumen wir tagtäglich von\nunserer Heimat. Wir werden jetzt nicht aufgeben.',
        msgh2   = 'Sammeln Sie Ihre Kräfte um durchzubrechen! Suchen\nSie nach dem Tor!',

        msg99   = 'Letzter Tag des 10.Jahres.\nDes Octavius letzter Eintrag in dieses Tagebuch\n\nWir haben es geschafft. Morgen werden wir nach Rom\nzurückkehren. Genau zehn Jahre, nachdem es uns in\ndiese Inselwelt verschlagen hat. Zehn Jahre, in\ndenen wir auf den Spuren unserer Väter wandelten.\nZehn Jahre, in denen wir lernten, was Rom groß\ngemacht hat. Wir werden den Göttern ein großes\nDankesfest bieten und unsere Familien und Freunde\nwiedersehen.\nIch kann es kaum erwarten.',
        msgh99  = 'Sie haben es geschafft!'
    },
    en =
    { 
        Diary   = 'Diary',

        msg1    = 'The air is filled with anticipation and even a little\nreluctance, as we prepare to embark on the final and\nmost dangerous chapter of our long journey home. Will\nwe ever see Rome again?\nWe must summon all our strength because this island\nseems to be our best chance...',
        msgh1   = 'Be careful. Do not attack until you are sure you can\ndefeat both enemies.',

        msg2    = 'The 17th Day of the Sixth Month of the 10th Year.\n\nRomans are actually standing in front of the tenth\ngateway. They are not at all friendly. Perhaps they\nare the other side of the coin, the enemy brother\nRemus. Maybe their gateway is the one that will take\nus back to our beloved Rome.\nAs the old legend says:\n\"For the sake of Rome brother shall fight against\nbrother.\" For almost ten years we have been\nconstantly dreaming of our homeland. We shall not\ngive up now.',
        msgh2   = 'Muster all of your forces in order to break through!\nCapture the gateway!',

        msg99   = 'Last Day of the 10th Year.\nLast entry of Octavius in this Diary\n\nWe have succeeded. Tomorrow we shall return to Rome\nexactly ten years after becoming castaways on this\nisland. Ten years during which we traveled in the\nfootsteps of our forefathers. Ten years during which\nwe learned what made Rome so great. We shall offer a\ngreat feast of thanks to the gods and will see our\nfamilies and friends again.\nI can hardly wait.',
        msgh99  = 'You have succeeded!'
    },
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
    rttr:Log("-----------------------\n MISS209.lua loaded... \n-----------------------\n")
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
    rttr:GetPlayer(1):SetNation(NAT_ROMANS)     -- nation
    rttr:GetPlayer(1):SetColor(1)               -- yellow
    rttr:GetPlayer(1):SetName('Brutus')         -- Enemy Name
    rttr:GetPlayer(1):SetTeam(TM_TEAM1)

    rttr:GetPlayer(2):SetAI(3)                  -- hard AI
    rttr:GetPlayer(2):SetNation(NAT_VIKINGS)    -- nation
    rttr:GetPlayer(2):SetColor(2)               -- red
    rttr:GetPlayer(2):SetName('Olof')           -- Enemy Name
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

    rttr:GetWorld():SetComputerBarrier(10, 57, 73)
    rttr:GetWorld():SetComputerBarrier(10, 39, 29)
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
    rttr:GetPlayer(p):DisableBuilding(BLD_SHIPYARD, false)
    rttr:GetPlayer(p):DisableBuilding(BLD_HARBORBUILDING, false)

    if(p == 1) then
        if onLoad then return end

        rttr:GetPlayer(p):AIConstructionOrder(57, 73, BLD_FORTRESS)
        rttr:GetPlayer(p):AIConstructionOrder(61, 68, BLD_FORTRESS)
        rttr:GetPlayer(p):AIConstructionOrder(53, 71, BLD_FORTRESS)
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
            [GD_BEER        ] =  8,
            [GD_TONGS       ] =  1,
            [GD_HAMMER      ] =  2,
            [GD_AXE         ] =  1,
            [GD_SAW         ] =  1,
            [GD_PICKAXE     ] =  1,
            [GD_SHOVEL      ] =  1,
            [GD_CRUCIBLE    ] =  1,
            [GD_RODANDLINE  ] =  3,
            [GD_SCYTHE      ] =  3,
            [GD_WATER       ] = 20,
            [GD_CLEAVER     ] =  1,
            [GD_ROLLINGPIN  ] =  1,
            [GD_BOW         ] =  2,
            [GD_BOAT        ] =  2,
            [GD_SWORD       ] =  4,
            [GD_IRON        ] = 10,
            [GD_FLOUR       ] =  8,
            [GD_FISH        ] =  4,
            [GD_BREAD       ] =  6,
            [GD_SHIELD      ] =  4,
            [GD_WOOD        ] = 30,
            [GD_BOARDS      ] = 50,
            [GD_STONES      ] = 46,
            [GD_GRAIN       ] = 10,
            [GD_COINS       ] =  4,
            [GD_GOLD        ] = 15,
            [GD_IRONORE     ] = 15,
            [GD_COAL        ] = 30,
            [GD_MEAT        ] =  3,
            [GD_HAM         ] =  6
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
            [GD_IRON        ] =  10,
            [GD_FLOUR       ] =   0,
            [GD_FISH        ] =  20,
            [GD_BREAD       ] =  20,
            [GD_SHIELD      ] =   0,
            [GD_WOOD        ] =   0,
            [GD_BOARDS      ] = 120,
            [GD_STONES      ] = 120,
            [GD_GRAIN       ] =   0,
            [GD_COINS       ] =   4,
            [GD_GOLD        ] =  35,
            [GD_IRONORE     ] =  15,
            [GD_COAL        ] =  40,
            [GD_MEAT        ] =  20,
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
            [JOB_PIGBREEDER         ] = 10,
            [JOB_DONKEYBREEDER      ] = 10,
            [JOB_IRONFOUNDER        ] =  8,
            [JOB_MINTER             ] =  7,
            [JOB_METALWORKER        ] = 10,
            [JOB_ARMORER            ] = 10,
            [JOB_BUILDER            ] = 20,
            [JOB_PLANER             ] = 20,
            [JOB_GEOLOGIST          ] = 10,
            [JOB_PRIVATE            ] = 40,
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
            [GD_IRON      ] =  0,
            [GD_FLOUR     ] =  0,
            [GD_FISH      ] = 20,
            [GD_BREAD     ] = 20,
            [GD_SHIELD    ] =  0,
            [GD_WOOD      ] =  0,
            [GD_BOARDS    ] = 90,
            [GD_STONES    ] = 60,
            [GD_GRAIN     ] =  0,
            [GD_COINS     ] =  0,
            [GD_GOLD      ] =  0,
            [GD_IRONORE   ] =  0,
            [GD_COAL      ] = 30,
            [GD_MEAT      ] = 20,
            [GD_HAM       ] =  0
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
            [JOB_MINER              ] = 10,
            [JOB_BREWER             ] = 10,
            [JOB_PIGBREEDER         ] = 10,
            [JOB_DONKEYBREEDER      ] = 10,
            [JOB_IRONFOUNDER        ] =  8,
            [JOB_MINTER             ] =  5,
            [JOB_METALWORKER        ] = 10,
            [JOB_ARMORER            ] = 10,
            [JOB_BUILDER            ] = 20,
            [JOB_PLANER             ] = 20,
            [JOB_GEOLOGIST          ] = 10,
            [JOB_PRIVATE            ] = 20,
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

    if( (x == 75) and (y == 40) ) then MissionEvent(99)
    end
end

function onExplored(p, x, y, o)
    -- onContact events
    if( ((p == 0) and (o == 1)) or ((p == 1) and (o == 0)) ) then MissionEvent(2)
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
        -- Show opened arc
        rttr:GetWorld():AddStaticObject(75, 40, 561, 0xFFFF, 2)
        rttr:SetCampaignChapterCompleted("roman", 10)
        rttr:SetCampaignCompleted("roman")
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