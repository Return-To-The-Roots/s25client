// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "BuildingConsts.h"
#include "mygettext/mygettext.h"
#include <type_traits>

const helpers::EnumArray<const char*, BuildingType> BUILDING_NAMES = {
  gettext_noop("Headquarters"),
  gettext_noop("Barracks"),
  gettext_noop("Guardhouse"),
  "",
  gettext_noop("Watchtower"),
  gettext_noop("Vineyard"),
  gettext_noop("Winery"),
  gettext_noop("Temple"),
  "",
  gettext_noop("Fortress"),
  gettext_noop("Granite mine"),
  gettext_noop("Coal mine"),
  gettext_noop("Iron mine"),
  gettext_noop("Gold mine"),
  gettext_noop("Lookout tower"),
  "",
  gettext_noop("Catapult"),
  gettext_noop("Woodcutter"),
  gettext_noop("Fishery"),
  gettext_noop("Quarry"),
  gettext_noop("Forester"),
  gettext_noop("Slaughterhouse"),
  gettext_noop("Hunter"),
  gettext_noop("Brewery"),
  gettext_noop("Armory"),
  gettext_noop("Metalworks"),
  gettext_noop("Iron smelter"),
  gettext_noop("Charburner"),
  gettext_noop("Pig farm"),
  gettext_noop("Storehouse"),
  "",
  gettext_noop("Mill"),
  gettext_noop("Bakery"),
  gettext_noop("Sawmill"),
  gettext_noop("Mint"),
  gettext_noop("Well"),
  gettext_noop("Shipyard"),
  gettext_noop("Farm"),
  gettext_noop("Donkey breeding"),
  gettext_noop("Harbor building"),
};

