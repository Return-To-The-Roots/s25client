------------------------------------------------------------------------------
-- LUA-Script for MISS200.WLD (mission 1 of the original "Roman Campaign"   --
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

local requiredFeature = 6
function checkVersion()
    local featureLevel = rttr:GetFeatureLevel()
    if(featureLevel < requiredFeature) then
        rttr:MsgBox("LUA-Version Error", "Your Return to the Roots version is outdated. The required LUA-Feature level is " ..requiredFeature.. ", your version is "..featureLevel..". The script can possibly crash or run unexpectedly!\n\nPlease update the game!", true)
    end
end

-------------------------------- mission events and texts ---------------------
-- Message-Window (mission statement and hints): 52 chars wide
eIdx = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 99}

rttr:RegisterTranslations(
{
    cs =
    {
        Diary   = 'Deník',

        msg1    = 'Deník Oktaviána:\n\n\nČtvrtý den po ztroskotání lodi.\n\n\nVčera se všichni přeživší setkali, aby diskutovali o situaci. Protože nenašli žádné východisko na včasnou záchranu, rozhodli se usadit na tomto cizím ostrově. Zachráněné zboží z lodi nám bude velkou pomocí do začátku. Nejdůležitější věcí je využití místních suroviny k založení osady. Naléhavě potřebujeme vybudovat dílnu pro dřevorubce, pilaře a kameníka.', 
        msgh1   = 'Postavte dílnu pro dřevorubce, tesaře a kameníka.',

        msg2    = 'Osmý den po ztroskotání.\n\n\nZákladní požadavky na vybudování naší osady byly splněny. Množství stromů v této oblasti nám však nevydrží na dlouho. Potřebujeme lesníka, abychom zajistili dlouhodobé dodávky dřeva.',
        msgh2   = 'Postavte lesnickou chatu.',

        msg3    = 'Jedenáctý den druhého měsíce po ztroskotání.\n\n\nStále si nejsme jisti, zda jsme na tomto ostrově sami. Pro jsitotu bychom měli poslat vojsko k severní hranici.',
        msgh3   = 'Postavte kasárna u severní hranice našeho území.',

        msg4    = 'Pátý den třetího měsíce.\n\n\nNaše území se rozšířilo až k horám na severu. Náš geolog tam provede terénní průzkum. Z dlouhodobého hlediska potřebujeme uhlí a stavební materiály, stejně jako zbraně a zlato. Musíme tedy najít železnou rudu, zlato, uhlí a kámen.',
        msgh4   = 'Postavte cestu do hor a\npošlete tam našeho geologa.',

        msg5    = 'Šestnáctý den šestého měsíce.\n\n\nNašli jsme železnou rudu a nyní jsme schopni vyrábět nzbraně. Poté můžeme bezpečně provést další průzkum ostrova. Nejprve musíme postavit důl na železnou rudu a slévárnu. Zbrojíř pak může začít vyrábět meče a štíty. Horníkům musíme také poskytnout jídlo, zásoby jsou téměř vyčerpané. Potřebujeme lovce a rybáře!',
        msgh5   = 'Postavte důl na železnou rudu, slévárnu a zbrojírnu.',

        msg6    = 'Poslední den osmého měsíce po příchodu k břehům.\n\n\nSeverně od naší nové hranice se rozléhá úrodná planina. Ta poskytne spoustu nových příležitostí pro rozšíření naší osady, ale k tomu potřebujeme více vojáků.',
        msgh6   = 'Rozšiř severní teritórium a pokračuj v opevňování hranic.',

        msg7    = 'Dvanáctý den devátého měsíce.\n\n\nPo dalším postupu dále na sever jsme konečně našli dostatek prostoru pro stavbu větších budov a farem. Nyní bychom měli hledat suroviny v okolních horách.',
        msgh7   = 'Hledejte nové suroviny a zajistěte dodávky potravin. Prozkoumej celý ostrov.',

        msg8    = 'Náš geolog našel pramen vody. Nyní jsme schopni čerpat pitnou vodu.',

        msg9    = 'Nalezením uhlí v západních horách jsme si zajistili dodávky paliva pto naše slévárny a zrojnice. Nyní bez problémů můžeme začít zpracovávat železnou rudu. Všechno jde velmi dobře.',

        msg10   = 'Geologové našli v horninách žulové usazeniny, což prozatím poskytne nový zdroj kamene.',

        msg11   = 'Objevili jsme zlatou žílu a nyní můžeme v mincovně vyrábět mince. Díky tomu bude život téměř tak pohodlný jako doma.',

        msg12   = 'Varizili jsme první zlaté mince a nyní můžeme platit za výcvik našich vojáků.',

        msg13   = 'Stavba naší první farmy na obilí byla dokončena. Obilí lze zpracovat na mouku v mlýně, nebo použít jako krmivo pro prasata. Zásobování potravinami by se nyní mělo nadále zlepšovat.',

        msg14   = 'Náš nový vepřín může dodávat poražené vepře řezníkovi na jatka.',

        msg15   = 'Větrný mlýn může poskytnout pekaři dostatek mouky pro pečení chleba.',

        msg16   = 'Naši učenci objevili podivný monument. Vypadá to jako brána. Zjevně nejsme první obyvatelé tohoto ostrova. Na bráně dokonce našli nápis v latině „Consiste ut procederas!“ - „Usaď te se, abyste dosáhli pokroku!“. Je to bizarní a rozporuplné. Co to může znamenat?!',

        msg99   = 'Druhý den třináctého měsíce po stroskotání u břehů tohoto ostrova.\n\n\nVčera začal druhý rok a zdá se, že se nám tento ostrov podařilo osídlit. Naši učenci hlásí podivné události u brány. Vypadá to, že se brána otevřela a může nás zavést na cestu, která nás vyvede z tohoto ostrova. Významem hádanky byl zřejmě takovýto: „Aby se člověk dostal dál musí oběvovat a osidlovat nové území“. No uvydíme, snad skutečně všechny cesty vedou do Říma.',
        msgh99  = 'Dokončili jste tuto misi. Čeká na tebe druhá kapitola...\n\nDalší misi můžeš zahájit z hlavní nabídky kdykoli budeš chtít.',
    },
    de =
    {
        Diary   = 'Tagebuch',

        msg1    = 'Tagebuch des Octavius:\n\n\nVierter Tag nach dem Schiffbruch.\n\n\nGestern fanden sich alle Überlebenden zusammen, um\ndie Lage zu beraten. Da keine Aussicht auf eine\nbaldige Rettung besteht, beschlossen wir, uns auf\ndieser fremden Insel niederzulassen. Dabei werden\nuns die geretteten Güter aus dem Schiff eine große\nHilfe sein. Das Wichtigste ist, die Rohstoffe der\nInsel für den Aufbau einer Siedlung nutzbar zu\nmachen. Wir benötigen dringend Unterkünfte für einen\nHolzfäller, Schreiner und den Steinmetz.',
        msgh1   = 'Bauen Sie Unterkünfte für einen Holzfäller,\nSchreiner und Steinmetz.',

        msg2    = 'Achter Tag nach dem Schiffbruch.\n\n\nDie Grundlagen für den Aufbau unserer Siedlung sind\ngeschaffen. Der Baumbestand in diesem Gebiet wird\naber nicht lange vorhalten. Um die langfristige\nVersorgung mit Holz zu gewährleisten, benötigen wir\neinen Förster.',
        msgh2   = 'Bauen Sie ein Forsthaus.',

        msg3    = 'Elfter Tag im zweiten Monat nach dem Schiffbruch.\n\n\nWir wissen immer noch nicht, ob wir alleine auf\ndieser Insel sind. Vorsichtshalber sollten wir einen\nWachposten an den offenen Grenzen zum Landesinneren\nerrichten.',
        msgh3   = 'Bauen Sie eine Baracke an der nördlichen Grenze\nIhrer Siedlung.',

        msg4    = 'Fünfter Tag im dritten Monat.\n\n\nDurch die Ausbreitung unseres Landes haben wir den\nBerg im Norden erreicht. Unser Gelehrter sollte sich\ndort den Boden mal anschauen. Langfristig brauchen\nwir Brennstoff, Baumaterial und natürlich auch\nWaffen und gemünztes Gold. Wir müssen also Eisenerz,\nGold, Kohle und Granit finden.',
        msgh4   = 'Bauen Sie einen Weg auf den Berg und schicken Sie\nIhren Gelehrten dorthin.',

        msg5    = 'Sechzehnter Tag im sechsten Monat.\n\n\nWir haben Eisenerz gefunden und wären jetzt in der\nLage, Waffen zu produzieren. Der weiteren Erkundung\nder Insel stünde dann nichts mehr im Wege. Dazu\nmüssen wir aber erst ein Eisenbergwerk und die\ndazugehörige Eisenschmelze bauen. Eine\nWaffenschmiede könnte dann die Produktion von\nSchwertern und Schilden übernehmen. Wir müssen aber\nauch die Bergarbeiter mit Nahrungsmitteln versorgen.\nUnsere Vorräte sind fast aufgebraucht. Wir brauchen\nJäger und Fischer!',
        msgh5   = 'Bauen Sie ein Eisenbergwerk, eine Eisenschmelze und\neine Waffenschmiede.',

        msg6    = 'Letzter Tag im achten Monat nach der Landung.\n\n\nNördlich unseres neuen Außenpostens liegt eine\nweite, fruchtbare Ebene. Dort ergeben sich ganz neue\nMöglichkeiten, unsere Siedlung zu erweitern. Dazu\nbrauchen wir allerdings Soldaten.',
        msgh6   = 'Erreichen Sie die nördliche Ebene, indem Sie an der\nNordgrenze weitere Baracken bauen.',

        msg7    = 'Zwölfter Tag des neunten Monats.\n\n\nNachdem wir weiter nach Norden vorgedrungen sind,\nhaben wir endlich genug Platz, um größere Gebäude\nund Bauernhöfe errichten zu können. Jetzt wollen wir\ndas umliegende Gebirge nach Rohstoffen absuchen.',
        msgh7   = 'Suchen Sie nach neuen Rohstoffen und sichern Sie die\nNahrungsversorgung. Erkunden Sie die Insel\nvollständig.',

        msg8    = 'Unser Gelehrter hat eine Wasserader gefunden. Nun\nwären wir in der Lage mit einem Brunnen Wasser zu\nfördern.',

        msg9    = 'Die Versorgung der Schmelzereien ist gewährleistet,\ndenn wir haben Kohle im westlichen Gebirge gefunden.\nNun können wir unsere Metallerze problemlos\nweiterverarbeiten. Alles entwickelt sich sehr gut.',

        msg10   = 'Da Gelehrte Granitvorkommen in den Bergen gefunden\nhaben, ist der Nachschub an Steinen bis auf weiteres\ngesichert.',

        msg11   = 'Wir haben eine Goldader erschlossen und wären nun in\nder Lage, in einer Münzprägerei kostbare Münzen\nherzustellen. So langsam wird das Leben hier beinahe\nheimisch komfortabel.',

        msg12   = 'Wir haben die ersten Goldmünzen geprägt und können\njetzt die Ausbildung unserer Soldaten bezahlen.',

        msg13   = 'Der Bau unserer ersten Getreidefarm ist\nabgeschlossen. Das Getreide könnte nun in einer\nMühle zu Mehl verarbeitet oder auf einem Schweinehof\nverfüttert werden. Die Versorgungslage würde dann\nimmer besser.',

        msg14   = 'Unser neue Schweinehof könnte nun eine Metzgerei mit\nSchlachtvieh versorgen.',

        msg15   = 'Die Windmühle könnte eine Bäckerei mit ausreichend\nMehl versorgen.',

        msg16   = 'Ein seltsames Ding haben unsere Gelehrten entdeckt.\nEs sieht aus wie ein Tor. Offensichtlich waren also\ndoch bereits Menschen vor uns auf dieser Insel.\nSogar eine Inschrift in latinischen Buchstaben\nentdeckten sie auf dem Tor. Sie ist sehr seltsam und\nwidersprüchlich, vielleicht ein Rätsel: \'Consiste ut\nprocederas!\' \'Laß Dich nieder, um fortzuschreiten!\'\nWer weiß, was das bedeutet?!',

        msg99   = 'Zweiter Tag des dreizehnten Monats nach der Landung.\n\n\nGestern begann das zweite Jahr, und wir scheinen es\ngeschafft zu haben. Die Gelehrten berichten von\nseltsamen Aktivitäten am Tor. Es sieht so aus, als\nhabe sich ein Weg geöffnet, der uns von dieser Insel\nwegführen kann. Das scheint der Sinn des Rätsels zu\nsein: Man muß sich niederlassen und siedeln, um\nweiterzukommen. Mal sehen. Vielleicht führen ja\nwirklich alle Wege nach Rom.',
        msgh99  = 'Sie haben diese Mission erfüllt. Das zweite Kapitel\nwartet schon auf Sie...\n\nWann immer Sie wollen, können Sie im Hauptmenü die\nnächste Mission starten.'
    },
    en =
    {
        Diary   = 'Diary',

        msg1    = 'Diary of Octavius:\n\n\nFourth Day After Shipwreck.\n\n\nYesterday all the survivors met in order to discuss\nthe situation. Because there is no prospect of\nearly rescue, we decided to settle on this foreign\nisland. The items salvaged from the ship will be a\ngreat help to us. The most important thing is to\nmake use of the raw materials on the island in order\nto establish a settlement. We urgently need\naccommodations for a woodcutter, carpenter and\nstonemason.', 
        msgh1   = 'Build accommodations for a woodcutter, carpenter and\nstonemason.',

        msg2    = 'Eighth Day After Shipwreck.\n\n\nThe basic requirements for building up our\nsettlement have been met. However, the supply of\ntrees in this region will not last long. We need a\nforester to ensure a long-term supply of timber.',
        msgh2   = 'Build a forester\'s cabin.',

        msg3    = 'Eleventh Day of Second Month After Shipwreck.\n\n\nWe are still not sure whether we are the only\npersons on this island. As a precautionary measure\nwe should erect a barracks on our exposed border\ntowards the interior of the island.',
        msgh3   = 'Build a barracks on the northern\nborder of your settlement.',

        msg4    = 'Fifth Day of Third Month.\n\n\nOur territory has spread as far as the mountain in\nthe north. Our geologist will carry out a land\nsurvey there. In the long term we need fuel and\nbuilding materials as well as weapons and coined\ngold. We must therefore find iron ore, gold, coal\nand granite.',
        msgh4   = 'Build a road to the mountain and\nsend your geologist there.',

        msg5    = 'Sixteenth Day of Sixth Month.\n\n\nWe have found iron ore and are now able to produce\nweapons. There will then be no obstacles to further\nexploration of the island. First of all we must\nbuild an iron mine and an iron smelter for it. An\narmorer can then start to produce swords and\nshields. We must also provide the miners with food,\nour stocks are almost exhausted. We need hunters and\nfishermen!',
        msgh5   = 'Build an iron mine, an iron smelter and a smithy.',

        msg6    = 'Last Day of Eighth Month After Coming Ashore.\n\n\nThere is an open, fertile plain to the north of our\nnew border post. This will provide lots of new\nopportunities for expanding our settlement but we\nneed more soldiers to do this.',
        msgh6   = 'Go to the northern plain and\ncontinue fortifying the border.',

        msg7    = 'Twelfth Day of Ninth Month.\n\n\nAfter advancing further north we have finally found\nenough space to build larger buildings and farms. We\nnow intend to prospect for raw materials in the\nsurrounding mountains.',
        msgh7   = 'Search for new raw materials and secure a supply of\nfood. Explore the entire island.',

        msg8    = 'Our geologist has found a spring. We are now able\nto supply fresh water from it.',

        msg9    = 'We have ensured a fuel supply for the metalworks by\nfinding coal in the western mountains. Now we can\nprocess our metal ores with no problems. Everything\nis going extremely well.',
    
        msg10   = 'Geologists have found granite deposits in the\nmountains and this will provide a fresh supply of\nstone for the time being.',

        msg11   = 'We have discovered a gold vein\nand can now produce valuable coins in a mint. This\nwill make life here almost as comfortable as at home.',

        msg12   = 'We have minted the first gold coins and can now pay\nfor the training of our soldiers.',

        msg13   = 'Construction of our first grain farm has been\ncompleted. The grain can be processed into flour in\na mill or fed to pigs. The food supply situation\nshould now continue to improve.',

        msg14   = 'Our new pig farm can supply slaughtered animals to a\nbutcher.',

        msg15   = 'The windmill can supply a baker with sufficient\nflour for baking bread.',

        msg16   = 'Our geologists have discovered a strange object. It\nlooks like a gateway. We are obviously not the first\nhumans on this island. They have even found an\ninscription on the gateway in Latin. It is bizarre\nand contradictory, perhaps it is a riddle:\n\'Consiste ut procederas!\' - \'Settle down in order to make progress!\'.\nWhat can this mean?!',

        msg99   = 'Second Day of Thirteenth Month After Coming Ashore.\n\n\nThe second year began yesterday and we seem to have\nsucceeded in settling this island. The geologists\nreport strange goings on near the gateway. It\nappears that a path has opened up that can lead us\nfrom this island. This seems to be the meaning of\nthe riddle: we must settle in order to move on. We\nshall take a look, perhaps all roads really do lead\nto Rome.',
        msgh99  = 'You have completed this mission. The second Chapter\nawaits you...\n\nYou can begin the next mission from the main menu\nwhenever you want.'
    },
    pl =
    {
        Diary   = 'Dziennik',

        msg1    = 'Dziennik Oktawiusza:\n\n\nCzwarty Dzień po rozbiciu statku.\n\n\nDnia wczorajszego, wszyscy, którzy przetrwali, zgromadzili się, aby przedyskutować naszą obecną sytuację.\n\nZ racji, iż nie mamy perspektyw na szybki ratunek, zdecydowaliśmy, że osiądziemy na tej nieznanej wyspie.\n\nRzeczy uratowane ze statku będą dla nas wielką pomocą.\n\nNajważniejsze jest, by wykorzystać surowce z wyspy w celu założenia osady.\n\nPilnie potrzebujemy usług:\ndrwala, cieśli i kamieniarza.',
        msgh1   = 'Zbuduj kwatery dla drwala, cieśli i kamieniarza.',

        msg2    = 'Ósmy Dzień po rozbiciu statku.\n\n\nPodstawy naszej osady zostały stworzone.\n\nJednak znajdujące się w tej okolicy drzewa nie wystarczą nam na długo.\n\nPotrzebujemy leśnika, który zapewni nam długotrwałe dostawy drewna.',
        msgh2   = 'Zbuduj kwaterę dla leśnika.',

        msg3    = 'Jedenasty Dzień Drugiego Miesiąca po rozbiciu statku.\n\n\nWciąż nie jesteśmy pewni, czy jesteśmy jedynymi ludźmi na tej wyspie.\n\nJako środek ostrożności powinniśmy postawić koszary\nna naszej granicy na północ od osady.',
        msgh3   = 'Zbuduj koszary na północnej granicy swojej osady.',

        msg4    = 'Piąty Dzień Trzeciego Miesiąca.\n\n\nNasze terytorium rozciągnęło się aż do gór na północy.\n\nNasz geolog przeprowadzi tam badania terenu.\n\nNa dłuższą metę potrzebujemy paliwa i materiałów budowlanych oraz broni i złotych monet.\n\nMusimy więc znaleźć rudę żelaza, złoto, węgiel i granit.',
        msgh4   = 'Zbuduj drogę w górach i wyślij tam swojego geologa.',

        msg5    = 'Szesnasty Dzień Szóstego Miesiąca.\n\n\nZnaleźliśmy rudę żelaza i teraz możemy produkować broń.\nNie będzie wtedy żadnych przeszkód dla dalszej eksploracji wyspy.\n\nNajpierw musimy zbudować kopalnię żelaza i hutę żelaza.\nKowal będzie mógł rozpocząć  produkcję mieczy i tarcz.\n\nMusimy również zapewnić górnikom jedzenie, a nasze zapasy są prawie wyczerpane.\n\nPotrzebujemy myśliwych i rybaków!',
        msgh5   = 'Zbuduj kopalnię żelaza, hutę żelaza i kuźnię.',

        msg6    = 'Ostatni Dzień Ósmego Miesiąca po przybyciu na ląd.\n\n\nNa północ od naszego nowego posterunku granicznego znajduje się otwarta, żyzna równina.\n\nTo zapewni wiele nowych możliwości rozbudowy naszej osady, ale potrzebujemy więcej żołnierzy, aby to zrobić.',
        msgh6   = 'Idź na północną równinę i kontynuuj umacnianie granicy.',

        msg7    = 'Dwunasty Dzień Dziewiątego Miesiąca.\n\n\nPo dalszej ekspansji na północ w końcu znaleźliśmy dostatecznie dużo miejsca, aby zbudować większe budynki i farmy.\n\nTeraz zamierzamy poszukiwać surowców w okolicznych górach.',
        msgh7   = 'Rozpocznij poszukiwania nowych surowców i zabezpiecz dostawy jedzenia.\nZbadaj całą wyspę.',

        msg8    = 'Nasz geolog znalazł źródło.\n\nTeraz możemy zapewnić sobie zapas świeżej wody.',

        msg9    = 'Zapewniliśmy dostawy paliwa dla kuźni, znajdując węgiel w zachodnich górach.\n\nTeraz możemy bez problemów przetwarzać nasze rudy metali.\n\nWszystko idzie niezwykle dobrze.',

        msg10   = 'Geolodzy odkryli złoża granitu w górach,\nco na jakiś czas zapewni zapas kamienia.',

        msg11   = 'Odkryliśmy żyłę złota i możemy teraz produkować cenne monety w mennicy.\n\nŻycie staje się powoli znośne - wraz ze środkiem płatniczym, stanie się ono niemal tak wygodne jak w domu.',

        msg12   = 'Wybiliśmy pierwsze złote monety i teraz możemy płacić za szkolenie naszych żołnierzy.',

        msg13   = 'Budowa naszej pierwszej farmy zbożowej została zakończona.\n\nZboże można przetworzyć na mąkę w młynie lub karmić nim świnie.\n\nSytuacja z zaopatrzeniem żywności powinna się teraz poprawić.',

        msg14   = 'Nasz nowy chlew zapewnia dostawy zwierząt dla rzeźnika.',

        msg15   = 'Wiatrak zapewnia piekarzowi dostawy odpowiednich ilości mąki do wypieku chleba.',

        msg16   = 'Nasi zwiadowcy odkryli przedziwną konstrukcję. Wygląda ona jak swego rodzaju wrota.\n\nOczywiście oznacza to, że nie jesteśmy pierwszymi ludźmi, jacy dotarli na tę wyspę.\n\nZwiadowcy znaleźli nawet łacińską inskrypcję na tych wrotach.\nJest dziwaczna i sprzeczna... A może to zagadka?\n\n\"Consiste ut procederas!\" - \"Osiadając ruszaj dalej!\"\n\nCo to może oznaczać?!',

        msg99   = 'Drugi Dzień Trzynastego Miesiąca po przybyciu na ląd.\n\n\nDrugi rok rozpoczął się wczoraj i wydaje się, że pomyślnie osiedliśmy na tej wyspie.\n\nZwiadowcy zameldowali o dziwnych wydarzeniach w pobliżu wrót.\n\nWygląda na to, że otworzył się portal, który może nas wyprowadzić z tej wyspy.\n\nChyba o to chodziło w tej przedziwnej zagadce: \"aby posunąć się naprzód, trzeba nam było osiąść w miejscu.\"\n\nSprawdzimy to - być może wszystkie drogi naprawdę prowadzą do Rzymu?',
        msgh99  = 'Ukończyłeś tę misję. Drugi Rozdział czeka na ciebie...\n\nMożesz rozpocząć następną misję z menu głównego kiedy tylko chcesz.'
    }
})

