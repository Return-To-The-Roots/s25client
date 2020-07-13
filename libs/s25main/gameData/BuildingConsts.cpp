// Copyright (c) 2005 - 2017 Settlers Freaks (sf-team at siedler25.org)
//
// This file is part of Return To The Roots.
//
// Return To The Roots is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 2 of the License, or
// (at your option) any later version.
//
// Return To The Roots is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with Return To The Roots. If not, see <http://www.gnu.org/licenses/>.

#include "BuildingConsts.h"
#include "mygettext/mygettext.h"

const std::array<const char*, NUM_BUILDING_TYPES> BUILDING_NAMES = {
  gettext_noop("Headquarters"),
  gettext_noop("Barracks"),
  gettext_noop("Guardhouse"),
  "",
  gettext_noop("Watchtower"),
  "",
  "",
  "",
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

const std::array<const char*, NUM_BUILDING_TYPES> BUILDING_HELP_STRINGS = {
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
  // 4x Nothing
  "",
  "",
  "",
  "",
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
};
