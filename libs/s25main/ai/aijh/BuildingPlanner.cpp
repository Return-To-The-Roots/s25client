// Copyright (C) 2005 - 2021 Settlers Freaks (sf-team at siedler25.org)
//
// SPDX-License-Identifier: GPL-2.0-or-later

#include "BuildingPlanner.h"
#include "AIPlayerJH.h"
#include "GlobalGameSettings.h"
#include "addons/const_addons.h"
#include "buildings/nobBaseWarehouse.h"
#include "buildings/nobMilitary.h"
#include "gameTypes/BuildingType.h"
#include "gameTypes/GoodTypes.h"
#include "gameTypes/MineResourceBehavior.h"
#include "gameData/BuildingProperties.h"
#include <algorithm>
#include <cmath>

namespace AIJH {
namespace {
    bool hasAnyInexhaustibleOreMine(const GlobalGameSettings& ggs)
    {
        return !isMineResourceDepletable(ggs, BuildingType::CoalMine)
               || !isMineResourceDepletable(ggs, BuildingType::IronMine)
               || !isMineResourceDepletable(ggs, BuildingType::GoldMine);
    }
} // namespace

BuildingPlanner::BuildingPlanner(const AIPlayerJH& aijh) : buildingsWanted(), expansionRequired(false)
{
    refreshBuildingNums(aijh);
    initBuildingsWanted(aijh);
    updateBuildingsWanted(aijh);
}

void BuildingPlanner::update(unsigned gf, AIPlayerJH& aijh)
{
    refreshBuildingNums(aijh);
    expansionRequired = calcIsExpansionRequired(aijh, gf > 500 && gf % 500 == 0);
}

void BuildingPlanner::refreshBuildingNums(const AIPlayerJH& aijh)
{
    buildingNums = aijh.player.getBuildingRegister().getBuildingNums();
}

bool BuildingPlanner::calcIsExpansionRequired(AIPlayerJH& aijh, bool recalc) const
{
    if(!expansionRequired && !recalc)
        return false;
    if(getNumMilitaryBldSites() > 0 || getNumMilitaryBlds() > 4)
        return false;
    bool hasWood = getNumBuildings(BuildingType::Woodcutter) > 0;
    bool hasBoards = getNumBuildings(BuildingType::Sawmill) > 0;
    bool hasStone = getNumBuildings(BuildingType::Quarry) > 0;
    if(expansionRequired)
    {
        if(hasWood && hasBoards && hasStone)
            return false;
    } else
    {
        // Check if we could build the missing building around any military building or storehouse. If not, expand.
        const BuildingRegister& buildingRegister = aijh.player.getBuildingRegister();
        std::vector<noBuilding*> blds(buildingRegister.getMilitaryBuildings().begin(),
                                      buildingRegister.getMilitaryBuildings().end());
        blds.insert(blds.end(), buildingRegister.getStorehouses().begin(), buildingRegister.getStorehouses().end());
        for(const noBuilding* bld : blds)
        {
            if(!hasWood)
                hasWood = aijh.findPositionForBuildingAround(BuildingType::Woodcutter, bld->getPos()).isValid();
            if(!hasBoards)
                hasBoards = aijh.findPositionForBuildingAround(BuildingType::Sawmill, bld->getPos()).isValid();
            if(!hasStone)
                hasStone = aijh.findPositionForBuildingAround(BuildingType::Quarry, bld->getPos()).isValid();
        }
        return !(hasWood && hasBoards && hasStone);
    }
    return expansionRequired;
}

unsigned BuildingPlanner::getNumBuildings(BuildingType type) const
{
    return buildingNums.buildings[type] + buildingNums.buildingSites[type];
}

unsigned BuildingPlanner::getNumBuildingSites(BuildingType type) const
{
    return buildingNums.buildingSites[type];
}

unsigned BuildingPlanner::getNumMilitaryBlds() const
{
    unsigned result = 0;
    for(BuildingType bld : BuildingProperties::militaryBldTypes)
        result += getNumBuildings(bld);
    return result;
}

unsigned BuildingPlanner::getNumMilitaryBldSites() const
{
    unsigned result = 0;
    for(BuildingType bld : BuildingProperties::militaryBldTypes)
        result += getNumBuildingSites(bld);
    return result;
}

void BuildingPlanner::initBuildingsWanted(const AIPlayerJH& aijh)
{
    std::fill(buildingsWanted.begin(), buildingsWanted.end(), 0u);
    buildingsWanted[BuildingType::Forester] = 1;
    buildingsWanted[BuildingType::Sawmill] = 1;
    buildingsWanted[BuildingType::Woodcutter] = 12;
    buildingsWanted[BuildingType::GraniteMine] = 0;
    buildingsWanted[BuildingType::CoalMine] = 4;
    buildingsWanted[BuildingType::IronMine] = 2;
    buildingsWanted[BuildingType::GoldMine] = 1;
    buildingsWanted[BuildingType::Catapult] = 5;
    buildingsWanted[BuildingType::Fishery] = 6;
    buildingsWanted[BuildingType::Quarry] = 6;
    buildingsWanted[BuildingType::Hunter] = 2;
    buildingsWanted[BuildingType::Farm] =
      aijh.player.getInventory().goods[GoodType::Scythe] + aijh.player.getInventory().people[Job::Farmer];

    unsigned numAIRelevantSeaIds = aijh.getNumAIRelevantSeaIds();
    if(numAIRelevantSeaIds > 0)
    {
        buildingsWanted[BuildingType::HarborBuilding] = 99;
        buildingsWanted[BuildingType::Shipyard] = numAIRelevantSeaIds == 1 ? 1 : 99;
    }
}

void BuildingPlanner::updateBuildingsWanted(const AIPlayerJH& aijh)
{
    // no military buildings -> usually start only
    const unsigned numMilitaryBlds = aijh.player.getBuildingRegister().getMilitaryBuildings().size();

    if(numMilitaryBlds == 0 && aijh.player.getBuildingRegister().getStorehouses().size() < 2)
    {
        buildingsWanted[BuildingType::Forester] = 1;
        // probably only has 1 saw+carpenter but if that is the case the ai will try to produce 1 additional saw very
        // quickly
        buildingsWanted[BuildingType::Sawmill] = 2;
        buildingsWanted[BuildingType::Woodcutter] = 2;
        buildingsWanted[BuildingType::Quarry] = 2;
        buildingsWanted[BuildingType::GraniteMine] = 0;
        buildingsWanted[BuildingType::CoalMine] = 0;
        buildingsWanted[BuildingType::IronMine] = 0;
        buildingsWanted[BuildingType::GoldMine] = 0;
        buildingsWanted[BuildingType::Catapult] = 0;
        buildingsWanted[BuildingType::Fishery] = 0;
        buildingsWanted[BuildingType::Hunter] = 0;
        buildingsWanted[BuildingType::Farm] = 0;
        buildingsWanted[BuildingType::Charburner] = 0;
    } else
    {
        // at least some expansion happened -> more buildings wanted
        // building wanted usually limited by profession workers+tool for profession with some arbitrary limit. Some
        // buildings which are linked to others in a chain / profession-tool-rivalry have additional limits.
        const Inventory& inventory = aijh.player.getInventory();

        // foresters
        unsigned max_available_forester = inventory[Job::Forester] + inventory[GoodType::Shovel];
        unsigned additional_forester = getNumBuildings(BuildingType::Charburner);

        // 1 mil -> 1 forester, 2 mil -> 2 forester, 4 mil -> 3 forester, 8 mil -> 4 forester, 16 mil -> 5 forester, ...
        // wanted
        if(numMilitaryBlds > 0)
            buildingsWanted[BuildingType::Forester] = (unsigned)(1.45 * log(numMilitaryBlds) + 1);
        // If we are low on wood, we need more foresters
        if(aijh.player.getBuildingRegister().getBuildingSites().size()
           > (inventory[GoodType::Boards] + inventory[GoodType::Wood]) * 2)
            additional_forester++;

        buildingsWanted[BuildingType::Forester] += additional_forester;
        buildingsWanted[BuildingType::Forester] =
          std::min(max_available_forester, buildingsWanted[BuildingType::Forester]);

        // woodcutters
        unsigned max_available_woodcutter = inventory.goods[GoodType::Axe] + inventory.people[Job::Woodcutter];
        buildingsWanted[BuildingType::Woodcutter] =
          std::min(max_available_woodcutter,
                   buildingsWanted[BuildingType::Forester] * 2 + 1); // two per forester + 1 for 'natural' forest

        ////on maps with many trees the ai will build woodcutters all over the place which means the foresters are not
        /// really required
        // TODO: get number of trees in own territory. use it relatively to total size of own territory to adapt number
        // of foresters (less) and woodcutters (more)

        // fishery & hunter
        buildingsWanted[BuildingType::Fishery] =
          std::min(inventory.goods[GoodType::RodAndLine] + inventory.people[Job::Fisher], numMilitaryBlds + 1u);
        buildingsWanted[BuildingType::Hunter] =
          std::min(inventory.goods[GoodType::Bow] + inventory.people[Job::Hunter], 4u);

        // quarry: low ware games start at 2 otherwise build as many as we have stonemasons, higher ware games up to 6
        // quarries
        if(inventory.goods[GoodType::PickAxe] + inventory.people[Job::Miner] < 7
           && inventory.people[Job::Stonemason] > 0 && inventory.people[Job::Miner] < 3)
        {
            buildingsWanted[BuildingType::Quarry] =
              std::max(std::min(inventory.people[Job::Stonemason], numMilitaryBlds), 2u);
        } else
        {
            //>6miners = build up to 6 depending on resources, else max out at miners/2
            if(inventory.people[Job::Miner] > 6)
                buildingsWanted[BuildingType::Quarry] =
                  std::min(inventory.goods[GoodType::PickAxe] + inventory.people[Job::Stonemason], 6u);
            else
                buildingsWanted[BuildingType::Quarry] = inventory.people[Job::Miner] / 2;

            if(buildingsWanted[BuildingType::Quarry] > numMilitaryBlds)
                buildingsWanted[BuildingType::Quarry] = numMilitaryBlds;
        }
        // sawmills limited by woodcutters and carpenter+saws reduced by char burners minimum of 3
        int resourcelimit = inventory.people[Job::Carpenter] + inventory.goods[GoodType::Saw];
        int numSawmillsFed = (static_cast<int>(getNumBuildings(BuildingType::Woodcutter))
                              - static_cast<int>(getNumBuildings(BuildingType::Charburner) * 2))
                             / 2;
        buildingsWanted[BuildingType::Sawmill] =
          std::max(std::min(numSawmillsFed, resourcelimit), 3); // min 3
                                                                // iron smelters limited by iron mines or crucibles
        buildingsWanted[BuildingType::Ironsmelter] =
          std::min(inventory.goods[GoodType::Crucible] + inventory.people[Job::IronFounder],
                   getNumBuildings(BuildingType::IronMine));

        buildingsWanted[BuildingType::Mint] = getNumBuildings(BuildingType::GoldMine);
        // armory count = smelter -metalworks if there is more than 1 smelter or 1 if there is just 1.
        buildingsWanted[BuildingType::Armory] =
          (getNumBuildings(BuildingType::Ironsmelter) > 1) ?
            std::max(0, static_cast<int>(getNumBuildings(BuildingType::Ironsmelter))
                          - static_cast<int>(getNumBuildings(BuildingType::Metalworks))) :
            getNumBuildings(BuildingType::Ironsmelter);
        if(aijh.ggs.isEnabled(AddonId::HALF_COST_MIL_EQUIP))
            buildingsWanted[BuildingType::Armory] *= 2;
        // brewery count = 1+(armory/5) if there is at least 1 armory or armory /6 for exhaustible mines
        if(getNumBuildings(BuildingType::Armory) > 0 && getNumBuildings(BuildingType::Farm) > 0)
        {
            if(hasAnyInexhaustibleOreMine(aijh.ggs))
                buildingsWanted[BuildingType::Brewery] = 1 + getNumBuildings(BuildingType::Armory) / 5;
            else
                buildingsWanted[BuildingType::Brewery] = 1 + getNumBuildings(BuildingType::Armory) / 6;
        } else
            buildingsWanted[BuildingType::Brewery] = 0;
        // metalworks is 1 if there is at least 1 smelter, 2 if mines are inexhaustible and we have at least 4 iron
        // smelters
        buildingsWanted[BuildingType::Metalworks] = (getNumBuildings(BuildingType::Ironsmelter) > 0) ? 1 : 0;

        // max processing
        unsigned foodusers = getNumBuildings(BuildingType::Charburner) + getNumBuildings(BuildingType::Mill)
                             + getNumBuildings(BuildingType::Brewery) + getNumBuildings(BuildingType::PigFarm)
                             + getNumBuildings(BuildingType::DonkeyBreeder);

        if(getNumBuildings(BuildingType::Farm) >= foodusers - getNumBuildings(BuildingType::Mill))
            buildingsWanted[BuildingType::Mill] =
              std::min(getNumBuildings(BuildingType::Farm) - (foodusers - getNumBuildings(BuildingType::Mill)),
                       getNumBuildings(BuildingType::Bakery) + 1);
        else
            buildingsWanted[BuildingType::Mill] = getNumBuildings(BuildingType::Mill);

        resourcelimit = inventory.people[Job::Baker] + inventory.goods[GoodType::Rollingpin] + 1;
        buildingsWanted[BuildingType::Bakery] = std::min<unsigned>(getNumBuildings(BuildingType::Mill), resourcelimit);

        buildingsWanted[BuildingType::PigFarm] = (getNumBuildings(BuildingType::Farm) < 8) ?
                                                   getNumBuildings(BuildingType::Farm) / 4 :
                                                   (getNumBuildings(BuildingType::Farm) - 2) / 4;
        if(buildingsWanted[BuildingType::PigFarm] > getNumBuildings(BuildingType::Slaughterhouse) + 1)
            buildingsWanted[BuildingType::PigFarm] = getNumBuildings(BuildingType::Slaughterhouse) + 1;
        buildingsWanted[BuildingType::Slaughterhouse] = std::min(
          inventory.goods[GoodType::Cleaver] + inventory.people[Job::Butcher], getNumBuildings(BuildingType::PigFarm));

        buildingsWanted[BuildingType::Well] =
          buildingsWanted[BuildingType::Bakery] + buildingsWanted[BuildingType::PigFarm]
          + buildingsWanted[BuildingType::DonkeyBreeder] + buildingsWanted[BuildingType::Brewery];

        buildingsWanted[BuildingType::Farm] =
          std::min<unsigned>(inventory.goods[GoodType::Scythe] + inventory.people[Job::Farmer], foodusers + 3);

        if(inventory.goods[GoodType::PickAxe] + inventory.people[Job::Miner] < 3)
        {
            // almost out of new pickaxes and miners - emergency program: get coal,iron,smelter&metalworks
            buildingsWanted[BuildingType::CoalMine] = 1;
            buildingsWanted[BuildingType::IronMine] = 1;
            buildingsWanted[BuildingType::GoldMine] = 0;
            buildingsWanted[BuildingType::Ironsmelter] = 1;
            buildingsWanted[BuildingType::Metalworks] = 1;
            buildingsWanted[BuildingType::Armory] = 0;
            buildingsWanted[BuildingType::GraniteMine] = 0;
            buildingsWanted[BuildingType::Mint] = 0;
        } else // more than 2 miners
        {
            // coal mine count now depends on iron & gold not linked to food or material supply - might have to add a
            // material check if this makes problems
            if(getNumBuildings(BuildingType::IronMine) > 0)
                buildingsWanted[BuildingType::CoalMine] =
                  getNumBuildings(BuildingType::IronMine) * 2 - 1 + getNumBuildings(BuildingType::GoldMine);
            else
                buildingsWanted[BuildingType::CoalMine] = std::max(getNumBuildings(BuildingType::GoldMine), 1u);
            // more mines planned than food available? -> limit mines
            if(buildingsWanted[BuildingType::CoalMine] > 2
               && buildingsWanted[BuildingType::CoalMine] * 2
                    > getNumBuildings(BuildingType::Farm) + getNumBuildings(BuildingType::Fishery) + 1)
                buildingsWanted[BuildingType::CoalMine] =
                  (getNumBuildings(BuildingType::Farm) + getNumBuildings(BuildingType::Fishery)) / 2 + 2;
            if(getNumBuildings(BuildingType::Farm) > 7) // quite the empire just scale mines with farms
            {
                if(hasAnyInexhaustibleOreMine(aijh.ggs)) // inexhaustible mines? -> more farms required for each mine
                    buildingsWanted[BuildingType::IronMine] = std::min(getNumBuildings(BuildingType::Ironsmelter) + 1,
                                                                       getNumBuildings(BuildingType::Farm) * 2 / 5);
                else
                    buildingsWanted[BuildingType::IronMine] =
                      std::min(getNumBuildings(BuildingType::Farm) / 2, getNumBuildings(BuildingType::Ironsmelter) + 1);
                buildingsWanted[BuildingType::GoldMine] =
                  (getNumBuildings(BuildingType::Mint) > 0) ?
                    getNumBuildings(BuildingType::Ironsmelter) > 6 && getNumBuildings(BuildingType::Mint) > 1 ?
                    getNumBuildings(BuildingType::Ironsmelter) > 10 ? 4 : 3 :
                    2 :
                    1;
                buildingsWanted[BuildingType::DonkeyBreeder] = 1;
                if(aijh.ggs.isEnabled(AddonId::CHARBURNER)
                   && (buildingsWanted[BuildingType::CoalMine] > getNumBuildings(BuildingType::CoalMine) + 4))
                {
                    resourcelimit = inventory.people[Job::CharBurner] + inventory.goods[GoodType::Shovel] + 1;
                    buildingsWanted[BuildingType::Charburner] = std::min<unsigned>(
                      std::min(buildingsWanted[BuildingType::CoalMine] - (getNumBuildings(BuildingType::CoalMine) + 1),
                               3u),
                      resourcelimit);
                }
            } else
            {
                // probably still limited in food supply go up to 4 coal 1 gold 2 iron (gold+coal->coin,
                // iron+coal->tool, iron+coal+coal->weapon)
                unsigned numFoodProducers =
                  getNumBuildings(BuildingType::Bakery) + getNumBuildings(BuildingType::Slaughterhouse)
                  + getNumBuildings(BuildingType::Hunter) + getNumBuildings(BuildingType::Fishery);
                buildingsWanted[BuildingType::IronMine] =
                  (inventory.people[Job::Miner] + inventory.goods[GoodType::PickAxe]
                     > getNumBuildings(BuildingType::CoalMine) + getNumBuildings(BuildingType::GoldMine) + 1
                   && numFoodProducers > 4) ?
                    2 :
                    1;
                buildingsWanted[BuildingType::GoldMine] = (inventory.people[Job::Miner] > 2) ? 1 : 0;
                resourcelimit = inventory.people[Job::CharBurner] + inventory.goods[GoodType::Shovel];
                if(aijh.ggs.isEnabled(AddonId::CHARBURNER)
                   && (getNumBuildings(BuildingType::CoalMine) < 1
                       && (getNumBuildings(BuildingType::IronMine) + getNumBuildings(BuildingType::GoldMine) > 0)))
                    buildingsWanted[BuildingType::Charburner] = std::min(1, resourcelimit);
            }
            if(getNumBuildings(BuildingType::Quarry) + 1 < buildingsWanted[BuildingType::Quarry]
               && aijh.amountInStorage(GoodType::Stones) < 100) // no quarry and low stones -> try granitemines.
            {
                if(inventory.people[Job::Miner] > 6)
                {
                    buildingsWanted[BuildingType::GraniteMine] = std::min<int>(
                      numMilitaryBlds / 15 + 1, // limit granite mines to military / 15
                      std::max<int>(buildingsWanted[BuildingType::Quarry] - getNumBuildings(BuildingType::Quarry), 1));
                } else
                    buildingsWanted[BuildingType::GraniteMine] = 1;
            } else
                buildingsWanted[BuildingType::GraniteMine] = 0;
        }

        // Only build catapults if we have more than 5 military blds and 50 stones. Then reserve 4 stones per catapult
        // but do not build more than 4 additions catapults Note: This is a max amount. A catapult is only placed if
        // reasonable
        buildingsWanted[BuildingType::Catapult] =
          numMilitaryBlds <= 5 || inventory.goods[GoodType::Stones] < 50 ?
            0 :
            std::min((inventory.goods[GoodType::Stones] - 50) / 4, getNumBuildings(BuildingType::Catapult) + 4);
    }
    if(aijh.ggs.getMaxMilitaryRank() == 0)
    {
        buildingsWanted[BuildingType::GoldMine] = 0; // max rank is 0 = private / recruit ==> gold is useless!
    }
}

int BuildingPlanner::getNumAdditionalBuildingsWanted(BuildingType type) const
{
    return static_cast<int>(buildingsWanted[type]) - static_cast<int>(getNumBuildings(type));
}

bool BuildingPlanner::wantMoreMilitaryBlds(const AIPlayerJH& aijh) const
{
    if(getNumMilitaryBldSites() >= getNumMilitaryBlds() + 3)
        return false;
    if(expansionRequired)
        return true;
    if(getNumBuildings(BuildingType::Sawmill) > 0)
        return true;
    if(aijh.player.getInventory().goods[GoodType::Boards] > 30 && getNumBuildingSites(BuildingType::Sawmill) > 0)
        return true;
    return (getNumMilitaryBlds() + getNumMilitaryBldSites() > 0);
}

} // namespace AIJH