function getNumPlayers()
    return 1
end

-- format mission texts
-- BUG:     NewLine at the end is wrongly interpreted, adding 2x Space resolves this issue
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
    rttr:Log("-----------------------\n MISS200.lua loaded... \n-----------------------\n")
    rttr:ResetAddons()
    rttr:SetAddon(ADDON_FRONTIER_DISTANCE_REACHABLE, true)
    rttr:SetGameSettings({
        ["fow"] = EXP_CLASSIC,
        ["teamView"] = false,
        ["lockedTeams"] = false
    })

    rttr:GetPlayer(0):SetNation(NAT_ROMANS)     -- nation
    rttr:GetPlayer(0):SetColor(0)               -- 0:blue, 1:red, 2:yellow, 
    rttr:GetPlayer(0):SetPortrait(0)
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

-- start callback
function onStart(isFirstStart)
    rttr:GetPlayer(0):ModifyHQ(1)               -- tent
    addPlayerRes(0, not isFirstStart)           -- set resources
    addPlayerBld(0, not isFirstStart)

    eState = {}                                 -- disable all events
    for _, i in ipairs(eIdx) do
        eState[i] = 0
    end

    for _, i in ipairs({1, 8, 9, 10, 11, 99}) do-- set events as active
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

    if isFirstStart then
        rttr:GetWorld():AddAnimal(41, 44, SPEC_DUCK)
        rttr:GetWorld():AddAnimal(41, 44, SPEC_DUCK)
        rttr:GetWorld():AddAnimal(41, 44, SPEC_DUCK)
        rttr:GetWorld():AddAnimal(30, 36, SPEC_RABBITWHITE)
        rttr:GetWorld():AddAnimal(28, 37, SPEC_RABBITGREY)
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
    if(p == 0) then
        rttr:GetPlayer(p):DisableAllBuildings()
        rttr:GetPlayer(p):EnableBuilding(BLD_WOODCUTTER,    false)
        rttr:GetPlayer(p):EnableBuilding(BLD_QUARRY,        false)
        rttr:GetPlayer(p):EnableBuilding(BLD_SAWMILL,       false)
    end
