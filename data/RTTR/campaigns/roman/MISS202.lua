------------------------------------------------------------------------------
-- LUA-Script for MISS202.WLD (mission 3 of the original "Roman Campaign"   --
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

local requiredFeature = 5
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

        msg1    = '17. den třetího měsíce 3. roku.\n\nBudování životaschopné kolonie se stalo rutinou. Všechno by teď mělo šlapat jako hodinky.\n\nNarazili jsme na dalšího trosečníka. Varoval nás před nepřátelskými núbijskými kmeny na východě ostrova. Vyprávěl také o východních horách ve kterých by se údajně měla nacházet veliká naleziště zlata. Ptali jsme se ho na bránu, o té nic neví, zná jen malou část ostrovsa.',
        msgh1   = 'Přesuň se na východ.',

        msg2    = '23. den 12. měsíce 3. roku.\n\nPřišli jsme do kontaktu s Núbijci. Jejich postoj je výhružný, ale ne agresivní. To ale pravděpodobně nevydrží dlouho. Měli bychom postavit rozhlednu, abychom mohli lépe sledovat jejich území.\n\nZprávy o bohatých nalezištích zlata se zdají být pravdivé. Núbijský náčelník se se svími zlatými šperky natřásá jako páv.',
        msgh2   = 'Hledej bránu.',

        msg3    = 'První den druhého měsíce 4. roku po stroskotání.\n\nNarazli jsme na další Núbijský kmen. Ptali jsme se jich na bránu a mluví o ní jako o svaté relikvii která je na posvátem místě kam mohou jen jejich šamani. Nikdy nás k ní nepustí. Nemáme jinou možnost, než boj.',
        msgh3   = 'Získejte přístup k bráně na severu.',

        msg99   = 'Dostali jsme se k bráně a aktivovali ji. Nevíme, kam nás brána zavede, ale projdeme srze ní a uvydíme.',
        msgh99  = 'Dokončil jsi tuto misi. Další kapitola na tebe čeká ...'
    },
    de =
    {
        Diary   = 'Tagebuch',

        msg1    = '17.Tag im dritten Monat des 3.Jahres.\n\nInzwischen haben wir eine gewisse Routine im Aufbau\neiner lebensfähigen Siedlung entwickelt. Es sollte\neigentlich alles wie am Schnürchen klappen.\n\nWir sind wieder auf einen Schiffbrüchigen getroffen.\nEr warnt uns vor aggressiven nubischen Stämmen im\nOsten der Insel. Dort gibt es ein Gebirge, in dem\nman angeblich große Mengen Gold finden kann. Von\neinem Tor wußte der Mann allerdings nichts. Er kennt\naber auch nur einen kleinen Teil der Insel.',
        msgh1   = 'Rücken Sie weiter nach Osten vor.',

        msg2    = '23.Tag im 12.Monat des 3.Jahres.\n\nWir sind auf die Nubier getroffen. Sie verhalten\nsich sehr drohend, aber noch nicht offen aggressiv.\nDas wird wahrscheinlich nicht lange so bleiben. Wir\nsollten einen Spähturm bauen, um ihr Land besser\nüberwachen zu können.\n\nDie Berichte von den reichen Goldvorkommen scheinen\nzu stimmen. Ihr Häuptling stolziert in seinem\nGoldschmuck einher wie ein Pfau.',
        msgh2   = 'Suchen Sie nach dem Tor.',

        msg3    = 'Erster Tag des zweiten Monats \nim 4.Jahr der Landung. \n\nDer Stamm im Norden versperrt uns den Weg zu einem\nweiteren Tor. Auch er betrachtet dieses als sein\nHeiligtum. Uns bleibt wieder nichts anderes übrig,\nals zu kämpfen.',
        msgh3   = 'Suchen Sie nach dem Tor im Norden.',

        msg99   = 'Wir haben das Tor erreicht und aktiviert. Wir wissen\nnicht, wohin uns der nächste Schritt führt, aber wir\nwerden ihn gehen.',
        msgh99  = 'Sie haben diese Mission erfüllt. Das nächste Kapitel\nwartet schon auf Sie...'
    },
    en =
    { 
        Diary   = 'Diary',

        msg1    = '17th Day of the Third Month of the 3rd Year.\n\nWe have now created something of a routine in\nestablishing a viable settlement. Everything should\nnow work like clockwork.\n\nWe came across another castaway. He warned us against\nhostile Nubian tribes in the east of the island. The\nmountains in the east also allegedly have large\nquantities of gold. The man knew nothing about a\ngateway but he was only familiar with a small part\nof the island.',
        msgh1   = 'Move eastwards.',

        msg2    = '23rd Day of the 12th Month of the 3rd Year.\n\nWe came into contact with the Nubians. Their behavior\nis very threatening but not openly aggressive. Their\nbehavior will probably change before long. We should\nbuild a lookout tower so that we can keep a closer eye\non their lands.\n\nThe reports of rich gold deposits seem to be true.\nThe Nubian chieftain struts around in his gold\njewelry like a peacock.',
        msgh2   = 'Search for the gateway.',

        msg3    = 'First Day of the Second Month\nof the 4th Year of Coming Ashore.\n\nThe tribe in the north is blocking our path to\nanother gateway. They also regard it as their holy\nrelic. We have no alternative but to fight.',
        msgh3   = 'Obtain access to the gateway in the north. ',

        msg99   = 'We have reached the gateway and activated it. We\nknow not where our next step will take us, but we\nshall go on.',
        msgh99  = 'You have completed this mission.\nThe next Chapter awaits you... '
    },
    pl =
    { 
        Diary   = 'Dziennik',

        msg1    = 'Siedemnasty Dzień Trzeciego Miesiąca Trzeciego Roku.\n\nPopadliśmy już w coś na kształt rutyny w przygotowywaniu nowej osady. \n\nMam nadzieję, że tym razem wszystko pójdzie jak należy. \n\nNatknęliśmy się na kolejnego rozbitka. Ostrzegł nas przed wrogimi plemionami Nubijczyków na wschodzie wyspy. \n\nW górach na wschodzie rzekomo znajdują się również bogate złoża złota. Jest tylko jeden sposób, aby sprawdzić, ile warte są te plotki.\n\nCzłowiek ten nie wiedział nic o bramie, gdyż znał jedynie niewielką część wyspy.',
        msgh1   = 'Przesuwaj się na wschód.',

        msg2    = 'Dwudziesty Trzeci Dzień Dwunastego Miesiąca Trzeciego Roku.\n\nNawiązaliśmy kontakt z Nubijczykami. \n\nIch postawa jest wroga, ale nie otwarcie agresywne. \n\nGotowym iść o zakład, że ich zachowanie wkrótce zmieni się. Na gorsze. \n\nPowinniśmy zbudować wieżę obserwacyjną, aby móc lepiej kontrolować ich poczynania. \n\nPogłoski o bogatych złożach złota wydają się być prawdziwe.\n\nWódz Nubijczyków paraduje w swojej złotej biżuterii niczym paw.',
        msgh2   = 'Szukaj bramy.',

        msg3    = 'Pierwszy Dzień Drugiego Miesiąca Czwartego Roku od przybycia na brzeg.\n\nPlemię na północy blokuje nam drogę do kolejnej bramy. \n\nOni także pojmują ją jako swe święte miejsce. \n\nNie mamy wyboru. Będziemy walczyć.',
        msgh3   = 'Zdobądź dostęp do bramy na północy.',

        msg99   = 'Dotarliśmy do bramy i ją aktywowaliśmy. \n\nNie wiemy, dokąd dotrzemy tym razem, lecz nie zamierzamy się zatrzymać.',
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
    rttr:Log("-----------------------\n MISS202.lua loaded... \n-----------------------\n")
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

    rttr:GetWorld():SetComputerBarrier(6, 70, 71)
    rttr:GetWorld():SetComputerBarrier(6, 59, 60)

    if isFirstStart then
        -- type 8 == 7 in rttr
        rttr:GetWorld():AddAnimal( 70,  72, SPEC_DUCK)
        rttr:GetWorld():AddAnimal( 71,  71, SPEC_DUCK)
        rttr:GetWorld():AddAnimal( 37, 100, SPEC_DUCK)
        rttr:GetWorld():AddAnimal( 37, 104, SPEC_DUCK)
        rttr:GetWorld():AddAnimal( 35, 104, SPEC_DUCK)
        rttr:GetWorld():AddAnimal( 26,  86, SPEC_RABBITWHITE)
        rttr:GetWorld():AddAnimal( 26,  87, SPEC_RABBITGREY)
        rttr:GetWorld():AddAnimal( 27,  88, SPEC_FOX)
        rttr:GetWorld():AddAnimal( 30,  88, SPEC_STAG)
        rttr:GetWorld():AddAnimal( 31,  86, SPEC_DEER)
        rttr:GetWorld():AddAnimal( 26, 101, SPEC_SHEEP)
        rttr:GetWorld():AddAnimal( 26, 102, SPEC_SHEEP)
        rttr:GetWorld():AddAnimal( 31, 104, SPEC_RABBITWHITE)
        rttr:GetWorld():AddAnimal( 32, 106, SPEC_RABBITGREY)
        rttr:GetWorld():AddAnimal( 30, 102, SPEC_FOX)
        rttr:GetWorld():AddAnimal( 30, 106, SPEC_STAG)
        rttr:GetWorld():AddAnimal( 35, 108, SPEC_SHEEP)
        rttr:GetWorld():AddAnimal( 44,  78, SPEC_RABBITWHITE)
        rttr:GetWorld():AddAnimal( 45,  76, SPEC_RABBITGREY)
        rttr:GetWorld():AddAnimal( 42,  77, SPEC_RABBITWHITE)
        rttr:GetWorld():AddAnimal( 46,  77, SPEC_FOX)
        rttr:GetWorld():AddAnimal( 45,  76, SPEC_DEER)
        rttr:GetWorld():AddAnimal( 87,  26, SPEC_DUCK)
        rttr:GetWorld():AddAnimal( 86,  26, SPEC_DUCK)
        rttr:GetWorld():AddAnimal(109,  53, SPEC_DUCK)
        rttr:GetWorld():AddAnimal( 54,  70, SPEC_SHEEP)
        rttr:GetWorld():AddAnimal( 49,  75, SPEC_SHEEP)
        rttr:GetWorld():AddAnimal( 79,  39, SPEC_SHEEP)
        rttr:GetWorld():AddAnimal( 58,  28, SPEC_SHEEP)
        rttr:GetWorld():AddAnimal( 29,  94, SPEC_SHEEP)
        rttr:GetWorld():AddAnimal( 29,  94, SPEC_DUCK)
        rttr:GetWorld():AddAnimal( 29,  94, SPEC_DEER)
        rttr:GetWorld():AddAnimal( 29,  94, SPEC_DEER)
        rttr:GetWorld():AddAnimal( 26,  93, SPEC_STAG)
        rttr:GetWorld():AddAnimal(109,  94, SPEC_DUCK)
        rttr:GetWorld():AddAnimal(109,  93, SPEC_DUCK)
        rttr:GetWorld():AddAnimal(103,  89, SPEC_RABBITWHITE)
        rttr:GetWorld():AddAnimal(104,  87, SPEC_RABBITGREY)
        rttr:GetWorld():AddAnimal( 96, 106, SPEC_RABBITGREY)
        rttr:GetWorld():AddAnimal( 97, 103, SPEC_RABBITWHITE)
        rttr:GetWorld():AddAnimal( 98, 105, SPEC_RABBITWHITE)
        rttr:GetWorld():AddAnimal(100, 105, SPEC_FOX)
        rttr:GetWorld():AddAnimal(100, 100, SPEC_SHEEP)
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

    if(p == 0) then
        rttr:GetPlayer(p):DisableBuilding(BLD_LOOKOUTTOWER, false)
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
            [GD_TONGS       ] =  4,
            [GD_HAMMER      ] =  3,
            [GD_AXE         ] =  2,
            [GD_SAW         ] =  5,
            [GD_PICKAXE     ] =  4,
            [GD_SHOVEL      ] =  4,
            [GD_CRUCIBLE    ] =  5,
            [GD_RODANDLINE  ] =  2,
            [GD_SCYTHE      ] =  3,
            [GD_WATER       ] = 20,
            [GD_CLEAVER     ] =  1,
            [GD_ROLLINGPIN  ] =  2,
            [GD_BOW         ] =  3,
            [GD_BOAT        ] = 15,
            [GD_SWORD       ] =  3,
            [GD_IRON        ] = 10,
            [GD_FLOUR       ] =  8,
            [GD_FISH        ] =  4,
            [GD_BREAD       ] =  6,
            [GD_SHIELD      ] =  4,
            [GD_WOOD        ] = 20,
            [GD_BOARDS      ] = 50,
            [GD_STONES      ] = 46,
            [GD_GRAIN       ] = 10,
            [GD_COINS       ] =  4,
            [GD_GOLD        ] =  8,
            [GD_IRONORE     ] = 15,
            [GD_COAL        ] = 30,
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
            [JOB_DONKEYBREEDER      ] =   1,
            [JOB_IRONFOUNDER        ] =   2,
            [JOB_MINTER             ] =   2,
            [JOB_METALWORKER        ] =   1,
            [JOB_ARMORER            ] =   4,
            [JOB_BUILDER            ] =   8,
            [JOB_PLANER             ] =   2,
            [JOB_GEOLOGIST          ] =   3,
            [JOB_PRIVATE            ] =  16,
            [JOB_PRIVATEFIRSTCLASS  ] =   6,
            [JOB_SERGEANT           ] =   4,
            [JOB_OFFICER            ] =   2,
            [JOB_GENERAL            ] =   1,
            [JOB_SCOUT              ] =   5,
            [JOB_SHIPWRIGHT         ] =   1,
            [JOB_PACKDONKEY         ] =   0,
            [JOB_CHARBURNER         ] =   0
        })

    elseif(p == 1) then
        -- goods
        rttr:GetPlayer(p):AddWares({
            [GD_BEER        ] = 10,
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
            [GD_FISH        ] = 10,
            [GD_BREAD       ] = 10,
            [GD_SHIELD      ] =  0,
            [GD_WOOD        ] =  0,
            [GD_BOARDS      ] = 80,
            [GD_STONES      ] = 80,
            [GD_GRAIN       ] =  0,
            [GD_COINS       ] =  0,
            [GD_GOLD        ] =  0,
            [GD_IRONORE     ] =  0,
            [GD_COAL        ] = 20,
            [GD_MEAT        ] = 10,
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
            [JOB_DONKEYBREEDER      ] =  5,
            [JOB_IRONFOUNDER        ] = 10,
            [JOB_MINTER             ] = 10,
            [JOB_METALWORKER        ] =  5,
            [JOB_ARMORER            ] = 10,
            [JOB_BUILDER            ] = 20,
            [JOB_PLANER             ] = 20,
            [JOB_GEOLOGIST          ] =  1,
            [JOB_PRIVATE            ] = 20,
            [JOB_PRIVATEFIRSTCLASS  ] =  8,
            [JOB_SERGEANT           ] =  4,
            [JOB_OFFICER            ] =  1,
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
            [GD_BOARDS      ] = 80,
            [GD_STONES      ] = 80,
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
            [JOB_DONKEYBREEDER      ] =  5,
            [JOB_IRONFOUNDER        ] = 10,
            [JOB_MINTER             ] = 10,
            [JOB_METALWORKER        ] =  5,
            [JOB_ARMORER            ] = 10,
            [JOB_BUILDER            ] = 20,
            [JOB_PLANER             ] = 20,
            [JOB_GEOLOGIST          ] =  1,
            [JOB_PRIVATE            ] = 20,
            [JOB_PRIVATEFIRSTCLASS  ] = 10,
            [JOB_SERGEANT           ] =  6,
            [JOB_OFFICER            ] =  1,
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

    if( (x == 89) and (y == 20) ) then MissionEvent(99)
    end
end

function onExplored(p, x, y, o)
    -- onContact events
    if(     ((p == 0) and (o == 2)) or ((p == 2) and (o == 0)) ) then MissionEvent(2)
    elseif( ((p == 0) and (o == 1)) or ((p == 1) and (o == 0)) ) then MissionEvent(3)
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
        rttr:GetPlayer(0):EnableBuilding(BLD_LOOKOUTTOWER, not onLoad)

    elseif(e == 99) then
        -- TODO: EnableNextMissions()
        -- Show opened arc
        rttr:GetWorld():AddStaticObject(89, 20, 561, 0xFFFF, 2)
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
