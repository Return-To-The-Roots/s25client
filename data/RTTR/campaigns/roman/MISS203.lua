------------------------------------------------------------------------------
-- LUA-Script for MISS203.WLD (mission 4 of the original "Roman Campaign")  --
--                                                                          --
-- Authors: CrazyL, Spikeone, ArthurMurray47                                --
------------------------------------------------------------------------------


-------------------------------- TODO -----------------------------------------
-- EnableNextMissions()
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

local requiredFeature = 4
function checkVersion()
    local featureLevel = rttr:GetFeatureLevel()
    if(featureLevel < requiredFeature) then
        rttr:MsgBox("LUA-Version Error", "Your Return to the Roots version is outdated. The required LUA-Feature level is " ..requiredFeature.. ", your version is "..featureLevel..". The script can possibly crash or run unexpectedly!\n\nPlease update the game!", true)
    end
end
-------------------------------- mission events and texts ---------------------
-- Message-Window (mission statement and hints): 52 chars wide
eIdx = {1, 2, 3, 98, 99}

rttr:RegisterTranslations(
{
    cs =
    {
        Diary   = 'Deník',

        msg1    = 'Devátý den devátého měsíce čtvrtého roku.\n\nVčera jsme potkali cizince se skutečně děsivým vzhledem. Vypadá jako obr, má světle modré oči a zářící žluté vlasy. Kdysi dávno jsem v jedné krčmě v římském přístavu slyšel vyprávění od starého mořského vlka o obrech žijíchích na severu se světlou pletí, modrými oči a vlasech ze zlata. No, ty histroky byly trochu přehnané, ale jinak ten popis dost odpovídá. Jak se sem asi dostal? Je hodně podezíravý a odmítá s námi mluvit. Zkusíme se k němu chovat jako k našemu hostovi a za pár dní třeba změní svůj postoj. Zdá se, že je hladový, chudák.',
        msgh1   = 'Prozkoumej okolí.',

        msg2    = '16. den devátého měsíce čtvrtého roku.\n\nNo vida, čeho se dá dosáhnout s trochou štědrosti. Blonďatý obr nám řekl, že je členem národa žijícího na severu. Říká že jeho národu mnozí říkají různě, jako třeba Normané, Dánové, Ascomané, Varjagové ale nejspíš je budeme znát pod jménem Vikingové. Říká, že žil na velkém zeleném ostrově nedaleko na západě. Jmenuje se Erik. Jaké divné jméno! Pokud mu máme věřit, je stavitel lodí. Moje srdce bije rychleji při vyhlídce, že bych brzy mohl znovu nastoupit na loď, slyšet hukot vln pod kýlem, výkřiky racků a vítr v plachtách ...\n\nNejprve ale musíme postavit loděnici a přístav.',
        msgh2   = 'Postav přístav a loděnici. V loděnici postav velkou obchodní loď a v přístavu zahaj přípravy na expedici.',

        msg3    = 'Na malém ostrově uprostřed souostroví jsme objevili obrovský hřbitov velryb, kde jsou stovky gigantických koster. Jak impozantní pohled. Z počátku jsme nebili schopni pochopit, jak se tam ti tvorové dostali. Erik nám vysvětlil, že jeho lidé žijí z těchto tvorů a uctívali je jako svatá zvířata. Proto byly jejich kosti přeneseny na toto posvátné místo poté, co byly chyceny. Erik je nesvůj, chce odejít, protože se bojí duchů mrtvých tvorů.',
        msgh3   = 'Prozkoumej ostrov dál do vnitrozemí. Hledej další bránu.',

        msg99   = 'Dostali jsme se k bráně a aktivovali ji. Cesta je volná.',
        msgh99  = 'Dokončil jsi tuto misi. Další kapitola na tebe čeká ...'
    },
    de =
    {
        Diary   = 'Tagebuch',

        msg1    = '9.Tag im 9.Monat des 4. Jahres.\n\nWir trafen gestern auf einen Fremden, der wirklich\nzum Fürchten aussieht. Er ist riesengroß, hat\nhellblaue Augen und leuchtend gelbe Haare. In einer\nrömischen Hafenkneipe hörte ich einst von solchen\nVölkern hoch im Norden. Aber wie kam er hierher? Er\nist sehr mißtrauisch und redet nicht mit uns. Wir\nwerden ihn ein paar Tage als Gast bewirten (der arme\nKerl scheint völlig ausgehungert zu sein).\nVielleicht ändert er dann seine Meinung.',
        msgh1   = 'Erkunden Sie die Umgebung.',

        msg2    = '16.Tag.\n\nNa, bitte. Was ein wenig Freigiebigkeit doch alles\nbewirken kann. Der blonde Riese erzählte uns, daß er\nAngehöriger eines Volkes sei, das sich selbst als\n\"Wikinger\" bezeichnet. Es habe auf einer großen\nInsel nicht weit im Osten gelebt, sei aber von einem\nverfeindeten Stamm besiegt worden. Der Mann heißt\nErik. Seltsamer Name! Von Beruf ist er, wenn man ihm\nglauben kann, Schiffsbauer. Mein Herz schlägt höher\nbei dem Gedanken, daß ich bald wieder an Bord eines\nSchiffes gehen könnte, das Meer unter den Planken,\ndie Schreie der Möwen im Ohr, das Rauschen des\nWindes...\nAber zunächst müssen wir eine Werft und einen Hafen\nerrichten.',
        msgh2   = 'Bauen Sie einen Hafen und eine Werft. Bauen Sie in\nder Werft ein großes Handelsschiff und starten im\nHafen eine Expedition.',

        msg3    = 'Auf einer kleinen Insel in der Mitte des Archipels\nentdeckten wir einen riesigen Walfriedhof. Hunderte\nvon gigantischen Skeletten. Ein imposanter Anblick.\nWir konnten uns zunächst nicht erklären, wie diese\nTiere dort hinkamen. Erik erklärte uns aber dann,\ndaß sein Volk von diesen Tieren lebt und sie als\nheilig verehrt. Deshalb werden ihre Gebeine nach dem\nFang an diesen geweihten Ort gebracht. Erik ist\nunwohl. Er möchte schnell wieder weg, da er die\nGeister der toten Tiere fürchtet.',
        msgh3   = 'Erkunden Sie weitere Inseln. Suchen Sie nach dem\nnächsten Tor.',

        msg99   = 'Wir haben das Tor erreicht und aktiviert. Der Weg\nist frei.\n\n\n\n\n',
        msgh99  = 'Sie haben diese Mission erfüllt. Das nächste Kapitel\nwartet schon auf Sie...'
    },
    en =
    {
        Diary   = 'Diary',

        msg1    = 'The Ninth Day of the Ninth Month of the Fourth Year.\n\nYesterday we met a stranger of truly frightening\nappearance. He is huge in size, has light blue eyes\nand glowing golden hair. Long ago in a Roman harbor\ntavern I once heard stories of such people who live\nin the far north. How did he get here? He is\nsuspicious and refuses to talk to us. We shall treat\nhim as our guest for a few days (the poor fellow\nappears to be starving). Perhaps then he will change\nhis attitude.',
        msgh1   = 'Explore the surrounding area.',

        msg2    = '16th Day of the Ninth Month of the Fourth Year.\n\nWell, what wonders a little generosity can achieve.\nThe blond giant told us that he is a member of a\nrace that calls itself the \"Vikings\". He says that\nthey lived on a large island not far to the east but\nthey were conquered by a hostile tribe. His name is\nErik. What a strange name! If we are to believe him,\nhe is a shipwright by trade. My heart beats faster\nat the prospect of soon being able to board a ship\nagain, hearing the waves under the keel, the cries\nof the seagulls and the wind in the sails...\n\nBut first of all we must construct a shipyard and a\nharbor.',
        msgh2   = 'Build a harbor and a shipyard. In the shipyard build\na large trading ship and organize an expedition from\nthe harbor.',

        msg3    = 'On a small island in the middle of a archipelago, we\ndiscovered an enormous whale graveyard, in which\nthere were hundreds of gigantic skeletons. What an\nimposing sight. At first we were unable to\nunderstand how these creatures got there. Erik\nexplained that his people lived off these creatures\nand worshipped them as holy animals. This was why\ntheir bones were brought to this sacred place after\nthey were caught. Erik is uneasy, he wants to leave\nbecause he is afraid of the spirits of the dead\ncreatures.',
        msgh3   = 'Explore the island further. Search for the next\ngateway.',

        msg99   = 'We have reached the gateway and activated it. The\nway is now clear.',
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
    rttr:Log("-----------------------\n MISS203.lua loaded... \n-----------------------\n")
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
    rttr:GetPlayer(2):SetName('Knut')           -- Enemy Name
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
    for i = 0, 2 do                         -- set resources and buildings
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
    
    if isFirstStart then
        -- type 8 == 7 in rttr
        rttr:GetWorld():AddAnimal(  8,  17, SPEC_POLARBEAR)
        rttr:GetWorld():AddAnimal( 16,  11, SPEC_POLARBEAR)
        rttr:GetWorld():AddAnimal( 24,   7, SPEC_POLARBEAR)
        rttr:GetWorld():AddAnimal( 40,   7, SPEC_POLARBEAR)
        rttr:GetWorld():AddAnimal( 45,  14, SPEC_POLARBEAR)
        rttr:GetWorld():AddAnimal( 43,  13, SPEC_POLARBEAR)
        rttr:GetWorld():AddAnimal( 13,  18, SPEC_STAG)
        rttr:GetWorld():AddAnimal(  8,  45, SPEC_DUCK)
        rttr:GetWorld():AddAnimal(  7,  38, SPEC_DUCK)
        rttr:GetWorld():AddAnimal( 92,  39, SPEC_DUCK)
        rttr:GetWorld():AddAnimal( 58,  24, SPEC_DUCK)
        rttr:GetWorld():AddAnimal( 33,  25, SPEC_RABBITWHITE)
        rttr:GetWorld():AddAnimal( 34,  23, SPEC_RABBITGREY)
        rttr:GetWorld():AddAnimal( 32,  22, SPEC_RABBITWHITE)
        rttr:GetWorld():AddAnimal( 33,  24, SPEC_FOX)
        rttr:GetWorld():AddAnimal( 32,  28, SPEC_DEER)
        rttr:GetWorld():AddAnimal( 27,  33, SPEC_RABBITWHITE)
        rttr:GetWorld():AddAnimal( 28,  35, SPEC_RABBITGREY)
        rttr:GetWorld():AddAnimal( 29,  33, SPEC_RABBITWHITE)
        rttr:GetWorld():AddAnimal( 22,  25, SPEC_SHEEP)
        rttr:GetWorld():AddAnimal( 10,  33, SPEC_SHEEP)
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

    if not (p == 0) then
        -- set restriction area for all AIs
        rttr:GetPlayer(p):SetRestrictedArea(
            nil, nil,           -- enable the whole map
                  0,   0,
                  0, 127,
                127, 127,
                127,   0,
            nil, nil,           -- R=12, X=77, Y=37 V   R=12->8, X=79, Y=24
                 83,  16,
                 87,  24,
                 89,  37,
                 83,  49,
                 71,  49,
                 65,  37,
                 71,  24,
                 83,  16,
            nil, nil,           -- R=14, X=61, Y=75 V   R=13, X=62, Y=92
                 68,  61,
                 75,  75,
                 68,  79,
                 75,  92,
                 68, 105,
                 55, 105,
                 49,  92,
                 54,  89,
                 47,  75,
                 54,  61,
                 68,  61,
            nil, nil
        )
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
            [GD_TONGS       ] =  0,
            [GD_HAMMER      ] = 13,
            [GD_AXE         ] =  6,
            [GD_SAW         ] =  2,
            [GD_PICKAXE     ] =  5,
            [GD_SHOVEL      ] =  3,
            [GD_CRUCIBLE    ] =  4,
            [GD_RODANDLINE  ] =  5,
            [GD_SCYTHE      ] =  2,
            [GD_WATER       ] = 25,
            [GD_CLEAVER     ] =  3,
            [GD_ROLLINGPIN  ] =  1,
            [GD_BOW         ] =  2,
            [GD_BOAT        ] =  2,
            [GD_SWORD       ] =  3,
            [GD_IRON        ] = 15,
            [GD_FLOUR       ] =  8,
            [GD_FISH        ] =  4,
            [GD_BREAD       ] =  6,
            [GD_SHIELD      ] =  2,
            [GD_WOOD        ] = 25,
            [GD_BOARDS      ] = 60,
            [GD_STONES      ] = 46,
            [GD_GRAIN       ] = 10,
            [GD_COINS       ] =  4,
            [GD_GOLD        ] = 24,
            [GD_IRONORE     ] = 33,
            [GD_COAL        ] = 40,
            [GD_MEAT        ] =  3,
            [GD_HAM         ] =  6
        })

        -- people
        rttr:GetPlayer(p):AddPeople({
            [JOB_HELPER             ] = 100,
            [JOB_WOODCUTTER         ] =   4,
            [JOB_FISHER             ] =   2,
            [JOB_FORESTER           ] =   2,
            [JOB_CARPENTER          ] =   2,
            [JOB_STONEMASON         ] =   4,
            [JOB_HUNTER             ] =   2,
            [JOB_FARMER             ] =   2,
            [JOB_MILLER             ] =   1,
            [JOB_BAKER              ] =   1,
            [JOB_BUTCHER            ] =   1,
            [JOB_MINER              ] =   6,
            [JOB_BREWER             ] =   3,
            [JOB_PIGBREEDER         ] =   1,
            [JOB_DONKEYBREEDER      ] =   2,
            [JOB_IRONFOUNDER        ] =   2,
            [JOB_MINTER             ] =   2,
            [JOB_METALWORKER        ] =   1,
            [JOB_ARMORER            ] =   4,
            [JOB_BUILDER            ] =   8,
            [JOB_PLANER             ] =   2,
            [JOB_GEOLOGIST          ] =   8,
            [JOB_PRIVATE            ] =  16,
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
            [GD_BEER        ] =  8,
            [GD_TONGS       ] =  0,
            [GD_HAMMER      ] = 13,
            [GD_AXE         ] =  6,
            [GD_SAW         ] =  2,
            [GD_PICKAXE     ] =  5,
            [GD_SHOVEL      ] =  3,
            [GD_CRUCIBLE    ] =  4,
            [GD_RODANDLINE  ] =  5,
            [GD_SCYTHE      ] =  2,
            [GD_WATER       ] = 25,
            [GD_CLEAVER     ] =  3,
            [GD_ROLLINGPIN  ] =  1,
            [GD_BOW         ] =  2,
            [GD_BOAT        ] =  2,
            [GD_SWORD       ] =  3,
            [GD_IRON        ] = 15,
            [GD_FLOUR       ] =  8,
            [GD_FISH        ] =  4,
            [GD_BREAD       ] =  6,
            [GD_SHIELD      ] =  2,
            [GD_WOOD        ] = 25,
            [GD_BOARDS      ] = 60,
            [GD_STONES      ] = 46,
            [GD_GRAIN       ] = 10,
            [GD_COINS       ] =  4,
            [GD_GOLD        ] = 30,
            [GD_IRONORE     ] = 13,
            [GD_COAL        ] = 40,
            [GD_MEAT        ] =  3,
            [GD_HAM         ] =  6
        })

        -- people
        rttr:GetPlayer(p):AddPeople({
            [JOB_HELPER             ] = 50,
            [JOB_WOODCUTTER         ] = 10,
            [JOB_FISHER             ] = 10,
            [JOB_FORESTER           ] =  5,
            [JOB_CARPENTER          ] =  5,
            [JOB_STONEMASON         ] = 10,
            [JOB_HUNTER             ] = 10,
            [JOB_FARMER             ] = 10,
            [JOB_MILLER             ] = 10,
            [JOB_BAKER              ] =  5,
            [JOB_BUTCHER            ] =  5,
            [JOB_MINER              ] = 20,
            [JOB_BREWER             ] =  5,
            [JOB_PIGBREEDER         ] = 10,
            [JOB_DONKEYBREEDER      ] =  5,
            [JOB_IRONFOUNDER        ] = 10,
            [JOB_MINTER             ] = 10,
            [JOB_METALWORKER        ] =  5,
            [JOB_ARMORER            ] = 10,
            [JOB_BUILDER            ] = 15,
            [JOB_PLANER             ] = 15,
            [JOB_GEOLOGIST          ] =  8,
            [JOB_PRIVATE            ] = 36,
            [JOB_PRIVATEFIRSTCLASS  ] = 16,
            [JOB_SERGEANT           ] =  4,
            [JOB_OFFICER            ] =  2,
            [JOB_GENERAL            ] =  1,
            [JOB_SCOUT              ] =  7,
            [JOB_SHIPWRIGHT         ] =  1,
            [JOB_PACKDONKEY         ] = 15,
            [JOB_CHARBURNER         ] =  0
        })

    elseif(p == 2) then
        -- goods
        rttr:GetPlayer(p):AddWares({
            [GD_BEER        ] =  8,
            [GD_TONGS       ] =  0,
            [GD_HAMMER      ] = 13,
            [GD_AXE         ] =  6,
            [GD_SAW         ] =  2,
            [GD_PICKAXE     ] =  5,
            [GD_SHOVEL      ] =  3,
            [GD_CRUCIBLE    ] =  4,
            [GD_RODANDLINE  ] =  5,
            [GD_SCYTHE      ] =  2,
            [GD_WATER       ] = 25,
            [GD_CLEAVER     ] =  3,
            [GD_ROLLINGPIN  ] =  1,
            [GD_BOW         ] =  2,
            [GD_BOAT        ] =  2,
            [GD_SWORD       ] =  3,
            [GD_IRON        ] = 15,
            [GD_FLOUR       ] =  8,
            [GD_FISH        ] =  4,
            [GD_BREAD       ] =  6,
            [GD_SHIELD      ] =  2,
            [GD_WOOD        ] = 25,
            [GD_BOARDS      ] = 60,
            [GD_STONES      ] = 46,
            [GD_GRAIN       ] = 10,
            [GD_COINS       ] =  8,
            [GD_GOLD        ] = 13,
            [GD_IRONORE     ] = 33,
            [GD_COAL        ] = 40,
            [GD_MEAT        ] =  3,
            [GD_HAM         ] =  6
        })

        -- people
        rttr:GetPlayer(p):AddPeople({
            [JOB_HELPER             ] = 50,
            [JOB_WOODCUTTER         ] = 10,
            [JOB_FISHER             ] = 10,
            [JOB_FORESTER           ] =  5,
            [JOB_CARPENTER          ] =  5,
            [JOB_STONEMASON         ] = 10,
            [JOB_HUNTER             ] = 10,
            [JOB_FARMER             ] = 10,
            [JOB_MILLER             ] = 10,
            [JOB_BAKER              ] =  5,
            [JOB_BUTCHER            ] =  5,
            [JOB_MINER              ] = 20,
            [JOB_BREWER             ] =  5,
            [JOB_PIGBREEDER         ] = 10,
            [JOB_DONKEYBREEDER      ] =  5,
            [JOB_IRONFOUNDER        ] = 10,
            [JOB_MINTER             ] = 10,
            [JOB_METALWORKER        ] =  5,
            [JOB_ARMORER            ] = 10,
            [JOB_BUILDER            ] = 15,
            [JOB_PLANER             ] = 15,
            [JOB_GEOLOGIST          ] =  8,
            [JOB_PRIVATE            ] = 26,
            [JOB_PRIVATEFIRSTCLASS  ] = 16,
            [JOB_SERGEANT           ] = 12,
            [JOB_OFFICER            ] =  8,
            [JOB_GENERAL            ] =  1,
            [JOB_SCOUT              ] =  7,
            [JOB_SHIPWRIGHT         ] =  1,
            [JOB_PACKDONKEY         ] = 15,
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

    if(     (x == 10) and (y == 37) ) then MissionEvent(2)
    elseif( (x == 97) and (y == 68) ) then MissionEvent(99)
    end

    if(not rttr:GetPlayer(1):IsInRestrictedArea(x, y)) then 
        MissionEvent(98) -- for lifting restrictions
    end
end

function onExplored(p, x, y)
    if(p ~= 0) then
        return
    end

    -- onExplored events
    if(     (x == 36) and (y == 47) ) then MissionEvent(3)
    end
end

-- execute mission events, e == 1 is initial event, e == 99 is final event
function MissionEvent(e, onLoad)
    -- event e is inactive
    if(eState[e] <= 0) then
        return
    end

    -- call side effects for active events, check "eState[e] == 1" for multiple call events!
    if(e == 2) then
        rttr:GetPlayer(0):EnableBuilding(BLD_HARBORBUILDING, not onLoad)
        rttr:GetPlayer(0):EnableBuilding(BLD_SHIPYARD, not onLoad)

    elseif(e == 98) then
        rttr:GetPlayer(1):SetRestrictedArea()
        rttr:GetPlayer(2):SetRestrictedArea()

    elseif(e == 99) then
        -- TODO: EnableNextMissions()
        -- Show opened arc
        rttr:GetWorld():AddStaticObject(97, 68, 561, 0xFFFF, 2)
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