const helpers::EnumArray<const char*, BuildingType> BUILDING_HELP_STRINGS = {{
  // Headquarters
  gettext_noop("The headquarters represents the "
               "center of your realm. The large "
               "amount of storage space "
               "available means a great many "
               "things can be safely stored "
               "here. You can release certain "
               "merchandise from the "
               "headquarters, as and when "
               "required or stop its storage. To "
               "do this, first choose the "
               "corresponding icon followed by "
               "the desired merchandise or job "
               "symbol. On the third page, you "
               "can adjust the number of reserve "
               "soldiers who are responsible for "
               "guarding the headquarters. There "
               "are two values given: the first "
               "value indicates the current "
               "number of men, the second value "
               "indicates the desired number. "),
  // Barracks
  gettext_noop("The barracks is a very small hut "
               "which can be used by your "
               "soldiers as quarters. Using the "
               "gold coin button you can stop "
               "the delivery of gold coins to "
               "the soldiers stationed here. "
               "However, without gold coins the "
               "soldiers here can not train and "
               "improve their skills."),
  // Guardhouse
  gettext_noop("The guardhouse is a comfortable "
               "place for the military which is "
               "also protected by solid stone "
               "walls. Using the gold coin "
               "button you can stop the delivery "
               "of gold coins to the soldiers "
               "stationed here. However, without "
               "gold coins the soldiers here can "
               "not train and improve their "
               "skills."),
  // Nothing
  "",
  // Watchtower
  gettext_noop("The watchtower with its large "
               "amount of space is best suited "
               "for stationing a large number "
               "of your troops. Using the gold coin "
               "button you can stop the delivery "
               "of gold coins to the soldiers "
               "stationed here. However, without "
               "gold coins the soldiers here can "
               "not train and improve their "
               "skills."),
  // Vineyard building
  gettext_noop("The winegrower plants and harvests "
               "grapes in the surrounding fields. A "
               "vineyard requires a steady supply of "
               "sturdy logs for trellises and fresh "
               "water to irrigate the fields. Ripe "
               "grapes are sent to the winery to "
               "be pressed into wine."),
  // Winery building
  gettext_noop("At the winery, grapes are stomped in "
               "a vat to produce wine. The vintner's "
               "wine is of high quality, worthy of "
               "sacrifice at the temple."),
  // Temple building
  gettext_noop("The temple servant sacrifices wine and "
               "food granting a blessing of gold, iron "
               "ore, coal or granite. This allows your "
               "iron founder and minter to continue "
               "working when mines become exhausted. The "
               "desired mineral can be selected by "
               "toggling the output button."),
  "",
  // Nothing
  // Fortress
  gettext_noop("The defensive capabilities and "
               "size of the fortress are "
               "unsurpassed. This stronghold "
               "ensures that other valuable "
               "buildings and commodities are "
               "protected. Using the gold coin "
               "button you can stop the delivery "
               "of gold coins to the soldiers "
               "stationed here. However, without "
               "gold coins the soldiers here can "
               "not train and improve their "
               "skills."),
  // Granite mine
  gettext_noop("The quarrying of stone in a "
               "granite mine guarantees the "
               "supply of stone for buildings. "
               "However, even a granite mine has "
               "to feed its workers."),
  // Coal mine
  gettext_noop("The mining of coal supports the "
               "metalworks and smithy. This hard "
               "work requires an adequate supply "
               "of food."),
  // Iron mine
  gettext_noop("Deep within the mountains, "
               "miners dig for iron ore. They "
               "will obviously need a lot of "
               "food for the strenuous work."),
  // Gold mine
  gettext_noop("A gold mine allows you to "
               "prospect for valuable gold "
               "deposits. For this, it is "
               "necessary to ensure that the "
               "miners are provided with plenty "
               "of food."),
  // Lookout-tower
  gettext_noop("From the lookout tower you can "
               "see far into previously "
               "unexplored lands."),
  // Nothing
  "",
  // Catapult
  gettext_noop("Thanks to its immense strength, "
               "the catapults represents an "
               "effective weapon against enemy "
               "military buildings."),
  // Woodcutter
  gettext_noop("A woodcutter provides the "
               "sawmill with logs. A forester is "
               "able to replant the depleted "
               "forest."),
  // Fishery
  gettext_noop("The fish man is responsible for "
               "finding water rich in fish. His "
               "fish feed a great many miners."),
  // Quarry
  gettext_noop("The stonemason works the stone "
               "near his quarry into bricks. "
               "These are needed mainly for "
               "building houses and as "
               "ammunition for catapults."),
  // Forester
  gettext_noop("Within his area, the forester "
               "ensures the survival of the "
               "forest. He plants all types of "
               "trees."),
  // Slaughterhouse
  gettext_noop("The butcher processes the "
               "livestock delivered into "
               "nutritious ham on which you "
               "miners are fed."),
  // Hunter
  gettext_noop("Meat the hunter acquires is used "
               "to feed the miners."),
  // Brewery
  gettext_noop("The brewer produces fine beer "
               "from grain and water. This drink "
               "is needed to fill the soldiers "
               "with courage."),
  // Armory
  gettext_noop("The armory produces swords and "
               "strong shields. This equipment "
               "is vital for your soldiers."),
  // Metalworks
  gettext_noop("The countless tools which your "
               "workers need are made in the "
               "metalworks. This requires boards "
               "and iron."),
  // Iron smelter
  gettext_noop("Raw iron ore is smelted in the "
               "iron smelters with the help of "
               "coal. The processed iron is "
               "then used to making weapons "
               "(in the Armory) and tools "
               "(in the metalworks)."),
  // Charburner
  gettext_noop("The charburner stacks up piles of "
               "wood and straw which is then burned "
               "to create charcoal. This can be used "
               "just like the coal from the mine without "
               "any loss in quality."),
  // Pig farm
  gettext_noop("Grain and water are needed for "
               "rearing pigs. The meat thus "
               "obtained can then be processed "
               "by a butcher."),
  // Storehouse
  gettext_noop("The storehouse can help reduce "
               "long transportation journeys and "
               "is suited to the temporary "
               "storage of merchandise and "
               "inhabitants. You can release "
               "certain merchandise from the "
               "storehouse, as and when "
               "required. Alternatively, the "
               "storage function can be "
               "disabled. To do this, first "
               "choose the relevant icon "
               "followed by the desired "
               "merchandise or job symbol."),
  // Nothing
  "",
  // Mill
  gettext_noop("The grain is ground in the "
               "windmill. The flour from the "
               "windmill is later used at the "
               "bakery to bake bread."),
  // Bakery
  gettext_noop("The flour produced at the "
               "windmill can be combined with "
               "water in the bakery to make "
               "oven-fresh bread. It's your "
               "miners' favorite!"),
  // Sawmill
  gettext_noop("The carpenter turns the "
               "woodcutter's logs into "
               "made-to-measure planks. These "
               "form the basic for building "
               "houses and ships."),
  // Mint
  gettext_noop("The mint is responsible for "
               "producing valuable gold coins. "
               "These precious objects are "
               "produced using coal and gold."),
  // Well
  gettext_noop("A well supplies water to the "
               "bakery, brewery, donkey breeder "
               "and pig farm."),
  // Shipyard
  gettext_noop("It is possible to build small "
               "rowing boats as well as huge "
               "cargo ships in a shipyard. The "
               "boards required for this are "
               "skillfully worked by "
               "shipwrights."),
  // Farm
  gettext_noop("The farmer plants and harvests "
               "grain in the surrounding fields. "
               "A windmill then processes the "
               "harvested grain into flour or "
               "can be used to feed the pigs."),
  // Donkey breeder
  gettext_noop("The pack donkeys bred here are "
               "used to transport your "
               "merchandise more efficiently. "
               "They are reared on water and "
               "grain."),
  // Harbor building
  gettext_noop("Ships can only be loaded and "
               "unloaded in a harbor. "
               "Expeditions can also be prepared "
               "here. You can release certain "
               "merchandise from the storehouse, "
               "as and when required. "
               "Alternatively, the storage "
               "function can be disabled. To do "
               "this, first choose the relevant "
               "icon followed by the desired "
               "merchandise or job symbol."),
}};

