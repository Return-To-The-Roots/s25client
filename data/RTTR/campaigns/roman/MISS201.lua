------------------------------------------------------------------------------
-- LUA-Script for MISS201.WLD (mission 2 of the original "Roman Campaign"   --
--                                                                          --
-- Authors: CrazyL, Spikeone, ArthurMurray47, kubaau                        --
------------------------------------------------------------------------------


-------------------------------- TODO -----------------------------------------
-- EnableNextMissions()
-- Set Portraits
-- RttR: AI doesn't go south
-------------------------------------------------------------------------------


------------------------------- Lua Version used -----------------------------
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
eIdx = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 98, 99}

rttr:RegisterTranslations(
{
    cs =
    {
        Diary   = 'Deník',

        msg1    = 'Třetí den druhého roku.\n\n\nDostali jsme se na druhý ostrov, zdá se, že je neobydlený. Zásoby nástrojů ze staré dobré lodi Tortius pomalu dochází. Zoufale potřebujeme začít vyrábět nové nástroje.',
        msgh1   = 'Postav nářaďovnu a rozšiř naše území do vnitrozemí. Věnuj zvláštní pozornost dodávkám stavebního materiálu a zásob.',

        msg2    = 'Postoupili jsme zase o kousek na sever a narazili jsme na krajana. Byl jediný, kdo přežil další ztroskotání lodi, a je velmi šťastný, že znovu vidí lidskou tvář. Také nás to potěšilo. Máme veliké štěstí, ten muž je sládek. Pokud se někdy dostaneme do vojenského konfliktu, pivo se nám bude hodit pro verbování nových vojáků. Voják potřebuje mnohem víc než jen štít a meč!\n\nMuž nám také vypráví o ruinách, které objevil na ostrově. Obávám se, že to nebudeme dlouho trvat a budeme se muset bránit.',
        msgh2   = 'Postupuj do vnitrozemí ostrova. Věnuj zvláštní pozornost dodávkám stavebního materiálu a zásob.',

        msg3    = 'Objevili jsme ruiny stavby. Obsahovaly plány na stavbu strážnice. Nyní bychom měli zvážit stavbu opevnění. Jsem více než kdy jindy přesvědčen, že na tomto ostrově nejsme sami.',
        msgh3   = 'Postupuj do vnitrozemí ostrova. Věnuj zvláštní pozornost dodávkám stavebního materiálu a zásob.',

        msg4    = 'Našli jsme ruiny. Obsahovaly stavební plány pro stavbu strážní věže. Jaký je původ těchto staveb? Kdo je postavil? Obávám se, že mír brzy skončí.',
        msgh4   = 'Postupuj do vnitrozemí ostrova. Věnuj zvláštní pozornost dodávkám stavebního materiálu a zásob.',

        msg5    = 'Kdokoli byl, nebo stále je na tomto ostrově, ví hodně o stavitelství. Našli jsme ruiny mohutné pevnosti. Naši architekti již vypracovali stavební plány.',
        msgh5   = 'Postupuj do vnitrozemí ostrova. Věnuj zvláštní pozornost dodávkám stavebního materiálu a zásob.',

        msg6    = '26. den třetího měsíce druhého roku.\n\n\nMé obavy se naplnili. Moji muži našli hadrovou panenku a dva hroty kopí typické pro Núbijské domorodce. Vypadá to, že tyto artefakty nejsou příliš staré. Jak je ale možné, že rujny jsou římského stylu?',

        msg7    = 'Naše území se rozšiřuje a dopravní trasy se prodlužují. Musíme postavit skladiště, abychom mohli rychleji reagovat na potřeby našich lidí.',

        msg8    = 'Deník Oktaviána.\n\nČtvrtý den čtvrtého měsíce druhého roku.\n\nTo se dalo čekat: narazili jsme na Núbijce. Zpočátku bylo všechno přátelské. Vyprávěli nám o svaté relikivi a jak se později ukázalo jedná se o jednu z bran. K bráně nás ale nechtějí pustit. Přístup si budeme muset vybojovat.',
        msgh8   = 'Zaútoč na Núbijce a získej přístup k bráně.',

        msg9    = 'Núbijci jsou mizerní bojovníci, ale mají katapult, který musíme zničit. Doufejme, že naši architekti z trosek budou schopni zjistit, jak takovou zbraň postavit.',
        msgh9   = 'Zajmi katapult a poté získáte přístup k bráně.',

        msg99   = '27. den devátého měsíce.\n\n\nVítěztví v bitvě je naše a ostrov jsme osídlili. Brána se znovu aktivovala. Jsem nadšený z vyhlídky, na jakou cestu nás zavede.Teď už o tom nemůže být pochyb, svědší o tom i latinské nápisi na bráně a objevené rujny které jsou římské architektury. Před námi tu musely být Římané. Kde ale jsou, jak se sem dostali, proč odešli, je tu mnoho otázek. Dostaneme na ně někdy odpovědí? Doufám, že vystopujeme kroky našich legendárních předků.',
        msgh99  = 'Dokončil jsi tuto misi. Třetí kapitola čeká na tebe ...',
    },
    de =
    {
        Diary   = 'Tagebuch',

        msg1    = 'Dritter Tag des zweiten Jahres.\n\n\nWir haben offensichtlich eine zweite Insel erreicht.\nAuch diese scheint unbewohnt zu sein. So langsam\ngehen aber die Werkzeugvorräte der guten alten\nTortius zu Ende. Wir brauchen also eine Schlosserei.',
        msgh1   = 'Errichten Sie eine Schlosserei und dringen Sie ins\nInnere der Insel vor. Denken Sie an die Versorgung\nmit Nahrungsmitteln und Rohstoffen.',

        msg2    = 'Wir stießen ein wenig nach Norden vor und dabei auf\neinen Landsmann. Er ist der einzige Überlebende\neines Schiffsunglücks und natürlich sehr froh,\nwieder Menschen zu begegnen. Aber auch wir hatten\nGlück dabei: Der Mann ist Brauer. Falls es doch\neinmal zu kriegerischen Auseinandersetzungen kommen\nsollte, wird das Bier den Mut unserer Leute schon\nheben. Zu einem Soldaten gehört halt mehr als ein\nSchild und ein Schwert!\n\nDer Mann berichtete uns auch von Ruinen, die er auf\nder Insel entdeckt hat. Wir müssen, fürchte ich,\ndemnächst vorsichtiger sein.',
        msgh2   = 'Dringen Sie ins Innere der Insel vor. Denken Sie an\ndie Versorgung mit Nahrungsmitteln und Rohstoffen.',

        msg3    = 'Wir haben eine Ruine entdeckt. Darin befanden sich\nBaupläne für eine Wachhütte. Jetzt können und\nsollten wir möglichst ebenfalls Befestigungen bauen.\nIch habe immer mehr das Gefühl, daß wir nicht allein\nauf dieser Insel sind.',
        msgh3   = 'Dringen Sie ins Innere der Insel vor. Denken Sie an\ndie Versorgung mit Nahrungsmitteln und Rohstoffen.',

        msg4    = 'In der Ruine eines Wachturms fanden wir die\ndazugehörigen Baupläne. Wo kommen diese Bauwerke\nher? Wer errichtete sie? Ich fürchte, mit dem\nFrieden ist es bald vorbei.',
        msgh4   = 'Dringen Sie ins Innere der Insel vor. Denken Sie an\ndie Versorgung mit Nahrungsmitteln und Rohstoffen.',

        msg5    = 'Wer auch immer sich auf dieser Insel befindet oder\nbefand, er versteht das Bauhandwerk. Wir fanden die\nRuine einer gewaltigen Festung, deren Konstruktion\nhöchst nachahmenswert erscheint. Unsere Gelehrten\nhaben bereits einen Bauplan entworfen.',
        msgh5   = 'Dringen Sie ins Innere der Insel vor. Denken Sie an\ndie Versorgung mit Nahrungsmitteln und Rohstoffen.',

        msg6    = '26. Tag des 3. Monats.\n\n\nMeine Befürchtungen haben weitere Nahrung erhalten.\nMeine Leute fanden eine Stoffpuppe und zwei\nSpeerspitzen, wie sie bei nubischen Völkern üblich\nsind. Und diese Artefakte sehen nicht sehr alt aus.',

        msg7    = 'Unser Reich wächst und die Transportwege werden\nimmer länger. Wir sollten ein Lagerhaus bauen, um\nschneller reagieren zu können.',

        msg8    = 'Tagebuch des Octavius.\n\nVierter Tag im vierten Monat des zweiten Jahres.\n\nEs war zu erwarten: Wir sind auf ein nubisches Volk\ngetroffen. Zunächst verlief alles sehr friedlich.\nSie erzählten uns sogar von ihrem Heiligtum, und\nschnell wurde klar, daß es sich dabei um eines\ndieser Tore handelt. Aber sie weigern sich, uns\nZutritt zu ihrem Heiligtum zu gewähren. Wir werden\ndarum kämpfen müssen.',
        msgh8   = 'Greifen Sie die Nubier an und suchen sie nach deren\nTor.',

        msg9    = 'Die Nubier sind lausige Kämpfer, aber sie haben\ndieses Katapult. Wir müssen es zerstören. Vielleicht\nkommen wir dann auch an die Pläne, wie man so etwas\nbaut.',
        msgh9   = 'Erobern Sie das Katapult und suchen Sie dann weiter\nnach dem Tor.',

        msg99   = '27. Tag im neunten Monat.\n\n\nWir haben den Kampf gewonnen und die Insel ist\nunser. Und wieder ist das Tor aktiv geworden. Ich\nbin gespannt, wohin die Reise jetzt geht. Aber die\nInschrift auf diesem Tor ist wieder latinisch, und\nso hoffe ich, daß wir vielleicht auf jenem Pfad\nwandern, den unsere sagenhaften Vorfahren gingen,\nbis sie nach Rom kamen...',
        msgh99  = 'Sie haben diese Mission erfüllt. Das dritte Kapitel\nwartet schon auf Sie...'
    },
    en =
    {
        Diary   = 'Diary',

        msg1    = 'Third Day of the Second Year.\n\n\nWe have reached a second island which appears to be\nuninhabited. The stock of tools belonging to the\ngood old Tortius is slowly being used up. We\ndesperately need a metalworks to make new tools.',
        msgh1   = 'Build a metalworks and move into the interior of the\nisland. Pay close attention to the supply of\nprovisions and raw materials.',

        msg2    = 'We advanced a short distance northward and came\nacross a fellow countryman. He was the only survivor\nof another shipwreck and he is very happy to see\nother humans again. We were also pleased; the man is\na brewer. If we ever get into warlike disputes, beer\nwill lift the courage of our people. A soldier needs\nmuch more than just a shield and a sword!\n\nThe man also tells us of ruins that he has\ndiscovered on the island. I fear that before long we\nwill need to be more careful.',
        msgh2   = 'Advance into the interior of the island. Pay close\nattention to the supply of provisions and raw\nmaterials.',

        msg3    = 'We have found the ruins. They contained building\nplans for a guardhouse. Now we should consider\nbuilding fortifications. I am more convinced than\never that we are not alone on this island.',
        msgh3   = 'Advance into the interior of the island. Pay close\nattention to the supply of provisions and raw\nmaterials.',

        msg4    = 'We have found the ruins. They contained building\nplans for a watchtower. What is the origin of these\nstructures? Who built them? I fear that peace will\nsoon be at an end.',
        msgh4   = 'Advance into the interior of the island. Pay close\nattention to the supply of provisions and raw\nmaterials.',

        msg5    = 'Whoever is or was on this island knows all about the\nbuilding trade. We found the ruins of a mighty\nfortress of exemplary construction. Our architects\nhave already drawn up a building plan.',
        msgh5   = 'Advance into the interior of the island. Pay close\nattention to the supply of provisions and raw\nmaterials.',

        msg6    = '26th Day of the Third Month of the Second Year.\n\n\nMy fears have been given further confirmation. My\nmen found a rag doll and two spear tips of the type\ncustomarily associated with the Nubian tribes. These\nartifacts do not appear to be very old.',

        msg7    = 'Our territory is expanding and transportation routes\nare becoming longer and longer. We must build a\nstorehouse so that we can respond more quickly to\nthe needs of our people.',

        msg8    = 'Diary of Octavius.\n\nFourth Day of the Fourth Month of the Second Year.\n\nIt was only to be expected: we came across the\nNubians. At first everything was very friendly. They\neven told us about their holy relic and it quickly\nbecame obvious that it was one of the gateways. They\ndenied us access to their holy relic, we shall have\nto fight to obtain access.',
        msgh8   = 'Attack the Nubians and gain access to their gateway.',

        msg9    = 'The Nubians are lousy fighters but they have a\ncatapult, which we must destroy. Perhaps then we\nshall be able to capture plans showing how to build\nsuch a weapon.',
        msgh9   = 'Capture the catapult and then gain access to the\ngateway.',

        msg99   = '27th Day of the Ninth Month.\n\n\nWe were victorious in battle and the island is ours.\nThe gateway has become active again. I am excited by\nthe prospect of where our journey will take us now.\nThe inscription on the gateway is in Latin. I hope\nthat we will retrace the steps of our\nlegendary predecessors when they journeyed to Rome...',
        msgh99  = 'You have completed this mission. The third Chapter\nawaits you...',
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
    rttr:Log("-----------------------\n MISS201.lua loaded... \n-----------------------\n")
    rttr:ResetAddons()
    rttr:SetAddon(ADDON_FRONTIER_DISTANCE_REACHABLE, true)
    rttr:SetGameSettings({
        ["fow"] = EXP_CLASSIC,
        ["teamView"] = false,
        ["lockedTeams"] = false
    })

    rttr:GetPlayer(0):SetNation(NAT_ROMANS)     -- nation
    rttr:GetPlayer(0):SetColor(0)               -- blue

    rttr:GetPlayer(1):SetAI(3)                  -- hard AI
    rttr:GetPlayer(1):SetNation(NAT_AFRICANS)   -- nation
    rttr:GetPlayer(1):SetColor(1)               -- yellow
    rttr:GetPlayer(1):SetName('Shaka')          -- Enemy Name
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
        rttr:GetPlayer(1):MakeOneSidedAllianceTo(0) -- !GLOBAL_SET_COMPUTER_ALLIANCE  1 0
    end

    for i = 0, 1 do                          -- set resources
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
        rttr:GetWorld():AddAnimal(43, 24, SPEC_DUCK)

        rttr:GetWorld():AddAnimal(16, 21, SPEC_DUCK)

        rttr:GetWorld():AddAnimal(33, 50, SPEC_DUCK)
        rttr:GetWorld():AddAnimal(33, 50, SPEC_DUCK)
        rttr:GetWorld():AddAnimal(33, 50, SPEC_DUCK)

        rttr:GetWorld():AddAnimal(29, 29, SPEC_DUCK)
        rttr:GetWorld():AddAnimal(29, 29, SPEC_DUCK)
        rttr:GetWorld():AddAnimal(29, 29, SPEC_DUCK)

        rttr:GetWorld():AddAnimal(16, 76, SPEC_RABBITWHITE)
        rttr:GetWorld():AddAnimal(16, 76, SPEC_RABBITWHITE)
        rttr:GetWorld():AddAnimal(16, 76, SPEC_RABBITWHITE)

        rttr:GetWorld():AddAnimal(17, 75, SPEC_RABBITGREY)
        rttr:GetWorld():AddAnimal(17, 75, SPEC_RABBITGREY)
        rttr:GetWorld():AddAnimal(17, 75, SPEC_RABBITGREY)
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
        rttr:GetPlayer(p):DisableBuilding(BLD_BREWERY, false)
        rttr:GetPlayer(p):DisableBuilding(BLD_STOREHOUSE, false)
        rttr:GetPlayer(p):DisableBuilding(BLD_GUARDHOUSE, false)
        rttr:GetPlayer(p):DisableBuilding(BLD_WATCHTOWER, false)
        rttr:GetPlayer(p):DisableBuilding(BLD_FORTRESS, false)
        rttr:GetPlayer(p):DisableBuilding(BLD_LOOKOUTTOWER, false)
        rttr:GetPlayer(p):DisableBuilding(BLD_CATAPULT, false)

    elseif(p == 1) then
        if onLoad then return end

        rttr:GetPlayer(p):AIConstructionOrder(41,16, BLD_CATAPULT)
        rttr:GetPlayer(p):AIConstructionOrder(42,18, BLD_WOODCUTTER)
        rttr:GetPlayer(p):AIConstructionOrder(43,16, BLD_FORESTER)
        rttr:GetPlayer(p):AIConstructionOrder(45,16, BLD_SAWMILL)
        --rttr:GetPlayer(p):AIConstructionOrder(35,31, BLD_GUARDHOUSE)
        --rttr:GetPlayer(p):AIConstructionOrder(36,30, BLD_GUARDHOUSE)
        --rttr:GetPlayer(p):AIConstructionOrder(35,24, BLD_GUARDHOUSE)
        rttr:GetPlayer(p):AIConstructionOrder(37,27, BLD_QUARRY)
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
            [GD_WATER       ] = 10,
            [GD_CLEAVER     ] =  0,
            [GD_ROLLINGPIN  ] =  0,
            [GD_BOW         ] =  0,
            [GD_BOAT        ] =  4,
            [GD_SWORD       ] = 10,
            [GD_IRON        ] =  0,
            [GD_FLOUR       ] =  0,
            [GD_FISH        ] = 20,
            [GD_BREAD       ] = 20,
            [GD_SHIELD      ] = 10,
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
            [JOB_STONEMASON         ] =  5,
            [JOB_HUNTER             ] =  3,
            [JOB_FARMER             ] =  8,
            [JOB_MILLER             ] =  5,
            [JOB_BAKER              ] =  5,
            [JOB_BUTCHER            ] =  5,
            [JOB_MINER              ] = 10,
            [JOB_BREWER             ] =  1,
            [JOB_PIGBREEDER         ] =  5,
            [JOB_DONKEYBREEDER      ] =  5,
            [JOB_IRONFOUNDER        ] =  5,
            [JOB_MINTER             ] =  5,
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
            [GD_BEER        ] =  5,
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
            [GD_FISH        ] = 30,
            [GD_BREAD       ] =  0,
            [GD_SHIELD      ] =  0,
            [GD_WOOD        ] = 40,
            [GD_BOARDS      ] = 30,
            [GD_STONES      ] = 50,
            [GD_GRAIN       ] =  0,
            [GD_COINS       ] =  0,
            [GD_GOLD        ] =  0,
            [GD_IRONORE     ] =  0,
            [GD_COAL        ] = 10,
            [GD_MEAT        ] =  1,
            [GD_HAM         ] =  0
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
            [JOB_FARMER             ] =  5,
            [JOB_MILLER             ] =  5,
            [JOB_BAKER              ] =  5,
            [JOB_BUTCHER            ] =  5,
            [JOB_MINER              ] = 10,
            [JOB_BREWER             ] =  5,
            [JOB_PIGBREEDER         ] =  5,
            [JOB_DONKEYBREEDER      ] =  5,
            [JOB_IRONFOUNDER        ] =  5,
            [JOB_MINTER             ] =  5,
            [JOB_METALWORKER        ] =  5,
            [JOB_ARMORER            ] =  5,
            [JOB_BUILDER            ] = 15,
            [JOB_PLANER             ] = 10,
            [JOB_GEOLOGIST          ] =  5,
            [JOB_PRIVATE            ] = 12,
            [JOB_PRIVATEFIRSTCLASS  ] =  0,
            [JOB_SERGEANT           ] =  0,
            [JOB_OFFICER            ] =  0,
            [JOB_GENERAL            ] =  0,
            [JOB_SCOUT              ] = 10,
            [JOB_SHIPWRIGHT         ] =  0,
            [JOB_PACKDONKEY         ] =  0,
            [JOB_CHARBURNER         ] =  0
        })
    end
end


-------------------------------- mission events -------------------------------
function onGameFrame(gf)
    -- events called by GetBuildingCount(), GetWareCount() or GetPeopleCount()
    if((gf%20)~=0) then
        return
    end

    if(rttr:GetPlayer(1):GetNumBuildings(BLD_CATAPULT) + rttr:GetPlayer(1):GetNumBuildingSites(BLD_CATAPULT) > 0) then
        MissionEvent(98)
    end

    if (rttr:GetPlayer(0).GetStatisticsValue ~= nil) then 
        if(rttr:GetPlayer(0):GetStatisticsValue(STAT_COUNTRY) > 800) then
            MissionEvent(7)
        end
    end
end

function onOccupied(p, x, y)
    -- only check human player
    if(p ~= 0) then
        return
    end

    if (rttr:GetPlayer(0).GetStatisticsValue == nil) then 
        if(     (x == 37) and (y == 67) ) then 
            MissionEvent(7)
        elseif( (x == 31) and (y == 66) ) then
            MissionEvent(7)
        end
    end

    if( (x == 41) and (y == 16) ) then MissionEvent(10)
    elseif( (x == 48) and (y ==  9) ) then MissionEvent(99)
    end
end

function onExplored(p, x, y, o)
    -- onContact events
    if( ((p == 0) and (o == 1)) or ((p == 1) and (o == 0)) ) then MissionEvent(8)
    end

    if(p ~= 0) then
        return
    end

    -- onExplored events
    if(     (x == 20) and (y == 63) ) then MissionEvent(2)
    elseif( (x == 18) and (y == 56) ) then MissionEvent(3)
    elseif( (x == 31) and (y == 61) ) then MissionEvent(4)
    elseif( (x == 33) and (y == 73) ) then MissionEvent(5)
    elseif( (x == 19) and (y == 43) ) then MissionEvent(6)
    elseif( (x == 41) and (y == 16) ) then MissionEvent(9)
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
        rttr:GetPlayer(0):EnableBuilding(BLD_BREWERY, not onLoad)

    elseif(e == 3) then
        rttr:GetPlayer(0):EnableBuilding(BLD_GUARDHOUSE, not onLoad)

    elseif(e == 4) then
        rttr:GetPlayer(0):EnableBuilding(BLD_WATCHTOWER, not onLoad)

    elseif(e == 5) then
        rttr:GetPlayer(0):EnableBuilding(BLD_FORTRESS, not onLoad)

    elseif(e == 7) then
        rttr:GetPlayer(0):EnableBuilding(BLD_STOREHOUSE, not onLoad)

    elseif(e == 10) then
        rttr:GetPlayer(0):EnableBuilding(BLD_CATAPULT, not onLoad)

    elseif(e == 98) then 
        rttr:GetPlayer(1):DisableBuilding(BLD_CATAPULT, false)

    elseif(e == 99) then
        -- TODO: EnableNextMissions()
        -- Show opened arc
        rttr:GetWorld():AddStaticObject(48, 9, 561, 0xFFFF, 2)
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