end

-------------------------------- set resources --------------------------------
-- Don't add goods/people onLoad!
function addPlayerRes(p, onLoad)
    if onLoad then return end

    if(p == 0) then
        -- goods
        rttr:GetPlayer(p):ClearResources()
        rttr:GetPlayer(p):AddWares({
            [GD_BEER      ] = 12,
            [GD_TONGS     ] = 0,
            [GD_HAMMER    ] = 0,
            [GD_AXE       ] = 0,
            [GD_SAW       ] = 0,
            [GD_PICKAXE   ] = 0,
            [GD_SHOVEL    ] = 0,
            [GD_CRUCIBLE  ] = 0,
            [GD_RODANDLINE] = 0,
            [GD_SCYTHE    ] = 0,
            [GD_WATER     ] = 12,
            [GD_CLEAVER   ] = 0,
            [GD_ROLLINGPIN] = 0,
            [GD_BOW       ] = 0,
            [GD_BOAT      ] = 0,
            [GD_SWORD     ] = 0,
            [GD_IRON      ] = 0,
            [GD_FLOUR     ] = 0,
            [GD_FISH      ] = 2,
            [GD_BREAD     ] = 4,
            [GD_SHIELD    ] = 0,
            [GD_WOOD      ] = 0,
            [GD_BOARDS    ] = 15,
            [GD_STONES    ] = 12,
            [GD_GRAIN     ] = 0,
            [GD_COINS     ] = 0,
            [GD_GOLD      ] = 0,
            [GD_IRONORE   ] = 0,
            [GD_COAL      ] = 20,
            [GD_MEAT      ] = 2,
            [GD_HAM       ] = 0
        })

        -- people
        rttr:GetPlayer(p):AddPeople({
        [JOB_HELPER             ] = 40,
        [JOB_WOODCUTTER         ] = 8,
        [JOB_FISHER             ] = 4,
        [JOB_FORESTER           ] = 4,
        [JOB_CARPENTER          ] = 3,
        [JOB_STONEMASON         ] = 4,
        [JOB_HUNTER             ] = 4,
        [JOB_FARMER             ] = 6,
        [JOB_MILLER             ] = 3,
        [JOB_BAKER              ] = 3,
        [JOB_BUTCHER            ] = 3,
        [JOB_MINER              ] = 8,
        [JOB_BREWER             ] = 0,
        [JOB_PIGBREEDER         ] = 4,
        [JOB_DONKEYBREEDER      ] = 0,
        [JOB_IRONFOUNDER        ] = 4,
        [JOB_MINTER             ] = 4,
        [JOB_METALWORKER        ] = 4,
        [JOB_ARMORER            ] = 4,
        [JOB_BUILDER            ] = 4,
        [JOB_PLANER             ] = 4,
        [JOB_GEOLOGIST          ] = 0,
        [JOB_PRIVATE            ] = 8,
        [JOB_PRIVATEFIRSTCLASS  ] = 0,
        [JOB_SERGEANT           ] = 0,
        [JOB_OFFICER            ] = 0,
        [JOB_GENERAL            ] = 0,
        [JOB_SCOUT              ] = 8,
        [JOB_SHIPWRIGHT         ] = 0,
        [JOB_PACKDONKEY         ] = 15,
        [JOB_CHARBURNER         ] = 0
        })
    end