const helpers::MultiEnumArray<SmokeConst, Nation, BuildingType> BUILDING_SMOKE_CONSTS = []() {
    std::remove_const_t<decltype(BUILDING_SMOKE_CONSTS)> result{};
    auto& africans = result[Nation::Africans];
    africans[BuildingType::Quarry] = SmokeConst(1, {3, -32});
    africans[BuildingType::Armory] = SmokeConst(1, {-32, -23});
    africans[BuildingType::Metalworks] = SmokeConst(4, {-26, -47});
    africans[BuildingType::Ironsmelter] = SmokeConst(2, {-20, -37});
    africans[BuildingType::Charburner] = SmokeConst(2, {-18, -52});
    africans[BuildingType::Bakery] = SmokeConst(4, {27, -39});
    africans[BuildingType::Mint] = SmokeConst(1, {17, -52});
    auto& japanese = result[Nation::Japanese];
    japanese[BuildingType::Armory] = SmokeConst(1, {-22, -43});
    japanese[BuildingType::Charburner] = SmokeConst(2, {-32, -55});
    japanese[BuildingType::Bakery] = SmokeConst(4, {-30, -39});
    japanese[BuildingType::Mint] = SmokeConst(3, {18, -58});
    auto& romans = result[Nation::Romans];
    romans[BuildingType::Brewery] = SmokeConst(1, {-26, -45});
    romans[BuildingType::Armory] = SmokeConst(2, {-36, -34});
    romans[BuildingType::Ironsmelter] = SmokeConst(1, {-16, -34});
    romans[BuildingType::Charburner] = SmokeConst(2, {-36, -38});
    romans[BuildingType::Bakery] = SmokeConst(4, {-15, -26});
    romans[BuildingType::Mint] = SmokeConst(4, {20, -50});
    auto& vikings = result[Nation::Vikings];
    vikings[BuildingType::Woodcutter] = SmokeConst(1, {2, -36});
    vikings[BuildingType::Fishery] = SmokeConst(1, {4, -36});
    vikings[BuildingType::Quarry] = SmokeConst(1, {0, -34});
    vikings[BuildingType::Forester] = SmokeConst(1, {-5, -29});
    vikings[BuildingType::Slaughterhouse] = SmokeConst(1, {7, -41});
    vikings[BuildingType::Hunter] = SmokeConst(1, {-6, -38});
    vikings[BuildingType::Brewery] = SmokeConst(3, {5, -39});
    vikings[BuildingType::Armory] = SmokeConst(3, {-23, -36});
    vikings[BuildingType::Metalworks] = SmokeConst(1, {-9, -35});
    vikings[BuildingType::Ironsmelter] = SmokeConst(2, {-2, -38});
    vikings[BuildingType::Charburner] = SmokeConst(2, {-22, -55});
    vikings[BuildingType::PigFarm] = SmokeConst(2, {-30, -37});
    vikings[BuildingType::Bakery] = SmokeConst(4, {-21, -26});
    vikings[BuildingType::Sawmill] = SmokeConst(1, {-11, -45});
    vikings[BuildingType::Mint] = SmokeConst(1, {16, -38});
    vikings[BuildingType::Farm] = SmokeConst(1, {-17, -48});
    vikings[BuildingType::DonkeyBreeder] = SmokeConst(4, {-27, -40});
    vikings[BuildingType::Vineyard] = SmokeConst(1, {18, -48});
    vikings[BuildingType::Winery] = SmokeConst(1, {-14, -32});
    auto& babylonians = result[Nation::Babylonians];
    babylonians[BuildingType::Brewery] = SmokeConst(2, {-18, -43});
    babylonians[BuildingType::Armory] = SmokeConst(1, {-22, -47});
    babylonians[BuildingType::Ironsmelter] = SmokeConst(2, {-23, -36});
    babylonians[BuildingType::Bakery] = SmokeConst(4, {-27, -32});
    babylonians[BuildingType::Mint] = SmokeConst(3, {11, -58});
    return result;
}();