end


-------------------------------- mission events -------------------------------
function onGameFrame(gf)
    -- events called by GetBuildingCount(), GetWareCount() or GetPeopleCount()
    MissionEvent(2)
    MissionEvent(3)
    MissionEvent(6)
    MissionEvent(12)
    MissionEvent(13)
    MissionEvent(14)
    MissionEvent(15)
end

function onOccupied(p, x, y)
    -- only check human player
    if(p ~= 0) then
        return
    end

    if(     (x == 34) and (y == 28) ) then MissionEvent(4) -- RttR calls onOccupied once a pole is on that spot, SettlersII called the function once the spot is really owned
    elseif( (x == 39) and (y == 19) ) then MissionEvent(7)
    elseif( (x == 14) and (y ==  8) ) then MissionEvent(99)
    end
end

function onExplored(p, x, y)
    if(p ~= 0) then
        return
    end

    -- onExplored events
    if(     (x == 14) and (y ==  8) ) then MissionEvent(16)
    end
end

function onResourceFound(p, x, y, rIdx, q)
    -- only check human player
    if(p ~= 0) then
        return
    end

    if(rIdx == RES_IRON) then           MissionEvent(5)
    elseif(rIdx == RES_WATER) then      MissionEvent(8)
    elseif(rIdx == RES_COAL) then       MissionEvent(9)
    elseif(rIdx == RES_GRANITE) then    MissionEvent(10)
    elseif(rIdx == RES_GOLD) then       MissionEvent(11)
    end
end

-- execute mission events, e == 1 is initial event, e == 99 is final event
function MissionEvent(e, onLoad)
    -- event e is inactive
    if(eState[e] <= 0) then
        return
    end

    -- call side effects for active events, check "eState[e] == 1" for multiple call events!
    if(e == 1) then
        eState[2] = 1
    
    elseif(e == 2) then
        if not( (rttr:GetPlayer(0):GetBuildingCount(BLD_WOODCUTTER) > 0)
            and (rttr:GetPlayer(0):GetBuildingCount(BLD_QUARRY) > 0) 
            and (rttr:GetPlayer(0):GetBuildingCount(BLD_SAWMILL) > 0)
        ) then return end
        rttr:GetPlayer(0):EnableBuilding(BLD_FORESTER, not onLoad)
        eState[3] = 1

    elseif(e == 3) then
        if not( (rttr:GetPlayer(0):GetBuildingCount(BLD_FORESTER) > 0)
        ) then return end
        rttr:GetPlayer(0):EnableBuilding(BLD_BARRACKS, not onLoad)
        eState[4] = 1

    elseif(e == 4) then
        rttr:GetPlayer(0):DisableBuilding(BLD_BARRACKS, not onLoad)
        if not onLoad then 
            rttr:GetPlayer(0):AddPeople({[JOB_GEOLOGIST] = 4}) 
        end
        eState[5] = 1

    elseif(e == 5) then
        rttr:GetPlayer(0):EnableBuilding(BLD_IRONMINE, not onLoad)
        rttr:GetPlayer(0):EnableBuilding(BLD_IRONSMELTER, not onLoad)
        rttr:GetPlayer(0):EnableBuilding(BLD_ARMORY, not onLoad)
        rttr:GetPlayer(0):EnableBuilding(BLD_HUNTER, not onLoad)
        rttr:GetPlayer(0):EnableBuilding(BLD_FISHERY, not onLoad)
        eState[6] = 1

    elseif(e == 6) then
        if not( (rttr:GetPlayer(0):GetBuildingCount(BLD_IRONSMELTER) > 0)
            and (rttr:GetPlayer(0):GetBuildingCount(BLD_ARMORY) > 0)
            and (rttr:GetPlayer(0):GetBuildingCount(BLD_IRONMINE) > 0)
            ) then return end
        rttr:GetPlayer(0):EnableBuilding(BLD_BARRACKS, not onLoad)
        eState[7] = 1

    elseif(e == 7) then
        rttr:GetPlayer(0):EnableBuilding(BLD_FARM, not onLoad)
        eState[13] = 1
        eState[16] = 1

    elseif(e == 8) then
        rttr:GetPlayer(0):EnableBuilding(BLD_WELL, not onLoad)
        
    elseif(e == 9) then
        rttr:GetPlayer(0):EnableBuilding(BLD_COALMINE, not onLoad)

    elseif(e == 10) then
        rttr:GetPlayer(0):EnableBuilding(BLD_GRANITEMINE, not onLoad)

    elseif(e == 11) then
        rttr:GetPlayer(0):EnableBuilding(BLD_MINT, not onLoad)
        rttr:GetPlayer(0):EnableBuilding(BLD_GOLDMINE, not onLoad)
        eState[12] = 1

    elseif(e == 12) then
        if not( (rttr:GetPlayer(0):GetWareCount(GD_COINS) > 0)
        ) then return end

    elseif(e == 13) then
        if not( (rttr:GetPlayer(0):GetBuildingCount(BLD_FARM) > 0)
        ) then return end
        rttr:GetPlayer(0):EnableBuilding(BLD_PIGFARM, not onLoad)
        rttr:GetPlayer(0):EnableBuilding(BLD_MILL, not onLoad)
        eState[14] = 1
        eState[15] = 1

    elseif(e == 14) then
        if not( (rttr:GetPlayer(0):GetBuildingCount(BLD_PIGFARM) > 0)
        ) then return end
        rttr:GetPlayer(0):EnableBuilding(BLD_SLAUGHTERHOUSE, not onLoad)

    elseif(e == 15) then
        if not ( (rttr:GetPlayer(0):GetBuildingCount(BLD_MILL) > 0)
        ) then return end
        rttr:GetPlayer(0):EnableBuilding(BLD_BAKERY, not onLoad)

    elseif(e == 99) then
        -- TODO: EnableNextMissions()
        -- Show opened arc
        rttr:GetWorld():AddStaticObject(14, 8, 561, 0xFFFF, 2)
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
